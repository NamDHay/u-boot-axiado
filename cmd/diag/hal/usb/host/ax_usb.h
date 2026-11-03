/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __AX_USB_H
#define __AX_USB_H

#include <dm.h>

struct ax_usb_info {
	char vendor[9];
	char product[17];
	u64  blocks;
	u32  blksz;
	u8   port;	/* xHCI root port */
	u8   speed;	/* 1 FS, 2 LS, 3 HS, 4 SS */
};

/*
 * pcie_port: PCIe port whose root port is already initialised
 * (ax_pcie_init() done, link up). Returns number of storage devices found
 * or a negative errno.
 */
int  ax_usb_init(u8 pcie_port);

/*
 * Local-MMIO backend (EP side test, no PCIe): base = CPU physical address of
 * the xHCI capability registers; dma_off = CPU->controller DMA address offset
 * (0 = identity).
 */
int  ax_usb_init_mmio(ulong base, u64 dma_off);
void ax_usb_stop(void);

int  ax_usb_dev_count(void);
int  ax_usb_dev_info(int idx, struct ax_usb_info *info);

int  ax_usb_read(int idx, u64 lba, u32 count, void *buf);
int  ax_usb_write(int idx, u64 lba, u32 count, const void *buf);
int  ax_usb_erase(int idx, u64 lba, u32 count);

#endif