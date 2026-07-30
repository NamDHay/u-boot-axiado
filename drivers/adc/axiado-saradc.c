// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2014, Axiado Co., Ltd
 *
 * Axiado SARADC driver for U-Boot
 */

#include <config.h>
#include <asm/io.h>
#include <adc.h>
#include <clk.h>
#include <dm.h>
#include <errno.h>
#include <reset.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/printk.h>
#include <power/regulator.h>

#define usleep_range(a, b) udelay((b))

/* Register offsets from SARADC base */
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

struct axiado_saradc_priv {
	void __iomem *regs;
	struct clk *clk;
	unsigned long clk_rate;
	struct udevice *vref;
	// struct regulator *vref;
};

/**
 * axiado_saradc_conversion() - trigger and read a polled ADC conversion
 * @priv:   driver state pointer
 * @chan:   channel number [0..15]
 * val:     raw voltage
 *
 * Returns 0 on success, negative error code on failure.
 *
 * Starts manual conversion for channel chan, waits for 13 cycles as per
 * hardware spec, then reads the 10-bit result.
 */

static int axiado_saradc_conversion(struct axiado_saradc_priv *priv,
				    unsigned int channel, int *val)
{
	unsigned long usecs;
	/* Select the channel to be used and trigger conversion */
	writel(AX_SARADC_MANUAL_CTRL_EN(channel),
			priv->regs + AX_SARADC_MANUAL_CTRL);

	if (!priv->clk_rate)
		return -EINVAL;

	/* Hardware requires 13 conversion cycles at clk_rate */
	usecs = DIV_ROUND_UP(AX_SARADC_CONV_CYCLES * 1000000, priv->clk_rate);
	usleep_range(usecs, usecs + 10);

	*val = readl(priv->regs + AX_SARADC_DOUT);

	/* Stop manual conversion */
	writel(0, priv->regs + AX_SARADC_MANUAL_CTRL);
	return 0;
}

int axiado_saradc_channel_data(struct udevice *dev, int channel,
				 unsigned int *data)
{
	struct axiado_saradc_priv *priv = dev_get_priv(dev);
	struct adc_uclass_plat *uc_pdata = dev_get_uclass_plat(dev);

	axiado_saradc_conversion(priv, channel, data);

	*data &= uc_pdata->data_mask;

	return 0;
}

static const struct adc_ops axiado_saradc_ops = {
	.channel_data = axiado_saradc_channel_data,
};

int axiado_saradc_probe(struct udevice *dev)
{
	struct adc_uclass_plat *uc_pdata = dev_get_uclass_plat(dev);
	struct axiado_saradc_priv *priv = dev_get_priv(dev);
	int ret;
	u32 reg;

	priv->clk = devm_clk_get(dev, "refclk");
	if (IS_ERR(priv->clk)) {
		pr_err("failed to get clk\n");
		return PTR_ERR(priv->clk);
	}

	ret = clk_prepare_enable(priv->clk);
	if (ret < 0) {
		pr_err("failed to enable clk\n");
		return ret;
	}

	priv->clk_rate = clk_get_rate(priv->clk);

	ret = device_get_supply_regulator(dev, "vref-supply", &priv->vref);
	if (ret && uc_pdata->vdd_microvolts <= 0) {
		pr_err("can't get vref-supply: %d\n", ret);
		goto err_clk_disable;
	}

	ret = regulator_set_enable(priv->vref, true);
	if (ret < 0) {
		pr_err("failed to enable vref regulator\n");
		goto err_clk_disable;
	}

	reg = AX_SARADC_CH_EN_MASK(AX_SARADC_ALL_CHANNELS) |
		AX_SARADC_SAMPLE_16 |
		AX_SARADC_MODE |
		AX_SARADC_ENABLE;

	writel(AX_SARADC_PD, priv->regs + AX_SARADC_GLOBAL_CTRL);
	writel(reg, priv->regs + AX_SARADC_GLOBAL_CTRL);

	return 0;

err_clk_disable:
	clk_disable_unprepare(priv->clk);
	return ret;
}

int axiado_saradc_of_to_plat(struct udevice *dev)
{
	struct adc_uclass_plat *uc_pdata = dev_get_uclass_plat(dev);
	struct axiado_saradc_priv *priv = dev_get_priv(dev);

	priv->regs = dev_read_addr_ptr(dev);
	if (!priv->regs) {
		pr_err("Dev: %s - can't get address!", dev->name);
		return -EINVAL;
	}

	uc_pdata->data_mask = (1 << AX_RESOLUTION_BITS) - 1;
	uc_pdata->data_format = ADC_DATA_FORMAT_BIN;
	uc_pdata->data_timeout_us = AX_SARADC_TIMEOUT / 5;
	uc_pdata->multidata_timeout_us = AX_SARADC_TIMEOUT / 5;
	uc_pdata->channel_mask = AX_SARADC_ALL_CHANNELS;

	return 0;
}

static const struct udevice_id axiado_saradc_ids[] = {
	{ .compatible = "axiado,ax3000-saradc", },
};

U_BOOT_DRIVER(axiado_adc) = {
	.name  = "axiado-adc",
	.id = UCLASS_ADC,
	.of_match = axiado_saradc_ids,
	.probe = axiado_saradc_probe,
	.ops = &axiado_saradc_ops,
	.of_to_plat = axiado_saradc_of_to_plat,
	.priv_auto	= sizeof(struct axiado_saradc_priv),
};
