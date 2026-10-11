/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __AX_XHCI_PCI_H
#define __AX_XHCI_PCI_H

#include <errno.h>
#include "ax_pcie.h"
#include "ax_pcie_conf.h"

/* PCI config offsets (prefixed to avoid clashing with U-Boot's pci.h) */
#define XPCI_VENDOR_ID		0x00
#define XPCI_COMMAND		0x04
#define XPCI_STATUS		0x06
#define XPCI_CLASS_REV		0x08
#define XPCI_HEADER_TYPE	0x0e
#define XPCI_BAR0		0x10
#define XPCI_CAP_PTR		0x34

#define XPCI_CMD_MEM		BIT(1)
#define XPCI_CMD_MASTER		BIT(2)
#define XPCI_CMD_INTX_DIS	BIT(10)
#define XPCI_STS_CAP_LIST	BIT(4)

#define XPCI_CAP_ID_MSI		0x05
#define XPCI_CLASS_XHCI		0x0c0330	/* base 0C, sub 03, prog-if 30 */

/* xHCI capability registers */
#define XHCI_CAPLENGTH		0x00
#define XHCI_HCSPARAMS1		0x04
#define XHCI_HCSPARAMS2		0x08
#define XHCI_HCCPARAMS1		0x10
#define XHCI_DBOFF		0x14
#define XHCI_RTSOFF		0x18

/* used by the MSI handler */
#define XPCI_USBSTS		0x04	/* from op base */
#define XPCI_STS_EINT		BIT(3)
#define XPCI_IMAN0		0x20	/* from runtime base */
#define XPCI_IMAN_IP		BIT(0)

/* CPU physical -> PCI bus address offset for DMA (inbound mapping). 0 = identity */
#ifndef AX_XHCI_PCI_DMA_OFFSET
#define AX_XHCI_PCI_DMA_OFFSET	0ULL
#endif

struct ax_xhci_pci {
	u8  port;
	u8  bus, dev, func;
	u16 vendor, device;

	struct ax_pcie_bar bar0;
	u64 mmio_off;		/* offset passed to ax_pcie_mem_read/write */

	/* cached xHCI register block offsets (from BAR0) */
	u8  cap_len;
	u16 hci_version;
	u32 op_off, rt_off, db_off;

	/* MSI */
	u8  msi_cap;
	struct ax_pcie_msi *msi;
	void (*event_cb)(void *arg);
	void *event_arg;
	volatile u32 irq_count;
};

int  ax_xhci_pci_probe(struct ax_xhci_pci *x, u8 port, u8 first_bus, u8 last_bus);
int  ax_xhci_pci_setup_bridge(u8 port, u8 bus, u8 dev, u8 func,
			      u8 sec, u8 sub, u32 mem_base, u32 mem_limit);
int  ax_xhci_pci_enable(struct ax_xhci_pci *x, bool use_msi);
void ax_xhci_pci_disable(struct ax_xhci_pci *x);
void ax_xhci_pci_dump(struct ax_xhci_pci *x);

u32  ax_xhci_rd32(struct ax_xhci_pci *x, u32 off);
void ax_xhci_wr32(struct ax_xhci_pci *x, u32 off, u32 val);
u64  ax_xhci_rd64(struct ax_xhci_pci *x, u32 off);
void ax_xhci_wr64(struct ax_xhci_pci *x, u32 off, u64 val);

static inline u64 ax_xhci_cpu_to_bus(const void *p)
{
	return (u64)(uintptr_t)p + AX_XHCI_PCI_DMA_OFFSET;
}

#endif