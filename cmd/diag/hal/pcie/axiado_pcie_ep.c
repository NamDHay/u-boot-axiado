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
#include <linux/math64.h>
#include <malloc.h>
#include <linux/log2.h>
#include <display_options.h>

#include "ax_diag.h"
#include "ax_pcie.h"
#include "axiado_pcie.h"
#include "axiado_pcie_ep.h"
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

static bool axiado_pcie_validate_bar(enum axiado_pcie_bars bar_num)
{
    return ((bar_num == BAR_01) || (bar_num == BAR_23) ||
            (bar_num == BAR_45));
}

int axiado_pcie_rom_init(struct axiado_pcie *pcie, enum axiado_pcie_pf func)
{
    u32 rom_size, rom_mask;
    u32 val;

    rom_size = pcie->func[func].rom.size;
    rom_mask = (u64)(~((u64)rom_size - 1));

    val = readl(pcie->bridge + REG_PCIE_WINROM_ADRS_OFFSET);
    val = PCIE_WINROM_EXP_ROM_SIZE_MASK_SET(val, rom_mask);
    writel(val, pcie->bridge + REG_PCIE_WINROM_ADRS_OFFSET);

    return 0;
}

int axiado_pcie_bar_init(struct axiado_pcie *pcie,
        enum axiado_pcie_pf pf,
        enum axiado_pcie_bars bar_num)
{
    u32 bar_lo, bar_hi;
    u64 vf_bar_size_mask;
    u8 cur_vf = 0, num_vfs = 0;
    u64 bar_addr;
    u32 bar_flag;
    size_t bar_size;

    if (!axiado_pcie_validate_bar(bar_num)) {
        printf("Invalid bar number %d\n", bar_num);
        return -EINVAL;
    }

    /* pf bar apeture size must be multiple of 4KB */
    bar_size = pcie->func[pf].pf[bar_num / 2].size;
    bar_flag = pcie->func[pf].pf[bar_num / 2].flags;
    if ((bar_size % (AXIADO_PAGE_SIZE)) != 0) {
        printf("0x%010lx must be multiple of 4KB\n", bar_size);
        return -EINVAL;
    }

    if (bar_size) {
        bar_lo = (~(bar_size - 1) & 0xFFFFFFF0) | bar_flag;
        bar_hi = 0xFFFFFFFF;
        printf("PCIe_x%d 1:0:%d BAR%d%d: 0x%010lx [0x%010x]\n", 
                pcie->lanes, pf, bar_num, (bar_num + 1), bar_size, bar_flag);
    } else {
        bar_lo = 0x0;
        bar_hi = 0x0;
    }
    writel(bar_lo, pcie->bridge + PCIE_BAR_01_OFFSET + sizeof(u32) * bar_num);
    writel(bar_hi, pcie->bridge + PCIE_BAR_01_OFFSET + sizeof(u32) * bar_num + 4);

    /* PCIe_x1 don't support VF */
    if (pcie->pcie_x1)
        goto end;

    cur_vf = pcie->func[pf].vf_num;
    num_vfs = pcie->func[pf].num_vfs;
    if (!num_vfs)
        goto end;

    if ((cur_vf + num_vfs) > MAX_VF) {
        printf("Out of max VF\n");
        return -EINVAL;
    }

    /* vf bar apeture size must be multiple of vf * 4KB */
    bar_size = pcie->func[pf].vf[bar_num / 2].size;
    bar_flag = pcie->func[pf].vf[bar_num / 2].flags;
    if ((bar_size % (num_vfs * AXIADO_PAGE_SIZE)) != 0) {
        printf("0x%010lx must be multiple of %d * 4KB\n", bar_size, num_vfs);
        return -EINVAL;
    }

    bar_lo = readl(pcie->bridge + REG_PCIE_PEX_DEV2_ADRS_OFFSET);
    bar_lo = PCIE_PEX_DEV2_ARI_FORWARD_SUP_SET(bar_lo, 0x1);
    writel(bar_lo, pcie->bridge + REG_PCIE_PEX_DEV2_ADRS_OFFSET);

    bar_lo = readl(pcie->bridge + REG_PCIE_SRIOV_31_0_ADRS_OFFSET);
    bar_lo = PCIE_SRIOV_VF_DEV_ID_SET(bar_lo, pf);
    writel(bar_lo, pcie->bridge + REG_PCIE_SRIOV_31_0_ADRS_OFFSET);

    bar_lo = readl(pcie->bridge + REG_PCIE_SRIOV_63_32_ADRS_OFFSET);
    bar_lo = PCIE_SRIOV_VF_APP_SPECIFIC_CAP_IMP_SET(bar_lo, PCIE_SRIOV_VF_APP_SPECIFIC_CAP_OFF);
    bar_lo = PCIE_SRIOV_VF_FUNC_DEPEND_LINK_SET(bar_lo, pf);
    writel(bar_lo, pcie->bridge + REG_PCIE_SRIOV_63_32_ADRS_OFFSET);

    if (pcie->func[pf].vf[bar_num].size) {
        bar_lo = (~(bar_size - 1) & 0xFFFFFFF0) | bar_flag;
        bar_hi = 0xFFFFFFFF;
        printf("PCIe_x%d 1:0:%d VF BAR%d%d: 0x%010lx [0x%010x]\n", 
                pcie->lanes, pf, bar_num, (bar_num + 1), bar_size, bar_flag);
    } else {
        bar_lo = 0x0;
        bar_hi = 0x0;
    }
    writel(bar_lo, pcie->bridge + REG_PCIE_SRIOV_95_64_ADRS_OFFSET + sizeof(u32) * bar_num);
    writel(bar_hi, pcie->bridge + REG_PCIE_SRIOV_95_64_ADRS_OFFSET + sizeof(u32) * bar_num + 4);

end:
    return 0;
}

int axiado_pcie_alloc_pf_vf_bar(struct axiado_pcie *pcie)
{
    u8 pf, pf_num;
    u8 vf_base, num_vfs;
    u32 val;

    /* Don't support VF on x1 port */
    if (pcie->pcie_x1)
        return 0;

    vf_base = 0;
    pf_num = pcie->pf_num;
    for (pf = 0; pf < pf_num; pf++) {
        num_vfs = pcie->func[pf].num_vfs;

        val = readl(pcie->ext + REG_PCIE_X2_PF0_VF_ALLOC_ADRS_OFFSET + pf * sizeof(u32));
        val = EXT_PF_VF_ALLOC_BASE_SET(val, vf_base);
        val = EXT_PF_VF_ALLOC_TOP_SET(val, vf_base + num_vfs);
        writel(val, pcie->ext + REG_PCIE_X2_PF0_VF_ALLOC_ADRS_OFFSET + pf * sizeof(u32));

        vf_base += num_vfs;
    }

    return 0;
}

static void axiado_pcie_set_dma(struct axiado_pcie *pcie,
        struct dma_cfg d_cfg,
        enum axiado_pcie_port_num port,
        enum axiado_pcie_dma_channel channel_num,
        enum axiado_pcie_dma_mode mode)
{
    u32 ctrl = 0;
    u32 offset[8];

    if (mode != DIRECT_MODE)
        ctrl = ctrl |
            DMA_SG_MODE_ENABLE; /* for SG mode enable SG bit */

    /* set 25:24 bits as per the requested scatter gather mode */
    if (mode == SCATTER_GATHER_01)
        ctrl = ctrl | (0x01 << DMA_SG_MODE_SHIFT);
    else if (mode == SCATTER_GATHER_10)
        ctrl = ctrl | (0x02 << DMA_SG_MODE_SHIFT);
    else if (mode == SCATTER_GATHER_11)
        ctrl = ctrl | (0x03 << DMA_SG_MODE_SHIFT);
    else if (mode == SCATTER_GATHER_00)
        ctrl = ctrl | (0x00 << DMA_SG_MODE_SHIFT);
    /* Stop if DMA_LENGTH is reached */
    ctrl |= DMA_CTRL;

    offset[0] = REG_PCIE_DMA_BASE +
        (REG_PCIE_DMA0_CONFIG_SPACE_DMA_SRCPARAM_ADRS_OFFSET +
         (channel_num * DMA_CHANNEL_MF));
    offset[1] = REG_PCIE_DMA_BASE +
        (REG_PCIE_DMA0_CONFIG_SPACE_DMA_DESTPARAM_ADRS_OFFSET +
         (channel_num * DMA_CHANNEL_MF));
    offset[2] = REG_PCIE_DMA_BASE +
        (REG_PCIE_DMA0_CONFIG_SPACE_DMA_SRCADDR_31_0_ADRS_OFFSET +
         (channel_num * DMA_CHANNEL_MF));
    offset[3] = REG_PCIE_DMA_BASE +
        (REG_PCIE_DMA0_CONFIG_SPACE_DMA_SRCADDR_63_32_ADRS_OFFSET +
         (channel_num * DMA_CHANNEL_MF));
    offset[4] = REG_PCIE_DMA_BASE +
        (REG_PCIE_DMA0_CONFIG_SPACE_DMA_DESTADDR_31_0_ADRS_OFFSET +
         (channel_num * DMA_CHANNEL_MF));
    offset[5] = REG_PCIE_DMA_BASE +
        (REG_PCIE_DMA0_CONFIG_SPACE_DMA_DESTADDR_63_32_ADRS_OFFSET +
         (channel_num * DMA_CHANNEL_MF));
    offset[6] = REG_PCIE_DMA_BASE +
        (REG_PCIE_DMA0_CONFIG_SPACE_DMA_LENGTH_ADRS_OFFSET +
         (channel_num * DMA_CHANNEL_MF));
    offset[7] = REG_PCIE_DMA_BASE +
        (REG_PCIE_DMA0_CONFIG_SPACE_DMA_CONTROL_ADRS_OFFSET +
         (channel_num * DMA_CHANNEL_MF));

    writel(
            d_cfg.src_inf,
            pcie->cfg +
            offset[0]); /* [3:0] source interface set to axi4 master */
    writel(
            d_cfg.dest_inf,
            pcie->cfg +
            offset[1]); /* [3:0] destination interface set to PCIe interface */
    writel(
            d_cfg.src_addr_31_0,
            pcie->cfg +
            offset[2]); /* set source address to SRAM base address */
    writel(d_cfg.src_addr_63_32, pcie->cfg + offset[3]);
    writel(
            d_cfg.dest_addr_31_0,
            pcie->cfg +
            offset[4]); /* destination rootport's (x1) sram mapped address */
    writel(d_cfg.dest_addr_63_32, pcie->cfg + offset[5]);
    writel(d_cfg.length,
            pcie->cfg + offset[6]); /* transfer size 4k bytes */
    writel(ctrl, pcie->cfg + offset[7]);
}

/* helper functions to find control, status offsets and start DMA */
static u32 find_dma_control_offset(enum axiado_pcie_port_num port,
        enum axiado_pcie_dma_channel channel_num)
{
    return (REG_PCIE_DMA_BASE +
            (REG_PCIE_DMA0_CONFIG_SPACE_DMA_CONTROL_ADRS_OFFSET +
             (channel_num * DMA_CHANNEL_MF)));
}

static void axiado_pcie_start_dma(struct axiado_pcie *ax_pcie,
        enum axiado_pcie_port_num port,
        enum axiado_pcie_dma_channel chan_num)
{
    u32 offset = 0;

    offset = find_dma_control_offset(port, chan_num);
    writel(DMA_START, ax_pcie->cfg + offset);
}

/*check if a specific DMA channel is free. 0 for free, 1 for busy */
int axiado_pcie_is_dma_busy(struct axiado_pcie *ax_pcie,
        enum axiado_pcie_port_num port,
        enum axiado_pcie_dma_channel chan_num)
{
    u32 offset = 0;

    offset = find_dma_control_offset(port, chan_num);
    return ((readl(ax_pcie->cfg + offset) & 0x1) != 0x0);
}

    static int
axiado_pcie_validate_dma_request(struct axiado_pcie_dma_request *dma_request)
{
    u64 diff = 0;

    if (dma_request->dma_mode == DIRECT_MODE) {
        diff = abs_diff(dma_request->dma_source_ptr[0],
                dma_request->dma_dest_ptr[0]);

        if (unlikely(!IS_ALIGNED(diff, 32))) {
            printf("source: 0x%lx destination: 0x%lx diff: 0x%llx\n",
                    (unsigned long)dma_request->dma_source_ptr[0],
                    (unsigned long)dma_request->dma_dest_ptr[0],
                    diff);
            return -EINVAL;
        }
    }

    return 0;
}

int axiado_pcie_dma_xfer(struct axiado_pcie *ax_pcie,
        enum axiado_pcie_port_num port,
        struct axiado_pcie_dma_request *dma_request)
{
    int ret = 0;
    struct dma_cfg d_cfg;
    int i = 0;
    u8 buffer[32];
    u32 *temp = (u32 *)buffer;
    u32 timeout = 0;
    u64 *temp_64 = (u64 *)buffer;
    u64 val = 0;

    switch (dma_request->dma_dir) {
        case AX_EP_MEMORY_HOST_TO_PDEV:
            /*
             * On EP, Read host memory using PCIe bus
             * and transmit to local mem over AXI
             */
            d_cfg.src_inf = PCIE_INTF;
            d_cfg.dest_inf = AXI4_M0;
            ret = axiado_pcie_validate_dma_request(dma_request);
            if (ret)
                goto err;
            break;
        case AX_EP_MEMORY_PDEV_TO_HOST:
            /*
             * On EP, read local memory using AXI interface
             * and post to host over PCIe
             */
            d_cfg.src_inf = AXI4_M0;
            d_cfg.dest_inf = PCIE_INTF;
            ret = axiado_pcie_validate_dma_request(dma_request);
            if (ret)
                goto err;
            break;
        case AX_RC_VDM_HOST_TO_PDEV:
        case AX_EP_VDM_PDEV_TO_HOST:
            /*
             * Note: In both these cases, the message has to be read
             * from local memory via axi4_m0 and post to pcie.
             * Set the special options required for VDM
             */
            d_cfg.src_inf = AXI4_M0;
            d_cfg.dest_inf = (AX_PCIE_DMA_VDM_TRSF_PARAM << 16) | PCIE_INTF;
            // No need to call validate
            break;
        default:
            printf("Unknown DMA direction\n");
            return -EINVAL;
    }

    d_cfg.se_cond = DMA_SE_COND_SET;
    d_cfg.irq = DMA_IRQ_SET;

    switch (dma_request->dma_mode) {
        case DIRECT_MODE:
            d_cfg.dest_addr_31_0 = dma_request->dma_dest_ptr[0];
            d_cfg.dest_addr_63_32 = dma_request->dma_dest_ptr[0] >> 32;
            d_cfg.src_addr_31_0 = dma_request->dma_source_ptr[0];
            d_cfg.src_addr_63_32 = dma_request->dma_source_ptr[0] >> 32;
            d_cfg.length = dma_request->dma_size;
            break;

        case SCATTER_GATHER_00:
            for (i = 0; i < dma_request->source_list_size; i++) {
                temp[0] = DMA_DESC_STATUS;
                temp[1] = DMA_DESC_CONTROL |
                    (dma_request->dma_source_size[i] << 8);
                if (i == (dma_request->source_list_size - 1)) {
                    val = ax_pcie->sg_buffer_dma_desc_src[0] >> 5;
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT_FIRST;
                } else {
                    val = ax_pcie->sg_buffer_dma_desc_src[i + 1] >>
                        5; /* set next address to next descriptor */
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT;
                }

                temp_64[2] = dma_request->dma_source_ptr
                    [i]; /* this is DESC_SRC_ADDR */
                memcpy((u8 *)ax_pcie->sg_buffer_desc_src[i],
                        (u8 *)buffer, 32);
            }
            for (i = 0; i < dma_request->dest_list_size; i++) {
                temp[0] = DMA_DESC_STATUS;
                temp[1] = DMA_DESC_CONTROL |
                    (dma_request->dma_dest_size[i] << 8);
                if (i == (dma_request->dest_list_size - 1)) {
                    val = ax_pcie->sg_buffer_dma_desc_dest[0] >> 5;
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT_FIRST;
                } else {
                    val = ax_pcie->sg_buffer_dma_desc_dest[i + 1] >>
                        5;
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT;
                }

                temp_64[2] = dma_request->dma_dest_ptr[i];
                memcpy((u8 *)ax_pcie->sg_buffer_desc_dest[i],
                        (u8 *)buffer, 32);
            }

            d_cfg.dest_addr_31_0 = ax_pcie->sg_buffer_dma_desc_src[0];
            d_cfg.dest_addr_63_32 = ax_pcie->sg_buffer_dma_desc_src[0] >>
                32;
            d_cfg.src_addr_31_0 = ax_pcie->sg_buffer_dma_desc_dest[0];
            d_cfg.src_addr_63_32 = ax_pcie->sg_buffer_dma_desc_dest[0] >>
                32;
            break;

        case SCATTER_GATHER_01:
            for (i = 0; i < dma_request->source_list_size; i++) {
                temp[0] = DMA_DESC_STATUS;
                temp[1] = DMA_DESC_CONTROL |
                    (dma_request->dma_source_size[i] << 8);
                if (i == (dma_request->source_list_size - 1)) {
                    val = ax_pcie->sg_buffer_dma_desc_src[0] >> 5;
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT_FIRST;
                } else {
                    val = ax_pcie->sg_buffer_dma_desc_src[i + 1] >>
                        5;
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT;
                }

                temp_64[2] = dma_request->dma_source_ptr[i];
                memcpy((u8 *)ax_pcie->sg_buffer_desc_src[i],
                        (u8 *)buffer, 32);
            }

            d_cfg.dest_addr_31_0 = ax_pcie->sg_buffer_dma_desc_src[0];
            d_cfg.dest_addr_63_32 = ax_pcie->sg_buffer_dma_desc_src[0] >>
                32;
            d_cfg.src_addr_31_0 = dma_request->dma_dest_ptr[0];
            d_cfg.src_addr_63_32 = dma_request->dma_dest_ptr[0] >> 32;
            break;

        case SCATTER_GATHER_10:

            for (i = 0; i < dma_request->dest_list_size; i++) {
                temp[0] = DMA_DESC_STATUS;
                temp[1] = DMA_DESC_CONTROL |
                    (dma_request->dma_dest_size[i] << 8);
                if (i == (dma_request->dest_list_size - 1)) {
                    val = ax_pcie->sg_buffer_dma_desc_dest[0] >> 5;
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT_FIRST;
                } else {
                    val = ax_pcie->sg_buffer_dma_desc_dest[i + 1] >>
                        5;
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT;
                }

                temp_64[2] = dma_request->dma_dest_ptr[i];
                memcpy((u8 *)ax_pcie->sg_buffer_desc_dest[i],
                        (u8 *)buffer, 32);
            }

            d_cfg.dest_addr_31_0 = dma_request->dma_source_ptr[0];
            d_cfg.dest_addr_63_32 = dma_request->dma_source_ptr[0] >> 32;
            d_cfg.src_addr_31_0 = ax_pcie->sg_buffer_dma_desc_dest[0];
            d_cfg.src_addr_63_32 = ax_pcie->sg_buffer_dma_desc_dest[0] >>
                32;
            break;

        case SCATTER_GATHER_11:
            for (i = 0; i < dma_request->source_list_size; i++) {
                temp[0] = DMA_DESC_STATUS;
                temp[1] = DMA_DESC_CONTROL |
                    (dma_request->dma_source_size[i] << 8);
                if (i == (dma_request->source_list_size - 1)) {
                    val = ax_pcie->sg_buffer_dma_desc_src[0] >> 5;
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT_FIRST;
                } else {
                    val = ax_pcie->sg_buffer_dma_desc_src[i + 1] >>
                        5;
                    temp_64[1] = (val << 5) & DMA_DESC_NEXT_SHIFT;
                    temp_64[1] |= DMA_DESC_NEXT;
                }

                temp_64[2] = dma_request->dma_source_ptr[i];
                temp_64[3] = dma_request->dma_dest_ptr[i];
                memcpy((u8 *)ax_pcie->sg_buffer_desc_src[i],
                        (u8 *)buffer, 32);
            }
            d_cfg.dest_addr_31_0 = ax_pcie->sg_buffer_dma_desc_src[0];
            d_cfg.dest_addr_63_32 = ax_pcie->sg_buffer_dma_desc_src[0] >>
                32;
            d_cfg.src_addr_31_0 = ax_pcie->sg_buffer_dma_desc_src[0];
            d_cfg.src_addr_63_32 = ax_pcie->sg_buffer_dma_desc_src[0] >> 32;
            break;

        default:
            printf("Unknown DMA direction\n");
            return -EINVAL;
    }

    axiado_pcie_set_dma(ax_pcie, d_cfg, port, dma_request->chan_num,
            dma_request->dma_mode);
    __asm__ volatile("dsb sy");
    __asm__ volatile("isb");
    axiado_pcie_start_dma(ax_pcie, port, dma_request->chan_num);

    timeout = DMA_TIMEOUT;
    do {
        if (!axiado_pcie_is_dma_busy(ax_pcie, port, dma_request->chan_num))
            break;
        udelay(1000);
    } while (--timeout);

    if (!timeout) {
        ret = -ETIMEDOUT;
        goto err;
    }
    __asm__ volatile("dsb sy");
    __asm__ volatile("isb");

err:
    return ret;
}

/* API to trigger msi to the host for the specific PF:VF:vector number */
int axiado_pcie_trigger_msi(u8 port,
        uint pf_num,
        uint vector_no)
{
    struct axiado_pcie *pcie;
    u32 msi_trg;
    u32 temp;
    uint64_t ext_pf_ctrl_status_adrs_off;

    pcie = axiado_pcie_get_port(port);
    if (!pcie)
        return -EINVAL;

    if (pcie->pcie_x1)
        ext_pf_ctrl_status_adrs_off = REG_PCIE_X1_PF_CTRL_STATUS_ADRS_OFFSET;
    else
        ext_pf_ctrl_status_adrs_off = REG_PCIE_X2_PF_CTRL_STATUS_N5_ADRS_OFFSET;

    if (vector_no >= MAX_MSI_ID) {
        printf("ERROR: Invalid MSI ID\n\r");
        return -EINVAL;
    }

    if (pf_num > PCIE_MAX_PF_NUM) {
        printf("ERROR: Invalid Physical Function\n\r");
        return -EINVAL;
    }

    if (pcie->pcie_x1)
        msi_trg = BIT(vector_no + 5);
    else
        msi_trg = BIT(vector_no);
    writel(msi_trg, pcie->ext + ext_pf_ctrl_status_adrs_off + pf_num * sizeof(uint32_t));

    temp = readl(pcie->bridge + ISTATUS_HOST);
    temp |= VEC_CLR_BIT;
    writel(temp, pcie->bridge + ISTATUS_HOST);

    printf("MSI trigger: port=%u pf=%u vector=%u reg_off=0x%llx value=0x%08x\n",
            port, pf_num, vector_no,
            (unsigned long long)(ext_pf_ctrl_status_adrs_off +
                pf_num * sizeof(uint32_t)),
            msi_trg);

    return 0;
}

unsigned int axiado_pcie_msgbox_read(u8 port, u32 offset)
{
    struct axiado_pcie *pcie = axiado_pcie_get_port(port);

    return readl(pcie->ext + offset);
}

void axiado_pcie_msgbox_write(u8 port, u32 offset, u32 val)
{
    struct axiado_pcie *pcie = axiado_pcie_get_port(port);

    writel(val, (pcie->ext + offset));
}

unsigned int axiado_pcie_ocm_csr_read(u8 port)
{
    struct axiado_pcie *pcie = axiado_pcie_get_port(port);

    return readl(pcie->cfg + REG_PCIE_X2_CSR_INT_OCM_OFFSET);
}

void axiado_pcie_ocm_csr_write(u8 port,	u32 val)
{
    struct axiado_pcie *pcie = axiado_pcie_get_port(port);

    writel(val, (pcie->cfg + REG_PCIE_X2_CSR_INT_OCM_OFFSET));
}
