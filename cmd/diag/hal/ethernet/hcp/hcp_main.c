// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <clk.h>
#include <net.h>
#include <env.h>
#include <dm.h>
#include <log.h>
#include <config.h>
#include <console.h>
#include <malloc.h>
#include <asm/global_data.h>
#include <malloc.h>
#include <net.h>
#include <phy.h>
#include <time.h>
#include <miiphy.h>
#include <eth_phy.h>
#include <wait_bit.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <linux/err.h>
#include <hexdump.h>

#include "hcp_main.h"

DECLARE_GLOBAL_DATA_PTR;

/* Number of MACs (4 x 1G + 1 x 10G); not used in this file */
#define MAX_MAC_CNT 5
#define MAC_10G_APPID 5

/* Per-MAC offset from shim_phy_base_addr; not used in this file */
#define MAC_BASE_x_OFFSET		0x400
#define HCP_TIMEOUT_RETRY       1000
#define HCP_RX_TIMEOUT_MS       10
#define hcp_get_mac_cfg(hcp) \
	(((struct shim_mem_admin *)((hcp)->shim_priv))->mac_cfg)
struct ax_hcp_ops hcp = {
    .mac_init = hcp_mac_init,
    .phy_init = hcp_phy_init,
    .start_link_up = hcp_start_link_up,
    .stop_link_up = hcp_stop_link_up,
    .recv = hcp_recv,
    .send = hcp_send
};

/*
 * PHY config applied by the last hcp_phy_init() on each SGMII port, so that
 * hcp_stop_link_up() can undo exactly the loopback mode that was set up.
 * Indexed by mac_idx (1..GMII_PORT_COUNT); slot 0 (XGMII) is unused.
 */
static struct ax_phy_info hcp_phy_state[GMII_PORT_COUNT + 1];
static bool hcp_phy_configured[GMII_PORT_COUNT + 1];

static bool hcp_mac_idx_valid(unsigned mac_idx)
{
	return mac_idx >= 1 && mac_idx <= GMII_PORT_COUNT;
}

static int hcp_mac_init(unsigned mac_idx)
{
	int ret = 0;

	mac_reset_1g(mac_idx);
	mac_init_1g(mac_idx);
	mdelay(200);
	eth_set_eipbypass(mac_idx);
	/* HW specific config for MAC. */
	sgmii_innosilicon_pi_config(mac_idx);
	sgmii_fast_sim(mac_idx);
	sgmii_fast_sim_other(mac_idx);
	sgmii_innosilicon_adjust(mac_idx);
	/* Reset the internal PHY (SGMII) */
	mac_reset_sgmii(mac_idx);

	/* The mac_reset_1g() result is not checked: this always returns 0 */
	return ret;
}
static int hcp_phy_init(unsigned mac_idx, int test_ops, struct ax_phy_info *info){
   u8 phyaddr= 0,phy_type = UNKNOWNED_PHY;
   u16 id1 = 0;
   u16 id2 = 0 ;
   int test_result = 1;
   eth_test_options opt = { test_ops, DIS_EX_PHY_AN_BYBASS,ENA_EX_PHY_MODE };
   printf("Start Init: Port[%d]\n", mac_idx - 1);
   //Config MDIO, Using C22
   mdio_config_c22(mac_idx);
   //Read ID Phy
	phyaddr = mdio_find_phy_id(mac_idx, &id1, &id2);
	if (phy_type == UNKNOWNED_PHY) {
		if (id1 == 0x1c && id2 == 0xc916) {
			phy_type = REALTEK_PHY;
			printf("Realtek phy\n");
		}
		else if (id1 == 0x141 && id2 == 0xdd1) {
			phy_type = MARVELL_PHY;
			printf("Marvell phy\n");
		}
	}

	test_result = eth_init_exPhy(mac_idx, phyaddr, opt, phy_type);

	if (hcp_mac_idx_valid(mac_idx)) {
		hcp_phy_state[mac_idx].phyaddr = phyaddr;
		hcp_phy_state[mac_idx].phy_type = phy_type;
		hcp_phy_state[mac_idx].opt = opt;
		hcp_phy_configured[mac_idx] = true;
	}

	if (info) {
		info->phyaddr = phyaddr;
		info->phy_type = phy_type;
		info->opt = opt;
	}
	return test_result;
}
/*
 * Wait for the SGMII link (sync + auto-neg complete) of an already
 * initialised port. Returns 0 on link up, -ETIMEDOUT otherwise.
 */
static int hcp_start_link_up(unsigned mac_idx)
{
	if (!hcp_mac_idx_valid(mac_idx))
		return -EINVAL;

	return eth_wait_internal_phy_linkup(mac_idx) ? 0 : -ETIMEDOUT;
}

/*
 * Tear down everything a test may have enabled on the port, so the next
 * test starts from a clean state: external PHY loopback (only what
 * hcp_phy_init() set up), SerDes near-end loopback, MAC TX/RX and the
 * host TX FIFO lock.
 */
static int hcp_stop_link_up(unsigned mac_idx)
{
	struct ax_phy_info *phy;
	u32 mac_base, regval;

	if (!hcp_mac_idx_valid(mac_idx))
		return -EINVAL;

	phy = &hcp_phy_state[mac_idx];
	if (hcp_phy_configured[mac_idx] && phy->phyaddr != INVALID_MAC_IDX) {
		if (phy->opt.eth_run_test_ops == EXTERNAL_PHY_RJ45_LOOPBACK)
			mdio_disable_stub_lb(mac_idx, phy->phyaddr, phy->phy_type);
		else if (phy->opt.eth_run_test_ops == EXTERNAL_PHY_LOOPBACK)
			mdio_disable_exphy_lb(mac_idx, phy->phyaddr, phy->phy_type);
	}
	hcp_phy_configured[mac_idx] = false;

	disable_internal_near_end_lb(mac_idx);

	mac_base = XGMII_MAC_BASE + (mac_idx * MAC_CONFIG_BYTE_CNT);
	regval = readl(mac_base + R_COMMAND_CONFIG);
	regval &= ~(MAC_CC_TX_ENA | MAC_CC_RX_ENA);
	writel(regval, mac_base + R_COMMAND_CONFIG);

	regval = readl(AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));
	regval &= ~BIT(HOST_TX_FIFO_EN);
	writel(regval, AXILS_HCP_BASE + gmii_port_ctrl_offset(mac_idx));

	return 0;
}
static int hcp_recv(unsigned mac_idx, unsigned char buf[], int blen)
{
  u8 mod = 0;
	int len = 0;
	int rlen = -1;
	volatile u32 read_data;
	int count;
	ulong start;
	u32 wb;
	u8 *bp;
	u8 port = mac_idx;
	if (mac_idx == 0) {
		port = 4;
	} else {
		port = mac_idx - 1;
	}

	/*
	 * Wait for data available in the host receive packet interface.
	 * The wait is bounded by time, not by a poll count: with the caches
	 * on, a fixed number of polls ends before the frame is back.
	 */
	start = get_timer(0);
	do {
		read_data = readl(AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	} while (!(read_data & BIT(RX_FIFO_DAV)) &&
		 get_timer(start) < HCP_RX_TIMEOUT_MS);
	if (!(read_data & BIT(RX_FIFO_DAV))) {
		printf("Timeout on rx \n");
		// return sgmii_link_try(mac_idx, true);
		return rlen;
	}

	// frame length in 32-bit words and MOD (valid bytes in the last word)
	read_data = readl(AXILS_HCP_BASE+RX_PACKET_FIFO_1);

	len = read_data >> RX_FIFO_FRMLEN & 0x0000ffff;
	mod = (read_data >> RX_FIFO_MOD) & RX_FIFO_DAV;

	rlen = len * 4 - ((4 - mod) % 4);

	/* packet doesn't fit in the buffer: drop it */
	if (len * 4 > blen) {
		len = mod = 0;
		rlen = -2;
	}

	/* read packet from FIFO to buffer */
	bp = buf; /* index buffer */
	for (count = 0; count < len; count++) {
		wb = readl(AXILS_HCP_BASE+RX_PACKET_FIFO_RDATA);
		memcpy(bp, &wb, sizeof(wb));
		bp += 4;
	}

	// write 1 to clear EOP and advance to the next packet
	writel(BIT(RX_FIFO_EOP), AXILS_HCP_BASE + RX_PACKET_FIFO_1);

	/* EOP issue workaround: reset the RX FIFO, select the port again */
	writel(BIT(RX_FIFO_RST), AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	writel(port << RX_FIFO_SEL, AXILS_HCP_BASE + RX_PACKET_FIFO_0);
	/* end workaround */

	/* return length in bytes */
	return (rlen); 
}
static int hcp_send(unsigned char buf[], int len)
{
  u32 mod;
	int pkt_int;
	u32 wb;
	u32 count = HCP_TIMEOUT_RETRY;

	/* reset the TX FIFO before writing a new frame */
	writel(BIT(TX_FIFO_RST), AXILS_HCP_BASE + TX_PACKET_FIFO_0);

	// wait for TX FIFO ready (bit 3), at most HCP_TIMEOUT_RETRY polls
	do {
		// mdelay(1);
	} while (!(readl(AXILS_HCP_BASE + TX_PACKET_FIFO_0) & (1 << 3)) &&
	       --count);
	if (count <= 0) {
		printf("Timeout waiting for TX FIFO ready\n");
		return -1;
	}

	/* shim-doc-page-25 */
	// set start of packet (SOP)
	writel(BIT(TX_FIFO_SOP), AXILS_HCP_BASE + TX_PACKET_FIFO_1);
	for (pkt_int = 0; pkt_int < len; pkt_int += 4) {
		memcpy(&wb, buf, sizeof(wb));
		buf += 4;
		writel(wb, AXILS_HCP_BASE + TX_PACKET_FIFO_WDATA);
	}

	// write MOD (valid bytes in the last word) and EOP
	mod = len % 4;
	writel(BIT(TX_FIFO_EOP) | mod << TX_FIFO_MOD,
	       AXILS_HCP_BASE + TX_PACKET_FIFO_1);
	return pkt_int - ((4 - mod) % 4); 
}

