/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __XHCI_HW_H
#define __XHCI_HW_H

#include "ax_xhci_pci.h"

/*
 * Hardware access layer for the xHCI core. The core only uses these
 * accessors, so the same code runs against:
 *   - "pcie": BAR0 reached through ax_pcie_mem_read/write (RP side)
 *   - "mmio": the controller's register block mapped directly in the CPU
 *             address space (EP side / SoC-local test, no PCIe involved)
 */
struct xhci_hw_ops {
	const char *name;
	u32  (*rd32)(void *priv, u32 off);
	void (*wr32)(void *priv, u32 off, u32 val);
};

struct xhci_hw {
	const struct xhci_hw_ops *ops;
	void *priv;

	/* register block layout (filled by read_caps / attach) */
	u8  cap_len;
	u16 hci_version;
	u32 op_off, rt_off, db_off;
	bool msi;
};

/*
 * CPU address -> address the controller must use for DMA.
 * 0 offset = identity (typical for SoC-local MMIO backend).
 */
extern u64 xhci_dma_offset;

static inline u64 xhci_cpu_to_bus(const void *p)
{
	return (u64)(uintptr_t)p + xhci_dma_offset;
}

static inline u32 xhci_hw_rd32(struct xhci_hw *hw, u32 off)
{
	return hw->ops->rd32(hw->priv, off);
}

static inline void xhci_hw_wr32(struct xhci_hw *hw, u32 off, u32 val)
{
	hw->ops->wr32(hw->priv, off, val);
}

/* 64-bit registers are accessed as two dwords, low first */
static inline u64 xhci_hw_rd64(struct xhci_hw *hw, u32 off)
{
	u64 lo = xhci_hw_rd32(hw, off);
	u64 hi = xhci_hw_rd32(hw, off + 4);

	return (hi << 32) | lo;
}

static inline void xhci_hw_wr64(struct xhci_hw *hw, u32 off, u64 val)
{
	xhci_hw_wr32(hw, off, (u32)val);
	xhci_hw_wr32(hw, off + 4, (u32)(val >> 32));
}

/* Read + validate capability registers; fills cap_len/op_off/rt_off/db_off */
int xhci_hw_read_caps(struct xhci_hw *hw);

/* Backends */
int xhci_hw_attach_pcie(struct xhci_hw *hw, struct ax_xhci_pci *pci);
/* base: CPU-visible (mapped) pointer to the xHCI capability registers */
int xhci_hw_attach_mmio(struct xhci_hw *hw, void *base, u64 dma_offset);

#endif