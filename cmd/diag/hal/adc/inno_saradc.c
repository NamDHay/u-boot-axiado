// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>

#include "ax_diag.h"
#include "ax_adc.h"

/* Register offsets from SARADC base */
#define AX_SARADC_FIFO_ACCESS 0x0000
#define AX_SARADC_GLOBAL_CTRL 0x0004
#define AX_SARADC_MANUAL_CTRL 0x0008
#define AX_SARADC_DOUT        0x001C

/* Field definitions for GLOBAL_CTRL register */
#define AX_SARADC_CH_EN_SHIFT      16  /* Bits 31:16 channel enable */
#define AX_SARADC_SAMPLE_SHIFT      5  /* Bits 6:5 sample number */
#define AX_SARADC_MODE_SHIFT        3  /* Bits 4:3 mode */
#define AX_SARADC_PD                BIT(2)  /* Power down */
#define AX_SARADC_ENABLE            BIT(0)  /* ADC controller enable */

/* Channel enables: 16b mask for 16 single-ended inputs */
#define AX_SARADC_ALL_CHANNELS     0xFFFF
#define AX_SARADC_CH_EN_MASK(x)    ((u32)((x) << AX_SARADC_CH_EN_SHIFT))

/* Sample, mode fields */
#define AX_SARADC_SAMPLE_16        (0 << AX_SARADC_SAMPLE_SHIFT)
#define AX_SARADC_MODE             (1 << AX_SARADC_MODE_SHIFT)

#define AX_SARADC_MANUAL_CTRL_EN(n) ((2 * (n)) + 1)
#define AX_RESOLUTION_BITS          10
#define AX_VREF_FIXED               1100
#define AX_SARADC_MAX_CHANNELS      16
#define AX_SARADC_CONV_CYCLES       13

#define AX_SARADC_TIMEOUT			(100 * 1000)

static inline int inno_saradc_check_init(void)
{
	void __iomem *base = (void __iomem *)AX3000_CSR_BASE_ADRS_SARADC;
    u32 reg;
    reg = readl(base + AX_SARADC_GLOBAL_CTRL);
    if (reg & BIT(0))
        return 0;

    return -ENODEV;
}

static int inno_saradc_init(unsigned int channel_mask)
{
	void __iomem *base = (void __iomem *)AX3000_CSR_BASE_ADRS_SARADC;
    u32 reg;
    u32 ret;

    ret = inno_saradc_check_init();
    if (ret < 0) {
        printf("SARADC already init\n");
        return 0;
    }

    writel(AX_SARADC_PD, base + AX_SARADC_GLOBAL_CTRL);

    reg = AX_SARADC_CH_EN_MASK(channel_mask) |
        AX_SARADC_SAMPLE_16 |
        AX_SARADC_MODE |
        AX_SARADC_ENABLE;

    writel(reg, base + AX_SARADC_GLOBAL_CTRL);

    ret = inno_saradc_check_init();
    if (ret < 0) {
        printf("SARADC initialize failed\n");
        return ret;
    }
    return 0;
}

static int inno_saradc_start_channel(int channel) 
{
    if (channel > AX_SARADC_MAX_CHANNELS) {
        printf("Invalid ADC channel: %d\n", channel);
        return -ECHRNG;
    }

    return inno_saradc_init(channel);
}

static int inno_saradc_start_channels(unsigned int channel_mask) 
{
    if (channel_mask > AX_SARADC_ALL_CHANNELS) {
        printf("Invalid ADC channel mask: %d\n", channel_mask);
        return -ECHRNG;
    }

    return inno_saradc_init(channel_mask);
}

static int inno_saradc_channel_data(int channel, unsigned int *data) 
{
	void __iomem *base = (void __iomem *)AX3000_CSR_BASE_ADRS_SARADC;
    int ret;
    if (channel > AX_SARADC_MAX_CHANNELS) {
        printf("Invalid ADC channel: %d\n", channel);
        return -ECHRNG;
    }

    ret = inno_saradc_check_init();
    if (ret < 0) {
        printf("SARADC not initialize\n");
        return ret;
    }

    writel(AX_SARADC_MANUAL_CTRL_EN(channel), 
            base + AX_SARADC_MANUAL_CTRL);

    *data = readl(base + AX_SARADC_DOUT);

    writel(0x0, base + AX_SARADC_MANUAL_CTRL);

    return 0;
}

extern struct ax_adc_ops adc = {
    .start_channel = inno_saradc_start_channel,
    .start_channels = inno_saradc_start_channels,
    .channel_data = inno_saradc_channel_data,
};
