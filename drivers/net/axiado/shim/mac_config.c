// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * mac_config.c - MAC initialization and configuration routines for the Axiado SHIM driver.
 * Handles 10G/1G MAC setup, RXAUI/SGMII PHY initialization, and FIFO resets.
 *
 * Copyright (C) 2022-2026 Axiado Corporation
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/iopoll.h>
#include <asm/io.h>
#include <linux/compat.h>
#include <linux/bug.h>
#include <dm/device.h>
#include <env.h>

#include "hcp.h"
#include "mac_config.h"
#include "shim_common.h"
#include "shim_mac.h"
#include "shim_platform.h"

/* RXAUI Timeout Configuration */
#define RXAUI_POLL_TIMEOUT_US 600000 /* 600ms total timeout */
#define RXAUI_POLL_INTERVAL_US 2000 /* 2ms sleep interval */

/* RXAUI register offsets */
#define RXAUI_REG_EEE_CTRL 0x0e4c
#define RXAUI_REG_MODE_CTRL 0x0e00
#define RXAUI_REG_STATUS 0x0fd8

/* RXAUI configuration values */
#define RXAUI_EEE_CTRL_VALUE 0x000000f8
#define RXAUI_MODE_DUNE_VALUE 0x00000000
#define RXAUI_MODE_DUNE_EXPECTED 0x00000003

/**
 * mac_is_enabled - Check if a specific MAC is enabled and verify hardware revision.
 * @mac_idx: MAC index to check (0-4).
 *
 * Return: true if enabled and hardware revision matches, false otherwise.
 */
bool mac_is_enabled(int mac_idx)
{
	u32 reg_rev, exp_rev;
	u32 mac_base;
	struct mac_phy *mac_cfg;
	struct udevice *dev = shim_admin.dev;

	if (!dev || mac_idx < 0 || mac_idx >= MAX_MAC_CNT)
		return false;

	mac_cfg = &shim_admin.mac_cfg[mac_idx];
	if (!mac_cfg->enabled)
		return false;

	/* Determine expected revision based on MAC type */
	if (mac_idx == 0)
		exp_rev = MAC_XG_REVISION_VAL; /* XGMII MAC */
	else
		exp_rev = MAC_1G_REVISION_VAL; /* GMII MACs */

	mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);
	reg_rev = shim_read_word(mac_base + R_REVISION);

	if (reg_rev != exp_rev) {
		printf(
			 "MAC-%d: Revision mismatch (Read: 0x%x, Exp: 0x%x)\n",
			 mac_idx, reg_rev, exp_rev);
		return false;
	}

	return true;
}

/**
 * rxaui_init_10g - Initialize RXAUI for the 10G PHY.
 * @dev: Device structure for logging.
 *
 * Configures EEE control and sets the PHY to Dune mode.
 * Waits for the mode to be established.
 */
void rxaui_init_10g(void)
{
	int ret;
	u32 val;

	printf("Initializing RXAUI for 10G\n");

	/* EEE Support (LPIDLE) Enable, RX FIFO auto clear enable, active HIGH */
	rxaui_write_phy_word(RXAUI_REG_EEE_CTRL, RXAUI_EEE_CTRL_VALUE);

	/* Set Dune Mode */
	printf("Setting DUNE mode\n");
	udelay(PHY_INIT_DELAY_US);
	rxaui_write_phy_word(RXAUI_REG_MODE_CTRL, RXAUI_MODE_DUNE_VALUE);

	/* Poll for expected status.
	 * Using readx_poll_timeout allows to sleep between checks
	 * instead of the original code's udelay() busy-wait loop.
	 */
	ret = readx_poll_timeout(rxaui_read_phy_word, RXAUI_REG_STATUS, val,
				 (val == RXAUI_MODE_DUNE_EXPECTED),
				 RXAUI_POLL_TIMEOUT_US);

	if (ret) {
		printf("Failed to set DUNE Mode (Status: 0x%x)\n", val);
		return;
	}

	printf("RXAUI 10G Configured (DUNE Mode Set)\n");
	mdelay(PLATFORM_INIT_DELAY_MS / 10); /* Allow stabilization */
}

/**
 * mac_init_10g - Configure 10G MAC registers.
 * @dev: Device structure.
 * @mac_idx: MAC index (Always 0 for 10G).
 *
 * Return: SHIM_STATUS_SUCCESS on success.
 */
static enum AX_SHIM_STATUS mac_init_10g(int mac_idx)
{
	u32 cmd_cfg;
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

	printf("Configuring 10G MAC at base 0x%x\n", mac_base);

	/* Configure FIFO Sections for Cut-through */
	shim_write_word(mac_base + R_RX_FIFO_SECTIONS, XG_MAC_R_RX_FIFO_VAL);
	shim_write_word(mac_base + R_TX_FIFO_SECTIONS, XG_MAC_R_TX_FIFO_VAL);

	/* Configure MAC Command Config Enable TX, RX, Padding */
	shim_write_word(mac_base + R_COMMAND_CONFIG,
			MAC_CC_NULL | MAC_CC_TX_ENA | MAC_CC_RX_ENA |
				MAC_CC_TX_PAD_EN);

	/* Set Max Frame Length */
	shim_write_word(mac_base + R_FRM_LENGTH, XG_MAC_R_FRM_LENGTH_VAL);

	/* Set Interface Mode (XGMII) */
	shim_write_word(mac_base + R_IF_MODE, XG_MAC_R_IF_MODE_VAL);

	/* Verify Configuration */
	cmd_cfg = shim_read_word(mac_base + R_COMMAND_CONFIG);
	printf("10G MAC Configured (Cmd: 0x%08x)\n", cmd_cfg);

	return SHIM_STATUS_SUCCESS;
}

/**
 * sgmii_fast_sim - Hardware specific fixups for 1G MACs.
 * @dev: Device structure.
 * @mac_idx: MAC index (1-4).
 *
 * Applies undocumented register writes required for SGMII signal integrity
 * and stabilization on specific hardware revisions.
 */
static void sgmii_fast_sim(int mac_idx)
{
	u32 sgmii_base = SGMII_BASE + ((mac_idx - 1) * SGMII_CONFIG_RANGE);

	if (mac_idx < 1 || mac_idx >= MAX_MAC_CNT)
		return;

#ifndef CONFIG_TARGET_SCM3005
	/* SGMII PHY tuning sequence required for link stability. */
	shim_write_phy_word(sgmii_base + 0x0dd4, 0x00);
	shim_write_phy_word(sgmii_base + 0x070c, 0x00800000);
	shim_write_phy_word(sgmii_base + 0x0710, 0x00a00090);
	shim_write_phy_word(sgmii_base + 0x470c, 0x00800000);
	shim_write_phy_word(sgmii_base + 0x4710, 0x00a00090);
	shim_write_phy_word(sgmii_base + 0x0b24, 0x00000061);
	shim_write_phy_word(sgmii_base + 0x0b28, 0xffffff00);
	shim_write_phy_word(sgmii_base + 0x4b24, 0x00000061);
	shim_write_phy_word(sgmii_base + 0x4b28, 0xffffff00);
	shim_write_phy_word(sgmii_base + 0x0b88, 0x00000100);
	shim_write_phy_word(sgmii_base + 0x4b88, 0x00000100);
	shim_write_phy_word(sgmii_base + 0x0b8c, 0x00000000);
	shim_write_phy_word(sgmii_base + 0x4b8c, 0x00000000);
	shim_write_phy_word(sgmii_base + 0x0b6c, 0x00000000);
	shim_write_phy_word(sgmii_base + 0x0b70, 0x00000001);
	shim_write_phy_word(sgmii_base + 0x0b74, 0x40000102);
	shim_write_phy_word(sgmii_base + 0x0664, 0xb8000380);
	shim_write_phy_word(sgmii_base + 0x4b6c, 0x00000000);
	shim_write_phy_word(sgmii_base + 0x4b70, 0x00000001);
	shim_write_phy_word(sgmii_base + 0x4b74, 0x40000102);
	shim_write_phy_word(sgmii_base + 0x4664, 0xb8000380);
	shim_write_phy_word(sgmii_base + 0x1540, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1544, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1548, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x154c, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1550, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1554, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1558, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x155c, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1560, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1564, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1568, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x156c, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1570, 0xfffefffe);
	shim_write_phy_word(sgmii_base + 0x1574, 0x0000fffe);

	udelay(1); /* Wait for reset propagation */

	shim_write_phy_word(sgmii_base + 0x1954, 0x00);
	shim_write_phy_word(sgmii_base + 0x1958, 0x00);
	shim_write_phy_word(sgmii_base + 0x195c, 0x7d);
	shim_write_phy_word(sgmii_base + 0x1800, 0x000d);
#else
	shim_write_phy_word(sgmii_base + ((0x04 << 10) + (0x24 << 2)),
			    0x23810c81);
	shim_write_phy_word(sgmii_base + ((0x03 << 10) + (0x75 << 2)),
			    0x00000000);
	shim_write_phy_word(sgmii_base + ((0x05 << 10) + (0x24 << 2)),
			    0x00f08098);
	shim_write_phy_word(sgmii_base + ((0x6 << 10) + (0x55 << 2)),
			    0x00000000);
	shim_write_phy_word(sgmii_base + ((0x6 << 10) + (0x56 << 2)),
			    0x00000000);
	shim_write_phy_word(sgmii_base + ((0x6 << 10) + (0x57 << 2)),
			    0x0000007e);
	shim_write_phy_word(sgmii_base + ((0x6 << 10) + (0x00 << 2)),
			    0x0000000d);
#endif
}

static void mac_reset_sgmii(int mac_idx)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * GMII_PORT_CONTROL_OFFSET);
	u32 gmii_port_ctrl;
	gmii_port_ctrl = shim_read_word(mac_base);
	gmii_port_ctrl |= ~(1 << 4); //Set bit 4 (SGMII Reset)
	shim_write_word(mac_base, gmii_port_ctrl);

	mdelay(500);

	gmii_port_ctrl = shim_read_word(mac_base);
	gmii_port_ctrl &= ~(1 << 4); //Clear bit 4 (SGMII Reset)
	shim_write_word(mac_base, gmii_port_ctrl);

	mdelay(500);
}

/**
 * mac_init_1g - Configure 1G MAC registers.
 * @dev: Device structure.
 * @mac_idx: MAC index (1-4).
 *
 * Return: SHIM_STATUS_SUCCESS on success.
 */
static enum AX_SHIM_STATUS mac_init_1g(int mac_idx)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);
	u32 cmd_cfg;

	printf("Configuring 1G MAC-%d at base 0x%x\n", mac_idx, mac_base);

	/* Setting Mac address as 34:12:00:00:00:0 */
	shim_write_word(mac_base + R_MAC_0, XG_R_MAC_0_VAL);
	shim_write_word(mac_base + R_MAC_1, XG_R_MAC_1_VAL);

	/* Configure FIFO Sections */
	shim_write_word(mac_base + R_RX_FIFO_SECTIONS, MAC_1G_R_RX_FIFO_VAL);
	shim_write_word(mac_base + R_TX_FIFO_SECTIONS, MAC_1G_R_TX_FIFO_VAL);

	/* Configure MAC Command Config,Enable TX, RX, Padding */
	shim_write_word(mac_base + R_COMMAND_CONFIG,
                MAC_CC_NULL |
			    MAC_CC_TX_ENA | /* Enable outbound flow of pkts. */
			    MAC_CC_RX_ENA | /* Enable inbound flow of pkts. */
			    MAC_CC_CRC_FWD |
			    MAC_CC_TX_PAD_EN); /* Enable padding to the frame */

	/* Set Max Frame Length */
	shim_write_word(mac_base + R_FRM_LENGTH, MAC_1G_R_FRM_LENGTH_VAL);

	/* Set Interface Mode (GMII) */
	shim_write_word(mac_base + R_IF_MODE, MAC_1G_R_IF_MODE_VAL);

	/* Apply hardware fixups */
	sgmii_fast_sim(mac_idx);

	/* Verify Configuration */
	cmd_cfg = shim_read_word(mac_base + R_COMMAND_CONFIG);
	printf("1G MAC-%d Configured (Cmd: 0x%08x)\n", mac_idx, cmd_cfg);

	return SHIM_STATUS_SUCCESS;
}

/**
 * mac_basic_init - Initialize all enabled MACs.
 *
 * Calls initialization routines for 10G and 1G MACs based on configuration.
 *
 * Return: SHIM_STATUS_SUCCESS on success, error code otherwise.
 */
enum AX_SHIM_STATUS mac_basic_init(void)
{
	enum AX_SHIM_STATUS ret;
    u8 mac_idx;

	/* Configuration for MACs (Iterate 0 to 4) */
	for (mac_idx = 0; mac_idx < MAX_MAC_CNT; mac_idx++) {
		/* Check if the mac is enabled */
		if (mac_is_enabled(mac_idx)) {
			if (!mac_idx) {
				/* 10G MAC (Index 0) */
				rxaui_init_10g();
				ret = mac_init_10g(mac_idx);
			} else {
				/* 1G MACs (Indices 1-4) */
                /* mac_reset_sgmii(mac_idx); */
				ret = mac_init_1g(mac_idx);
			}

			if (ret != SHIM_STATUS_SUCCESS) {
			    pr_err(
					"mac_init for mac_idx(%d) failed. ret_status: %d\n",
					mac_idx, ret);
				return ret;
			}
		}
	}

	return SHIM_STATUS_SUCCESS;
}

/**
 * mac_flow_config - Clear MAC flow control registers to enable normal operation.
 *
 * The hardware default for these registers is 0x10. Writing 0x0 is required
 * to disable loopback and enable normal packet flow.
 *
 * Return: SHIM_STATUS_SUCCESS.
 */
enum AX_SHIM_STATUS mac_flow_config(void)
{
	static const u32 flow_vals[MAX_MAC_CNT] = {
		MAC_XG_FLOW_VAL,
		MAC_1G_0_FLOW_VAL,
		MAC_1G_1_FLOW_VAL,
		MAC_1G_2_FLOW_VAL,
		MAC_1G_3_FLOW_VAL,
	};
	int i;

	for (i = 0; i < MAX_MAC_CNT; i++) {
		if (mac_is_enabled(i))
			shim_write_word(i * MAC_FLOW_CONFIG_BYTE_CNT, flow_vals[i]);
	}

	return SHIM_STATUS_SUCCESS;
}

/**
 * port_set_hfifo_mode - Set a MAC-port for Host FIFO Mode.
 *
 * The particular MAC port will be used by Host FIFO for send/recv data
 */
void port_set_hfifo_mode(u8 mac_idx)
{
	u32 val, reg;
	u8 port;
    u8 i;

	if (!mac_idx) {
		port = 4;
	} else if (mac_idx < MAX_MAC_CNT) {
		port = mac_idx - 1;
	}

	/* Port selection and reset */
	/* refer, for one example: $HOME-DVT/.../02172023/dv/KIRKWOOD/tests/gmii_host_pkt/mac_run_sim_test.c
	* #define RX_FIFO_SEL			 4
	*  RX_FIFO_0: 6(portselect)	5(portselect) 	4(portselect)	3	2	1	0(rst)
	*  654	0	=>port0 -mac0   (GMII-0)
	*  654	1	=>port1 -mac1	(GMII-1..)
	*  654	2	=>port2	-mac2	(SGMII-2: SB-1)
	*  654	3	=>port3	-mac3	(GMII-3: SB-2)
	*  654	4	=>port4	-mac4	(XGMII) //This is MAC4 here is XGE!!!!!
	*/
	/* Enable-capture on given port so that pkt goes to HostPktBuffer instead of EIP-inline (Applies to both tx/rx)*/
	/* NOTE: Following sequence MUST be adhered to lock on host processing */

	/* Reset host rx-fifo */
	reg = RX_PACKET_FIFO_0;
	val = shim_read_word(reg);
	val |= BIT(RX_FIFO_RST);
	shim_write_word(reg, val);

	/* Port Selection */
	val = shim_read_word(reg);
	val |= port << RX_FIFO_SEL;
	shim_write_word(reg, val);

    for(i = 0; i < MAX_MAC_CNT; i++) {
        reg = REG_XGE_PORT_CTRL + (i * sizeof(u32));
        val = shim_read_word(reg);
        val &= ~BIT(HOST_TX_FIFO_EN);
        shim_write_word(reg, val);
    }

	/* lock MAC port for host-tx-buffer */
	reg = REG_XGE_PORT_CTRL + (mac_idx * sizeof(u32));
	val = shim_read_word(reg);
	val |= BIT(HOST_TX_FIFO_EN);
	shim_write_word(reg, val);

	/* Reset tx fifos */
	reg = TX_PACKET_FIFO_0;
	val = shim_read_word(reg);
	val |= BIT(TX_FIFO_RST);
	shim_write_word(reg, val);

	printf("MAC-%d is set to Host FIFO mode\n", mac_idx);
}

/**
 * mac_addr_rd - Read the MAC address from hardware registers.
 * @mac_idx: MAC Index.
 * @mac_0: Pointer for lower 32 bits of MAC address.
 * @mac_1: Pointer for upper 16 bits of MAC address.
 *
 */
static void mac_addr_rd(u8 mac_idx, u32 *mac_0, u32 *mac_1)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

	*mac_0 = shim_read_word(mac_base + R_MAC_0);
	*mac_1 = shim_read_word(mac_base + R_MAC_1);
}

/**
 * mac_addr_app_id_rd - Read the MAC address from hardware registers.
 * @app_id: Application ID (Logic mapping).
 * @mac_0: Pointer for lower 32 bits of MAC address.
 * @mac_1: Pointer for upper 16 bits of MAC address.
 *
 * Return: true.
 */
bool mac_addr_app_id_rd(u8 app_id, u32 *mac_0, u32 *mac_1)
{
	u8 mac_idx = (app_id == MAC_10G_APPID) ? 0 : app_id;

	mac_addr_rd(mac_idx, mac_0, mac_1);

	return true;
}
EXPORT_SYMBOL_GPL(mac_addr_app_id_rd);

/**
 * mac_addr_mac_idx_rd - Read the MAC address from hardware registers.
 * @mac_idx: MAC Index.
 * @mac_0: Pointer for lower 32 bits of MAC address.
 * @mac_1: Pointer for upper 16 bits of MAC address.
 *
 * Return: true.
 */
bool mac_addr_mac_idx_rd(int mac_idx, u32 *mac_0, u32 *mac_1)
{
	mac_addr_rd(mac_idx, mac_0, mac_1);

	return true;
}
EXPORT_SYMBOL_GPL(mac_addr_mac_idx_rd);

/**
 * mac_addr_wr - Configure the MAC address in hardware registers.
 * @mac_idx: MAC Index.
 * @mac_0: Lower 32 bits of MAC address.
 * @mac_1: Upper 16 bits of MAC address.
 *
 * Return: true.
 */
static void mac_addr_wr(u8 mac_idx, u32 mac_0, u32 mac_1)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

	shim_write_word(mac_base + R_MAC_0, mac_0);
	shim_write_word(mac_base + R_MAC_1, mac_1);
}

/**
 * mac_addr_app_id_wr - Configure the MAC address in hardware registers.
 * @app_id: Application ID (Logic mapping).
 * @mac_0: Lower 32 bits of MAC address.
 * @mac_1: Upper 16 bits of MAC address.
 *
 * Return: true.
 */
bool mac_addr_app_id_wr(u8 app_id, u32 mac_0, u32 mac_1)
{
	u8 mac_idx = (app_id == MAC_10G_APPID) ? 0 : app_id;

	mac_addr_wr(mac_idx, mac_0, mac_1);

	return true;
}
EXPORT_SYMBOL_GPL(mac_addr_app_id_wr);

/**
 * mac_addr_mac_idx_wr - Configure the MAC address in hardware registers.
 * @mac_idx: MAC Index.
 * @mac_0: Lower 32 bits of MAC address.
 * @mac_1: Upper 16 bits of MAC address.
 *
 * Return: true.
 */
bool mac_addr_mac_idx_wr(u8 mac_idx, u32 mac_0, u32 mac_1)
{
	mac_addr_wr(mac_idx, mac_0, mac_1);

	return true;
}
EXPORT_SYMBOL_GPL(mac_addr_mac_idx_wr);

/**
 * mac_update_promisc - Toggle promiscuous mode.
 * @app_id: Application ID.
 * @promisc: true to enable, false to disable.
 */
void mac_update_promisc(int app_id, bool promisc)
{
	u8 mac_idx = (app_id == MAC_10G_APPID) ? 0 : app_id;
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);
	u32 cmd_cfg;

	cmd_cfg = shim_read_word(mac_base + R_COMMAND_CONFIG);

	if (!!(cmd_cfg & MAC_CC_PROMIS_EN) != promisc) {
		cmd_cfg ^= MAC_CC_PROMIS_EN;
		shim_write_word(mac_base + R_COMMAND_CONFIG, cmd_cfg);
	}
}
EXPORT_SYMBOL_GPL(mac_update_promisc);

/**
 * is_rx_local_fault - Check if MAC has RX local fault.
 * @mac_idx: MAC index (0-4).
 *
 * Return: true if RX local fault is set, false otherwise.
 */
bool is_rx_local_fault(u8 mac_idx)
{
	u32 mac_base;
	u32 mac_status;

	if (mac_idx >= MAX_MAC_CNT)
		return false;

	mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);
	mac_status = shim_read_word(mac_base + R_STATUS);
	if (mac_status & BIT(0)) {
		/* R_STATUS BIT(0) is a latching fault indicator. The first read
		 * clears the latch. Read again to get the live fault state,
		 * so transient faults during RXAUI link negotiation do not
		 * incorrectly trigger the phy-reset retry loop.
		 */
		mac_status = shim_read_word(mac_base + R_STATUS);
	}

	return !!(mac_status & BIT(0));
}
EXPORT_SYMBOL(is_rx_local_fault);

/**
 * shim_fifo_reset - Reset the SHIM FIFO for a specific MAC.
 * @mac_idx: MAC index.
 *
 * Return: true on success, false on timeout.
 */
bool shim_fifo_reset(u8 mac_idx)
{
	u32 val;
	int ret;
	u32 reg_offset = REG_XGE_SHIM_FIFO + (mac_idx * 4);
	struct udevice *dev = shim_admin.dev;

	if (!dev || mac_idx >= MAX_MAC_CNT)
		return false;

	/* Poll condition:
	 * Wait until (TX_EMPTY && RX_EMPTY) AND (No TX Overflow && No RX Overflow).
	 * If condition fails, perform reset.
	 */
	ret = readx_poll_timeout(shim_read_word, reg_offset, val,
				 ((val & BIT(BIT_TX_EMPTY)) &&
				  (val & BIT(BIT_RX_EMPTY)) &&
				  !(val & BIT(SHIM_MAC_TX_OVFLOW_BIT)) &&
				  !(val & BIT(SHIM_MAC_RX_OVFLOW_BIT))),
				 SHIM_RESET_RETRY_DELAY * SHIM_RESET_RETRY_MAX);

	if (ret) {
		/* Timeout occurred, try forcing a reset */
		val = shim_read_word(reg_offset);
		val |= (BIT(BIT_TX_RST) | BIT(BIT_RX_RST));
		shim_write_word(reg_offset, val);
		udelay(SHIM_RESET_RETRY_DELAY);

		/* Re-read to check if reset cleared the bits (optional check) */
		val = shim_read_word(reg_offset);
		if (!((val & BIT(BIT_TX_EMPTY)) && (val & BIT(BIT_RX_EMPTY)))) {
			printf("MAC-%d: FIFO reset failed (Val: 0x%x)\n",
				 mac_idx, val);
			return false;
		}
	}

	return true;
}
EXPORT_SYMBOL_GPL(shim_fifo_reset);

void shim_mac_soft_reset(u8 mac_idx)
{
	u32 port_ctrl = REG_XGE_PORT_CTRL + (mac_idx * 4);
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);
	u32 val;

	pr_info("Port Soft Reset MAC-%u\n", mac_idx);

	val = shim_read_word(port_ctrl);

	/* RXAUI/SGMII RESET - reset */
	val &= ~(BIT(BIT_PHY_SOFT_RESET));
	shim_write_word(port_ctrl, val);
	udelay(SHIM_RESET_RETRY_DELAY);

	/* RXAUI/SGMII RESET - set */
	val |= (BIT(BIT_PHY_SOFT_RESET));
	shim_write_word(port_ctrl, val);
	udelay(SHIM_RESET_RETRY_DELAY);

	/* RXAUI/SGMII RESET - reset */
	val &= ~(BIT(BIT_PHY_SOFT_RESET));
	shim_write_word(port_ctrl, val);
	/* delay for reset to propagate */
	udelay(PHY_INIT_DELAY_US);

	/* disable TX/RX */
	val = shim_read_word(mac_base + R_COMMAND_CONFIG);
	val &= ~(MAC_CC_TX_ENA);
	val &= ~(MAC_CC_RX_ENA);
	shim_write_word(mac_base + R_COMMAND_CONFIG, val);

	/* MAC SW Reset */
	val |= MAC_CC_SW_RESET;
	shim_write_word(mac_base + R_COMMAND_CONFIG, val);
	udelay(PHY_INIT_DELAY_US);

	/* Reset Tx/Rx */
	shim_fifo_reset(mac_idx);

	/* Enable Tx/Rx */
	val &= ~(MAC_CC_SW_RESET);
	val |= MAC_CC_TX_ENA | MAC_CC_RX_ENA;
	shim_write_word(mac_base + R_COMMAND_CONFIG, val);
}
EXPORT_SYMBOL_GPL(shim_mac_soft_reset);

/**
 * shim_fifo_reset_all - Reset FIFOs for all enabled MACs.
 */
void shim_fifo_reset_all(void)
{
	int i;

	for (i = 0; i < MAX_MAC_CNT; i++) {
		if (shim_admin.mac_cfg[i].enabled)
			shim_fifo_reset(i);
	}
}
EXPORT_SYMBOL_GPL(shim_fifo_reset_all);

/**
 * shim_mac_disable - Disable TX/RX for a specific MAC.
 * @mac_idx: MAC index (0-4).
 */
void shim_mac_disable(int mac_idx)
{
	u32 mac_base;

	if (mac_idx < 0 || mac_idx >= MAX_MAC_CNT)
		return;

	mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);
	shim_write_word(mac_base + R_COMMAND_CONFIG, MAC_CC_NULL);
	shim_admin.mac_cfg[mac_idx].enabled = false;
}
EXPORT_SYMBOL_GPL(shim_mac_disable);
