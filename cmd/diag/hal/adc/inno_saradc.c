// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * Bare-metal AX3000 SAR ADC driver, ported from the Linux
 * axiado-saradc IIO driver (manual conversion, polled).
 */

#include <dm.h>
#include <log.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>

#include "ax_diag.h"
#include "ax_adc.h"

/* Register offsets from SARADC base */
#define AX_SARADC_GLOBAL_CTRL		0x0004
#define AX_SARADC_MANUAL_CTRL		0x0008
#define AX_SARADC_DOUT			0x001C

/* GLOBAL_CTRL fields */
#define AX_SARADC_CH_EN_SHIFT		16	/* Bits 31:16 channel enable */
#define AX_SARADC_SAMPLE_SHIFT		5	/* Bits 6:5 sample number */
#define AX_SARADC_MODE_SHIFT		3	/* Bits 4:3 mode */
#define AX_SARADC_PD			BIT(2)	/* Power down */
#define AX_SARADC_ENABLE		BIT(0)	/* ADC controller enable */

#define AX_SARADC_ALL_CHANNELS		0xFFFF
#define AX_SARADC_CH_EN_MASK(x)		((u32)(x) << AX_SARADC_CH_EN_SHIFT)

#define AX_SARADC_SAMPLE_16		(0 << AX_SARADC_SAMPLE_SHIFT)
#define AX_SARADC_MODE			(1 << AX_SARADC_MODE_SHIFT)

/* MANUAL_CTRL: bits 4:1 = channel, bit 0 = select */
#define AX_SARADC_MANUAL_CTRL_EN(n)	((2 * (n)) + 1)

#define AX_RESOLUTION_BITS		10
#define AX_SARADC_DOUT_MASK		GENMASK(AX_RESOLUTION_BITS - 1, 0)
#define AX_SARADC_MAX_CHANNELS		16
#define AX_SARADC_CONV_CYCLES		13

/*
 * Linux reads the rate of the "refclk" clock. In bare-metal there is no
 * clock framework here, so set this to the real SARADC reference clock.
 */
#ifndef AX_SARADC_REF_CLK_HZ
#define AX_SARADC_REF_CLK_HZ		1000000UL	/* TODO: set real refclk */
#endif

static void __iomem *ax_saradc_base(void)
{
	return (void __iomem *)AX3000_CSR_BASE_ADRS_SARADC;
}

static bool ax_saradc_is_enabled(void)
{
	return readl(ax_saradc_base() + AX_SARADC_GLOBAL_CTRL) &
	       AX_SARADC_ENABLE;
}

static int ax_saradc_check_channel(int channel)
{
	if (channel < 0 || channel >= AX_SARADC_MAX_CHANNELS) {
		log_err("Invalid ADC channel: %d\n", channel);
		return -EINVAL;
	}

	return 0;
}

static int ax_saradc_check_mask(unsigned int channel_mask)
{
	if (!channel_mask || (channel_mask & ~AX_SARADC_ALL_CHANNELS)) {
		log_err("Invalid ADC channel mask: %#x\n", channel_mask);
		return -EINVAL;
	}

	return 0;
}

/* Power up the ADC (if needed) and enable the channels in @channel_mask */
static int ax_saradc_enable_channels(unsigned int channel_mask)
{
	void __iomem *base = ax_saradc_base();
	u32 reg;

	if (ax_saradc_is_enabled()) {
		/* Already running: only add the requested channels */
		reg = readl(base + AX_SARADC_GLOBAL_CTRL);
		reg |= AX_SARADC_CH_EN_MASK(channel_mask);
		writel(reg, base + AX_SARADC_GLOBAL_CTRL);
		return 0;
	}

	/* Same sequence as the Linux probe: PD first, then enable */
	writel(AX_SARADC_PD, base + AX_SARADC_GLOBAL_CTRL);

	reg = AX_SARADC_CH_EN_MASK(channel_mask) |
	      AX_SARADC_SAMPLE_16 |
	      AX_SARADC_MODE |
	      AX_SARADC_ENABLE;
	writel(reg, base + AX_SARADC_GLOBAL_CTRL);

	if (!ax_saradc_is_enabled()) {
		log_err("SARADC initialize failed\n");
		return -ENODEV;
	}

	return 0;
}

/* Trigger one manual conversion and read the 10-bit result */
static int ax_saradc_convert(int channel, unsigned int *data)
{
	void __iomem *base = ax_saradc_base();
	unsigned long usecs;
	u32 reg;

	reg = readl(base + AX_SARADC_GLOBAL_CTRL);
	if (!(reg & AX_SARADC_ENABLE)) {
		log_err("SARADC not initialized\n");
		return -ENODEV;
	}

	if (!(reg & AX_SARADC_CH_EN_MASK(BIT(channel)))) {
		log_err("ADC channel %d not started\n", channel);
		return -EINVAL;
	}

	/* Select the channel and trigger the conversion */
	writel(AX_SARADC_MANUAL_CTRL_EN(channel), base + AX_SARADC_MANUAL_CTRL);

	/* Hardware needs 13 conversion cycles at the reference clock */
	usecs = DIV_ROUND_UP(AX_SARADC_CONV_CYCLES * 1000000UL,
			     AX_SARADC_REF_CLK_HZ);
	udelay(usecs);

	*data = readl(base + AX_SARADC_DOUT) & AX_SARADC_DOUT_MASK;

	/* Stop manual conversion */
	writel(0x0, base + AX_SARADC_MANUAL_CTRL);

	return 0;
}

static int ax_saradc_start_channel(int channel)
{
	int ret;

	ret = ax_saradc_check_channel(channel);
	if (ret)
		return ret;

	return ax_saradc_enable_channels(BIT(channel));
}

static int ax_saradc_start_channels(unsigned int channel_mask)
{
	int ret;

	ret = ax_saradc_check_mask(channel_mask);
	if (ret)
		return ret;

	return ax_saradc_enable_channels(channel_mask);
}

static int ax_saradc_channel_data(int channel, unsigned int *data)
{
	int ret;

	if (!data)
		return -EINVAL;

	ret = ax_saradc_check_channel(channel);
	if (ret)
		return ret;

	return ax_saradc_convert(channel, data);
}

/*
 * Read every channel set in @channel_mask. Results are packed in
 * ascending channel order: data[0] is the lowest channel in the mask.
 * @data must hold hweight32(channel_mask) entries.
 */
static int ax_saradc_channels_data(unsigned int channel_mask,
				   unsigned int *data)
{
	unsigned int idx = 0;
	int ch, ret;

	if (!data)
		return -EINVAL;

	ret = ax_saradc_check_mask(channel_mask);
	if (ret)
		return ret;

	for (ch = 0; ch < AX_SARADC_MAX_CHANNELS; ch++) {
		if (!(channel_mask & BIT(ch)))
			continue;

		ret = ax_saradc_convert(ch, &data[idx]);
		if (ret)
			return ret;

		idx++;
	}

	return 0;
}

/* Deselect any channel and power the ADC down (clears ENABLE and CH_EN) */
static int ax_saradc_stop(void)
{
	void __iomem *base = ax_saradc_base();

	writel(0x0, base + AX_SARADC_MANUAL_CTRL);
	writel(AX_SARADC_PD, base + AX_SARADC_GLOBAL_CTRL);

	return 0;
}

struct ax_adc_ops adc = {
	.start_channel	= ax_saradc_start_channel,
	.start_channels	= ax_saradc_start_channels,
	.channel_data	= ax_saradc_channel_data,
	.channels_data	= ax_saradc_channels_data,
	.stop		= ax_saradc_stop,
};
