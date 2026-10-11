/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2026
 * Intel e1000 PCIe NIC driver built on the ax_pcie API
 */

#ifndef __AX_E1000_H
#define __AX_E1000_H

#include <dm.h>
#include <linux/types.h>
#include "ax_pcie.h"
#include "ax_pcie_conf.h"

#define AX_E1000_NUM_TX		8
#define AX_E1000_NUM_RX		8
#define AX_E1000_BUF_SIZE	2048

/* Add DMA offset here if the PCIe inbound window is not 1:1 */
#ifndef AX_E1000_DMA_OFFSET
#define AX_E1000_DMA_OFFSET	0ULL
#endif

/* Legacy TX descriptor */
struct ax_e1000_tx_desc {
	u64 addr;
	u16 length;
	u8 cso;
	u8 cmd;
	u8 status;
	u8 css;
	u16 special;
};

/* Legacy RX descriptor */
struct ax_e1000_rx_desc {
	u64 addr;
	u16 length;
	u16 csum;
	u8 status;
	u8 errors;
	u16 special;
};

struct ax_e1000 {
	int port;
	u8 bus;
	u8 dev;
	u8 func;
	u16 device_id;
	bool is_pcie_family;	/* 82571/82572/82574: e1000e-style regs */

	void __iomem *regs;
	u8 mac[6];

	struct ax_e1000_tx_desc *tx_ring;
	struct ax_e1000_rx_desc *rx_ring;
	u8 *tx_buf;
	u8 *rx_buf;
	u32 tx_cur;
	u32 rx_cur;

	bool up;
};

int ax_e1000_probe(struct ax_e1000 *hw, int port);
int ax_e1000_start(struct ax_e1000 *hw);
void ax_e1000_stop(struct ax_e1000 *hw);
int ax_e1000_send(struct ax_e1000 *hw, const void *pkt, int len);
int ax_e1000_recv(struct ax_e1000 *hw, void *buf, int maxlen);
bool ax_e1000_link_up(struct ax_e1000 *hw);

#endif /* __AX_E1000_H */
