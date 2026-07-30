/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * shim_platform.h - platform driver definitions for the Axiado SHIM driver.
 * Contains memory structures, register offsets, and function prototypes.
 *
 * Copyright (c) 2022-2026 Axiado Corporation
 */

#ifndef _SHIM_PLATFORM_H_
#define _SHIM_PLATFORM_H_

#include <linux/types.h>

#include "mac_config.h"
#include "phy_interrupt.h"
#include "shim_common.h"

/* SHIM Register Offsets */
#define GMII_PORT_CONTROL_OFFSET 	        0x4
#define SYNC_STATUS             0
#define AUTO_NEG_COMPLETE       1

/* RX Packet Statistics */
#define SHIM_RX_PKTS 0x060
#define SHIM_RX_GOOD_PKTS 0x080
#define SHIM_RX_BAD_PKTS 0x0a0

/* MAC RX Statistics */
#define SHIM_MAC_RX_GOOD 0x120
#define SHIM_MAC_RX_CRC_ERR 0x128
#define SHIM_MAC_RX_IF_IN_ERR 0x138
#define SHIM_MAC_RX_DROP 0x158
#define SHIM_MAC_RX_TOTAL 0x160
#define SHIM_MAC_RX_UNDER_SZ_ERR 0x168
#define SHIM_MAC_RX_OVR_SZ_ERR 0x1a8
#define SHIM_MAC_RX_JABBER_ERR 0x1b0
#define SHIM_MAC_RX_FRAG_ERR 0x1b8

/* MAC TX Statistics */
#define SHIM_MAC_TX_GOOD 0x220
#define SHIM_MAC_TX_CRC_ERR 0x228
#define SHIM_MAC_TX_IF_OUT_ERR 0x238
#define SHIM_MAC_TX_DROP 0x258
#define SHIM_MAC_TX_TOTAL 0x260

/* Timeouts and Delays */
#define STANDARD_DELAY_MS 10
#define PLATFORM_INIT_DELAY_MS 100
#define PHY_INIT_DELAY_US 100

/**
 * struct shim_mem_admin - Core driver state structure
 * @pdev:            Pointer to the platform_device
 * @pltf_driver:     The platform_driver definition
 * @virt_base_addr:  Kernel virtual address for SHIM main register block
 * @phy_csr_base:    Kernel virtual address for ETH_PHY CSR block
 * @res_byte_cnt:    Size of the SHIM register block
 * @init_done:       Initialization flag
 * @mac_cfg:         MAC configuration array
 * @mii:             MDIO bus structure
 */
struct shim_mem_admin {
    struct udevice *dev;

	void __iomem *virt_base_addr;
	void __iomem *phy_csr_base;
	void __iomem *ioctl_base;
    fdt_size_t res_byte_cnt;

	bool init_done;
	struct mac_phy mac_cfg[MAX_MAC_CNT];
	struct mii_dev *mii;
};

extern struct shim_mem_admin shim_admin;

/* Memory Accessor APIs */

/* SHIM Memory Accessors */
bool shim_is_init_done(void);
void __iomem *shim_get_virt_base_addr(void);

/* Register Access Functions */

/* Validation */
bool is_shim_offset_valid(const u32 byte_offset);

/* Main SHIM Register Accessors */
u32 shim_read_word(u32 offset);
void shim_write_word(u32 offset, u32 value_in);

/* ETH_PHY CSR Register Accessors */
u32 shim_read_phy_word(u32 offset);
void shim_write_phy_word(u32 offset, u32 value_in);

/* RXAUI wrappers */
void rxaui_write_phy_word(u32 offset, u32 value_in);
u32 rxaui_read_phy_word(u32 offset);

#endif /* _SHIM_PLATFORM_H_ */
