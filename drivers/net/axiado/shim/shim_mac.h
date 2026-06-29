/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * shim_mac.h - Defines MAC offsets, configuration values, and return status codes
 * for the Axiado SHIM driver.
 * Copyright (c) 2022-2026 Axiado Corporation
 */

#ifndef _SHIM_MAC_H_
#define _SHIM_MAC_H_

#include <linux/errno.h>
#include <linux/types.h>

/* Scratchpad register offset used for board detection (Legacy) */
#define REG_SCRATCH_PAD_REG_N2_ADRS_OFFSET 0x008

/* SGMII configuration constants */
#define SGMII_BASE 0x0
#define SGMII_CONFIG_RANGE 0x10000

/* * 1G/SGMII MAC Configuration
 * 0x03f00008: Default cut-through FIFO settings
 */
#define MAC_1G_R_RX_FIFO_VAL 0x03f00008
#define MAC_1G_R_TX_FIFO_VAL 0x03f00008
#define MAC_1G_R_FRM_LENGTH_VAL 9600

/* 0x02 : GMII mode */
#define MAC_1G_R_IF_MODE_VAL 0x02
#define MAC_1G_REVISION_VAL 0x10136
#define MAC_CC_NULL 0x00000000

/* 1G MAC Flow Control Values (0x0: Non-loopback, 0x1: Loopback) */
#define MAC_1G_0_FLOW_VAL 0x00000000
#define MAC_1G_1_FLOW_VAL 0x00000000
#define MAC_1G_2_FLOW_VAL 0x00000000
#define MAC_1G_3_FLOW_VAL 0x00000000

/* * 10G/XGMII MAC Configuration
 * Optimized for cut-through on RXAUI
 */

/* Setting Mac address as 00:11:22:33:44:55 */
#define XG_R_MAC_1_VAL			0x00005544
#define XG_R_MAC_0_VAL			0x33221100
#define XG_MAC_R_RX_FIFO_VAL 0x4
#define XG_MAC_R_TX_FIFO_VAL 0x6
#define XG_MAC_R_FRM_LENGTH_VAL 9600
#define XG_MAC_R_IF_MODE_VAL 0x0
#define MAC_XG_REVISION_VAL 0x10136
#define MAC_XG_FLOW_VAL 0x00000000

/* Return status for shim functions.
 * Mapped to standard Linux error codes to facilitate future cleanup.
 */
enum AX_SHIM_STATUS {
	SHIM_STATUS_SUCCESS = 0,
	SHIM_STATUS_INVALID_ARG = -EINVAL,
	SHIM_STATUS_NULL_POINTER = -EFAULT,
	SHIM_STATUS_INTERNAL_ERROR = -EIO
};

/* Prototypes */
enum AX_SHIM_STATUS shim_mac_init(void);
void port_set_hfifo_mode(u8 mac_idx);

#endif /* _SHIM_MAC_H_ */
