/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * shim_common.h - Common definitions for the SHIM driver, including MAC offsets,
 * board types, and the mac_phy configuration structure.
 *
 * Copyright (c) 2022-2026 Axiado Corporation
 */

#ifndef _SHIM_COMMON_H_
#define _SHIM_COMMON_H_

/* #include <miiphy.h> */
#include <phy.h>
#include <net.h>
#include <asm/io.h>

struct mii_bus;
struct device;

/* 10G PHY Macros (PHY Initialization) */
#define SWAP_DEV_ADDR 0x1
#define SWAP_REG 0xc000
#define SWAP_MASK 0x0001
#define SWAP_SET 0x0001

#define _10GBASE_X4_ADDR 0x4
#define _10GBASE_X4_REG 0x8000
#define _10GBASE_X4_MASK 0x0002
#define _10GBASE_X4_SET 0x0002

/* Total number of MACs supported */
#define MAX_MAC_CNT 5
#define MAC_10G_APPID 5

/* Base offset for MAC registers (relative to SHIM base) */
#define MAC_BASE_OFFSET 0x400
#define MAC_CONFIG_BYTE_CNT 0x400

/* Base offset for MAC flow config registers */
#define MAC_FLOW_CONFIG_BASE_OFFSET 0x0
#define MAC_FLOW_CONFIG_BYTE_CNT 0x4

/**
 * struct mac_phy - Per-MAC configuration and state
 * @phydev:    Pointer to the attached PHY device.
 * @enabled:   Whether this MAC interface is enabled.
 * @use_ncsi:  Whether this MAC is controlled by NC-SI.
 * @mdi_swap:  Whether MDI swapping is enabled (New feature).
 * @app_id:    Hardware application ID (used for indexing, 5 for 10G).
 * @phy_mode:  PHY interface mode (stored as u8).
 */
struct mac_phy {
	struct phy_device *phydev;
	bool enabled;
	bool use_ncsi;
	bool mdi_swap;
	u8 app_id;
	u8 phy_mode;
};

/* Exported functions */
void shim_read_mac_tx_stats(u32 shim_mac_tx_stat[], int mac_idx);
void shim_read_mac_rx_stats(u32 shim_mac_rx_stat[], int mac_idx);
void shim_read_shim_stats(u32 shim_stat[], int mac_idx);
bool shim_fifo_reset(u8 mac_idx);
void shim_fifo_reset_all(void);

/* MDIO register accessors */
int mdiobus_reg_read(int mac_idx, u32 reg, u32 *data);
void mdiobus_reg_write(int mac_idx, u32 reg, u32 data);
int mdiobus_reg_read_c45(int mac_idx, int dev, u32 reg, u32 *data);
void mdiobus_reg_write_c45(int mac_idx, int dev, u32 reg, u32 data);

/* Interrupt handling */
void shim_irq_enable(int mac_idx);
void shim_irq_disable(int mac_idx);
void shim_mac_enable_all_irq(void);
void shim_mac_disable_all_irq(void);
void shim_mac_disable(int mac_idx);

bool is_rx_local_fault(u8 mac_idx);

/* Host FIFO interface */
void hfifo_irq_enable(int mac_idx);
void hfifo_irq_disable(int mac_idx);
int hfifo_packet_tx(u8 *buf, u32 len);
int hfifo_packet_rx(u8 *buf, u32 buf_len, u8 mac_idx);
int hfifo_rx_pkt_len(u8 mac_idx);
void hfifo_reset_rx(u8 mac_idx);

#endif /* _SHIM_COMMON_H_ */
