/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021-2026 Axiado Corporation (or its affiliates). All rights reserved.
 *
 * HCP (Header and Crypto Processing Engine) - Internal shared structures
 *
 * This header defines the unified data structures for the consolidated
 * HCP platform driver that integrates SHIM, MAC, PHY, and Host FIFO
 * subsystems.
 */

#ifndef _AXIADO_HCP_H_
#define _AXIADO_HCP_H_

#include <miiphy.h>
#include <net.h>
#include "shim/shim_common.h"
#include "shim/shim_platform.h"
#include "shim/mac_config.h"

/* Total number of MACs supported */
#define MAX_MAC_CNT 5
#define MAC_10G_APPID 5

/* base offset for five MAC. base is shim_phy_base_addr */
#define MAC_BASE_x_OFFSET		0x400
#define HCP_TIMEOUT_RETRY       1000
#define SGMII_CSR_OFFSET        0x4

/**
 * struct hcp_device - Main HCP device structure
 *
 * Memory regions:
 * @shim_base: Virtual base address for SHIM registers
 * @phy_csr_base: Virtual base address for PHY CSR
 * @shim_size: Size of SHIM memory region
 *
 * Interrupts:
 * @mac_irqs: MAC interrupts [0-4] for 5 Ethernet MACs
 *
 * Subsystem private data:
 * @shim_priv: SHIM subsystem private data
 * @hfifo_priv: Host FIFO subsystem private data
 *
 */

/* ETH data */
struct hcp_eth_priv {
    struct udevice *dev;

	/* Memory regions */
	void __iomem *shim_base;
	void __iomem *phy_csr_base;
	void __iomem *ioctl_base;
    fdt_size_t shim_size;

	/* Subsystem private data pointers */
	void *shim_priv;
	void *hfifo_priv;
};

/**
 * struct hfifo_priv - Host FIFO private structure
 * @data: Host FIFO private data pointer
 * @mac_idx: MAC port index selected for Host FIFO
 */
struct hfifo_priv {
	u8 mac_idx;
};

/* Helper macro to access MAC configuration through shim_priv.
 * MAC configuration is part of shim subsystem's private data.
 */
#define hcp_get_mac_cfg(hcp) \
	(((struct shim_mem_admin *)((hcp)->shim_priv))->mac_cfg)

/* Subsystem initialization/cleanup functions */

/**
 * shim_subsystem_init - Initialize SHIM subsystem
 * @hcp: HCP device structure
 *
 * Initializes SHIM hardware, MACs, PHY CSR, and MDIO bus.
 *
 * Return: 0 on success, negative error code on failure
 */
int shim_subsystem_init(struct udevice *device);

void shim_mac_soft_reset(u8 mac_idx);

/**
 * hfifo_subsystem_init - Initialize Host FIFO subsystem
 * @hcp: HCP device structure
 *
 * Initializes the Host FIFO and network device.
 *
 * Return: 0 on success, negative error code on failure
 */
int hfifo_subsystem_init(struct udevice *dev);

#endif /* _AXIADO_HCP_H_ */
