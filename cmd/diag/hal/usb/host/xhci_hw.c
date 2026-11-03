// SPDX-License-Identifier: GPL-2.0+
#include <dm.h>
#include <asm/io.h>
#include "xhci_hw.h"

u64 xhci_dma_offset;

/* ---- backend: PCIe (BAR0 via ax_pcie_mem_*) ---- */
static u32 pcie_rd32(void *priv, u32 off)
{
	return ax_xhci_rd32(priv, off);
}

static void pcie_wr32(void *priv, u32 off, u32 val)
{
	ax_xhci_wr32(priv, off, val);
}

static const struct xhci_hw_ops pcie_ops = {
	.name = "pcie",
	.rd32 = pcie_rd32,
	.wr32 = pcie_wr32,
};

/* ---- backend: direct MMIO in the CPU address space ---- */
static u32 mmio_rd32(void *priv, u32 off)
{
	return readl((u8 *)priv + off);
}

static void mmio_wr32(void *priv, u32 off, u32 val)
{
	writel(val, (u8 *)priv + off);
}

static const struct xhci_hw_ops mmio_ops = {
	.name = "mmio",
	.rd32 = mmio_rd32,
	.wr32 = mmio_wr32,
};

/* ---- common ---- */
int xhci_hw_read_caps(struct xhci_hw *hw)
{
	u32 v = xhci_hw_rd32(hw, XHCI_CAPLENGTH);

	hw->cap_len = v & 0xff;
	hw->hci_version = v >> 16;
	if (v == 0xffffffff || hw->hci_version < 0x0090 || hw->hci_version > 0x0120) {
		printf("xhci: bad CAPLENGTH/HCIVERSION 0x%08x (wrong base / no access?)\n", v);
		return -ENODEV;
	}
	hw->op_off = hw->cap_len;
	hw->db_off = xhci_hw_rd32(hw, XHCI_DBOFF) & ~3U;
	hw->rt_off = xhci_hw_rd32(hw, XHCI_RTSOFF) & ~0x1fU;
	return 0;
}

int xhci_hw_attach_pcie(struct xhci_hw *hw, struct ax_xhci_pci *pci)
{
	memset(hw, 0, sizeof(*hw));
	hw->ops = &pcie_ops;
	hw->priv = pci;

	/* layout was already read by ax_xhci_pci_enable() */
	hw->cap_len = pci->cap_len;
	hw->hci_version = pci->hci_version;
	hw->op_off = pci->op_off;
	hw->rt_off = pci->rt_off;
	hw->db_off = pci->db_off;
	hw->msi = pci->msi != NULL;

	xhci_dma_offset = AX_XHCI_PCI_DMA_OFFSET;
	return 0;
}

int xhci_hw_attach_mmio(struct xhci_hw *hw, void *base, u64 dma_offset)
{
	memset(hw, 0, sizeof(*hw));
	hw->ops = &mmio_ops;
	hw->priv = base;
	xhci_dma_offset = dma_offset;
	return xhci_hw_read_caps(hw);
}