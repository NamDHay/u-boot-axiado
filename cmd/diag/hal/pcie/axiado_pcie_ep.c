// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <stdbool.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <time.h>
#include <linux/delay.h>
#include <malloc.h>
#include <linux/log2.h>
#include <display_options.h>

#include "ax_diag.h"
#include "ax_pcie.h"
#include "axiado_pcie.h"
#include "ax_pcie_conf.h"

#define AXIADO_PAGE_SIZE (4096)
#define MAX_DMA_CHANNELS (8)
#define DMA_DESC_SIZE (32)
#define MAX_PORT 2
#define PORT_X1 0
#define PORT_X2 1
#define MAX_MSI_VECTOR (8)
#define DMA_TIMEOUT 2000
#define DMA_START 0x21
#define DMA_CTRL 0x20
/* DMA Settings */
#define DMA_SE_SHIFT 4
#define DMA_IRQ_SHIFT 8
#define DMA_START_ABORT_BIT 0x1
#define DMA_SG_MODE_ENABLE (1 << 3)
#define DMA_SG_MODE_SHIFT 24
#define DMA_CHANNEL_MF 0x40
#define DMA_SE_COND_SET 0x2
#define DMA_IRQ_SET 0x3
#define DMA_DESC_STATUS 0x0
#define DMA_DESC_CONTROL 0x00000001
#define DMA_DESC_NEXT_SHIFT 0xffffffffffffffE0
#define DMA_DESC_NEXT_FIRST 0x11
#define DMA_DESC_NEXT 0x10
#define BAR_SIZE_BYTES 4
#define VEC_CLR_BIT 0xFF000000

struct axiado_pcie_config_data {
	u64 bar0_start;
	u64 bar2_start;
	u64 bar4_start;
	u32 bar0_aperture_size;
	u32 bar2_aperture_size;
	u32 bar4_aperture_size;
} axiado_pcie_conf;

static bool axiado_pcie_validate_bar(enum axiado_pcie_bars bar_num)
{
    return ((bar_num == BAR_01) || (bar_num == BAR_23) ||
            (bar_num == BAR_45));
}

int axiado_pcie_bar_init(struct axiado_pcie *pcie,
        enum axiado_pcie_bars bar_num, u64 base_address,
        u32 aperture_size)
{
    u32 high_offset = 0;
    u32 low_offset = 0;
    u64 temp_64 = 0;
    u32 temp = 0;
    u32 bar_offset = 0;
    u32 aperture_offset = 0;

    if (!axiado_pcie_validate_bar(bar_num)) {
        printf(" Invalid bar number %d\n", bar_num);
        return -EINVAL;
    }

    /* apeture size must be multiple of 4KB */
    if ((aperture_size % (AXIADO_PAGE_SIZE)) != 0) {
        printf("  must be multiple of 4KB %d\n", aperture_size);
        return -EINVAL;
    }

    /* store the start and aperture in the global struture pcie_config */
    if (bar_num == BAR_01) {
        axiado_pcie_conf.bar0_start = base_address;
        axiado_pcie_conf.bar0_aperture_size = aperture_size;
        low_offset = REG_PCIE_SRIOV_31_0_ADRS_OFFSET +
            2 * BAR_SIZE_BYTES;
        high_offset = REG_PCIE_SRIOV_31_0_ADRS_OFFSET +
            3 * BAR_SIZE_BYTES;
        bar_offset = 0;
    } else if (bar_num == BAR_23) {
        axiado_pcie_conf.bar2_start = base_address;
        axiado_pcie_conf.bar2_aperture_size = aperture_size;
        low_offset = REG_PCIE_SRIOV_31_0_ADRS_OFFSET +
            4 * BAR_SIZE_BYTES;
        high_offset = REG_PCIE_SRIOV_31_0_ADRS_OFFSET +
            5 * BAR_SIZE_BYTES;
        bar_offset = 4;
    } else if (bar_num == BAR_45) {
        axiado_pcie_conf.bar4_start = base_address;
        axiado_pcie_conf.bar4_aperture_size = aperture_size;
        low_offset = REG_PCIE_SRIOV_31_0_ADRS_OFFSET +
            6 * BAR_SIZE_BYTES;
        high_offset = REG_PCIE_SRIOV_31_0_ADRS_OFFSET +
            7 * BAR_SIZE_BYTES;
        bar_offset = 8;
    } else {
        pr_err("Unsupported BAR number %d\n", bar_num);
        return -EINVAL;
    }

    /* Prepare mask such that for a 4K size the value of temp_64 will be 0xFFFFFFFFFFFFF000 */
    /* number of LSB 0's determines the size */
    temp_64 = aperture_size - 1;
    temp_64 = ~(temp_64);
    /* split into 32 bit MSB and 32 bit LSB */
    temp = temp_64 & 0xFFFFFFFF;
    temp_64 = temp_64 >> 32;
    writel(temp, pcie->ext + low_offset);
    writel(temp_64, pcie->ext + high_offset);

    aperture_offset = (aperture_size / AXIADO_PAGE_SIZE) - 1;

    /* Need to set the aperture high, low and size */
    writel(base_address >> 3,
            pcie->ext +
            REG_PCIE_AXI_MST0_VF_APER_LO_LSBS_N3_ADRS_OFFSET +
            bar_offset);

    writel((0x0 << REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_FLD_REMAP_DIS_LSB) |
            (aperture_offset << REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_FLD_APER_SIZE_LSB),
            pcie->ext +
            REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_ADRS_OFFSET +
            bar_offset);

    writel((((base_address >> 3) & aperture_offset)
                << REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_FLD_ADRS_LO_LSB),
            pcie->ext +
            REG_PCIE_AXI_MST0_VF_APER_HI_LSBS_N3_ADRS_OFFSET +
            bar_offset);

    writel(0x0, pcie->ext +
            REG_PCIE_AXI_MST0_VF_APER_HI_MSBS_N3_ADRS_OFFSET +
            bar_offset);

    return 0;
}

/* helper function to find if the port,pf,vf numbers are valid */
static bool axiado_validate_func_info(struct axiado_pcie_func_info func_info)
{
	if (func_info.port > MAX_PORT) {
		pr_err(" invalid port number:%d\n", func_info.port);
		return false;
	}

	if ((func_info.port == PORT_X1) && (func_info.vf_num > 0)) {
		pr_err(" PORT_X1 does not have SRIOV enabled\n");
		return false;
	}

	/* there is only one PF */
	if (func_info.pf_num > 0) {
		pr_err(" invalid pf number %d\n", func_info.pf_num);
		return false;
	}

	/* vf_num "0" is considered as PF, hence need to accept till MAX_VF+1 */
	if (func_info.vf_num > (MAX_VF + 1)) {
		pr_err(" invalid vf number %d\n", func_info.vf_num);
		return false;
	}
	return true;
}
