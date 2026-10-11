// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __PCIE_H
#define __PCIE_H

#include <stdio.h>
#include <stdbool.h>
#include <irq_func.h>

struct ax_pcie_bar {
	u64 addr;
	u64 size;
	bool is_io;
	bool is_64bit;
	bool prefetch;
	int index;
};

struct ax_pci_region {
	void __iomem *bus_start;	/* Start on the bus */
	void __iomem *phys_start;	/* Start in physical address space */
	size_t size;			/* Size */
	unsigned long flags;		/* Resource flags */
};

struct ax_pcie_msi {
	u64 address;
	u16 data;

	u8 vector;
    u8 mme;          /* MME encoding: 0..5 */
    u8 nr_vectors;   /* Requested messages: 1, 2, 4, 8, 16, 32 */

    void (*handler)(void *arg);
    void *arg;

	bool enabled;
};

struct ax_pcie_ops {
	/* Common function */
	int (*init)(u8 port, u8 controller_mode, u8 speed);
	int (*reset)(u8 port);
	int (*debug_print)(u8 port);

	/* RP function */
	int (*get_regions)(u8 port,
			   struct ax_pci_region **regions,
			   int *region_cnt);

	int (*cfg_read)(int port, u8 bus, u8 dev, u8 func,
			u16 offset, u32 *val);

	int (*cfg_write)(int port, u8 bus, u8 dev, u8 func,
			 u16 offset, u32 val);

	int (*mem_read)(u8 port, u64 offset, void *buf, size_t len);
	int (*mem_write)(u8 port, u64 offset, const void *buf, size_t len);
    struct ax_pcie_msi * (*request_msi_handle)(u8 port, 
                                                   interrupt_handler_t *handler, 
                                                   void *arg, 
                                                   u8 nr_vectors);

	/* EP function */
	int (*set_msi)(u8 port, uint func_num, uint interrupt);
};

extern struct ax_pcie_ops pcie;

/*
 * Common
 */
static inline int ax_pcie_init(u8 port, u8 controller_mode, u8 speed)
{
	if (pcie.init == NULL)
		return -ENOSYS;

	return pcie.init(port, controller_mode, speed);
}

static inline int ax_pcie_debug_print(u8 port)
{
	if (pcie.debug_print == NULL)
		return -ENOSYS;

	return pcie.debug_print(port);
}

/*
 * PCIe Configuration Space
 */
static inline int ax_pcie_cfg_read(int port, u8 bus, u8 dev, u8 func,
				   u16 offset, u32 *val)
{
	if (pcie.cfg_read == NULL)
		return -ENOSYS;

	return pcie.cfg_read(port, bus, dev, func, offset, val);
}

static inline int ax_pcie_cfg_write(int port, u8 bus, u8 dev, u8 func,
				    u16 offset, u32 val)
{
	if (pcie.cfg_write == NULL)
		return -ENOSYS;

	return pcie.cfg_write(port, bus, dev, func, offset, val);
}

/*
 * Read 8-bit configuration register
 */
static inline int ax_pcie_cfg_read8(int port, u8 bus, u8 dev, u8 func,
				    u16 offset, u8 *val)
{
	u32 temp;
	u32 shift;
	int ret;

	shift = (offset & 0x3) * 8;

	ret = ax_pcie_cfg_read(port, bus, dev, func,
			       offset & ~0x3, &temp);
	if (ret)
		return ret;

	*val = (u8)(temp >> shift);

	return 0;
}

/*
 * Write 8-bit configuration register
 */
static inline int ax_pcie_cfg_write8(int port, u8 bus, u8 dev, u8 func,
				     u16 offset, u8 val)
{
	u32 temp;
	u32 shift;
	u32 mask;
	int ret;

	shift = (offset & 0x3) * 8;
	mask = 0xffU << shift;

	ret = ax_pcie_cfg_read(port, bus, dev, func,
			       offset & ~0x3, &temp);
	if (ret)
		return ret;

	temp = (temp & ~mask) | ((u32)val << shift);

	return ax_pcie_cfg_write(port, bus, dev, func,
				 offset & ~0x3, temp);
}

/*
 * Read 16-bit configuration register
 */
static inline int ax_pcie_cfg_read16(int port, u8 bus, u8 dev, u8 func,
				     u16 offset, u16 *val)
{
	u32 temp;
	u32 shift;
	int ret;

	shift = (offset & 0x2) * 8;

	ret = ax_pcie_cfg_read(port, bus, dev, func,
			       offset & ~0x3, &temp);
	if (ret)
		return ret;

	*val = (u16)(temp >> shift);

	return 0;
}

/*
 * Write 16-bit configuration register
 */
static inline int ax_pcie_cfg_write16(int port, u8 bus, u8 dev, u8 func,
				      u16 offset, u16 val)
{
	u32 temp;
	u32 shift;
	u32 mask;
	int ret;

	shift = (offset & 0x2) * 8;
	mask = 0xffffU << shift;

	ret = ax_pcie_cfg_read(port, bus, dev, func,
			       offset & ~0x3, &temp);
	if (ret)
		return ret;

	temp = (temp & ~mask) | ((u32)val << shift);

	return ax_pcie_cfg_write(port, bus, dev, func,
				 offset & ~0x3, temp);
}

/*
 * Read 32-bit configuration register
 */
static inline int ax_pcie_cfg_read32(int port, u8 bus, u8 dev, u8 func,
				     u16 offset, u32 *val)
{
	return ax_pcie_cfg_read(port, bus, dev, func, offset, val);
}

/*
 * Write 32-bit configuration register
 */
static inline int ax_pcie_cfg_write32(int port, u8 bus, u8 dev, u8 func,
				      u16 offset, u32 val)
{
	return ax_pcie_cfg_write(port, bus, dev, func, offset, val);
}

/*
 * Read 64-bit configuration register
 *
 * PCIe config space is accessed as two 32-bit accesses.
 */
static inline int ax_pcie_cfg_read64(int port, u8 bus, u8 dev, u8 func,
				     u16 offset, u64 *val)
{
	u32 lo;
	u32 hi;
	int ret;

	ret = ax_pcie_cfg_read32(port, bus, dev, func,
				 offset, &lo);
	if (ret)
		return ret;

	ret = ax_pcie_cfg_read32(port, bus, dev, func,
				 offset + 4, &hi);
	if (ret)
		return ret;

	*val = ((u64)hi << 32) | lo;

	return 0;
}

/*
 * Write 64-bit configuration register
 *
 * PCIe config space is accessed as two 32-bit accesses.
 */
static inline int ax_pcie_cfg_write64(int port, u8 bus, u8 dev, u8 func,
				      u16 offset, u64 val)
{
	int ret;

	ret = ax_pcie_cfg_write32(port, bus, dev, func,
				  offset, (u32)val);
	if (ret)
		return ret;

	return ax_pcie_cfg_write32(port, bus, dev, func,
				   offset + 4, (u32)(val >> 32));
}

/*
 * PCIe Memory Space
 */
static inline int ax_pcie_mem_read(u8 port, u64 offset,
				   void *buf, size_t len)
{
	if (pcie.mem_read == NULL)
		return -ENOSYS;

	return pcie.mem_read(port, offset, buf, len);
}

static inline int ax_pcie_mem_write(u8 port, u64 offset,
				    const void *buf, size_t len)
{
	if (pcie.mem_write == NULL)
		return -ENOSYS;

	return pcie.mem_write(port, offset, buf, len);
}

/*
 * PCIe Regions
 */
static inline int ax_pcie_get_regions(u8 port,
				      struct ax_pci_region **regions,
				      int *region_cnt)
{
	if (pcie.get_regions == NULL)
		return -ENOSYS;

	return pcie.get_regions(port, regions, region_cnt);
}

/*
 * PCIe RP MSI / MSI-X
 */
static inline struct ax_pcie_msi *ax_pcie_request_msi_handle(u8 port, 
                                                   interrupt_handler_t *handler, 
                                                   void *arg, 
                                                   u8 nr_vectors)
{
	if (pcie.request_msi_handle == NULL)
		return NULL;

	return pcie.request_msi_handle(port, handler, arg, nr_vectors);
}

/*
 * PCIe EP MSI / MSI-X
 */
static inline int ax_pcie_set_msi(u8 port, uint func_num, uint interrupt)
{
	if (pcie.set_msi == NULL)
		return -ENOSYS;

	return pcie.set_msi(port, func_num, interrupt);
}

#endif /* __PCIE_H */
