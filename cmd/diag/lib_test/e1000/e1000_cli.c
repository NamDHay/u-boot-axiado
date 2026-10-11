// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Intel e1000 PCIe NIC driver built on the ax_pcie API (RP mode, polling)
 */

#include <dm.h>
#include <command.h>
#include <cpu_func.h>
#include <display_options.h>
#include <malloc.h>
#include <memalign.h>
#include <vsprintf.h>
#include <time.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "e1000_cli.h"

/* ---------------------------------------------------------------- */
/* PCI constants                                                    */
/* ---------------------------------------------------------------- */
#define PCI_VENDOR_INTEL	0x8086
#define PCI_CFG_VENDOR		0x00
#define PCI_CFG_COMMAND		0x04
#define PCI_CFG_BAR0		0x10
#define PCI_CMD_MEMORY		0x0002
#define PCI_CMD_MASTER		0x0004
#define PCI_BAR_IO		0x1
#define PCI_BAR_TYPE_MASK	0x6
#define PCI_BAR_TYPE_64		0x4

#define AX_E1000_MAX_BUS	4

/* ---------------------------------------------------------------- */
/* e1000 registers                                                  */
/* ---------------------------------------------------------------- */
#define E1000_CTRL	0x0000
#define E1000_STATUS	0x0008
#define E1000_EECD	0x0010
#define E1000_EERD	0x0014
#define E1000_TXDCTL	0x3828
#define E1000_TARC0	0x3840
#define E1000_TARC1	0x3940
#define E1000_ICR	0x00C0
#define E1000_IMC	0x00D8
#define E1000_RCTL	0x0100
#define E1000_TCTL	0x0400
#define E1000_TIPG	0x0410
#define E1000_RDBAL	0x2800
#define E1000_RDBAH	0x2804
#define E1000_RDLEN	0x2808
#define E1000_RDH	0x2810
#define E1000_RDT	0x2818
#define E1000_TDBAL	0x3800
#define E1000_TDBAH	0x3804
#define E1000_TDLEN	0x3808
#define E1000_TDH	0x3810
#define E1000_TDT	0x3818
#define E1000_MTA	0x5200
#define E1000_RAL0	0x5400
#define E1000_RAH0	0x5404

#define E1000_CTRL_FD		BIT(0)
#define E1000_CTRL_GIO_MD	BIT(2)	/* GIO master disable */
#define E1000_CTRL_FRCSPD	BIT(11)
#define E1000_CTRL_FRCDPX	BIT(12)
#define E1000_CTRL_LRST		BIT(3)
#define E1000_CTRL_ASDE		BIT(5)
#define E1000_CTRL_SLU		BIT(6)
#define E1000_CTRL_ILOS		BIT(7)
#define E1000_CTRL_RST		BIT(26)
#define E1000_CTRL_PHY_RST	BIT(31)

#define E1000_STATUS_LU		BIT(1)
#define E1000_STATUS_GIO_ME	BIT(19)	/* GIO master enable */

#define E1000_EECD_AUTO_RD	BIT(9)
#define E1000_TXDCTL_FULL_WB	0x01010000	/* WTHRESH=1, GRAN=desc */

#define E1000_EERD_START	BIT(0)
#define E1000_EERD_DONE		BIT(4)	/* 8254x */
#define E1000_EERD_ADDR_SHIFT	8	/* 8254x */
#define E1000_EERD_DONE_PCIE	BIT(1)	/* 82571/82572/82574 */
#define E1000_EERD_ADDR_SHIFT_PCIE	2
#define E1000_EERD_DATA_SHIFT	16

#define E1000_RCTL_EN		BIT(1)
#define E1000_RCTL_BAM		BIT(15)
#define E1000_RCTL_SECRC	BIT(26)

#define E1000_TCTL_EN		BIT(1)
#define E1000_TCTL_PSP		BIT(3)
#define E1000_TCTL_CT_SHIFT	4
#define E1000_TCTL_COLD_SHIFT	12

#define E1000_TIPG_DEFAULT	0x0060200A

#define E1000_TXD_CMD_EOP	0x01
#define E1000_TXD_CMD_IFCS	0x02
#define E1000_TXD_CMD_RS	0x08
#define E1000_TXD_STAT_DD	0x01

#define E1000_RXD_STAT_DD	0x01
#define E1000_RXD_STAT_EOP	0x02

#define E1000_RAH_AV		BIT(31)

#define E1000_RESET_DELAY_MS	20
#define E1000_LINK_TIMEOUT_MS	5000
#define E1000_TX_TIMEOUT_US	100000

/* 8254x-style (legacy e1000) */
static const u16 e1000_ids[] = {
	0x100C, 0x100D, 0x100E, 0x100F, 0x1010, 0x1011, 0x1015, 0x1019,
	0x1026, 0x1027, 0x1028, 0x1075, 0x1076, 0x1077, 0x1078, 0x1079,
};

/* PCIe-native family (e1000e-style): 82572EI/82571EB copper, 82574L */
static const u16 e1000_pcie_ids[] = {
	0x107D,	/* 82572EI copper */
	0x10B9,	/* 82572EI copper */
	0x105E, 0x10A4, 0x10BC,	/* 82571EB copper (untested) */
	0x10D3,	/* 82574L (untested) */
};

/* ---------------------------------------------------------------- */
/* Helpers                                                          */
/* ---------------------------------------------------------------- */
static inline u32 e1000_rd(struct ax_e1000 *hw, u32 reg)
{
	return readl(hw->regs + reg);
}

static inline void e1000_wr(struct ax_e1000 *hw, u32 reg, u32 val)
{
	writel(val, hw->regs + reg);
}

static inline u64 e1000_dma_addr(void *ptr)
{
	return (u64)virt_to_phys(ptr) + AX_E1000_DMA_OFFSET;
}

static inline void e1000_flush(void *ptr, size_t len)
{
	ulong start = rounddown((ulong)ptr, ARCH_DMA_MINALIGN);
	ulong end = roundup((ulong)ptr + len, ARCH_DMA_MINALIGN);

	flush_dcache_range(start, end);
}

static inline void e1000_inval(void *ptr, size_t len)
{
	ulong start = rounddown((ulong)ptr, ARCH_DMA_MINALIGN);
	ulong end = roundup((ulong)ptr + len, ARCH_DMA_MINALIGN);

	invalidate_dcache_range(start, end);
}

static bool e1000_id_supported(u16 id, bool *pcie_family)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(e1000_ids); i++) {
		if (e1000_ids[i] == id) {
			*pcie_family = false;
			return true;
		}
	}

	for (i = 0; i < ARRAY_SIZE(e1000_pcie_ids); i++) {
		if (e1000_pcie_ids[i] == id) {
			*pcie_family = true;
			return true;
		}
	}

	return false;
}

/* ---------------------------------------------------------------- */
/* PCI discovery + BAR setup                                        */
/* ---------------------------------------------------------------- */
static int e1000_find_device(struct ax_e1000 *hw, int port)
{
	u32 id;
	int bus, dev, func;
	bool fam;

	for (bus = 0; bus < AX_E1000_MAX_BUS; bus++) {
		for (dev = 0; dev < 32; dev++) {
			for (func = 0; func < 8; func++) {
				if (ax_pcie_cfg_read32(port, bus, dev, func,
						       PCI_CFG_VENDOR, &id))
					continue;

				if ((id & 0xffff) != PCI_VENDOR_INTEL)
					continue;

				if (!e1000_id_supported(id >> 16, &fam))
					continue;

				hw->port = port;
				hw->bus = bus;
				hw->dev = dev;
				hw->func = func;
				hw->device_id = id >> 16;
				hw->is_pcie_family = fam;

				return 0;
			}
		}
	}

	return -ENODEV;
}

static int e1000_setup_bar0(struct ax_e1000 *hw, u64 *bar_bus, u64 *bar_size)
{
	struct ax_pci_region *regions;
	struct ax_pci_region *r = NULL;
	u32 bar, mask, sz;
	u64 base, size;
	int cnt, i, ret;
	bool is64;

	ret = ax_pcie_cfg_read32(hw->port, hw->bus, hw->dev, hw->func,
				 PCI_CFG_BAR0, &bar);
	if (ret)
		return ret;

	if (bar & PCI_BAR_IO)
		return -EINVAL;

	is64 = (bar & PCI_BAR_TYPE_MASK) == PCI_BAR_TYPE_64;

	/* Size probing */
	ret = ax_pcie_cfg_write32(hw->port, hw->bus, hw->dev, hw->func,
				  PCI_CFG_BAR0, 0xffffffff);
	if (ret)
		return ret;

	ret = ax_pcie_cfg_read32(hw->port, hw->bus, hw->dev, hw->func,
				 PCI_CFG_BAR0, &mask);
	if (ret)
		return ret;

	sz = mask & ~0xfU;
	if (!sz)
		return -EINVAL;

	size = (u64)(~sz + 1);

	/* Keep a BAR already assigned by firmware */
	base = bar & ~0xfULL;
	if (is64) {
		u32 hi;

		if (ax_pcie_cfg_read32(hw->port, hw->bus, hw->dev, hw->func,
				       PCI_CFG_BAR0 + 4, &hi) == 0)
			base |= (u64)hi << 32;
	}

	if (base) {
		ax_pcie_cfg_write32(hw->port, hw->bus, hw->dev, hw->func,
				    PCI_CFG_BAR0, bar);
		goto out;
	}

	ret = ax_pcie_get_regions(hw->port, &regions, &cnt);
	if (ret)
		return ret;

	for (i = 0; i < cnt; i++) {
		if (regions[i].size >= size) {
			r = &regions[i];
			break;
		}
	}
	if (!r)
		return -ENOMEM;

	/* Single-device assumption: place BAR at aligned region start */
	base = ALIGN((u64)(unsigned long)r->bus_start, size);

	ret = ax_pcie_cfg_write32(hw->port, hw->bus, hw->dev, hw->func,
				  PCI_CFG_BAR0, (u32)base | (bar & 0xf));
	if (ret)
		return ret;

	if (is64)
		ax_pcie_cfg_write32(hw->port, hw->bus, hw->dev, hw->func,
				    PCI_CFG_BAR0 + 4, (u32)(base >> 32));

out:
	*bar_bus = base;
	*bar_size = size;

	return 0;
}

static int e1000_map_regs(struct ax_e1000 *hw, u64 bar_bus)
{
	struct ax_pci_region *regions;
	int cnt, i, ret;

	ret = ax_pcie_get_regions(hw->port, &regions, &cnt);
	if (ret)
		return ret;

	for (i = 0; i < cnt; i++) {
		u64 bs = (u64)(unsigned long)regions[i].bus_start;

		if (bar_bus >= bs && bar_bus < bs + regions[i].size) {
			hw->regs = (void __iomem *)((unsigned long)
				   regions[i].phys_start + (bar_bus - bs));
			return 0;
		}
	}

	return -ENOENT;
}

/* ---------------------------------------------------------------- */
/* EEPROM / MAC                                                     */
/* ---------------------------------------------------------------- */
static int e1000_eeprom_read(struct ax_e1000 *hw, u8 addr, u16 *data)
{
	u32 done = hw->is_pcie_family ? E1000_EERD_DONE_PCIE : E1000_EERD_DONE;
	u32 shift = hw->is_pcie_family ? E1000_EERD_ADDR_SHIFT_PCIE :
						 E1000_EERD_ADDR_SHIFT;
	int i;
	u32 v;

	e1000_wr(hw, E1000_EERD, E1000_EERD_START | ((u32)addr << shift));

	for (i = 0; i < 1000; i++) {
		v = e1000_rd(hw, E1000_EERD);
		if (v & done) {
			*data = v >> E1000_EERD_DATA_SHIFT;
			return 0;
		}
		udelay(10);
	}

	return -ETIMEDOUT;
}

static void e1000_read_mac(struct ax_e1000 *hw)
{
	u16 w;
	int i;
	bool ok = true;

	for (i = 0; i < 3; i++) {
		if (e1000_eeprom_read(hw, i, &w)) {
			ok = false;
			break;
		}
		hw->mac[i * 2] = w & 0xff;
		hw->mac[i * 2 + 1] = w >> 8;
	}

	if (!ok) {
		/* Fallback: RAL0/RAH0 preloaded by hardware */
		u32 ral = e1000_rd(hw, E1000_RAL0);
		u32 rah = e1000_rd(hw, E1000_RAH0);

		hw->mac[0] = ral;
		hw->mac[1] = ral >> 8;
		hw->mac[2] = ral >> 16;
		hw->mac[3] = ral >> 24;
		hw->mac[4] = rah;
		hw->mac[5] = rah >> 8;
	}
}

/* ---------------------------------------------------------------- */
/* Public API                                                       */
/* ---------------------------------------------------------------- */
int ax_e1000_probe(struct ax_e1000 *hw, int port)
{
	u64 bar_bus, bar_size;
	u16 cmd;
	int ret;

	memset(hw, 0, sizeof(*hw));

	ret = e1000_find_device(hw, port);
	if (ret) {
		printf("e1000: no device found on port %d\n", port);
		return ret;
	}

	ret = e1000_setup_bar0(hw, &bar_bus, &bar_size);
	if (ret) {
		printf("e1000: BAR0 setup failed (%d)\n", ret);
		return ret;
	}

	ret = e1000_map_regs(hw, bar_bus);
	if (ret) {
		printf("e1000: BAR0 0x%llx not inside any RP region\n",
		       (unsigned long long)bar_bus);
		return ret;
	}

	ret = ax_pcie_cfg_read16(port, hw->bus, hw->dev, hw->func,
				 PCI_CFG_COMMAND, &cmd);
	if (ret)
		return ret;

	cmd |= PCI_CMD_MEMORY | PCI_CMD_MASTER;
	ret = ax_pcie_cfg_write16(port, hw->bus, hw->dev, hw->func,
				  PCI_CFG_COMMAND, cmd);
	if (ret)
		return ret;

	printf("e1000: %02x:%02x.%x id 8086:%04x BAR0 0x%llx size 0x%llx\n",
	       hw->bus, hw->dev, hw->func, hw->device_id,
	       (unsigned long long)bar_bus, (unsigned long long)bar_size);

	return 0;
}

bool ax_e1000_link_up(struct ax_e1000 *hw)
{
	return !!(e1000_rd(hw, E1000_STATUS) & E1000_STATUS_LU);
}

static void e1000_hw_reset(struct ax_e1000 *hw)
{
	int i;

	e1000_wr(hw, E1000_IMC, 0xffffffff);
	e1000_wr(hw, E1000_RCTL, 0);
	e1000_wr(hw, E1000_TCTL, E1000_TCTL_PSP);
	e1000_rd(hw, E1000_STATUS);	/* flush posted writes */

	/* PCIe family: stop bus-master activity before reset or it can hang */
	if (hw->is_pcie_family) {
		e1000_wr(hw, E1000_CTRL,
			 e1000_rd(hw, E1000_CTRL) | E1000_CTRL_GIO_MD);
		for (i = 0; i < 1000; i++) {
			if (!(e1000_rd(hw, E1000_STATUS) & E1000_STATUS_GIO_ME))
				break;
			udelay(100);
		}
	}
	mdelay(10);

	e1000_wr(hw, E1000_CTRL, e1000_rd(hw, E1000_CTRL) | E1000_CTRL_RST);
	mdelay(E1000_RESET_DELAY_MS);

	/* Wait for the NVM auto-read to finish before touching EEPROM/RAL */
	if (hw->is_pcie_family) {
		for (i = 0; i < 100; i++) {
			if (e1000_rd(hw, E1000_EECD) & E1000_EECD_AUTO_RD)
				break;
			mdelay(1);
		}
	}

	e1000_wr(hw, E1000_IMC, 0xffffffff);
	e1000_rd(hw, E1000_ICR);	/* clear pending causes */
}

static int e1000_alloc_rings(struct ax_e1000 *hw)
{
	size_t txr = AX_E1000_NUM_TX * sizeof(struct ax_e1000_tx_desc);
	size_t rxr = AX_E1000_NUM_RX * sizeof(struct ax_e1000_rx_desc);

	if (hw->tx_ring)
		return 0;

	hw->tx_ring = memalign(ARCH_DMA_MINALIGN,
			       ALIGN(txr, ARCH_DMA_MINALIGN));
	hw->rx_ring = memalign(ARCH_DMA_MINALIGN,
			       ALIGN(rxr, ARCH_DMA_MINALIGN));
	hw->tx_buf = memalign(ARCH_DMA_MINALIGN,
			      AX_E1000_NUM_TX * AX_E1000_BUF_SIZE);
	hw->rx_buf = memalign(ARCH_DMA_MINALIGN,
			      AX_E1000_NUM_RX * AX_E1000_BUF_SIZE);

	if (!hw->tx_ring || !hw->rx_ring || !hw->tx_buf || !hw->rx_buf) {
		free(hw->tx_ring);
		free(hw->rx_ring);
		free(hw->tx_buf);
		free(hw->rx_buf);
		hw->tx_ring = NULL;
		hw->rx_ring = NULL;
		hw->tx_buf = NULL;
		hw->rx_buf = NULL;
		return -ENOMEM;
	}

	return 0;
}

static void e1000_init_rx(struct ax_e1000 *hw)
{
	u64 ring = e1000_dma_addr(hw->rx_ring);
	int i;

	memset(hw->rx_ring, 0,
	       AX_E1000_NUM_RX * sizeof(struct ax_e1000_rx_desc));

	for (i = 0; i < AX_E1000_NUM_RX; i++)
		hw->rx_ring[i].addr =
			e1000_dma_addr(hw->rx_buf + i * AX_E1000_BUF_SIZE);

	e1000_flush(hw->rx_ring,
		    AX_E1000_NUM_RX * sizeof(struct ax_e1000_rx_desc));

	e1000_wr(hw, E1000_RDBAL, (u32)ring);
	e1000_wr(hw, E1000_RDBAH, (u32)(ring >> 32));
	e1000_wr(hw, E1000_RDLEN,
		 AX_E1000_NUM_RX * sizeof(struct ax_e1000_rx_desc));
	e1000_wr(hw, E1000_RDH, 0);
	e1000_wr(hw, E1000_RDT, AX_E1000_NUM_RX - 1);

	hw->rx_cur = 0;

	/* 2048-byte buffers (BSIZE = 0), accept unicast match + broadcast */
	e1000_wr(hw, E1000_RCTL,
		 E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC);
}

static void e1000_init_tx(struct ax_e1000 *hw)
{
	u64 ring = e1000_dma_addr(hw->tx_ring);
	int i;

	memset(hw->tx_ring, 0,
	       AX_E1000_NUM_TX * sizeof(struct ax_e1000_tx_desc));

	for (i = 0; i < AX_E1000_NUM_TX; i++)
		hw->tx_ring[i].status = E1000_TXD_STAT_DD;

	e1000_flush(hw->tx_ring,
		    AX_E1000_NUM_TX * sizeof(struct ax_e1000_tx_desc));

	e1000_wr(hw, E1000_TDBAL, (u32)ring);
	e1000_wr(hw, E1000_TDBAH, (u32)(ring >> 32));
	e1000_wr(hw, E1000_TDLEN,
		 AX_E1000_NUM_TX * sizeof(struct ax_e1000_tx_desc));
	e1000_wr(hw, E1000_TDH, 0);
	e1000_wr(hw, E1000_TDT, 0);

	hw->tx_cur = 0;

	if (hw->is_pcie_family) {
		u32 v;

		/* Full descriptor write-back, as e1000e does for 82571/82572 */
		e1000_wr(hw, E1000_TXDCTL,
			 (e1000_rd(hw, E1000_TXDCTL) & ~0x3f3f3f) |
			 E1000_TXDCTL_FULL_WB);

		v = e1000_rd(hw, E1000_TARC0);
		e1000_wr(hw, E1000_TARC0, v | BIT(23) | BIT(24) | BIT(25) | BIT(26));

		v = e1000_rd(hw, E1000_TARC1);
		e1000_wr(hw, E1000_TARC1, v | BIT(28));
	}

	e1000_wr(hw, E1000_TIPG, E1000_TIPG_DEFAULT);
	e1000_wr(hw, E1000_TCTL,
		 E1000_TCTL_EN | E1000_TCTL_PSP |
		 (0x0f << E1000_TCTL_CT_SHIFT) |
		 (0x40 << E1000_TCTL_COLD_SHIFT));
}

int ax_e1000_start(struct ax_e1000 *hw)
{
	u32 ctrl, ral, rah;
	int i, ret;

	if (!hw->regs)
		return -ENODEV;

	ret = e1000_alloc_rings(hw);
	if (ret)
		return ret;

	e1000_hw_reset(hw);
	e1000_read_mac(hw);

	/* Program receive address 0 */
	ral = hw->mac[0] | (hw->mac[1] << 8) | (hw->mac[2] << 16) |
	      ((u32)hw->mac[3] << 24);
	rah = hw->mac[4] | (hw->mac[5] << 8) | E1000_RAH_AV;
	e1000_wr(hw, E1000_RAL0, ral);
	e1000_wr(hw, E1000_RAH0, rah);

	/* Clear multicast table */
	for (i = 0; i < 128; i++)
		e1000_wr(hw, E1000_MTA + i * 4, 0);

	/* Link setup: auto-speed detect, set link up */
	ctrl = e1000_rd(hw, E1000_CTRL);
	ctrl |= E1000_CTRL_SLU | E1000_CTRL_FD;
	ctrl &= ~(E1000_CTRL_LRST | E1000_CTRL_ILOS | E1000_CTRL_PHY_RST);
	if (hw->is_pcie_family)
		ctrl &= ~(E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
	else
		ctrl |= E1000_CTRL_ASDE;
	e1000_wr(hw, E1000_CTRL, ctrl);

	e1000_init_rx(hw);
	e1000_init_tx(hw);

	for (i = 0; i < E1000_LINK_TIMEOUT_MS; i += 10) {
		if (ax_e1000_link_up(hw))
			break;
		mdelay(10);
	}

	hw->up = ax_e1000_link_up(hw);
	printf("e1000: MAC %02x:%02x:%02x:%02x:%02x:%02x link %s\n",
	       hw->mac[0], hw->mac[1], hw->mac[2],
	       hw->mac[3], hw->mac[4], hw->mac[5],
	       hw->up ? "up" : "down");

	return 0;
}

void ax_e1000_stop(struct ax_e1000 *hw)
{
	if (!hw->regs)
		return;

	e1000_wr(hw, E1000_RCTL, 0);
	e1000_wr(hw, E1000_TCTL, E1000_TCTL_PSP);
	e1000_wr(hw, E1000_IMC, 0xffffffff);
	hw->up = false;
}

int ax_e1000_send(struct ax_e1000 *hw, const void *pkt, int len)
{
	struct ax_e1000_tx_desc *d;
	u8 *buf;
	int i;

	if (!hw->up || len <= 0 || len > AX_E1000_BUF_SIZE)
		return -EINVAL;

	d = &hw->tx_ring[hw->tx_cur];
	buf = hw->tx_buf + hw->tx_cur * AX_E1000_BUF_SIZE;

	/* Wait until this slot has been consumed by hardware */
	for (i = 0; i < E1000_TX_TIMEOUT_US; i += 10) {
		e1000_inval(d, sizeof(*d));
		if (d->status & E1000_TXD_STAT_DD)
			break;
		udelay(10);
	}
	if (!(d->status & E1000_TXD_STAT_DD))
		return -ETIMEDOUT;

	memcpy(buf, pkt, len);
	e1000_flush(buf, len);

	d->addr = e1000_dma_addr(buf);
	d->length = len;
	d->cso = 0;
	d->css = 0;
	d->special = 0;
	d->status = 0;
	d->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
	e1000_flush(d, sizeof(*d));

	hw->tx_cur = (hw->tx_cur + 1) % AX_E1000_NUM_TX;
	e1000_wr(hw, E1000_TDT, hw->tx_cur);

	/* Wait for completion */
	for (i = 0; i < E1000_TX_TIMEOUT_US; i += 10) {
		e1000_inval(d, sizeof(*d));
		if (d->status & E1000_TXD_STAT_DD)
			return 0;
		udelay(10);
	}

	return -ETIMEDOUT;
}

/* Returns packet length, 0 if nothing received, <0 on error */
int ax_e1000_recv(struct ax_e1000 *hw, void *buf, int maxlen)
{
	struct ax_e1000_rx_desc *d = &hw->rx_ring[hw->rx_cur];
	u8 *rxb = hw->rx_buf + hw->rx_cur * AX_E1000_BUF_SIZE;
	int len;

	if (!hw->up)
		return -EINVAL;

	e1000_inval(d, sizeof(*d));
	if (!(d->status & E1000_RXD_STAT_DD))
		return 0;

	len = d->length;
	if (!(d->status & E1000_RXD_STAT_EOP) || d->errors)
		len = -EIO;

	if (len > 0) {
		e1000_inval(rxb, len);
		if (len > maxlen)
			len = maxlen;
		memcpy(buf, rxb, len);
	}

	/* Give the descriptor back to hardware */
	d->status = 0;
	d->errors = 0;
	d->length = 0;
	e1000_flush(d, sizeof(*d));
	e1000_wr(hw, E1000_RDT, hw->rx_cur);
	hw->rx_cur = (hw->rx_cur + 1) % AX_E1000_NUM_RX;

	return len;
}

/* ---------------------------------------------------------------- */
/* U-Boot test command                                              */
/*   ax_e1000 init <port>                                           */
/*   ax_e1000 status                                                */
/*   ax_e1000 send                                                  */
/*   ax_e1000 recv [timeout_ms]                                     */
/*   ax_e1000 stop                                                  */
/* ---------------------------------------------------------------- */
static struct ax_e1000 g_e1000;

static int do_ax_e1000(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	struct ax_e1000 *hw = &g_e1000;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "init")) {
		int port = argc > 2 ? dectoul(argv[2], NULL) : 0;

		if (ax_e1000_probe(hw, port))
			return CMD_RET_FAILURE;
		if (ax_e1000_start(hw))
			return CMD_RET_FAILURE;
		return CMD_RET_SUCCESS;
	}

	if (!hw->regs) {
		printf("e1000: run 'ax_e1000 init' first\n");
		return CMD_RET_FAILURE;
	}

	if (!strcmp(argv[1], "status")) {
		printf("link %s, STATUS 0x%08x, CTRL 0x%08x\n",
		       ax_e1000_link_up(hw) ? "up" : "down",
		       e1000_rd(hw, E1000_STATUS), e1000_rd(hw, E1000_CTRL));
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "send")) {
		u8 frame[64];
		int ret;

		memset(frame, 0, sizeof(frame));
		memset(frame, 0xff, 6);			/* dst: broadcast */
		memcpy(frame + 6, hw->mac, 6);		/* src */
		frame[12] = 0x88;			/* experimental ethertype */
		frame[13] = 0xb5;
		strcpy((char *)frame + 14, "ax_e1000 test frame");

		ret = ax_e1000_send(hw, frame, sizeof(frame));
		printf("send: %d\n", ret);
		return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "recv")) {
		u8 buf[AX_E1000_BUF_SIZE];
		ulong timeout = argc > 2 ? dectoul(argv[2], NULL) : 1000;
		ulong start = get_timer(0);
		int len;

		do {
			len = ax_e1000_recv(hw, buf, sizeof(buf));
			if (len != 0)
				break;
		} while (get_timer(start) < timeout);

		if (len > 0) {
			print_buffer(0, buf, 1, len, 16);
			return CMD_RET_SUCCESS;
		}
		printf("recv: %s\n", len ? "error" : "timeout");
		return CMD_RET_FAILURE;
	}

	if (!strcmp(argv[1], "stop")) {
		ax_e1000_stop(hw);
		return CMD_RET_SUCCESS;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(ax_e1000, 3, 0, do_ax_e1000,
	   "Intel e1000 PCIe NIC test",
	   "init [port] - probe and start NIC\n"
	   "ax_e1000 status      - show link status\n"
	   "ax_e1000 send        - send a broadcast test frame\n"
	   "ax_e1000 recv [ms]   - receive one frame\n"
	   "ax_e1000 stop        - stop NIC");
