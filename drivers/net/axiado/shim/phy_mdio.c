// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * phy_mdio.c - MDIO read/write implementations for the Axiado SHIM driver.
 * Handles Clause 22 and Clause 45 access via the SHIM MDIO controller.
 *
 * Copyright (c) 2022-2026 Axiado Corporation
 */

#include <miiphy.h>
#include <phy.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/iopoll.h>

#include "mac_config.h"
#include "shim_common.h"
#include "shim_mac.h"
#include "shim_platform.h"

/* MDIO configuration constants */
/* #define MDIO_CFG_CLAUSE_22 0x0b1c */
#define MDIO_CFG_CLAUSE_22 0x0b0c
#define MDIO_CFG_CLAUSE_45 0x0b4c

/* MDIO timeout and retry configuration */
#define MDIO_POLL_TIMEOUT_US 1000000 /* 1 second */
#define MDIO_POLL_INTERVAL_US 1 /* 1us sleep */

/* Forward declaration for the internal helper */
static int mdio_write_data(u32 mac_base, u8 reg_addr,
			   u32 val);

/**
 * mdio_phy_reg_read - Read from a PHY register using Clause 22 (Internal).
 * @mac_idx: MAC index.
 * @page: PHY page.
 * @reg: Register offset.
 * @data: (output) Pointer to store the read data.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int __maybe_unused mdio_phy_reg_read(int mac_idx,
					    u8 page, u32 reg, u32 *data)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

	/* Clause 22 setup */
	if (mdio_write_data(mac_base, R_MDIO_CFG_STATUS,
			    MDIO_CFG_CLAUSE_22))
		return -EIO;

	/* Page selector (Marvell specific 0x16 = Reg 22) */
	if (mdio_write_data(mac_base, R_MDIO_COMMAND, 0x0016))
		return -EIO;
	if (mdio_write_data(mac_base, R_MDIO_DATA, page))
		return -EIO;

	if (mdio_write_data(mac_base, R_MDIO_COMMAND, (0x8000 | reg)))
		return -EIO;

	*data = shim_read_word(mac_base + R_MDIO_DATA);

	return 0;
}

/**
 * mdio_phy_reg_write - Write to a PHY register using Clause 22.
 * @mac_idx: MAC index.
 * @page: PHY page.
 * @reg: Register offset.
 * @data: Value to write.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int __maybe_unused mdio_phy_reg_write(int mac_idx,
					     u8 page, u32 reg, u32 data)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

	if (mdio_write_data(mac_base, R_MDIO_CFG_STATUS,
			    MDIO_CFG_CLAUSE_22))
		return -EIO;

	if (mdio_write_data(mac_base, R_MDIO_COMMAND, 0x0016))
		return -EIO;
	if (mdio_write_data(mac_base, R_MDIO_DATA, page))
		return -EIO;

	if (mdio_write_data(mac_base, R_MDIO_COMMAND, reg))
		return -EIO;

	shim_write_word(mac_base + R_MDIO_DATA, data);

	return 0;
}

/**
 * get_phy_model_num - Get the phy model number associated with Phy.
 * @mac_idx: MAC index.
 *
 * Return: u8 value associated with Marvel or Vitesse model number.
 */
static u8 __maybe_unused get_phy_model_num(int mac_idx)
{
	u32 data = 0;

	if (mdio_phy_reg_read(mac_idx, PHY_DEFAULT_PAGE, R_PHY_MODEL_NUM,
			      &data) == 0) {
		return (data >> 4) & 0x3f;
	}
	return 0xff;
}

/**
 * config_led - Configure PHY LEDs.
 * @mac_idx: MAC index.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int __maybe_unused config_led(int mac_idx)
{
	int ret;

	/* Blink - Activity, Off - No Activity */
	ret = mdio_phy_reg_write(mac_idx, 3, R_LED_FUNCTION_CTRL, 0x1040);
	if (ret)
		return ret;

	/* Polarity and mix percentage */
	ret = mdio_phy_reg_write(mac_idx, 3, R_LED_POLARITY_CTRL, 0x4405);
	if (ret)
		return ret;

	/* Timer control */
	ret = mdio_phy_reg_write(mac_idx, 3, R_LED_TIMER_CTRL, 0x4985);
	return ret;
}

/**
 * set_sgmii_mode - Set the sgmii mode in Marvel Phy.
 * @mac_idx: MAC index.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int __maybe_unused set_sgmii_mode(int mac_idx)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);
	u32 rd_val = 0;

	if (mdio_phy_reg_read(mac_idx, 0x12, 0x16, &rd_val))
		return -EIO;

	if (rd_val != 0x0012) {
		printf("Failed to set SGMII mode for MAC-%d (Val: 0x%x)\n",
			mac_idx, rd_val);
		return -EIO;
	}

	if (mdio_write_data(mac_base, R_MDIO_COMMAND, 0x0014))
		return -EIO;
	if (mdio_write_data(mac_base, R_MDIO_DATA, 0x8001))
		return -EIO;
	if (mdio_write_data(mac_base, R_MDIO_COMMAND, 0x8014))
		return -EIO;

	rd_val = shim_read_word(mac_base + R_MDIO_DATA);
	if (rd_val != 0x1) {
		printf("Failed to confirm SGMII mode for MAC-%d\n",
			mac_idx);
		return -EIO;
	}
	return 0;
}

/**
 * mdio_write_data - Write data to an MDIO register and wait for completion.
 * @mac_base: Base address for the MAC.
 * @reg_addr: Register offset to write to.
 * @val: Value to write.
 *
 * Writes value to register and polls the status register until MDIO_BUSY clears.
 *
 * Return: 0 on success, -ETIMEDOUT or -EIO on failure.
 */
static int mdio_write_data(u32 mac_base, u8 reg_addr,
			   u32 val)
{
	void __iomem *status_reg;
	u32 status_val;
	int ret;

	/* Write the value */
	shim_write_word(mac_base + reg_addr, val);

	/* Calculate address for polling (SHIM base + MAC offset + Status Reg) */
	status_reg = shim_get_virt_base_addr() + mac_base + R_MDIO_CFG_STATUS;

	/* Poll until MDIO_BUSY (Bit 0) is 0.
	 * 1us interval, 1s timeout.
	 */
	ret = readl_poll_timeout(status_reg, status_val,
				 !(status_val & MDIO_BUSY),
				 MDIO_POLL_TIMEOUT_US);

	if (ret) {
		printf("MDIO Busy Timeout: Base=0x%x Reg=0x%x Val=0x%x\n",
			mac_base, reg_addr, val);
		return -ETIMEDOUT;
	}

	if (status_val & MDIO_READ_ERROR) {
		printf("MDIO Read Error: Base=0x%x Reg=0x%x Val=0x%x\n",
			mac_base, reg_addr, val);
		return -EIO;
	}

	return 0;
}

/**
 * mmd_read - Read from an MDIO register (Clause 45).
 * @port_no: Port number (Usually 0 for 10G).
 * @mmd: MMD address.
 * @addr: Register address.
 *
 * Return: Value read from the register.
 */
u32 mmd_read(u32 port_no, u32 mmd, u32 addr)
{
	u32 cmd;
	u32 val;
	u32 mac_10g_base = MAC_BASE_OFFSET; /* 10G is at index 0 */

	cmd = (port_no << 5) | mmd;

	/* Address Phase */
	mdio_write_data(mac_10g_base, R_MDIO_COMMAND, cmd);
	mdio_write_data(mac_10g_base, R_MDIO_REGADDR, addr);

	/* Read Phase (Set bit 15 of command for Read) */
	mdio_write_data(mac_10g_base, R_MDIO_COMMAND, (cmd | 0x8000));

	val = shim_read_word(mac_10g_base + R_MDIO_DATA);

	/* printf("%s: port=0x%x mmd=0x%x addr=0x%x val=0x%x\n", __func__, */
	/* 	port_no, mmd, addr, val); */

	return val;
}

/**
 * mmd_write - Write to an MDIO register (Clause 45).
 * @port_no: Port number.
 * @mmd: MMD address.
 * @addr: Register address.
 * @value: Value to write.
 */
void mmd_write(u32 port_no, u32 mmd, u32 addr, u32 value)
{
	u32 cmd;
	u32 mac_10g_base = MAC_BASE_OFFSET;

	cmd = (port_no << 5) | mmd;

	/* Address Phase */
	mdio_write_data(mac_10g_base, R_MDIO_COMMAND, cmd);
	mdio_write_data(mac_10g_base, R_MDIO_REGADDR, addr);

	/* Data Phase */
	mdio_write_data(mac_10g_base, R_MDIO_DATA, value);

	/* printf("%s: port=0x%x mmd=0x%x addr=0x%x val=0x%x\n", __func__, */
	/* 	port_no, mmd, addr, value); */
}

/**
 * mdiobus_reg_read - Read a PHY register over MDIO (Clause 22).
 * @mac_idx: MAC index (Bus address of the PHY).
 * @reg: Register to read.
 * @data: (output) Data read.
 *
 * Return: 0 on success or negative errno on failure.
 */
int mdiobus_reg_read(int mac_idx, u32 reg, u32 *data)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);
	int ret;

	if (!data)
		return -EINVAL;

	/* Set Clause 22 Mode */
	ret = mdio_write_data(mac_base, R_MDIO_CFG_STATUS,
			      MDIO_CFG_CLAUSE_22);
	if (ret)
		goto err;

	/* Issue Read Command (0x8000 | reg) */
	ret = mdio_write_data(mac_base, R_MDIO_COMMAND, (0x8000 | reg));
	if (ret)
		goto err;

	*data = (shim_read_word(mac_base + R_MDIO_DATA) & 0xFFFF);

	/* printf("%s: mac=%d reg=0x%x data=0x%x\n", __func__, mac_idx, reg, */
	/* 	*data); */
	return 0;

err:
	*data = 0;
	return ret;
}

/**
 * mdiobus_reg_write - Write to a PHY register over MDIO (Clause 22).
 * @mac_idx: MAC index.
 * @reg: Register to write.
 * @data: Data to write.
 */
void mdiobus_reg_write(int mac_idx, u32 reg, u32 data)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

	/* Set Clause 22 Mode */
	if (mdio_write_data(mac_base, R_MDIO_CFG_STATUS,
			    MDIO_CFG_CLAUSE_22))
		return;

	/* Issue Write Command */
	if (mdio_write_data(mac_base, R_MDIO_COMMAND, reg))
		return;

	/* Write Data */
	shim_write_word(mac_base + R_MDIO_DATA, data);

	/* printf("%s: mac=%d reg=0x%x data=0x%x\n", __func__, mac_idx, reg, */
	/* 	data); */
}

/**
 * mdiobus_reg_read_c45 - Read a PHY register over MDIO (Clause 45).
 * @mac_idx: MAC index.
 * @dev: MMD device address.
 * @reg: Register to read.
 * @data: (output) Data read.
 *
 * Return: 0 on success or negative errno on failure.
 */
int mdiobus_reg_read_c45(int mac_idx, int dev, u32 reg, u32 *data)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);
	int ret;

	if (!data)
		return -EINVAL;

	/* Set Clause 45 Mode */
	ret = mdio_write_data(mac_base, R_MDIO_CFG_STATUS,
			      MDIO_CFG_CLAUSE_45);
	if (ret) {
		*data = 0;
		return ret;
	}

	/* Perform Read via MMD helper */
	*data = mmd_read(0, dev, reg);

	/* printf("%s: mac=%d reg=0x%x data=0x%x\n", __func__, mac_idx, reg, */
	/* 	*data); */
	return 0;
}

/**
 * mdiobus_reg_write_c45 - Write to a PHY register over MDIO (Clause 45).
 * @mac_idx: MAC index.
 * @dev: MMD device address.
 * @reg: Register to write.
 * @data: Data to write.
 */
void mdiobus_reg_write_c45(int mac_idx, int dev, u32 reg, u32 data)
{
	u32 mac_base = MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

	/* Set Clause 45 Mode */
	mdio_write_data(mac_base, R_MDIO_CFG_STATUS, MDIO_CFG_CLAUSE_45);

	/* Perform Write via MMD helper */
	mmd_write(0, dev, reg, data);

	/* printf("%s: mac=%d dev=0x%x reg=0x%x data=0x%x\n", __func__, */
	/* 	mac_idx, dev, reg, data); */
}
