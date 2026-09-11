// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>

#include "ax_diag.h"
#include "ax_sgpio.h"
#include "ax_pinmux.h"

#define CONFIG_MAX_SGPIO        2
#define AX_SGPIO_MAX_DATA_LEN 16

#define SEND_LEN_REG_POS    0
#define RECV_LEN_REG_POS    15

//============ SGPIO ================
#define REG_SGPIO_SLICE_MUX_CFG0_ADRS_OFFSET           0x0000
#define REG_SGPIO_SLICE_MUX_CFG1_ADRS_OFFSET           0x0004
#define REG_SGPIO_SLICE_MUX_CFG2_ADRS_OFFSET           0x0008
#define REG_SGPIO_SLICE_MUX_CFG3_ADRS_OFFSET           0x000C
#define REG_SGPIO_SLICE_MUX_CFG4_ADRS_OFFSET           0x0010
#define REG_SGPIO_REG_LD_ADRS_OFFSET                   0x0014
#define REG_SGPIO_REG_DO_ADRS_OFFSET                   0x0054
#define REG_SGPIO_REG_DI_ADRS_OFFSET                   0x0094
#define REG_SGPIO_REG_OE_ADRS_OFFSET                   0x00D4
#define REG_SGPIO_REG_SS_LD_W_ADRS_OFFSET              0x00D8
#define REG_SGPIO_REG_SS_LD_R_ADRS_OFFSET              0x0118
#define REG_SGPIO_REG_SS_DO_ADRS_OFFSET                0x0158
#define REG_SGPIO_REG_SS_DI_ADRS_OFFSET                0x0198
#define REG_SGPIO_REG_SS_OE_ADRS_OFFSET                0x01D8
#define REG_SGPIO_PRESET0_ADRS_OFFSET                  0x01DC
#define REG_SGPIO_PRESET1_ADRS_OFFSET                  0x01E0
#define REG_SGPIO_PRESET2_ADRS_OFFSET                  0x01E4
#define REG_SGPIO_PRESET3_ADRS_OFFSET                  0x01E8
#define REG_SGPIO_PRESET4_ADRS_OFFSET                  0x01EC
#define REG_SGPIO_COUNT0_ADRS_OFFSET                   0x01F0
#define REG_SGPIO_COUNT1_ADRS_OFFSET                   0x01F4
#define REG_SGPIO_COUNT2_ADRS_OFFSET                   0x01F8
#define REG_SGPIO_COUNT3_ADRS_OFFSET                   0x01FC
#define REG_SGPIO_COUNT4_ADRS_OFFSET                   0x0200
#define REG_SGPIO_POS0_ADRS_OFFSET                     0x0204
#define REG_SGPIO_POS1_ADRS_OFFSET                     0x0208
#define REG_SGPIO_POS2_ADRS_OFFSET                     0x020C
#define REG_SGPIO_POS3_ADRS_OFFSET                     0x0210
#define REG_SGPIO_POS4_ADRS_OFFSET                     0x0214
#define REG_SGPIO_CTRL_ENABLE_ADRS_OFFSET              0x0218
#define REG_SGPIO_CTRL_ENABLE_POS_ADRS_OFFSET          0x021C
#define REG_SGPIO_CTRL_DISABLE_ADRS_OFFSET             0x0220
#define REG_SGPIO_MASK_ADRS_OFFSET                     0x0224
#define REG_SGPIO_STATUS_ADRS_OFFSET                   0x0228
#define REG_SGPIO_ALIGN_ADRS_OFFSET                    0x22C 

struct axiado_sgpio {
    void __iomem *base;
    int irq;
};

static struct axiado_sgpio sgpio_dev[CONFIG_MAX_SGPIO] = {
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_SGPIO_0, .irq = 157 },
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_SGPIO_1, .irq = 158 },
};

void axiado_sgpio_interrupt_handler(void *arg)
{
    struct axiado_sgpio *dev = arg;

    if (!dev)
        return;

    return;
}

static int axiado_sgpio_request(u8 bus, u16 total_pin) {  
    char name[8];
    sprintf(name, "SGPIO%d", bus);
    ax_pinmux_set_state(name);
    return 0;
}

static int axiado_sgpio_set_value(u8 bus, const u32 *data_out_slice, size_t len) {
    struct axiado_sgpio *dev;
    u32 load_data = 0x7fffffff;
    u32 preset_val = 0x1f;          /* 3.125 MHz */
    u32 count_val = 0x1f;
    u32 pos_val = 0x01FF01FF;
    int i;

    if (bus > CONFIG_MAX_SGPIO)
        return -EINVAL;

    if (len > AX_SGPIO_MAX_DATA_LEN)
        return -EINVAL;

    dev = &sgpio_dev[bus];

	/*Configuration slice-0(A) for clk_generation*/
	writel(0x306, dev->base + REG_SGPIO_SLICE_MUX_CFG0_ADRS_OFFSET); /* bit4 = 0:CTRL_EN_POS */
	writel(preset_val, dev->base + REG_SGPIO_PRESET0_ADRS_OFFSET);
	writel(count_val, dev->base + REG_SGPIO_COUNT0_ADRS_OFFSET);
	writel(pos_val, dev->base + REG_SGPIO_POS0_ADRS_OFFSET);

	/*Configuration slice-1(B) for load_capture*/
	writel(0x304, dev->base + REG_SGPIO_SLICE_MUX_CFG1_ADRS_OFFSET);
	for (i = 0; i < len; i++) {
        /* shift reg 0xfffffffe */
		writel(load_data, dev->base + REG_SGPIO_REG_LD_ADRS_OFFSET + i * 4); 
        /* shift reg ss (master mode) 0xfffffffe */
		writel(load_data, dev->base + REG_SGPIO_REG_SS_LD_W_ADRS_OFFSET + i * 4);
	}
	writel(preset_val, dev->base + REG_SGPIO_PRESET1_ADRS_OFFSET);
	writel(count_val, dev->base + REG_SGPIO_COUNT1_ADRS_OFFSET);
	writel(pos_val, dev->base + REG_SGPIO_POS1_ADRS_OFFSET);

	/*Configurating slice-2(C), dout to pin2*/
	writel(0x304, dev->base + REG_SGPIO_SLICE_MUX_CFG2_ADRS_OFFSET);
    for (i = 0; i < len; i++) {
        writel(data_out_slice[i], dev->base + REG_SGPIO_REG_DO_ADRS_OFFSET + i * 4);
        writel(data_out_slice[i], dev->base + REG_SGPIO_REG_SS_DO_ADRS_OFFSET + i * 4);
    }
	writel(preset_val, dev->base + REG_SGPIO_PRESET2_ADRS_OFFSET);
	writel(count_val, dev->base + REG_SGPIO_COUNT2_ADRS_OFFSET);
	writel(pos_val, dev->base + REG_SGPIO_POS2_ADRS_OFFSET);

	/*Configurating slice-3(D), din to pin3*/
	writel(0x144, dev->base + REG_SGPIO_SLICE_MUX_CFG3_ADRS_OFFSET); //0x14C note bit 6:0x104 or 0x304
	writel(preset_val, dev->base + REG_SGPIO_PRESET3_ADRS_OFFSET);
	writel(count_val, dev->base + REG_SGPIO_COUNT3_ADRS_OFFSET);
	writel(pos_val, dev->base + REG_SGPIO_POS3_ADRS_OFFSET);

	/*Configuration slice-4(E) (OE for output pin)*/
	writel(0x104, dev->base + REG_SGPIO_SLICE_MUX_CFG4_ADRS_OFFSET);
	writel(0xffffffff, dev->base + REG_SGPIO_REG_OE_ADRS_OFFSET);
	writel(0xffffffff, dev->base + REG_SGPIO_REG_SS_OE_ADRS_OFFSET);
	writel(preset_val, dev->base + REG_SGPIO_PRESET4_ADRS_OFFSET);
	writel(count_val, dev->base + REG_SGPIO_COUNT4_ADRS_OFFSET);
	writel(pos_val, dev->base + REG_SGPIO_POS4_ADRS_OFFSET);

	writel(0xffff, dev->base + REG_SGPIO_CTRL_ENABLE_ADRS_OFFSET);
	writel(0xffff, dev->base + REG_SGPIO_CTRL_ENABLE_POS_ADRS_OFFSET);

    u32 temp;
    for (i = 0; i < 3; i++) {
        temp = 0;
		while ((temp & 0x00000140) != 0x00000140) {
			temp = readl(dev->base + REG_SGPIO_STATUS_ADRS_OFFSET);
			/* printf("SGPIO:%8p STATUS REG: %08x\r\n", dev->base, temp); */
		}
    }

    return 0;
}

static int axiado_sgpio_get_value(u8 bus, u32 *data_in_slice, size_t len) {
    struct axiado_sgpio *dev;
    u32 preset_val = 0x1f;          /* 3.125 MHz */
    u32 count_val = 0x1f;
    u32 pos_val = 0x01FF01FF;
    int i;

    if (bus > CONFIG_MAX_SGPIO)
        return -EINVAL;

    if (len > AX_SGPIO_MAX_DATA_LEN)
        return -EINVAL;

    dev = &sgpio_dev[bus];

	/*Configurating slice-3(D), din to pin3*/
	writel(0x14C, dev->base + REG_SGPIO_SLICE_MUX_CFG3_ADRS_OFFSET); //0x14C note bit 6:0x104 or 0x304
	writel(preset_val, dev->base + REG_SGPIO_PRESET3_ADRS_OFFSET);
	writel(count_val, dev->base + REG_SGPIO_COUNT3_ADRS_OFFSET);
	writel(pos_val, dev->base + REG_SGPIO_POS3_ADRS_OFFSET);

	/*Configuration slice-1(B) for load_capture*/
	writel(0x14C, dev->base + REG_SGPIO_SLICE_MUX_CFG1_ADRS_OFFSET);
	writel(preset_val, dev->base + REG_SGPIO_PRESET1_ADRS_OFFSET);
	writel(count_val, dev->base + REG_SGPIO_COUNT1_ADRS_OFFSET);
	writel(pos_val, dev->base + REG_SGPIO_POS1_ADRS_OFFSET);

	writel(0xffff, dev->base + REG_SGPIO_CTRL_ENABLE_ADRS_OFFSET);
	writel(0xffff, dev->base + REG_SGPIO_CTRL_ENABLE_POS_ADRS_OFFSET);

    u32 temp;
    for (i = 0; i < 3; i++) {
        temp = 0;
		while ((temp & 0x00000140) != 0x00000140) {
			temp = readl(dev->base + REG_SGPIO_STATUS_ADRS_OFFSET);
			/* printf("SGPIO:%8p STATUS REG: %08x\r\n", dev->base, temp); */
		}
    }

	writel(0x3, dev->base + REG_SGPIO_ALIGN_ADRS_OFFSET);

	temp = 0;
	while ((temp & 0x00000004) != 0x00000004) {
		temp = readl(dev->base + REG_SGPIO_ALIGN_ADRS_OFFSET);
		/* printf("SGPIO ALIGN: %08x\r\n", temp); */
	}

	temp = 0;
	while ((temp & 0x00000140) != 0x00000140) {
		temp = readl(dev->base + REG_SGPIO_STATUS_ADRS_OFFSET);
		/* printf("SGPIO STATUS REG: %08x\r\n", temp); */
	}

	for (int i = 0; i < len; i++) {
		data_in_slice[i] = readl(
			dev->base + REG_SGPIO_REG_SS_DI_ADRS_OFFSET + (0x3C - i * 4));
	}
    return 0;
}

struct ax_sgpio_ops sgpio = {
    .request = axiado_sgpio_request,
    .set_value = axiado_sgpio_set_value,
    .get_value = axiado_sgpio_get_value,
};
