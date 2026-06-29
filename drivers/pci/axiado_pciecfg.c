// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2023-25 Axiado Corporation.
 *
 * Axiado PLDA PCIe Controller Driver
 *
 */

#include <linux/compat.h>
#include <linux/bug.h>
#include <dm/device.h>
#include <linux/delay.h>

#include "axiado-pcie.h"

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

/**
 * abs_diff - return absolute value of the difference between the arguments
 * @a: the first argument
 * @b: the second argument
 *
 * @a and @b have to be of the same type. With this restriction we compare
 * signed to signed and unsigned to unsigned. The result is the subtraction
 * the smaller of the two from the bigger, hence result is always a positive
 * value.
 *
 * Return: an absolute value of the difference between the @a and @b.
 */
#define abs_diff(a, b) ({			\
	typeof(a) __a = (a);			\
	typeof(b) __b = (b);			\
	(void)(&__a == &__b);			\
	__a > __b ? (__a - __b) : (__b - __a);	\
})

typedef void (*mbx_callback)(u32 mailbox_num, u32 *data, u32 count,
			     void *opaque);
mbx_callback mailbox_callback[MAX_PORT][MAX_PF][MAX_VF + 1][MAX_PF_MBX];
EXPORT_SYMBOL(mailbox_callback);

void *user_data[MAX_PF][MAX_VF + 1][MAX_MBX];

struct axiado_pcie_config_data {
	u64 bar0_start;
	u64 bar2_start;
	u64 bar4_start;
	u32 bar0_aperture_size;
	u32 bar2_aperture_size;
	u32 bar4_aperture_size;
} axiado_pcie_conf;

/* helper function for bar number sanity check */
static bool axiado_pcie_validate_bar(enum axiado_pcie_bars bar_num)
{
	return ((bar_num == BAR_01) || (bar_num == BAR_23) ||
		(bar_num == BAR_45));
}

/* API to initialize the BAR */
/* NOTE: For bar4/5 which is the mailbox mapping, aperture size should be 4K */
int axiado_pcie_bar_init(struct udevice *dev,
			 enum axiado_pcie_bars bar_num, u64 base_address,
			 u32 aperture_size)
{
	struct axiado_pcie *ax_pcie = dev_get_priv(dev);
	u32 high_offset = 0;
	u32 low_offset = 0;
	u64 temp_64 = 0;
	u32 temp = 0;
	u32 bar_offset = 0;
	u32 aperture_offset = 0;

	if (!axiado_pcie_validate_bar(bar_num)) {
		pr_err(" Invalid bar number %d\n", bar_num);
		return -EINVAL;
	}

	/* apeture size must be multiple of 4KB */
	if ((aperture_size % (AXIADO_PAGE_SIZE)) != 0) {
		pr_err("  must be multiple of 4KB %d\n", aperture_size);
		return -EINVAL;
	}

	/* store the start and aperture in the global struture pcie_config */
	if (bar_num == BAR_01) {
		axiado_pcie_conf.bar0_start = base_address;
		axiado_pcie_conf.bar0_aperture_size = aperture_size;
		low_offset = REG_PCIE_PCIE_SRIOV_31_0_ADRS_OFFSET +
			     2 * BAR_SIZE_BYTES;
		high_offset = REG_PCIE_PCIE_SRIOV_31_0_ADRS_OFFSET +
			      3 * BAR_SIZE_BYTES;
		bar_offset = 0;
	} else if (bar_num == BAR_23) {
		axiado_pcie_conf.bar2_start = base_address;
		axiado_pcie_conf.bar2_aperture_size = aperture_size;
		low_offset = REG_PCIE_PCIE_SRIOV_31_0_ADRS_OFFSET +
			     4 * BAR_SIZE_BYTES;
		high_offset = REG_PCIE_PCIE_SRIOV_31_0_ADRS_OFFSET +
			      5 * BAR_SIZE_BYTES;
		bar_offset = 4;
	} else if (bar_num == BAR_45) {
		axiado_pcie_conf.bar4_start = base_address;
		axiado_pcie_conf.bar4_aperture_size = aperture_size;
		low_offset = REG_PCIE_PCIE_SRIOV_31_0_ADRS_OFFSET +
			     6 * BAR_SIZE_BYTES;
		high_offset = REG_PCIE_PCIE_SRIOV_31_0_ADRS_OFFSET +
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
	writel(temp, ax_pcie->ext + low_offset);
	writel(temp_64, ax_pcie->ext + high_offset);

	aperture_offset = (aperture_size / AXIADO_PAGE_SIZE) - 1;

	/* Need to set the aperture high, low and size */
	writel(base_address >> 3,
		  ax_pcie->ext +
			  REG_PCIE_AXI_MST0_VF_APER_LO_LSBS_N3_ADRS_OFFSET +
			  bar_offset);

	writel(
		(0x0
		 << REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_FLD_REMAP_DIS_LSB) |
			(aperture_offset
			 << REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_FLD_APER_SIZE_LSB),
		ax_pcie->ext +
			REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_ADRS_OFFSET +
			bar_offset);

	writel((((base_address >> 3) & aperture_offset)
		   << REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_FLD_ADRS_LO_LSB),
		  ax_pcie->ext +
			  REG_PCIE_AXI_MST0_VF_APER_HI_LSBS_N3_ADRS_OFFSET +
			  bar_offset);

	writel(0x0,
		  ax_pcie->ext +
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

/* get the bar address (axiado address) for the requested PF:VF pair */
u64 axiado_pcie_get_bar(struct udevice *dev,
			struct axiado_pcie_func_info func_info,
			enum axiado_pcie_bars bar_num, u32 *bar_size)
{
	u64 bar_address = 0;

	if (!axiado_validate_func_info(func_info))
		return -EINVAL;

	if (!axiado_pcie_validate_bar(bar_num)) {
		pr_err("Invalid Bar number %d\n", bar_num);
		return -EINVAL;
	}

	if (bar_num == BAR_01) {
		if (func_info.vf_num == 0) {
			bar_address =
				axiado_pcie_conf.bar0_start +
				((func_info.port == PORT_X1) ?
					 0 :
					 (MAX_VF *
					  axiado_pcie_conf.bar0_aperture_size));
			*bar_size =
				axiado_pcie_conf.bar0_aperture_size * MAX_VF;
		} else {
			bar_address = axiado_pcie_conf.bar0_start +
				      (axiado_pcie_conf.bar0_aperture_size) *
					      (func_info.vf_num - 1);
			*bar_size = axiado_pcie_conf.bar0_aperture_size;
		}
	} else if (bar_num == BAR_23) {
		if (func_info.vf_num == 0) {
			bar_address =
				axiado_pcie_conf.bar2_start +
				((func_info.port == PORT_X1) ?
					 0 :
					 (MAX_VF *
					  axiado_pcie_conf.bar2_aperture_size));
			*bar_size =
				axiado_pcie_conf.bar2_aperture_size * MAX_VF;
		} else {
			bar_address = axiado_pcie_conf.bar2_start +
				      (axiado_pcie_conf.bar2_aperture_size) *
					      (func_info.vf_num - 1);
			*bar_size = axiado_pcie_conf.bar2_aperture_size;
		}
	} else if (bar_num == BAR_45) {
		if (func_info.vf_num == 0) {
			bar_address =
				axiado_pcie_conf.bar4_start +
				((func_info.port == PORT_X1) ?
					 0 :
					 (MAX_VF *
					  axiado_pcie_conf.bar4_aperture_size));
			*bar_size =
				axiado_pcie_conf.bar4_aperture_size * MAX_VF;
		} else {
			bar_address = axiado_pcie_conf.bar4_start +
				      ((axiado_pcie_conf.bar4_aperture_size) *
				       (func_info.vf_num - 1));
			*bar_size = axiado_pcie_conf.bar4_aperture_size;
		}
	}

	return bar_address;
}
EXPORT_SYMBOL(axiado_pcie_get_bar);

/* register callback for the specific PF:VF:MBX */
int axiado_pcie_register_mailbox_callback(struct axiado_pcie_func_info func_info,
					  u8 mailbox_num, mbx_callback callback,
					  void *opaque)
{
	if (!axiado_validate_func_info(func_info))
		return -EINVAL;

	if (mailbox_num > MAX_MBX) {
		pr_err("Invalid mailbox number : %d\n", mailbox_num);
		return -EINVAL;
	}

	if ((func_info.vf_num != 0) && (mailbox_num != 0)) {
		pr_err("For VF mailbox number should be 0\n");
		return -EINVAL;
	}

	if (func_info.vf_num > MAX_VF) {
		pr_err("Invalid VF function number : %d\n", func_info.vf_num);
		return -EINVAL;
	}

	if (callback == NULL) {
		pr_err("NULL callback provided\n");
		return -EINVAL;
	}

	if (func_info.vf_num == 0) {
		mailbox_callback[func_info.port][func_info.pf_num]
				[func_info.vf_num][mailbox_num] = callback;
		user_data[func_info.pf_num][func_info.vf_num][mailbox_num] =
			opaque;
	} else {
		/* Each VF function has 1 mailbox @0 */
		mailbox_callback[func_info.port][func_info.pf_num]
				[func_info.vf_num][0] = callback;
		user_data[func_info.pf_num][func_info.vf_num][0] = opaque;
	}
	return 0;
}
EXPORT_SYMBOL(axiado_pcie_register_mailbox_callback);

static void axiado_pcie_set_dma(struct udevice *dev,
				struct dma_cfg d_cfg,
				enum axiado_pcie_port_num port,
				enum axiado_pcie_dma_channel channel_num,
				enum axiado_pcie_dma_mode mode)
{
	struct axiado_pcie *ax_pcie = dev_get_priv(dev);
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
		ax_pcie->cfg +
			offset[0]); /* [3:0] source interface set to axi4 master */
	writel(
		d_cfg.dest_inf,
		ax_pcie->cfg +
			offset[1]); /* [3:0] destination interface set to PCIe interface */
	writel(
		d_cfg.src_addr_31_0,
		ax_pcie->cfg +
			offset[2]); /* set source address to SRAM base address */
	writel(d_cfg.src_addr_63_32, ax_pcie->cfg + offset[3]);
	writel(
		d_cfg.dest_addr_31_0,
		ax_pcie->cfg +
			offset[4]); /* destination rootport's (x1) sram mapped address */
	writel(d_cfg.dest_addr_63_32, ax_pcie->cfg + offset[5]);
	writel(d_cfg.length,
		  ax_pcie->cfg + offset[6]); /* transfer size 4k bytes */
	writel(ctrl, ax_pcie->cfg + offset[7]);
}

/* helper functions to find control, status offsets and start DMA */
static u32 find_dma_control_offset(enum axiado_pcie_port_num port,
				   enum axiado_pcie_dma_channel channel_num)
{
	return (REG_PCIE_DMA_BASE +
		(REG_PCIE_DMA0_CONFIG_SPACE_DMA_CONTROL_ADRS_OFFSET +
		 (channel_num * DMA_CHANNEL_MF)));
}

static void axiado_pcie_start_dma(struct udevice *dev,
				  enum axiado_pcie_port_num port,
				  enum axiado_pcie_dma_channel chan_num)
{
	struct axiado_pcie *ax_pcie = dev_get_priv(dev);
	u32 offset = 0;

	offset = find_dma_control_offset(port, chan_num);
	writel(DMA_START, ax_pcie->cfg + offset);
}

/*check if a specific DMA channel is free. 0 for free, 1 for busy */
int axiado_pcie_is_dma_busy(struct udevice *dev,
			    enum axiado_pcie_port_num port,
			    enum axiado_pcie_dma_channel chan_num)
{
	struct axiado_pcie *ax_pcie = dev_get_priv(dev);
	u32 offset = 0;

	offset = find_dma_control_offset(port, chan_num);
	return ((readl(ax_pcie->cfg + offset) & 0x1) != 0x0);
}
EXPORT_SYMBOL(axiado_pcie_is_dma_busy);

static int
axiado_pcie_validate_dma_request(struct axiado_pcie_dma_request *dma_request)
{
	u64 diff = 0;

	if (dma_request->dma_mode == DIRECT_MODE) {
		diff = abs_diff(dma_request->dma_source_ptr[0],
			   dma_request->dma_dest_ptr[0]);

		if (unlikely(!IS_ALIGNED(diff, 32))) {
			pr_err("source: 0x%lx destination: 0x%lx diff: 0x%llx\n",
			       (unsigned long)dma_request->dma_source_ptr[0],
			       (unsigned long)dma_request->dma_dest_ptr[0],
			       diff);
			return -EINVAL;
		}
	}

	return 0;
}

int axiado_pcie_dma_xfer(struct udevice *dev,
			 enum axiado_pcie_port_num port,
			 struct axiado_pcie_dma_request *dma_request)
{
	struct axiado_pcie *ax_pcie = dev_get_priv(dev);
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
		pr_err("Unknown DMA direction\n");
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
		pr_err("Unknown DMA direction\n");
		return -EINVAL;
	}

	axiado_pcie_set_dma(dev, d_cfg, port, dma_request->chan_num,
			    dma_request->dma_mode);
	__asm__ volatile("dsb sy");
	__asm__ volatile("isb");
	axiado_pcie_start_dma(dev, port, dma_request->chan_num);

	timeout = DMA_TIMEOUT;
	do {
		if (!axiado_pcie_is_dma_busy(dev, port, dma_request->chan_num))
			break;
		usleep_range(1000, 2000);
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
EXPORT_SYMBOL(axiado_pcie_dma_xfer);

/* API to trigger msi to the host for the specific PF:VF:vector number */
int axiado_pcie_trigger_msi(struct udevice *dev,
			    struct axiado_pcie_func_info func_info,
			    u8 vector_no)
{
	struct axiado_pcie *ax_pcie = dev_get_priv(dev);
	u32 temp;
	u8 port;
	u32 msi_trg;

	if (!axiado_validate_func_info(func_info))
		return -EINVAL;

	if ((func_info.vf_num == 0) && (vector_no > MAX_MSI_VECTOR)) {
		pr_err("invalid MSI vector number %d\n", vector_no);
		return -EINVAL;
	}

	/* Set msi_trg bits: 5-12 */
	msi_trg = 1 << (vector_no + 5);

	port = func_info.port;
	if (func_info.vf_num == 0) {
		if (port == PORT_X1) {
			writel(
				msi_trg,
				ax_pcie->ext +
					REG_PCIE_X1_PF_CTRL_STATUS_ADRS_OFFSET);
			/* Clear vector pending bit after trigger */
			temp = readl(ax_pcie->cfg +
					REG_PCIE_ISTATUS_HOST_ADRS_OFFSET);
			temp = temp | VEC_CLR_BIT;
			writel(temp,
				  ax_pcie->cfg +
					  REG_PCIE_ISTATUS_HOST_ADRS_OFFSET);
		} else {
			writel(
				msi_trg,
				ax_pcie->ext +
					REG_PCIE_X2_PF_CTRL_STATUS_ADRS_OFFSET);
			/* Clear vector pending bit after trigger */
			temp = readl(ax_pcie->cfg +
					REG_PCIE_ISTATUS_HOST_ADRS_OFFSET);
			temp = temp | VEC_CLR_BIT;
			writel(temp,
				  ax_pcie->cfg +
					  REG_PCIE_ISTATUS_HOST_ADRS_OFFSET);
		}
	} else {
		temp = func_info.vf_num - 1;
		if (temp < 32)
			writel(
				(1 << temp),
				ax_pcie->ext +
					REG_PCIE_X2_VF_MSI_TRG_N2_ADRS_OFFSET);
		else
			writel(
				(1 << (32 - temp)),
				ax_pcie->ext +
					REG_PCIE_X2_VF_MSI_TRG_N2_ADRS_OFFSET +
					4);
	}
	return 0;
}
EXPORT_SYMBOL(axiado_pcie_trigger_msi);

unsigned int axiado_pcie_msgbox_read(struct udevice *dev, u32 offset)
{
	struct axiado_pcie *ax_pcie = dev_get_priv(dev);

	return readl(ax_pcie->ext + offset);
}
EXPORT_SYMBOL(axiado_pcie_msgbox_read);

void axiado_pcie_msgbox_write(struct udevice *dev, u32 offset,
				      u32 val)
{
	struct axiado_pcie *ax_pcie = dev_get_priv(dev);

	writel(val, (ax_pcie->ext + offset));
}
EXPORT_SYMBOL(axiado_pcie_msgbox_write);

/* These APIs are used in MCTP driver to update the ring buffer address */
unsigned int axiado_pcie_ocm_csr_read(struct udevice *dev)
{
	struct axiado_pcie *ax_pcie = dev_get_priv(dev);

	return readl(ax_pcie->cfg + REG_PCIE_X2_CSR_INT_OCM_OFFSET);
}
EXPORT_SYMBOL(axiado_pcie_ocm_csr_read);

void axiado_pcie_ocm_csr_write(struct udevice *dev, u32 val)
{
    struct axiado_pcie *ax_pcie = dev_get_priv(dev);

	writel(val, (ax_pcie->cfg + REG_PCIE_X2_CSR_INT_OCM_OFFSET));
}
EXPORT_SYMBOL(axiado_pcie_ocm_csr_write);

