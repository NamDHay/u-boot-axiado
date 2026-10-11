// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __AXIADO_PCIE_EP_H
#define __AXIADO_PCIE_EP_H

#define MAX_DMA_LIST (16)
#define MAX_PF (1)
#define MAX_VF (64)
#define MAX_PF_MBX (4)
#define MAX_MBX (64)
#define MAX_PORT 2

#define REG_PCIE_X1_PF_CTRL_STATUS_ADRS_OFFSET 0x104
#define REG_PCIE_X1_INTR_STATUS_ADRS_OFFSET 0x108
#define REG_PCIE_X1_INTR_MASK_ADRS_OFFSET 0x10C
#define REG_PCIE_X2_PF_CTRL_STATUS_N5_ADRS_OFFSET 0x154
#define REG_PCIE_X2_INTR_STATUS_ADRS_OFFSET 0x168
#define REG_PCIE_X2_INTR_MASK_ADRS_OFFSET 0x16c
#define REG_PCIE_X2_VF_MSI_TRG_N2_ADRS_OFFSET 0x054
#define REG_PCIE_X2_CSR_INT_OCM_OFFSET 0x03f8

#define REG_PCIE_X1_USB_MSI_INTR_MASK_ADRS_OFFSET 0x384

enum PCIE_PF_NUM { PCIE_PF_NUM_0 = 0, PCIE_PF_NUM_1, PCIE_PF_NUM_2, PCIE_PF_NUM_3, PCIE_PF_NUM_4, PCIE_MAX_PF_NUM };

struct axiado_pcie_func_info {
	u8 port;
	u8 pf_num;
	u8 vf_num;
};

/* PCIE DMA Channel numbers */
enum axiado_pcie_dma_channel {
	DMA_CHANNEL_0,
	DMA_CHANNEL_1,
	DMA_CHANNEL_2,
	DMA_CHANNEL_3,
	DMA_CHANNEL_4,
	DMA_CHANNEL_5,
	DMA_CHANNEL_6,
	DMA_CHANNEL_7
};

/* PCIE DMA modes */
enum axiado_pcie_dma_mode {
	DIRECT_MODE,
	SCATTER_GATHER_00,
	SCATTER_GATHER_01,
	SCATTER_GATHER_10,
	SCATTER_GATHER_11,
};

enum ax_pcie_dma_dir {
	AX_EP_MEMORY_HOST_TO_PDEV, /** On EP, host memory read, host -> phy.func. */
	AX_EP_MEMORY_PDEV_TO_HOST, /** On EP, host memory write, phy.func. -> host */
	AX_RC_VDM_HOST_TO_PDEV, /** On RC, VDM transmit, host -> phy.func. */
	AX_EP_VDM_PDEV_TO_HOST, /** On EP, VDM transmit, phy.func. -> host */
};

/* PCIE DMA request structure */
struct axiado_pcie_dma_request {
	enum axiado_pcie_dma_channel chan_num;
	enum axiado_pcie_dma_mode dma_mode;
	enum ax_pcie_dma_dir dma_dir;
	u8 source_list_size;
	u8 dest_list_size;
	u32 dma_size;
	u64 dma_source_ptr[MAX_DMA_LIST];
	u64 dma_dest_ptr[MAX_DMA_LIST];
	u64 dma_source_size[MAX_DMA_LIST];
	u64 dma_dest_size[MAX_DMA_LIST];
};

struct dma_cfg {
	u32 src_inf;
	u32 dest_inf;
	u32 src_addr_31_0;
	u32 src_addr_63_32;
	u32 dest_addr_31_0;
	u32 dest_addr_63_32;
	u32 length;
	u32 se_cond; // start and end conditions
	u32 irq; // interrupt enable
};

typedef void (*mbx_callback)(u32 mailbox_num, u32 *data, u32 count,
			     void *opaque);

int axiado_pcie_bar_init(struct axiado_pcie *pcie,
        enum axiado_pcie_bars bar_num, u64 base_address,
        u32 aperture_size);

u64 axiado_pcie_get_bar(struct axiado_pcie *pcie,
			struct axiado_pcie_func_info func_info,
			enum axiado_pcie_bars bar_num, u32 *bar_size);

int axiado_pcie_is_dma_busy(struct axiado_pcie *pcie,
			    enum axiado_pcie_port_num port,
			    enum axiado_pcie_dma_channel chan_num);

int axiado_pcie_dma_xfer(struct axiado_pcie *pcie,
			 enum axiado_pcie_port_num port,
			 struct axiado_pcie_dma_request *dma_request);

int axiado_pcie_trigger_msi(u8 port,
			    uint pf_num,
			    uint vector_no);

unsigned int axiado_pcie_msgbox_read(u8 port, u32 offset);

void axiado_pcie_msgbox_write(u8 port, u32 offset,
				      u32 val);

unsigned int axiado_pcie_ocm_csr_read(u8 port);

void axiado_pcie_ocm_csr_write(u8 port,	u32 val);

#endif /* __AXIADO_PCIE_EP_H */
