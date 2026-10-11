/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <stdbool.h>
#include <linux/printk.h>
#include <time.h>
#include <linux/delay.h>
#include <malloc.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "ax_pcie.h"
#include "ax_pcie_conf.h"


/*
 * ---------------------------------------------------------------------------
 * Endpoint BAR pool
 * ---------------------------------------------------------------------------
 *
 * Endpoint BARs are carved out of the first usable outbound memory window
 * that the controller reports through ax_pcie_get_regions().  The window's
 * bus_start is the PCI bus address of its first byte, which is what has to
 * be programmed into the BARs (phys_start is the CPU-side address and is
 * not used here).
 */

#define AX_PCIE_MAX_REGIONS		8

/* A 32-bit memory BAR can only decode addresses below this limit */
#define AX_PCIE_4G			0x100000000ULL

/*
 * PCI address 0 is deliberately NOT handed out: a BAR that reads back 0 is
 * indistinguishable from an unassigned BAR.
 */
#define AX_PCIE_BAR_MIN_ADDR		0x1000ULL

static u64 ax_pcie_bar_next;
static u64 ax_pcie_bar_end;
static int ax_pcie_bar_port = -1;
static bool ax_pcie_bar_ready;

static u64 ax_pcie_align(u64 value, u64 align)
{
	return (value + align - 1) & ~(align - 1);
}

/* Forget the current pool; the next BAR init reloads it from the port */
void ax_pcie_reset_bar_pool(void)
{
	ax_pcie_bar_ready = false;
	ax_pcie_bar_port = -1;
	ax_pcie_bar_next = 0;
	ax_pcie_bar_end = 0;
}

/*
 * Can this region hold endpoint MMIO BARs?  I/O windows and inbound
 * (system memory) ranges cannot.  The flags are interpreted as U-Boot
 * PCI_REGION_* values.
 */
static bool ax_pcie_region_usable(const struct ax_pci_region *region)
{
	if (!region->size)
		return false;

	if (region->flags & (PCI_REGION_TYPE | PCI_REGION_SYS_MEMORY))
		return false;

	return true;
}

/*
 * Set up the BAR pool from the port's outbound memory window.  Does nothing
 * if the pool is already loaded for this port.
 */
static int ax_pcie_load_bar_pool(int port)
{
	struct ax_pci_region *regions;
	int cnt;
	int ret;
	int i;

	if (ax_pcie_bar_ready && ax_pcie_bar_port == port)
		return 0;

	ret = ax_pcie_get_regions(port, &regions, &cnt);
	if (ret) {
		printf("PCIe port %d: cannot get regions: %d\n", port, ret);
		return ret;
	}

	if (cnt > AX_PCIE_MAX_REGIONS)
		cnt = AX_PCIE_MAX_REGIONS;

	for (i = 0; i < cnt; i++) {
		u64 base = (u64)regions[i].bus_start;
		u64 end = base + regions[i].size;

		if (!ax_pcie_region_usable(&regions[i]) || end < base)
			continue;

		ax_pcie_bar_next = base ? base : AX_PCIE_BAR_MIN_ADDR;
		ax_pcie_bar_end = end;
		ax_pcie_bar_port = port;
		ax_pcie_bar_ready = true;

		printf("PCIe port %d: BAR pool bus 0x%llx-0x%llx\n",
		       port,
		       (unsigned long long)base,
		       (unsigned long long)(end - 1));

		return 0;
	}

	printf("PCIe port %d: no outbound memory region for BARs\n", port);

	return -ENODEV;
}

/*
 * Carve @size bytes (naturally aligned) out of the BAR pool.
 *
 * @addr_32bit: the BAR is a 32-bit BAR, so the block must also end below
 *              4 GiB.  Returns -ENOMEM when the pool cannot satisfy that,
 *              which is not an error for the pool itself.
 */
static int ax_pcie_alloc_bar(u64 size, bool addr_32bit, u64 *addr)
{
	u64 end = ax_pcie_bar_end;
	u64 base;

	/* BAR sizes are always a power of two */
	if (!size || !addr || (size & (size - 1)))
		return -EINVAL;

	if (!ax_pcie_bar_ready)
		return -ENODEV;

	if (addr_32bit && end > AX_PCIE_4G)
		end = AX_PCIE_4G;

	base = ax_pcie_align(ax_pcie_bar_next, size);

	if (base + size > end)
		return -ENOMEM;

	*addr = base;
	ax_pcie_bar_next = base + size;

	return 0;
}

int ax_pcie_wait_device_ready(int port, u8 bus, u8 dev, u8 func,
			      u32 timeout_ms)
{
	ulong start = get_timer(0);
	u32 id = 0xffffffff;
	int ret;

	do {
		ret = ax_pcie_cfg_read(port, bus, dev, func,
				       PCI_CFG_VENDOR_ID, &id);
		if (ret)
			return ret;

		/* 0xffff = nobody home, 0x0001 = CRS (device still busy) */
		if ((id & 0xffff) != 0xffff && (id & 0xffff) != 0x0001 && id)
			return 0;

		mdelay(10);
	} while (get_timer(start) < timeout_ms);

	printf("PCIe %02x:%02x.%x not ready, vendor/device = 0x%08x\n",
	       bus, dev, func, id);

	return -ETIMEDOUT;
}

int ax_pcie_find_cap(u8 port, u8 bus, u8 dev, u8 func,
                          int cap_type, u16 cap_id)
{
    u32 val;
    u16 offset;
    u16 next;
    int ret;
    int count = 0;

    if (cap_type) {
        /*
         * PCIe Extended Capability list starts at 0x100.
         *
         * Header:
         *   [15:0]  Capability ID
         *   [19:16] Version
         *   [31:20] Next Capability Pointer
         */
        offset = 0x100;

        while (offset && count++ < 256) {
            ret = ax_pcie_cfg_read(port,
                                   bus, dev, func,
                                   offset, &val);
            if (ret)
                return ret;

            if (val == 0xffffffff || val == 0x00000000)
                break;

            if ((val & PCI_EXT_CAP_ID_MASK) == cap_id)
                return offset;

            next = (val & PCI_EXT_CAP_NEXT_MASK) >> PCI_EXT_CAP_NEXT_SHIFT;

            if (next <= offset)
                break;

            offset = next;
        }
    } else {
        /*
         * Legacy PCI Capability list.
         *
         * Capability Pointer is at 0x34.
         */
        ret = ax_pcie_cfg_read(port,
                               bus, dev, func,
                               PCI_CFG_CAP_PTR, &val);
        if (ret)
            return ret;

        offset = val & 0xfc;

        while (offset >= 0x40 && count++ < 48) {
            ret = ax_pcie_cfg_read(port,
                                   bus, dev, func,
                                   offset, &val);
            if (ret)
                return ret;

            if (((val >> (PCI_CAP_ID * 8)) & 0xff) == cap_id)
                return offset;

            offset = (val >> (PCI_CAP_NEXT * 8)) & 0xfc;
        }
    }

    return -ENODEV;
}

/*
 * ---------------------------------------------------------------------------
 * String helpers
 * ---------------------------------------------------------------------------
 */

static const char *ax_pcie_speed_name(u32 speed)
{
	switch (speed) {
	case 1:
		return "2.5 GT/s";
	case 2:
		return "5.0 GT/s";
	case 3:
		return "8.0 GT/s";
	case 4:
		return "16.0 GT/s";
	case 5:
		return "32.0 GT/s";
	case 6:
		return "64.0 GT/s";
	default:
		return "Unknown";
	}
}

static const char *ax_pcie_type_name(u8 type)
{
	switch (type) {
	case PCI_EXP_TYPE_ENDPOINT:
		return "Endpoint";

	case PCI_EXP_TYPE_LEGACY_ENDPOINT:
		return "Legacy Endpoint";

	case PCI_EXP_TYPE_ROOT_PORT:
		return "Root Port";

	case PCI_EXP_TYPE_UPSTREAM_PORT:
		return "Upstream Port";

	case PCI_EXP_TYPE_DOWNSTREAM_PORT:
		return "Downstream Port";

	case PCI_EXP_TYPE_PCIE_BRIDGE:
		return "PCIe Bridge";

	case PCI_EXP_TYPE_RC_ENDPOINT:
		return "Root Complex Endpoint";

	case PCI_EXP_TYPE_RC_EVENT_COLLECTOR:
		return "Root Complex Event Collector";

	default:
		return "Unknown";
	}
}

static const char *ax_pcie_pm_state_name(u16 pmcsr)
{
	switch (pmcsr & PCI_PM_CTRL_STATE_MASK) {
	case 0:
		return "D0";
	case 1:
		return "D1";
	case 2:
		return "D2";
	case 3:
		return "D3hot";
	default:
		return "Unknown";
	}
}

static const char *ax_pcie_ext_cap_name(u16 id)
{
	switch (id) {
	case PCI_EXT_CAP_ID_AER:
		return "Advanced Error Reporting";

	case PCI_EXT_CAP_ID_VC:
		return "Virtual Channel";

	case PCI_EXT_CAP_ID_DSN:
		return "Device Serial Number";

	case PCI_EXT_CAP_ID_RCLD:
		return "Root Complex Link Declaration";

	case PCI_EXT_CAP_ID_RBER:
		return "Root Complex Event Collector";

	case PCI_EXT_CAP_ID_ACS:
		return "Access Control Services";

	case PCI_EXT_CAP_ID_ARI:
		return "Alternative Routing-ID Interpretation";

	case PCI_EXT_CAP_ID_ATS:
		return "Address Translation Service";

	case PCI_EXT_CAP_ID_SRIOV:
		return "Single Root I/O Virtualization";

	case PCI_EXT_CAP_ID_PRI:
		return "Page Request Interface";

	case PCI_EXT_CAP_ID_REBAR:
		return "Resizable BAR";

	case PCI_EXT_CAP_ID_TPH:
		return "TPH Requester";

	case PCI_EXT_CAP_ID_LTR:
		return "Latency Tolerance Reporting";

	case PCI_EXT_CAP_ID_DPC:
		return "Downstream Port Containment";

	case PCI_EXT_CAP_ID_L1SS:
		return "L1 PM Substates";

	case PCI_EXT_CAP_ID_PTM:
		return "Precision Time Measurement";

	case PCI_EXT_CAP_ID_DVSEC:
		return "Designated Vendor-Specific";

	case PCI_EXT_CAP_ID_IDE:
		return "Integrity and Data Encryption";

	default:
		return "Unknown";
	}
}

/*
 * ---------------------------------------------------------------------------
 * PCI Header
 * ---------------------------------------------------------------------------
 */

static void ax_pcie_print_command(u16 cmd)
{
	printf("\tCommand: 0x%04x\n", cmd);

	printf("\t\tI/O Space:       %s\n",
	       (cmd & PCI_COMMAND_IO) ? "Enable" : "Disable");

	printf("\t\tMemory Space:    %s\n",
	       (cmd & PCI_COMMAND_MEMORY) ? "Enable" : "Disable");

	printf("\t\tBus Master:      %s\n",
	       (cmd & PCI_COMMAND_MASTER) ? "Enable" : "Disable");

	printf("\t\tSERR:            %s\n",
	       (cmd & PCI_COMMAND_SERR) ? "Enable" : "Disable");

	printf("\t\tINTx Disable:    %s\n",
	       (cmd & PCI_COMMAND_INTX_DISABLE) ? "yes" : "no");
}

static void ax_pcie_print_status(u16 status)
{
	printf("\tStatus: 0x%04x\n", status);

	if (status & PCI_STATUS_CAP_LIST)
		printf("\t\tCapabilities List: yes\n");

	if (status & PCI_STATUS_66MHZ)
		printf("\t\t66 MHz: yes\n");

	if (status & PCI_STATUS_PARITY)
		printf("\t\tParity Error\n");

	if (status & PCI_STATUS_SIG_TARGET_ABORT)
		printf("\t\tSignaled Target Abort\n");

	if (status & PCI_STATUS_REC_TARGET_ABORT)
		printf("\t\tReceived Target Abort\n");

	if (status & PCI_STATUS_REC_MASTER_ABORT)
		printf("\t\tReceived Master Abort\n");

	if (status & PCI_STATUS_SIG_SYSTEM_ERROR)
		printf("\t\tSignaled System Error\n");

	if (status & PCI_STATUS_DETECTED_PARITY)
		printf("\t\tDetected Parity Error\n");
}

static void ax_pcie_print_bar(int port, u8 bus, u8 dev, u8 func,
			      int index, int nbars, u32 bar)
{
	u32 upper;
	u64 addr;

	if (!bar) {
		printf("\tBAR%d: disabled\n", index);
		return;
	}

	if (bar & PCI_BAR_IO) {
		printf("\tBAR%d: I/O 0x%08x\n",
		       index,
		       bar & PCI_BAR_IO_ADDR_MASK);
		return;
	}

	addr = bar & PCI_BAR_ADDR_MASK;

	if ((bar & PCI_BAR_MEM_TYPE_MASK) == PCI_BAR_MEM_TYPE_64 &&
	    index < nbars - 1) {
		if (ax_pcie_cfg_read32(port, bus, dev, func,
				       PCI_CFG_BAR0 + (index + 1) * 4,
				       &upper))
			return;

		addr |= (u64)upper << 32;

		printf("\tBAR%d: Memory 0x%016llx",
		       index,
		       (unsigned long long)addr);

		if (bar & PCI_BAR_PREFETCH)
			printf(" prefetchable");

		printf(" 64-bit\n");
	} else {
		printf("\tBAR%d: Memory 0x%08x",
		       index,
		       bar & PCI_BAR_ADDR_MASK);

		if (bar & PCI_BAR_PREFETCH)
			printf(" prefetchable");

		printf("\n");
	}
}

static int ax_pcie_probe_bar(int port, u8 bus, u8 dev, u8 func,
			     int index, int nbars, struct ax_pcie_bar *bar)
{
	u32 orig_lo, orig_hi = 0;
	u32 mask_lo, mask_hi = 0;
	u64 mask;

	if (index < 0 || index >= nbars)
		return -EINVAL;

	memset(bar, 0, sizeof(*bar));
	bar->index = index;

	if (ax_pcie_cfg_read32(port, bus, dev, func,
			       PCI_CFG_BAR0 + index * 4,
			       &orig_lo))
		return -EIO;

	bar->is_io = orig_lo & PCI_BAR_IO;

	if (!bar->is_io) {
		bar->is_64bit =
			((orig_lo & PCI_BAR_MEM_TYPE_MASK) ==
			 PCI_BAR_MEM_TYPE_64);

		/* The last BAR cannot be the low half of a 64-bit BAR */
		if (index == nbars - 1)
			bar->is_64bit = false;

		bar->prefetch = orig_lo & PCI_BAR_PREFETCH;

		if (bar->is_64bit) {
			if (ax_pcie_cfg_read32(port, bus, dev, func,
					       PCI_CFG_BAR0 +
					       (index + 1) * 4,
					       &orig_hi))
				return -EIO;
		}
	}

	/*
	 * Probe the size: write all ones and read back the mask.
	 * An unimplemented BAR reads back 0.  Do NOT decide "unused" from
	 * the current BAR value - a 32-bit BAR that has not been programmed
	 * yet legitimately reads 0.
	 */
	if (ax_pcie_cfg_write(port, bus, dev, func,
			      PCI_CFG_BAR0 + index * 4, 0xffffffff))
		return -EIO;

	if (bar->is_64bit &&
	    ax_pcie_cfg_write(port, bus, dev, func,
			      PCI_CFG_BAR0 + (index + 1) * 4, 0xffffffff))
		return -EIO;

	if (ax_pcie_cfg_read32(port, bus, dev, func,
			       PCI_CFG_BAR0 + index * 4, &mask_lo))
		return -EIO;

	if (bar->is_64bit &&
	    ax_pcie_cfg_read32(port, bus, dev, func,
			       PCI_CFG_BAR0 + (index + 1) * 4, &mask_hi))
		return -EIO;

	/* Restore BAR */
	ax_pcie_cfg_write(port, bus, dev, func,
			  PCI_CFG_BAR0 + index * 4, orig_lo);

	if (bar->is_64bit)
		ax_pcie_cfg_write(port, bus, dev, func,
				  PCI_CFG_BAR0 + (index + 1) * 4, orig_hi);

	if (bar->is_io) {
		u32 m = mask_lo & PCI_BAR_IO_ADDR_MASK;

		bar->size = (u32)(~m + 1);	/* stay in 32 bits */
	} else if (bar->is_64bit) {
		mask = ((u64)mask_hi << 32) | (mask_lo & PCI_BAR_ADDR_MASK);
		bar->size = ~mask + 1;
	} else {
		u32 m = mask_lo & PCI_BAR_ADDR_MASK;

		/* 32-bit BAR: must not be sign-extended to 64 bits */
		bar->size = (u32)(~m + 1);
	}

	return 0;
}

static int ax_pcie_program_bar(int port,
			       u8 bus, u8 dev, u8 func,
			       struct ax_pcie_bar *bar)
{
	u32 bar_lo;
	int ret;

	if (!bar->size)
		return 0;

	if (bar->is_io)
		return -ENOTSUPP;

	/* lower_32_bits() below would silently wrap a 32-bit BAR above 4 GiB */
	if (!bar->is_64bit && bar->addr + bar->size > AX_PCIE_4G)
		return -ERANGE;

	bar_lo = lower_32_bits(bar->addr);

	if (bar->is_64bit) {
		bar_lo &= PCI_BAR_ADDR_MASK;
		bar_lo |= PCI_BAR_MEM_TYPE_64;

		if (bar->prefetch)
			bar_lo |= PCI_BAR_PREFETCH;
	} else {
		bar_lo &= PCI_BAR_ADDR_MASK;

		if (bar->prefetch)
			bar_lo |= PCI_BAR_PREFETCH;
	}

	ret = ax_pcie_cfg_write(port, bus, dev, func,
				PCI_CFG_BAR0 + bar->index * 4,
				bar_lo);
	if (ret)
		return ret;

	if (bar->is_64bit) {
		ret = ax_pcie_cfg_write(port, bus, dev, func,
					PCI_CFG_BAR0 +
					(bar->index + 1) * 4,
					upper_32_bits(bar->addr));
		if (ret)
			return ret;
	}

	printf("BAR%d: addr=0x%llx size=0x%llx %s\n",
	       bar->index,
	       (unsigned long long)bar->addr,
	       (unsigned long long)bar->size,
	       bar->is_64bit ? "64-bit" : "32-bit");

	return 0;
}

int ax_pcie_init_ep_bars(int port, u8 bus, u8 dev, u8 func)
{
	struct ax_pcie_bar bar;
	u16 command;
	u8 header_type;
	int nbars;
	int i;
	int ret;

	/*
	 * Type 0 (endpoint) has six BARs.  Type 1 (bridge / root port) only
	 * has two: the registers after them hold bus numbers and forwarding
	 * windows and must not be probed as BARs.
	 */
	ret = ax_pcie_cfg_read8(port, bus, dev, func,
				PCI_CFG_HEADER_TYPE, &header_type);
	if (ret)
		return ret;

	switch (header_type & PCI_HEADER_TYPE_MASK) {
	case PCI_HEADER_TYPE_NORMAL:
		nbars = 6;
		break;
	case PCI_HEADER_TYPE_BRIDGE:
		nbars = 2;
		break;
	default:
		return -EINVAL;
	}

	/* Find out where BARs may be placed before touching the device */
	ret = ax_pcie_load_bar_pool(port);
	if (ret)
		return ret;

	/*
	 * Disable Memory Space while probing/programming BARs.
	 */
	ret = ax_pcie_cfg_read16(port, bus, dev, func,
				 PCI_CFG_COMMAND, &command);
	if (ret)
		return ret;

	command &= ~(PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);

	ret = ax_pcie_cfg_write(port, bus, dev, func,
				PCI_CFG_COMMAND, command);
	if (ret)
		return ret;

	for (i = 0; i < nbars; i++) {
		memset(&bar, 0, sizeof(bar));

		ret = ax_pcie_probe_bar(port, bus, dev, func,
					i, nbars, &bar);
		if (ret) {
			printf("Failed to probe BAR%d: %d\n",
			       i, ret);
			return ret;
		}

		if (!bar.size) {
			printf("BAR%d: unused\n", i);
			continue;
		}

		printf("BAR%d size: 0x%llx\n",
		       i,
		       (unsigned long long)bar.size);

		if (bar.is_io) {
			printf("BAR%d: I/O BAR ignored\n", i);
			continue;
		}

		ret = ax_pcie_alloc_bar(bar.size, !bar.is_64bit, &bar.addr);
		if (ret == -ENOMEM && !bar.is_64bit) {
			/*
			 * The pool lives above 4 GiB, so a 32-bit BAR cannot
			 * be placed in it.  Leave it unassigned rather than
			 * programming a truncated address.
			 */
			printf("BAR%d: 32-bit BAR does not fit below 4 GiB, left unassigned\n",
			       i);
			continue;
		}
		if (ret) {
			printf("Failed to allocate BAR%d: %d\n",
			       i, ret);
			return ret;
		}

		ret = ax_pcie_program_bar(port, bus, dev, func,
					  &bar);
		if (ret)
			return ret;

		if (bar.is_64bit)
			i++;
	}

	/*
	 * Enable MMIO access.
	 *
	 * Bus Master is needed if Endpoint will initiate DMA.
	 */
	command |= PCI_COMMAND_MEMORY;
	command |= PCI_COMMAND_MASTER;
    command &= ~(PCI_COMMAND_INTX_DISABLE);

	ret = ax_pcie_cfg_write(port, bus, dev, func,
				PCI_CFG_COMMAND, command);
	if (ret)
		return ret;

	printf("Endpoint BAR initialization completed\n");

	return 0;
}

static void ax_pcie_print_header_bdf(int port,
				     u8 bus, u8 dev, u8 func)
{
	u16 vendor = 0;
	u16 device = 0;
	u16 command = 0;
	u16 status = 0;
	u16 subsystem_vendor = 0;
	u16 subsystem_device = 0;
	u8 revision = 0;
	u8 class = 0;
	u8 subclass = 0;
	u8 prog_if = 0;
	u8 header_type = 0;
	u8 int_line = 0;
	u8 int_pin = 0;
	u8 cap_ptr = 0;
	u32 bar;
	int nbars;
	int i;

	if (ax_pcie_cfg_read16(port, bus, dev, func,
			       PCI_CFG_VENDOR_ID, &vendor))
		return;

	if (vendor == 0xffff)
		return;

	ax_pcie_cfg_read16(port, bus, dev, func,
			   PCI_CFG_DEVICE_ID, &device);

	ax_pcie_cfg_read16(port, bus, dev, func,
			   PCI_CFG_COMMAND, &command);

	ax_pcie_cfg_read16(port, bus, dev, func,
			   PCI_CFG_STATUS, &status);

	ax_pcie_cfg_read8(port, bus, dev, func,
			  PCI_CFG_REVISION_ID, &revision);

	ax_pcie_cfg_read8(port, bus, dev, func,
			  PCI_CFG_PROG_IF, &prog_if);

	ax_pcie_cfg_read8(port, bus, dev, func,
			  PCI_CFG_SUBCLASS, &subclass);

	ax_pcie_cfg_read8(port, bus, dev, func,
			  PCI_CFG_CLASS_CODE, &class);

	ax_pcie_cfg_read8(port, bus, dev, func,
			  PCI_CFG_HEADER_TYPE, &header_type);

	ax_pcie_cfg_read16(port, bus, dev, func,
			   PCI_CFG_SUBSYS_VENDOR_ID,
			   &subsystem_vendor);

	ax_pcie_cfg_read16(port, bus, dev, func,
			   PCI_CFG_SUBSYS_ID,
			   &subsystem_device);

	ax_pcie_cfg_read8(port, bus, dev, func,
			  PCI_CFG_INT_LINE, &int_line);

	ax_pcie_cfg_read8(port, bus, dev, func,
			  PCI_CFG_INT_PIN, &int_pin);

	ax_pcie_cfg_read8(port, bus, dev, func,
			  PCI_CFG_CAP_PTR, &cap_ptr);

	printf("\n");
	printf("PCI device %02x:%02x.%x\n", bus, dev, func);

	printf("\tVendor ID:      0x%04x\n", vendor);
	printf("\tDevice ID:      0x%04x\n", device);

	ax_pcie_print_command(command);
	ax_pcie_print_status(status);

	printf("\tClass:          0x%02x%02x%02x\n",
	       class, subclass, prog_if);

	printf("\tRevision:       0x%02x\n", revision);

	printf("\tHeader Type:    0x%02x%s\n",
	       header_type & PCI_HEADER_TYPE_MASK,
	       header_type & PCI_HEADER_TYPE_MULTI_FUNC ?
	       " Multi-function" : "");

	printf("\tSubsystem:      0x%04x:0x%04x\n",
	       subsystem_vendor,
	       subsystem_device);

	printf("\tInterrupt:      line %u, pin %c\n",
	       int_line,
	       int_pin ? ('A' + int_pin - 1) : '-');

	printf("\tCapability Ptr: 0x%02x\n", cap_ptr);

	/* Type 1 headers (root port / bridge) only have BAR0 and BAR1 */
	nbars = ((header_type & PCI_HEADER_TYPE_MASK) ==
		 PCI_HEADER_TYPE_BRIDGE) ? 2 : 6;

	printf("\n\tBARs:\n");

	for (i = 0; i < nbars; i++) {
		if (ax_pcie_cfg_read32(port, bus, dev, func,
				       PCI_CFG_BAR0 + i * 4,
				       &bar))
			break;

		ax_pcie_print_bar(port, bus, dev, func, i, nbars, bar);

		if (!(bar & PCI_BAR_IO) &&
		    (bar & PCI_BAR_MEM_TYPE_MASK) ==
		    PCI_BAR_MEM_TYPE_64)
			i++;
	}
}

/*
 * ---------------------------------------------------------------------------
 * Power Management Capability
 * ---------------------------------------------------------------------------
 */

void ax_pcie_print_pm_cap(int port,
				 u8 bus, u8 dev, u8 func,
				 u16 offset)
{
	u16 cap = 0;
	u16 ctrl = 0;

	ax_pcie_cfg_read16(port, bus, dev, func,
			   offset + PCI_PM_CAP, &cap);

	ax_pcie_cfg_read16(port, bus, dev, func,
			   offset + PCI_PM_CTRL, &ctrl);

	printf("\t\t[%02x] Power Management\n", offset);

	printf("\t\t\tVersion: %u\n",
	       cap & PCI_PM_CAP_VERSION_MASK);

	printf("\t\t\tD1 Support: %s\n",
	       cap & PCI_PM_CAP_D1 ? "yes" : "no");

	printf("\t\t\tD2 Support: %s\n",
	       cap & PCI_PM_CAP_D2 ? "yes" : "no");

	printf("\t\t\tPME Support: 0x%02x\n",
	       (cap & PCI_PM_CAP_PME_SUPPORT_MASK) >> 11);

	printf("\t\t\tPMCSR: 0x%04x (%s)\n",
	       ctrl,
	       ax_pcie_pm_state_name(ctrl));

	printf("\t\t\tPME Enable: %s\n",
	       ctrl & PCI_PM_CTRL_PME_ENABLE ? "yes" : "no");

	printf("\t\t\tPME Status: %s\n",
	       ctrl & PCI_PM_CTRL_PME_STATUS ? "yes" : "no");
}

/*
 * ---------------------------------------------------------------------------
 * MSI Capability
 * ---------------------------------------------------------------------------
 */

void ax_pcie_print_msi_cap(int port,
				  u8 bus, u8 dev, u8 func,
				  u16 offset)
{
	u16 ctrl = 0;
	u32 addr_lo = 0;
	u32 addr_hi = 0;
	u16 data = 0;
	bool is_64bit;

	ax_pcie_cfg_read16(port, bus, dev, func,
			   offset + PCI_MSI_CTRL, &ctrl);

	ax_pcie_cfg_read32(port, bus, dev, func,
			   offset + PCI_MSI_ADDR_LO,
			   &addr_lo);

	is_64bit = ctrl & PCI_MSI_CTRL_64BIT;

	if (is_64bit) {
		ax_pcie_cfg_read32(port, bus, dev, func,
				   offset + PCI_MSI_ADDR_HI,
				   &addr_hi);

		ax_pcie_cfg_read16(port, bus, dev, func,
				   offset + PCI_MSI_DATA_64,
				   &data);
	} else {
		ax_pcie_cfg_read16(port, bus, dev, func,
				   offset + PCI_MSI_DATA_32,
				   &data);
	}

	printf("\t\t[%02x] MSI\n", offset);

	printf("\t\t\tEnable: %s\n",
	       ctrl & PCI_MSI_CTRL_ENABLE ? "yes" : "no");

	printf("\t\t\t64-bit: %s\n",
	       is_64bit ? "yes" : "no");

	printf("\t\t\tMultiple Message Capable: %u\n",
	       1U << ((ctrl & PCI_MSI_CTRL_MMC_MASK) >> PCI_MSI_FLAGS_MMC_SHIFT));

	printf("\t\t\tMultiple Message Enable: %u\n",
	       1U << ((ctrl & PCI_MSI_CTRL_MME_MASK) >> PCI_MSI_FLAGS_MME_SHIFT));

	if (is_64bit)
		printf("\t\t\tMessage Address: 0x%08x%08x\n",
		       addr_hi, addr_lo);
	else
		printf("\t\t\tMessage Address: 0x%08x\n",
		       addr_lo);

	printf("\t\t\tMessage Data: 0x%04x\n", data);
}

/*
 * ---------------------------------------------------------------------------
 * MSI-X Capability
 * ---------------------------------------------------------------------------
 */

void ax_pcie_print_msix_cap(int port,
				   u8 bus, u8 dev, u8 func,
				   u16 offset)
{
	u16 ctrl = 0;
	u32 table = 0;
	u32 pba = 0;
	u32 table_size;

	ax_pcie_cfg_read16(port, bus, dev, func,
			   offset + PCI_MSIX_CTRL, &ctrl);

	ax_pcie_cfg_read32(port, bus, dev, func,
			   offset + PCI_MSIX_TABLE, &table);

	ax_pcie_cfg_read32(port, bus, dev, func,
			   offset + PCI_MSIX_PBA, &pba);

	table_size = (ctrl & PCI_MSIX_CTRL_TABLE_SIZE_MASK) + 1;

	printf("\t\t[%02x] MSI-X\n", offset);

	printf("\t\t\tEnable: %s\n",
	       ctrl & PCI_MSIX_CTRL_ENABLE ? "yes" : "no");

	printf("\t\t\tFunction Mask: %s\n",
	       ctrl & PCI_MSIX_CTRL_FUNCTION_MASK ? "yes" : "no");

	printf("\t\t\tTable Size: %u\n", table_size);

	printf("\t\t\tTable: BAR %u, offset 0x%08x\n",
	       table & PCI_MSIX_BIR_MASK,
	       table & PCI_MSIX_OFFSET_MASK);

	printf("\t\t\tPBA: BAR %u, offset 0x%08x\n",
	       pba & PCI_MSIX_BIR_MASK,
	       pba & PCI_MSIX_OFFSET_MASK);
}

/*
 * ---------------------------------------------------------------------------
 * PCI Express Device Capability
 * ---------------------------------------------------------------------------
 */

static void ax_pcie_print_pcie_dev_cap(int port,
				       u8 bus, u8 dev, u8 func,
				       u16 offset)
{
	u32 cap = 0;
	u16 ctl = 0;
	u16 sta = 0;

	ax_pcie_cfg_read32(port, bus, dev, func,
			   offset + PCI_EXP_DEVCAP, &cap);

	ax_pcie_cfg_read16(port, bus, dev, func,
			   offset + PCI_EXP_DEVCTL, &ctl);

	ax_pcie_cfg_read16(port, bus, dev, func,
			   offset + PCI_EXP_DEVSTA, &sta);

	printf("\t\t\tDevCap: 0x%08x\n", cap);

	printf("\t\t\t\tMaxPayload: %u bytes\n",
	       128U << (cap & PCI_EXP_DEVCAP_MPS_MASK));

	printf("\t\t\t\tPhantom Functions: %u\n",
	       (cap & PCI_EXP_DEVCAP_PHANTOM_MASK) >>
	       PCI_EXP_DEVCAP_PHANTOM_SHIFT);

	printf("\t\t\t\tExtended Tag: %s\n",
	       cap & PCI_EXP_DEVCAP_EXT_TAG ? "yes" : "no");

	printf("\t\t\tDevCtl: 0x%04x\n", ctl);

	printf("\t\t\t\tCorrectable Error: %s\n",
	       ctl & PCI_EXP_DEVCTL_CERE ? "enable" : "disable");

	printf("\t\t\t\tNon-Fatal Error: %s\n",
	       ctl & PCI_EXP_DEVCTL_NFERE ? "enable" : "disable");

	printf("\t\t\t\tFatal Error: %s\n",
	       ctl & PCI_EXP_DEVCTL_FERE ? "enable" : "disable");

	printf("\t\t\t\tUnsupported Request: %s\n",
	       ctl & PCI_EXP_DEVCTL_URRE ? "enable" : "disable");

	printf("\t\t\t\tRelaxed Ordering: %s\n",
	       ctl & PCI_EXP_DEVCTL_RELAX ? "enable" : "disable");

	printf("\t\t\t\tMaxPayload: %u bytes\n",
	       128U << ((ctl & PCI_EXP_DEVCTL_MPS_MASK) >>
			PCI_EXP_DEVCTL_MPS_SHIFT));

	printf("\t\t\t\tExtended Tag: %s\n",
	       ctl & PCI_EXP_DEVCTL_EXT_TAG ? "enable" : "disable");

	printf("\t\t\t\tMaxReadReq: %u bytes\n",
	       128U << ((ctl & PCI_EXP_DEVCTL_READRQ_MASK) >>
			PCI_EXP_DEVCTL_READRQ_SHIFT));

	printf("\t\t\tDevSta: 0x%04x\n", sta);

	if (sta & PCI_EXP_DEVSTA_CED)
		printf("\t\t\t\tCorrectable Error Detected\n");

	if (sta & PCI_EXP_DEVSTA_NFED)
		printf("\t\t\t\tNon-Fatal Error Detected\n");

	if (sta & PCI_EXP_DEVSTA_FED)
		printf("\t\t\t\tFatal Error Detected\n");

	if (sta & PCI_EXP_DEVSTA_URD)
		printf("\t\t\t\tUnsupported Request Detected\n");

	if (sta & PCI_EXP_DEVSTA_AUXPD)
		printf("\t\t\t\tAUX Power Detected\n");

	if (sta & PCI_EXP_DEVSTA_TRPND)
		printf("\t\t\t\tTransaction Pending\n");
}

/*
 * ---------------------------------------------------------------------------
 * PCI Express Link Capability
 * ---------------------------------------------------------------------------
 */

static void ax_pcie_print_pcie_link_cap(int port,
					u8 bus, u8 dev, u8 func,
					u16 offset, u8 version)
{
	u32 cap = 0;
	u16 ctl = 0;
	u16 sta = 0;
	u32 cap2 = 0;
	u16 ctl2 = 0;
	u16 sta2 = 0;

	ax_pcie_cfg_read32(port, bus, dev, func,
			   offset + PCI_EXP_LNKCAP, &cap);

	ax_pcie_cfg_read16(port, bus, dev, func,
			   offset + PCI_EXP_LNKCTL, &ctl);

	ax_pcie_cfg_read16(port, bus, dev, func,
			   offset + PCI_EXP_LNKSTA, &sta);

	/* The *2 registers only exist in version 2+ of the capability */
	if (version >= 2) {
		ax_pcie_cfg_read32(port, bus, dev, func,
				   offset + PCI_EXP_LNKCAP2, &cap2);

		ax_pcie_cfg_read16(port, bus, dev, func,
				   offset + PCI_EXP_LNKCTL2, &ctl2);

		ax_pcie_cfg_read16(port, bus, dev, func,
				   offset + PCI_EXP_LNKSTA2, &sta2);
	}

	printf("\t\t\tLnkCap: 0x%08x\n", cap);

	printf("\t\t\t\tSpeed: %s\n",
	       ax_pcie_speed_name(cap & PCI_EXP_LNKCAP_SPEED_MASK));

	printf("\t\t\t\tWidth: x%u\n",
	       (cap & PCI_EXP_LNKCAP_WIDTH_MASK) >>
	       PCI_EXP_LNKCAP_WIDTH_SHIFT);

	printf("\t\t\t\tASPM: %s\n",
	       (cap & PCI_EXP_LNKCAP_ASPM_MASK) == 0 ?
	       "not supported" :
	       (cap & PCI_EXP_LNKCAP_ASPM_MASK) == 0x400 ?
	       "L0s" :
	       (cap & PCI_EXP_LNKCAP_ASPM_MASK) == 0x800 ?
	       "L1" : "L0s L1");

	printf("\t\t\t\tClockPM: %s\n",
	       cap & PCI_EXP_LNKCAP_CLOCK_PM ? "yes" : "no");

	printf("\t\t\t\tDLL Active Reporting: %s\n",
	       cap & PCI_EXP_LNKCAP_DLL_ACTIVE ? "yes" : "no");

	printf("\t\t\tLnkCtl: 0x%04x\n", ctl);

	printf("\t\t\t\tASPM: %s\n",
	       (ctl & PCI_EXP_LNKCTL_ASPM_MASK) == 0 ?
	       "Disabled" :
	       (ctl & PCI_EXP_LNKCTL_ASPM_MASK) == 1 ?
	       "L0s" :
	       (ctl & PCI_EXP_LNKCTL_ASPM_MASK) == 2 ?
	       "L1" : "L0s L1");

	printf("\t\t\t\tLink Disable: %s\n",
	       ctl & PCI_EXP_LNKCTL_LINK_DISABLE ? "yes" : "no");

	printf("\t\t\t\tRetrain: %s\n",
	       ctl & PCI_EXP_LNKCTL_RETRAIN ? "requested" : "no");

	printf("\t\t\t\tCommon Clock: %s\n",
	       ctl & PCI_EXP_LNKCTL_COMMON_CLK ? "yes" : "no");

	printf("\t\t\tLnkSta: 0x%04x\n", sta);

	printf("\t\t\t\tSpeed: %s\n",
	       ax_pcie_speed_name(sta & PCI_EXP_LNKSTA_SPEED_MASK));

	printf("\t\t\t\tWidth: x%u\n",
	       (sta & PCI_EXP_LNKSTA_WIDTH_MASK) >>
	       PCI_EXP_LNKSTA_WIDTH_SHIFT);

	printf("\t\t\t\tTraining: %s\n",
	       sta & PCI_EXP_LNKSTA_TRAINING ? "yes" : "no");

	printf("\t\t\t\tClockPM: %s\n",
	       sta & PCI_EXP_LNKSTA_CLOCK_PM ? "yes" : "no");

	printf("\t\t\t\tDLL Active: %s\n",
	       sta & PCI_EXP_LNKSTA_DLL_ACTIVE ? "yes" : "no");

	if (version >= 2) {
		printf("\t\t\tLnkCap2: 0x%08x\n", cap2);
		printf("\t\t\tLnkCtl2: 0x%04x\n", ctl2);
		printf("\t\t\tLnkSta2: 0x%04x\n", sta2);
	}
}

/*
 * ---------------------------------------------------------------------------
 * PCI Express Capability
 * ---------------------------------------------------------------------------
 */

void ax_pcie_print_pcie_cap(int port,
				   u8 bus, u8 dev, u8 func,
				   u16 offset)
{
	u16 cap = 0;
	u8 version;
	u8 type;

	ax_pcie_cfg_read16(port, bus, dev, func,
			   offset + PCI_EXP_CAP, &cap);

	version = cap & PCI_EXP_CAP_VERSION_MASK;

	type = (cap & PCI_EXP_CAP_TYPE_MASK) >>
	       PCI_EXP_CAP_TYPE_SHIFT;

	printf("\t\t[%02x] PCI Express\n", offset);

	printf("\t\t\tVersion: %u\n", version);
	printf("\t\t\tType: %s (%u)\n",
	       ax_pcie_type_name(type), type);

	ax_pcie_print_pcie_dev_cap(port, bus, dev, func, offset);
	ax_pcie_print_pcie_link_cap(port, bus, dev, func, offset, version);

	if (type == PCI_EXP_TYPE_ROOT_PORT ||
	    type == PCI_EXP_TYPE_DOWNSTREAM_PORT) {
		u32 slot_cap = 0;
		u16 slot_ctl = 0;
		u16 slot_sta = 0;

		ax_pcie_cfg_read32(port, bus, dev, func,
				   offset + PCI_EXP_SLT_CAP,
				   &slot_cap);

		ax_pcie_cfg_read16(port, bus, dev, func,
				   offset + PCI_EXP_SLT_CTL,
				   &slot_ctl);

		ax_pcie_cfg_read16(port, bus, dev, func,
				   offset + PCI_EXP_SLT_STA,
				   &slot_sta);

		printf("\t\t\tSlotCap: 0x%08x\n", slot_cap);
		printf("\t\t\tSlotCtl: 0x%04x\n", slot_ctl);
		printf("\t\t\tSlotSta: 0x%04x\n", slot_sta);

		printf("\t\t\t\tPresence Detect: %s\n",
		       slot_sta & PCI_EXP_SLT_STA_PDS ?
		       "present" : "not present");

		printf("\t\t\t\tDLL State Changed: %s\n",
		       slot_sta & PCI_EXP_SLT_STA_DLLSC ?
		       "yes" : "no");
	}

	if (type == PCI_EXP_TYPE_ROOT_PORT) {
		u16 root_ctl = 0;
		u32 root_sta = 0;

		ax_pcie_cfg_read16(port, bus, dev, func,
				   offset + PCI_EXP_RTCTL,
				   &root_ctl);

		ax_pcie_cfg_read32(port, bus, dev, func,
				   offset + PCI_EXP_RTSTA,
				   &root_sta);

		printf("\t\t\tRootCtl: 0x%04x\n", root_ctl);
		printf("\t\t\tRootSta: 0x%08x\n", root_sta);

		printf("\t\t\t\tPME Status: %s\n",
		       root_sta & PCI_EXP_RTSTA_PME_STATUS ?
		       "yes" : "no");

		printf("\t\t\t\tPME Pending: %s\n",
		       root_sta & PCI_EXP_RTSTA_PME_PENDING ?
		       "yes" : "no");
	}
}

/*
 * ---------------------------------------------------------------------------
 * Standard Capability decoder
 * ---------------------------------------------------------------------------
 */

void ax_pcie_print_capability(int port,
				     u8 bus, u8 dev, u8 func,
				     u8 offset)
{
	u8 id;
	u8 next;

	if (ax_pcie_cfg_read8(port, bus, dev, func,
			      offset + PCI_CAP_ID, &id))
		return;

	if (ax_pcie_cfg_read8(port, bus, dev, func,
			      offset + PCI_CAP_NEXT, &next))
		return;

	switch (id) {
	case PCI_CAP_ID_PM:
		ax_pcie_print_pm_cap(port, bus, dev, func, offset);
		break;

	case PCI_CAP_ID_MSI:
		ax_pcie_print_msi_cap(port, bus, dev, func, offset);
		break;

	case PCI_CAP_ID_PCIE:
		ax_pcie_print_pcie_cap(port, bus, dev, func, offset);
		break;

	case PCI_CAP_ID_MSIX:
		ax_pcie_print_msix_cap(port, bus, dev, func, offset);
		break;

	default:
		printf("\t\t[%02x] Unknown Capability 0x%02x\n",
		       offset, id);
		break;
	}
}

/*
 * ---------------------------------------------------------------------------
 * Standard Capability list
 * ---------------------------------------------------------------------------
 */

void ax_pcie_print_capabilities(int port, u8 bus, u8 dev, u8 func)
{
	u8 ptr;
	u8 next;
	u8 id;
	int count = 0;

	if (ax_pcie_cfg_read8(port, bus, dev, func,
			      PCI_CFG_CAP_PTR, &ptr))
		return;

	if (!ptr) {
		printf("\n\tCapabilities: none\n");
		return;
	}

	printf("\n\tCapabilities:\n");

	while (ptr && count++ < 48) {
		if (ptr < 0x40 || (ptr & 0x3)) {
			printf("\t\tInvalid capability pointer: 0x%02x\n",
			       ptr);
			break;
		}

		if (ax_pcie_cfg_read8(port, bus, dev, func,
				      ptr + PCI_CAP_ID, &id))
			break;

		if (ax_pcie_cfg_read8(port, bus, dev, func,
				      ptr + PCI_CAP_NEXT, &next))
			break;

		ax_pcie_print_capability(port, bus, dev, func, ptr);

		if (next == ptr)
			break;

		ptr = next;
	}

	/*
	 * PCIe Extended Capability starts at 0x100.
	 */
	printf("\n\tExtended Capabilities:\n");

	{
		u16 offset = 0x100;
		int ext_count = 0;

		while (offset && ext_count++ < 256) {
			u32 header;
			u16 cap_id;
			u8 version;
			u16 next_offset;

			if (offset & 0x3)
				break;

			if (ax_pcie_cfg_read32(port, bus, dev, func,
					       offset, &header))
				break;

			if (header == 0xffffffff ||
			    header == 0x00000000)
				break;

			cap_id = header & PCI_EXT_CAP_ID_MASK;

			if (!cap_id)
				break;

			version = (header & PCI_EXT_CAP_VER_MASK) >>
				  PCI_EXT_CAP_VER_SHIFT;

			next_offset = (header & PCI_EXT_CAP_NEXT_MASK) >>
				      PCI_EXT_CAP_NEXT_SHIFT;

			printf("\t\t[%03x] %s\n",
			       offset,
			       ax_pcie_ext_cap_name(cap_id));

			printf("\t\t\tID: 0x%04x\n", cap_id);
			printf("\t\t\tVersion: %u\n", version);

			switch (cap_id) {
			case PCI_EXT_CAP_ID_AER:
			{
				u32 uncorr_status = 0;
				u32 uncorr_mask = 0;
				u32 uncorr_severity = 0;
				u32 corr_status = 0;
				u32 corr_mask = 0;

				ax_pcie_cfg_read32(
					port, bus, dev, func,
					offset + PCI_AER_UNCORR_STATUS,
					&uncorr_status);

				ax_pcie_cfg_read32(
					port, bus, dev, func,
					offset + PCI_AER_UNCORR_MASK,
					&uncorr_mask);

				ax_pcie_cfg_read32(
					port, bus, dev, func,
					offset + PCI_AER_UNCORR_SEVERITY,
					&uncorr_severity);

				ax_pcie_cfg_read32(
					port, bus, dev, func,
					offset + PCI_AER_CORR_STATUS,
					&corr_status);

				ax_pcie_cfg_read32(
					port, bus, dev, func,
					offset + PCI_AER_CORR_MASK,
					&corr_mask);

				printf("\t\t\tUncorrectable Status: 0x%08x\n",
				       uncorr_status);

				printf("\t\t\tUncorrectable Mask:   0x%08x\n",
				       uncorr_mask);

				printf("\t\t\tUncorrectable Severity: 0x%08x\n",
				       uncorr_severity);

				printf("\t\t\tCorrectable Status:   0x%08x\n",
				       corr_status);

				printf("\t\t\tCorrectable Mask:     0x%08x\n",
				       corr_mask);
				break;
			}

			case PCI_EXT_CAP_ID_ACS:
			{
				u16 cap = 0;
				u16 ctl = 0;

				ax_pcie_cfg_read16(
					port, bus, dev, func,
					offset + PCI_ACS_CAP, &cap);

				ax_pcie_cfg_read16(
					port, bus, dev, func,
					offset + PCI_ACS_CTRL, &ctl);

				printf("\t\t\tACS Cap: 0x%04x\n", cap);
				printf("\t\t\tACS Ctrl: 0x%04x\n", ctl);
				break;
			}

			case PCI_EXT_CAP_ID_SRIOV:
			{
				u16 ctrl = 0;
				u16 total_vf = 0;
				u16 num_vf = 0;
				u16 first_vf = 0;
				u16 vf_offset = 0;
				u16 vf_stride = 0;

				ax_pcie_cfg_read16(
					port, bus, dev, func,
					offset + PCI_SRIOV_CTRL,
					&ctrl);

				ax_pcie_cfg_read16(
					port, bus, dev, func,
					offset + PCI_SRIOV_TOTAL_VF,
					&total_vf);

				ax_pcie_cfg_read16(
					port, bus, dev, func,
					offset + PCI_SRIOV_NUM_VF,
					&num_vf);

				ax_pcie_cfg_read16(
					port, bus, dev, func,
					offset + PCI_SRIOV_FIRST_VF,
					&first_vf);

				ax_pcie_cfg_read16(
					port, bus, dev, func,
					offset + PCI_SRIOV_VF_OFFSET,
					&vf_offset);

				ax_pcie_cfg_read16(
					port, bus, dev, func,
					offset + PCI_SRIOV_VF_STRIDE,
					&vf_stride);

				printf("\t\t\tSR-IOV Control: 0x%04x\n",
				       ctrl);

				printf("\t\t\tTotal VFs: %u\n",
				       total_vf);

				printf("\t\t\tNum VFs: %u\n",
				       num_vf);

				printf("\t\t\tFirst VF: %u\n",
				       first_vf);

				printf("\t\t\tVF Offset: %u\n",
				       vf_offset);

				printf("\t\t\tVF Stride: %u\n",
				       vf_stride);

				break;
			}

			default:
				break;
			}

			if (next_offset == offset)
				break;

			offset = next_offset;
		}
	}
}

/*
 * ---------------------------------------------------------------------------
 * Public header print
 * ---------------------------------------------------------------------------
 *
 * Dumps the config header of the function at bus:dev.func, similar to
 * "lspci -vv".  Works for both endpoints (type 0) and root ports /
 * bridges (type 1).
 */

void ax_pcie_print_header(int port, u8 bus, u8 dev, u8 func)
{
	ax_pcie_print_header_bdf(port, bus, dev, func);
}

/* Root Error Command: correctable / non-fatal / fatal error reporting */
#define AX_AER_ROOT_ERR_CMD_ALL	(BIT(0) | BIT(1) | BIT(2))

/* Device Control: correctable / non-fatal / fatal / unsupported request */
#define AX_EXP_DEVCTL_ERR_ALL	(PCI_EXP_DEVCTL_CERE | PCI_EXP_DEVCTL_NFERE | \
				 PCI_EXP_DEVCTL_FERE | PCI_EXP_DEVCTL_URRE)

int ax_pcie_ep_enable_aer(int port, u8 bus, u8 dev, u8 func)
{
	u32 val;
	int cap;

	/* Root Error Command lives in the AER extended capability */
	cap = ax_pcie_find_cap(port, bus, dev, func, 1, PCI_EXT_CAP_ID_AER);
    if (!cap)
        return -ENODEV;

	if (cap > 0 &&
	    !ax_pcie_cfg_read32(port, bus, dev, func,
			      cap + PCI_AER_ROOT_ERR_CMD, &val)) {
		val |= AX_AER_ROOT_ERR_CMD_ALL;
		ax_pcie_cfg_write(port, bus, dev, func,
				  cap + PCI_AER_ROOT_ERR_CMD, val);
	}

	/*
	 * Error reporting enables live in Device Control of the PCI Express
	 * capability.  Device Control and Device Status share one dword and
	 * the status bits are write-1-to-clear, so drop the status half
	 * before writing back or pending errors would be wiped.
	 */
	cap = ax_pcie_find_cap(port, bus, dev, func, 0, PCI_CAP_ID_PCIE);
	if (cap > 0 &&
	    !ax_pcie_cfg_read(port, bus, dev, func,
			      cap + PCI_EXP_DEVCTL, &val)) {
		val &= 0xffff;
		val |= AX_EXP_DEVCTL_ERR_ALL;
		ax_pcie_cfg_write(port, bus, dev, func,
				  cap + PCI_EXP_DEVCTL, val);
	}
}

static int ax_pcie_msi_calc_mme(u8 nr_vectors, u8 mmc,
                                u8 *mme)
{
    u8 log2 = 0;
    u8 n;

    if (!mme || !nr_vectors)
        return -EINVAL;

    /* MSI message count must be a power of two. */
    if (nr_vectors & (nr_vectors - 1))
        return -EINVAL;

    n = nr_vectors;
    while (n > 1) {
        n >>= 1;
        log2++;
    }

    if (log2 > mmc)
        return -EINVAL;

    *mme = log2;
    return 0;
}

int ax_pcie_ep_program_msi(int port, u8 bus, u8 dev, u8 func, 
        struct ax_pcie_msi *msi)
{
    struct ax_pcie_msi *ep_msi;
    u16 msi_cap;
    u16 ctrl;
    u64 address;
    u8 mmc;
    u8 mme;
    u8 nr_vectors;
    int ret;

    if (!msi)
        return -EINVAL;

    /* Find MSI capability */
    ret = ax_pcie_find_cap(port, bus, dev, func,
            0, PCI_CAP_ID_MSI);
    if (ret < 0)
        return ret;

    msi_cap = (u16)ret;

    /* Read MSI Message Control */
    ret = ax_pcie_cfg_read16(port, bus, dev, func,
            msi_cap + PCI_MSI_FLAGS,
            &ctrl);
    if (ret)
        return ret;

    printf("MSI capability @ 0x%02x\n", msi_cap);
    printf("MSI Control = 0x%04x\n", ctrl);

    /*
     * Reject programming while MSI is enabled.
     * Disable MSI before changing Message Address/Data.
     */
    if (ctrl & PCI_MSI_FLAGS_ENABLE) {
        printf("Error: MSI is enabled; disable it first\n");
        return -EBUSY;
    }

    /*
     * A 32-bit MSI capability cannot represent an address
     * above 4 GiB.
     */
    if (!(ctrl & PCI_MSI_FLAGS_64BIT) &&
            upper_32_bits(msi->address)) {
        printf("Error: MSI address 0x%016llx exceeds 32-bit range\n",
                (unsigned long long)msi->address);
        return -ERANGE;
    }

    /* Extract MMC: Maximum Message Count encoding. */
    mmc = (ctrl & PCI_MSI_CTRL_MMC_MASK) >>
        PCI_MSI_FLAGS_MMC_SHIFT;

    nr_vectors = msi->nr_vectors;

    ret = ax_pcie_msi_calc_mme(nr_vectors, mmc, &mme);
    if (ret) {
        printf("Invalid MSI vector count: requested=%u, MMC=%u\n",
                nr_vectors, 1U << mmc);
        return ret;
    }

    ep_msi = ax_pcie_request_msi_handle(port,
            msi->handler,
            msi->arg,
            nr_vectors);
    if (!ep_msi)
        return -ENOSPC;

    msi->address = ep_msi->address;
    msi->data = ep_msi->data;
    msi->vector = ep_msi->vector;
    msi->mme = mme;
    msi->nr_vectors = nr_vectors;
    msi->enabled = ep_msi->enabled;

    /*
     * Set MME while preserving MMC and other control bits.
     * MSI must remain disabled during programming.
     */
    ctrl &= ~PCI_MSI_CTRL_MME_MASK;
    ctrl |= ((u16)mme << PCI_MSI_FLAGS_MME_SHIFT) &
        PCI_MSI_CTRL_MME_MASK;

    ret = ax_pcie_cfg_write16(port, bus, dev, func,
            msi_cap + PCI_MSI_FLAGS, ctrl);
    if (ret)
        return ret;

    /* Program Message Address Low */
    ret = ax_pcie_cfg_write32(port, bus, dev, func,
            msi_cap + PCI_MSI_ADDRESS_LO,
            lower_32_bits(msi->address));
    if (ret)
        return ret;

    if (ctrl & PCI_MSI_FLAGS_64BIT) {
        /* Program Message Address High */
        ret = ax_pcie_cfg_write32(port, bus, dev, func,
                msi_cap + PCI_MSI_ADDRESS_HI,
                upper_32_bits(msi->address));
        if (ret)
            return ret;

        /* Program Message Data for 64-bit MSI */
        ret = ax_pcie_cfg_write16(port, bus, dev, func,
                msi_cap + PCI_MSI_DATA_64,
                msi->data);
        if (ret)
            return ret;
    } else {
        /* Program Message Data for 32-bit MSI */
        ret = ax_pcie_cfg_write16(port, bus, dev, func,
                msi_cap + PCI_MSI_DATA_32,
                msi->data);
        if (ret)
            return ret;
    }

    printf("PCIe MSI programmed:\n");
    printf("  BDF    = %02x:%02x.%x\n", bus, dev, func);
    printf("  cap    = 0x%02x\n", msi_cap);
    printf("  addr   = 0x%016llx\n", (u64)msi->address);
    printf("  data   = 0x%04x\n", msi->data);
    printf("  vector = %u\n", msi->vector);

    return 0;
}

int ax_pcie_ep_enable_msi(int port,
        u8 bus, u8 dev, u8 func)
{
    u16 msi_cap;
    u16 ctrl;
    int ret;

    /* Find MSI capability */
    ret = ax_pcie_find_cap(port, bus, dev, func,
            0, PCI_CAP_ID_MSI);
    if (ret < 0)
        return ret;

    msi_cap = (u16)ret;

    /* Read MSI Message Control */
    ret = ax_pcie_cfg_read16(port, bus, dev, func,
            msi_cap + PCI_MSI_FLAGS,
            &ctrl);
    if (ret)
        return ret;

    printf("MSI capability @ 0x%02x\n", msi_cap);
    printf("MSI Control before enable = 0x%04x\n", ctrl);

    if (ctrl & PCI_MSI_FLAGS_ENABLE) {
        printf("MSI already enabled\n");
        return 0;
    }

    /* Preserve MMC, MME and capability flags */
    ctrl |= PCI_MSI_FLAGS_ENABLE;

    ret = ax_pcie_cfg_write16(port, bus, dev, func,
            msi_cap + PCI_MSI_FLAGS,
            ctrl);
    if (ret)
        return ret;

    printf("MSI enabled, control = 0x%04x\n", ctrl);

    return 0;
}

int ax_pcie_ep_disable_msi(int port,
        u8 bus, u8 dev, u8 func)
{
    u16 msi_cap;
    u16 ctrl;
    int ret;

    /* Find MSI capability */
    ret = ax_pcie_find_cap(port, bus, dev, func,
            0, PCI_CAP_ID_MSI);
    if (ret < 0)
        return ret;

    msi_cap = (u16)ret;

    /* Read MSI Message Control */
    ret = ax_pcie_cfg_read16(port, bus, dev, func,
            msi_cap + PCI_MSI_FLAGS,
            &ctrl);
    if (ret)
        return ret;

    printf("MSI capability @ 0x%02x\n", msi_cap);
    printf("MSI Control before disable = 0x%04x\n", ctrl);

    if (!(ctrl & PCI_MSI_FLAGS_ENABLE)) {
        printf("MSI already disabled\n");
        return 0;
    }

    /* Clear only MSI Enable */
    ctrl &= ~PCI_MSI_FLAGS_ENABLE;

    ret = ax_pcie_cfg_write16(port, bus, dev, func,
            msi_cap + PCI_MSI_FLAGS,
            ctrl);
    if (ret)
        return ret;

    printf("MSI disabled, control = 0x%04x\n", ctrl);

    return 0;
}

int ax_pcie_ep_msi_status(int port,
        u8 bus, u8 dev, u8 func)
{
    u16 msi_cap;
    u16 ctrl;
    u32 addr_lo;
    u32 addr_hi = 0;
    u16 data = 0;
    u64 address;
    u8 mmc, mme;
    int ret;

    /* Find MSI capability */
    ret = ax_pcie_find_cap(port, bus, dev, func,
            0, PCI_CAP_ID_MSI);
    if (ret < 0) {
        printf("MSI capability not found\n");
        return ret;
    }

    msi_cap = (u16)ret;

    /* Read MSI Message Control */
    ret = ax_pcie_cfg_read16(port, bus, dev, func,
            msi_cap + PCI_MSI_FLAGS,
            &ctrl);
    if (ret)
        return ret;

    /* Read Message Address Low */
    ret = ax_pcie_cfg_read32(port, bus, dev, func,
            msi_cap + PCI_MSI_ADDRESS_LO,
            &addr_lo);
    if (ret)
        return ret;

    if (ctrl & PCI_MSI_FLAGS_64BIT) {
        /* Read Message Address High */
        ret = ax_pcie_cfg_read32(port, bus, dev, func,
                msi_cap + PCI_MSI_ADDRESS_HI,
                &addr_hi);
        if (ret)
            return ret;

        /* Read Message Data for 64-bit MSI */
        ret = ax_pcie_cfg_read16(port, bus, dev, func,
                msi_cap + PCI_MSI_DATA_64,
                &data);
    } else {
        /* Read Message Data for 32-bit MSI */
        ret = ax_pcie_cfg_read16(port, bus, dev, func,
                msi_cap + PCI_MSI_DATA_32,
                &data);
    }

    if (ret)
        return ret;

    address = ((u64)addr_hi << 32) | addr_lo;
    mmc = (ctrl & PCI_MSI_CTRL_MMC_MASK) >>
        PCI_MSI_FLAGS_MMC_SHIFT;
    mme = (ctrl & PCI_MSI_CTRL_MME_MASK) >>
        PCI_MSI_FLAGS_MME_SHIFT;

    printf("PCIe MSI status: %02x:%02x.%x\n",
            bus, dev, func);

    printf("  MSI cap  : 0x%02x\n", msi_cap);
    printf("  Control  : 0x%04x\n", ctrl);
    printf("  Enable   : %s\n",
            (ctrl & PCI_MSI_FLAGS_ENABLE) ? "yes" : "no");
    printf("  64-bit   : %s\n",
            (ctrl & PCI_MSI_FLAGS_64BIT) ? "yes" : "no");
    printf("  MMC      : %u\n", 1U << mmc);
    printf("  MME      : %u\n", 1U << mme);
    printf("  Address  : 0x%016llx\n",
            (unsigned long long)address);
    printf("  Data     : 0x%04x\n", data);

    return 0;
}
