/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * phy_interrupt.h - Macro and register definations
 *
 * Copyright (c) 2022-2026 Axiado Corporation
 */

#ifndef _PHY_INTERRUPT_H_
#define _PHY_INTERRUPT_H_

#include <linux/bitops.h>

/* Monitor Reg Bits */
#define MON_REG_BIT_SYNC_STATUS_MON BIT(0)
#define MON_REG_BIT_AUTO_NEG_MON BIT(1)
#define HFIFO_RX_FIFO_OVF_MON BIT(10)
#define HFIFO_RX_FIFO_DAV_MON BIT(11)

#define HFIFO_RX_FIFO_OVF BIT(2)
#define HFIFO_RX_FIFO_DAV BIT(3)

/* SHIM/MAC Monitor and Interrupt CSR Offsets */
/* GE_MON0 */
#define SHIM_MAC_MON 0x0130
/* IRQ_GE0_CSR */
#define SHIM_MAC_IRQ_CSR 0x0204

/* CSR Reg Bits */
/* Same used for GE_IRQ_EN / XGE_IRQ_EN bit */
#define CSR_REG_BIT_MAC_IRQ_EN BIT(0)
/* Same used for SGMII_IRQ_EN / RXAUI_IRQ_EN bit */
#define CSR_REG_BIT_MACPHY_IRQ_EN BIT(1)
/* Same used for GE_MON / XGE_MON bit */
#define CSR_REG_BIT_MAC_MON BIT(17)
/* Same used for SGMII_MON / RXAUI_MON bit */
#define CSR_REG_BIT_MACPHY_MON BIT(18)

#define CSR_REG_BIT_RX_FIFO_MON BIT(20)
#define CSR_REG_BIT_HPI_IRQ_EN BIT(3)

#define SHIM_MAC_IRQ_NAME_LEN 32
#define SHIM_MAC_REG_ADDR_SIZE 4

#endif /* _PHY_INTERRUPT_H_ */
