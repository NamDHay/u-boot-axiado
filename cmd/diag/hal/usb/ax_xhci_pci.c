// SPDX-License-Identifier: GPL-2.0+
/*
 * PCI glue for an xHCI PCIe card, built only on the ax_pcie_* API (pcie.h):
 * enumeration, BAR0 assignment, command register, MSI, MMIO accessors.
 */
#include <dm.h>
#include <linux/ioport.h>
#include "ax_xhci_pci.h"

#ifndef IORESOURCE_IO
#define IORESOURCE_IO	0x00000100
#endif

/* ------------------------------------------------------------------ */
/* MMIO accessors: BAR0 is reached through ax_pcie_mem_read/write      */
/* ------------------------------------------------------------------ */
u32 ax_xhci_rd32(struct ax_xhci_pci *x, u32 off)
{
	u32 v;

	if (ax_pcie_mem_read(x->port, x->mmio_off + off, &v, sizeof(v)))
		return 0xffffffff;
	return v;
}

void ax_xhci_wr32(struct ax_xhci_pci *x, u32 off, u32 val)
{
	ax_pcie_mem_write(x->port, x->mmio_off + off, &val, sizeof(val));
}

/* xHCI 64-bit registers: access as two dwords, low first (spec 5.1) */
u64 ax_xhci_rd64(struct ax_xhci_pci *x, u32 off)
{
	u64 lo = ax_xhci_rd32(x, off);
	u64 hi = ax_xhci_rd32(x, off + 4);

	return (hi << 32) | lo;
}

void ax_xhci_wr64(struct ax_xhci_pci *x, u32 off, u64 val)
{
	ax_xhci_wr32(x, off, (u32)val);
	ax_xhci_wr32(x, off + 4, (u32)(val >> 32));
}

/* ------------------------------------------------------------------ */
/* Enumeration                                                         */
/* ------------------------------------------------------------------ */
static bool xpci_is_xhci(u8 port, u8 bus, u8 dev, u8 func)
{
	u32 v;

	if (ax_pcie_cfg_read32(port, bus, dev, func, XPCI_CLASS_REV, &v))
		return false;
	return (v >> 8) == XPCI_CLASS_XHCI;
}

int ax_xhci_pci_probe(struct ax_xhci_pci *x, u8 port, u8 first_bus, u8 last_bus)
{
	u32 id;
	u8 hdr;
	int bus, dev, func;

	memset(x, 0, sizeof(*x));

	for (bus = first_bus; bus <= last_bus; bus++) {
		for (dev = 0; dev < 32; dev++) {
			for (func = 0; func < 8; func++) {
				if (ax_pcie_cfg_read32(port, bus, dev, func,
						       XPCI_VENDOR_ID, &id) ||
				    (id & 0xffff) == 0xffff) {
					if (func == 0)
						break;	/* no device here */
					continue;
				}

				if (xpci_is_xhci(port, bus, dev, func)) {
					x->port = port;
					x->bus = bus;
					x->dev = dev;
					x->func = func;
					x->vendor = id & 0xffff;
					x->device = id >> 16;
					printf("xHCI PCI card %04x:%04x at %02x:%02x.%x\n",
					       x->vendor, x->device, bus, dev, func);
					return 0;
				}

				/* single-function device: skip func 1..7 */
				if (func == 0 &&
				    !ax_pcie_cfg_read8(port, bus, dev, 0,
						       XPCI_HEADER_TYPE, &hdr) &&
				    !(hdr & 0x80))
					break;
			}
		}
	}
	return -ENODEV;
}

/*
 * Optional: program the root port's bridge so config/mem cycles reach the
 * card (bus numbers + non-prefetchable window). Skip it if your RP init
 * already does this. mem_base/mem_limit are 32-bit bus addresses
 * (limit is the last inclusive address).
 */
int ax_xhci_pci_setup_bridge(u8 port, u8 bus, u8 dev, u8 func,
			     u8 sec, u8 sub, u32 mem_base, u32 mem_limit)
{
	u32 v;
	int ret;

	ret = ax_pcie_cfg_read32(port, bus, dev, func, 0x18, &v);
	if (ret)
		return ret;
	v = (v & 0xff000000) | bus | (sec << 8) | (sub << 16);
	ret = ax_pcie_cfg_write32(port, bus, dev, func, 0x18, v);
	if (ret)
		return ret;

	/* non-prefetchable memory window */
	v = ((mem_base >> 16) & 0xfff0) | (((mem_limit >> 16) & 0xfff0) << 16);
	ret = ax_pcie_cfg_write32(port, bus, dev, func, 0x20, v);
	if (ret)
		return ret;

	/* disable prefetchable window (base > limit) */
	ax_pcie_cfg_write32(port, bus, dev, func, 0x24, 0x0000fff0);
	ax_pcie_cfg_write32(port, bus, dev, func, 0x28, 0);
	ax_pcie_cfg_write32(port, bus, dev, func, 0x2c, 0);

	return ax_pcie_cfg_write16(port, bus, dev, func, XPCI_COMMAND,
				   XPCI_CMD_MEM | XPCI_CMD_MASTER);
}

/* ------------------------------------------------------------------ */
/* BAR0 sizing + assignment                                            */
/* ------------------------------------------------------------------ */
static int xpci_setup_bar0(struct ax_xhci_pci *x)
{
	struct ax_pci_region *regions, *r = NULL;
	int cnt, i, ret;
	u32 orig_lo, orig_hi = 0, lo, hi = 0;
	u64 mask, size, cur, base = 0, addr = 0;
	u16 cmd;
	bool is64;

	ret = ax_pcie_cfg_read16(x->port, x->bus, x->dev, x->func, XPCI_COMMAND, &cmd);
	if (ret)
		return ret;
	ax_pcie_cfg_write16(x->port, x->bus, x->dev, x->func, XPCI_COMMAND,
			    cmd & ~(XPCI_CMD_MEM | XPCI_CMD_MASTER));

	ax_pcie_cfg_read32(x->port, x->bus, x->dev, x->func, XPCI_BAR0, &orig_lo);
	if (orig_lo & 1) {
		printf("xhci: BAR0 is I/O, unexpected\n");
		return -EINVAL;
	}
	is64 = ((orig_lo >> 1) & 3) == 2;
	x->bar0.is_64bit = is64;
	x->bar0.prefetch = !!(orig_lo & 8);
	x->bar0.index = 0;

	if (is64)
		ax_pcie_cfg_read32(x->port, x->bus, x->dev, x->func, XPCI_BAR0 + 4, &orig_hi);

	/* sizing */
	ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func, XPCI_BAR0, 0xffffffff);
	ax_pcie_cfg_read32(x->port, x->bus, x->dev, x->func, XPCI_BAR0, &lo);
	ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func, XPCI_BAR0, orig_lo);
	if (is64) {
		ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func, XPCI_BAR0 + 4, 0xffffffff);
		ax_pcie_cfg_read32(x->port, x->bus, x->dev, x->func, XPCI_BAR0 + 4, &hi);
		ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func, XPCI_BAR0 + 4, orig_hi);
		mask = ((u64)hi << 32) | (lo & ~0xfULL);
		size = ~mask + 1;
	} else {
		size = (u32)(~(lo & ~0xfU) + 1);
	}
	if (!size)
		return -ENODEV;
	x->bar0.size = size;

	/* current BAR0 value */
	cur = (u64)(orig_lo & ~0xfU) | ((u64)orig_hi << 32);

	ret = ax_pcie_get_regions(x->port, &regions, &cnt);
	if (ret)
		return ret;
	for (i = 0; i < cnt; i++)
		printf("xhci: region[%d] bus 0x%llx phys 0x%llx size 0x%llx flags 0x%lx\n", i,
		       (u64)(uintptr_t)regions[i].bus_start,
		       (u64)(uintptr_t)regions[i].phys_start,
		       (u64)regions[i].size, regions[i].flags);

	if (cur) {
		/* already assigned: keep it, do not overwrite */
		addr = cur;
	} else {
		for (i = 0; i < cnt; i++) {
			if (regions[i].flags & IORESOURCE_IO)
				continue;
			base = (u64)(uintptr_t)regions[i].bus_start;
			/* never place a BAR at bus address 0 */
			addr = ALIGN(base ? base : size, size);
			if (addr + size <= base + regions[i].size)
				break;
			addr = 0;
		}
		if (!addr) {
			printf("xhci: no region fits BAR0 (size 0x%llx)\n", size);
			return -ENOMEM;
		}
		if (!is64 && (addr + size) > 0x100000000ULL)
			return -ERANGE;

		ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func, XPCI_BAR0,
				    (u32)addr | (orig_lo & 0xf));
		if (is64)
			ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func,
					    XPCI_BAR0 + 4, (u32)(addr >> 32));
	}

	/* find the region holding the BAR to derive the ax_pcie_mem_* offset */
	for (i = 0; i < cnt; i++) {
		base = (u64)(uintptr_t)regions[i].bus_start;
		if (addr >= base && addr + size <= base + regions[i].size) {
			r = &regions[i];
			break;
		}
	}
	if (!r)
		printf("xhci: WARNING BAR0 0x%llx is not inside any RP region\n", addr);

	x->bar0.addr = addr;
	/* ASSUMPTION: ax_pcie_mem_* offset is relative to the region's bus_start */
	x->mmio_off = r ? addr - (u64)(uintptr_t)r->bus_start : addr;

	printf("xhci: BAR0 bus 0x%llx off 0x%llx size 0x%llx (%s, %s)\n", addr,
	       x->mmio_off, size, is64 ? "64-bit" : "32-bit",
	       cur ? "pre-assigned" : "assigned here");
	return 0;
}

/* ------------------------------------------------------------------ */
/* MSI                                                                 */
/* ------------------------------------------------------------------ */
static u8 xpci_find_cap(struct ax_xhci_pci *x, u8 id)
{
	u16 st;
	u8 pos, cid, ttl = 48;

	if (ax_pcie_cfg_read16(x->port, x->bus, x->dev, x->func, XPCI_STATUS, &st) ||
	    !(st & XPCI_STS_CAP_LIST))
		return 0;
	if (ax_pcie_cfg_read8(x->port, x->bus, x->dev, x->func, XPCI_CAP_PTR, &pos))
		return 0;

	pos &= ~3;
	while (pos && ttl--) {
		ax_pcie_cfg_read8(x->port, x->bus, x->dev, x->func, pos, &cid);
		if (cid == id)
			return pos;
		ax_pcie_cfg_read8(x->port, x->bus, x->dev, x->func, pos + 1, &pos);
		pos &= ~3;
	}
	return 0;
}

static void xpci_irq_handler(void *arg)
{
	struct ax_xhci_pci *x = arg;
	u32 sts, iman;

	sts = ax_xhci_rd32(x, x->op_off + XPCI_USBSTS);
	if (sts & XPCI_STS_EINT) {
		ax_xhci_wr32(x, x->op_off + XPCI_USBSTS, XPCI_STS_EINT);	/* W1C */
		iman = ax_xhci_rd32(x, x->rt_off + XPCI_IMAN0);
		ax_xhci_wr32(x, x->rt_off + XPCI_IMAN0, iman | XPCI_IMAN_IP);	/* W1C IP, keep IE */
	}
	x->irq_count++;
	if (x->event_cb)
		x->event_cb(x->event_arg);
}

static int xpci_setup_msi(struct ax_xhci_pci *x)
{
	struct ax_pcie_msi *msi;
	u16 ctrl;
	u8 cap;
	bool is64;

	cap = xpci_find_cap(x, XPCI_CAP_ID_MSI);
	if (!cap)
		return -ENOENT;

	/* xHCI only needs interrupter 0 -> 1 vector */
	msi = ax_pcie_request_msi_handle(x->port, xpci_irq_handler, x, 1);
	if (!msi)
		return -ENOMEM;

	ax_pcie_cfg_read16(x->port, x->bus, x->dev, x->func, cap + 2, &ctrl);
	is64 = !!(ctrl & BIT(7));

	ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func, cap + 4, (u32)msi->address);
	if (is64) {
		ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func, cap + 8,
				    (u32)(msi->address >> 32));
		ax_pcie_cfg_write16(x->port, x->bus, x->dev, x->func, cap + 0xc, msi->data);
		if (ctrl & BIT(8))	/* per-vector masking: unmask */
			ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func, cap + 0x10, 0);
	} else {
		ax_pcie_cfg_write16(x->port, x->bus, x->dev, x->func, cap + 8, msi->data);
		if (ctrl & BIT(8))
			ax_pcie_cfg_write32(x->port, x->bus, x->dev, x->func, cap + 0xc, 0);
	}

	ctrl &= ~(7 << 4);	/* MME = 0 -> 1 vector */
	ctrl |= BIT(0);		/* MSI enable */
	ax_pcie_cfg_write16(x->port, x->bus, x->dev, x->func, cap + 2, ctrl);

	x->msi_cap = cap;
	x->msi = msi;
	return 0;
}

/* ------------------------------------------------------------------ */
/* Enable / disable                                                    */
/* ------------------------------------------------------------------ */
int ax_xhci_pci_enable(struct ax_xhci_pci *x, bool use_msi)
{
	u16 cmd;
	u32 v;
	int ret;

	ret = xpci_setup_bar0(x);
	if (ret)
		return ret;

	cmd = XPCI_CMD_MEM | XPCI_CMD_MASTER;
	if (use_msi)
		cmd |= XPCI_CMD_INTX_DIS;
	ret = ax_pcie_cfg_write16(x->port, x->bus, x->dev, x->func, XPCI_COMMAND, cmd);
	if (ret)
		return ret;

	/* read & validate capability registers */
	v = ax_xhci_rd32(x, XHCI_CAPLENGTH);
	x->cap_len = v & 0xff;
	x->hci_version = v >> 16;
	if (v == 0xffffffff || x->hci_version < 0x0090 || x->hci_version > 0x0120) {
		printf("xhci: bad CAPLENGTH/HCIVERSION 0x%08x (link/BAR/mem path?)\n", v);
		return -ENODEV;
	}
	x->op_off = x->cap_len;
	x->db_off = ax_xhci_rd32(x, XHCI_DBOFF) & ~3U;
	x->rt_off = ax_xhci_rd32(x, XHCI_RTSOFF) & ~0x1fU;

	if (use_msi) {
		ret = xpci_setup_msi(x);
		if (ret) {
			printf("xhci: MSI setup failed (%d), falling back to polling\n", ret);
			ax_pcie_cfg_write16(x->port, x->bus, x->dev, x->func,
					    XPCI_COMMAND,
					    XPCI_CMD_MEM | XPCI_CMD_MASTER);
		}
	}
	return 0;
}

void ax_xhci_pci_disable(struct ax_xhci_pci *x)
{
	u16 ctrl;

	if (x->msi_cap) {
		ax_pcie_cfg_read16(x->port, x->bus, x->dev, x->func, x->msi_cap + 2, &ctrl);
		ax_pcie_cfg_write16(x->port, x->bus, x->dev, x->func, x->msi_cap + 2,
				    ctrl & ~BIT(0));
		x->msi_cap = 0;
		x->msi = NULL;
	}
	ax_pcie_cfg_write16(x->port, x->bus, x->dev, x->func, XPCI_COMMAND, 0);
}

void ax_xhci_pci_dump(struct ax_xhci_pci *x)
{
	printf("xHCI %04x:%04x  HCIVERSION 0x%04x CAPLENGTH 0x%02x\n",
	       x->vendor, x->device, x->hci_version, x->cap_len);
	printf("  HCSPARAMS1 0x%08x  HCSPARAMS2 0x%08x  HCCPARAMS1 0x%08x\n",
	       ax_xhci_rd32(x, XHCI_HCSPARAMS1), ax_xhci_rd32(x, XHCI_HCSPARAMS2),
	       ax_xhci_rd32(x, XHCI_HCCPARAMS1));
	printf("  op 0x%x  doorbell 0x%x  runtime 0x%x  irq_count %u\n",
	       x->op_off, x->db_off, x->rt_off, x->irq_count);
}