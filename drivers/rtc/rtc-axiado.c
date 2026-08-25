// SPDX-License-Identifier: GPL-2.0-or-later

/* Copyright (c) 2021-25 Axiado Corporation (or its affiliates). All rights reserved.
 * Use, modification and redistribution of this file is subject to your possession
 * of a valid End User License Agreement (EULA) for the Axiado Product of which
 * these sources are part of and your compliance with all applicable terms and
 * conditions of such licence agreement.
 *
 * Nguyen Nam Huy <axiado-2561@axiado.com>
 *
 * Implementation extracted from the Linux kernel and adapted for u-boot.
 */

#include <clk.h>
#include <dm.h>
#include <malloc.h>
#include <rtc.h>
#include <asm/io.h>
#include <dm/device_compat.h>
#include <linux/time.h>
#include <bcd.h>
#include <linux/bitfield.h>

/* Registers */
#define CDNS_RTC_CTLR		0x00
#define CDNS_RTC_HMR		0x04
#define CDNS_RTC_TIMR		0x08
#define CDNS_RTC_CALR		0x0C
#define CDNS_RTC_TIMAR		0x10
#define CDNS_RTC_CALAR		0x14
#define CDNS_RTC_AENR		0x18
#define CDNS_RTC_EFLR		0x1C
#define CDNS_RTC_IENR		0x20
#define CDNS_RTC_IDISR		0x24
#define CDNS_RTC_IMSKR		0x28
#define CDNS_RTC_STSR		0x2C
#define CDNS_RTC_KRTCR		0x30

/* Control */
#define CDNS_RTC_CTLR_TIME	BIT(0)
#define CDNS_RTC_CTLR_CAL	BIT(1)
#define CDNS_RTC_CTLR_TIME_CAL	(CDNS_RTC_CTLR_TIME | CDNS_RTC_CTLR_CAL)

/* Status */
#define CDNS_RTC_STSR_VT	BIT(0)
#define CDNS_RTC_STSR_VC	BIT(1)
#define CDNS_RTC_STSR_VTA	BIT(2)
#define CDNS_RTC_STSR_VCA	BIT(3)
#define CDNS_RTC_STSR_VT_VC	(CDNS_RTC_STSR_VT | CDNS_RTC_STSR_VC)
#define CDNS_RTC_STSR_VTA_VCA	(CDNS_RTC_STSR_VTA | CDNS_RTC_STSR_VCA)

/* Keep RTC */
#define CDNS_RTC_KRTCR_KRTC	BIT(0)

/* Alarm, Event, Interrupt */
#define CDNS_RTC_AEI_HOS	BIT(0)
#define CDNS_RTC_AEI_SEC	BIT(1)
#define CDNS_RTC_AEI_MIN	BIT(2)
#define CDNS_RTC_AEI_HOUR	BIT(3)
#define CDNS_RTC_AEI_DATE	BIT(4)
#define CDNS_RTC_AEI_MNTH	BIT(5)
#define CDNS_RTC_AEI_ALRM	BIT(6)

/* Time */
#define CDNS_RTC_TIME_H		GENMASK(7, 0)
#define CDNS_RTC_TIME_S		GENMASK(14, 8)
#define CDNS_RTC_TIME_M		GENMASK(22, 16)
#define CDNS_RTC_TIME_HR	GENMASK(29, 24)
#define CDNS_RTC_TIME_PM	BIT(30)
#define CDNS_RTC_TIME_CH	BIT(31)

/* Calendar */
#define CDNS_RTC_CAL_DAY	GENMASK(2, 0)
#define CDNS_RTC_CAL_M		GENMASK(7, 3)
#define CDNS_RTC_CAL_D		GENMASK(13, 8)
#define CDNS_RTC_CAL_Y		GENMASK(23, 16)
#define CDNS_RTC_CAL_C		GENMASK(29, 24)
#define CDNS_RTC_CAL_CH		BIT(31)

#define CDNS_RTC_MAX_REGS_TRIES	3

struct cdns_rtc {
	void __iomem *regs;
	struct clk *pclk;
	struct clk *ref_clk;
};

static void cdns_rtc_set_enabled(struct cdns_rtc *crtc, bool enabled)
{
	u32 reg = enabled ? 0x0 : CDNS_RTC_CTLR_TIME_CAL;

	writel(reg, crtc->regs + CDNS_RTC_CTLR);
}

static bool cdns_rtc_get_enabled(struct cdns_rtc *crtc)
{
	return !(readl(crtc->regs + CDNS_RTC_CTLR) & CDNS_RTC_CTLR_TIME_CAL);
}

static u32 cdns_rtc_time2reg(const struct rtc_time *tm)
{
	return FIELD_PREP(CDNS_RTC_TIME_S,  bin2bcd(tm->tm_sec))
	     | FIELD_PREP(CDNS_RTC_TIME_M,  bin2bcd(tm->tm_min))
	     | FIELD_PREP(CDNS_RTC_TIME_HR, bin2bcd(tm->tm_hour));
}

static void cdns_rtc_reg2time(u32 reg, struct rtc_time *tm)
{
	tm->tm_sec  = bcd2bin(FIELD_GET(CDNS_RTC_TIME_S, reg));
	tm->tm_min  = bcd2bin(FIELD_GET(CDNS_RTC_TIME_M, reg));
	tm->tm_hour = bcd2bin(FIELD_GET(CDNS_RTC_TIME_HR, reg));
}

static int cdns_rtc_read_time(struct udevice *dev, struct rtc_time *tm)
{
	struct cdns_rtc *crtc = dev_get_priv(dev);
	u32 reg;
	
	if (!cdns_rtc_get_enabled(crtc)) {
		cdns_rtc_set_enabled(crtc, true);
	}

	reg = readl(crtc->regs + CDNS_RTC_TIMR);
	cdns_rtc_reg2time(reg, tm);

	reg = readl(crtc->regs + CDNS_RTC_CALR);
	tm->tm_mday = bcd2bin(FIELD_GET(CDNS_RTC_CAL_D, reg));
	tm->tm_mon  = bcd2bin(FIELD_GET(CDNS_RTC_CAL_M, reg)) - 1;
	tm->tm_year = bcd2bin(FIELD_GET(CDNS_RTC_CAL_Y, reg))
		    + bcd2bin(FIELD_GET(CDNS_RTC_CAL_C, reg)) * 100 - 1900;
	tm->tm_wday = bcd2bin(FIELD_GET(CDNS_RTC_CAL_DAY, reg)) - 1;

	cdns_rtc_set_enabled(crtc, true);
	return 0;
}

static int cdns_rtc_set_time(struct udevice *dev, const struct rtc_time *tm)
{
	struct cdns_rtc *crtc = dev_get_priv(dev);
	u32 timr, calr, stsr;
	int ret = -EIO;
	int year = tm->tm_year + 1900;
	int tries;

	cdns_rtc_set_enabled(crtc, false);

	timr = cdns_rtc_time2reg(tm);

	calr = FIELD_PREP(CDNS_RTC_CAL_D, bin2bcd(tm->tm_mday))
	     | FIELD_PREP(CDNS_RTC_CAL_M, bin2bcd(tm->tm_mon + 1))
	     | FIELD_PREP(CDNS_RTC_CAL_Y, bin2bcd(year % 100))
	     | FIELD_PREP(CDNS_RTC_CAL_C, bin2bcd(year / 100))
	     | FIELD_PREP(CDNS_RTC_CAL_DAY, tm->tm_wday + 1);

	/* Update registers, check valid flags */
	for (tries = 0; tries < CDNS_RTC_MAX_REGS_TRIES; tries++) {
		writel(timr, crtc->regs + CDNS_RTC_TIMR);
		writel(calr, crtc->regs + CDNS_RTC_CALR);
		stsr = readl(crtc->regs + CDNS_RTC_STSR);

		if ((stsr & CDNS_RTC_STSR_VT_VC) == CDNS_RTC_STSR_VT_VC) {
			ret = 0;
			break;
		}
	}

	cdns_rtc_set_enabled(crtc, true);
	return ret;
}

static int axiado_rtc_probe(struct udevice *dev)
{
	struct cdns_rtc *crtc = dev_get_priv(dev);
	unsigned long ref_clk_freq;
	int ret;

	crtc->regs = devfdt_get_addr_ptr(dev);
	if (!crtc->regs) {
		pr_err("RTC device not found");
		return -EINVAL;
	}

	crtc->pclk = devm_clk_get(dev, "pclk");
	if (IS_ERR(crtc->pclk)) {
		ret = PTR_ERR(crtc->pclk);
		pr_err("Failed to retrieve the peripheral clock, %d\n", ret);
		return PTR_ERR(crtc->pclk);
	}

	crtc->ref_clk = devm_clk_get(dev, "refclk");
	if (IS_ERR(crtc->ref_clk)) {
		ret = PTR_ERR(crtc->ref_clk);
		pr_err("Failed to retrieve the reference clock, %d\n", ret);
		return PTR_ERR(crtc->ref_clk);
	}

	ret = clk_enable(crtc->pclk);
	if (ret) {
		pr_err("Failed to enable the peripheral clock, %d\n", ret);
		return ret;
	}

	ret = clk_enable(crtc->ref_clk);
	if (ret) {
		pr_err("Failed to enable the reference clock, %d\n", ret);
		goto err_disable_pclk;
	}

	ref_clk_freq = clk_get_rate(crtc->ref_clk);
	if ((ref_clk_freq != 1) && (ref_clk_freq != 100)) {
		pr_err("Invalid reference clock frequency %lu Hz.\n",
			ref_clk_freq);
		ret = -EINVAL;
		goto err_disable_ref_clk;
	}
	
	writel(0, crtc->regs + CDNS_RTC_HMR);
	writel(CDNS_RTC_KRTCR_KRTC, crtc->regs + CDNS_RTC_KRTCR);
	
	cdns_rtc_set_enabled(crtc, true);

	return 0;

err_disable_ref_clk:
	clk_disable(crtc->ref_clk);

err_disable_pclk:
	clk_disable(crtc->pclk);

	return ret;
}

static const struct rtc_ops axiado_rtc_ops = {
	.get = cdns_rtc_read_time,
	.set = cdns_rtc_set_time,
};

static const struct udevice_id axiado_rtc_ids[] = {
	{ .compatible = "cdns,rtc-r109v3" },
	{ }
};

U_BOOT_DRIVER(rtc_axiado) = {
	.name	= "rtc-axiado",
	.id	= UCLASS_RTC,
	.probe	= axiado_rtc_probe,
	.of_match = axiado_rtc_ids,
	.ops	= &axiado_rtc_ops,
	.priv_auto	= sizeof(struct cdns_rtc),
};

