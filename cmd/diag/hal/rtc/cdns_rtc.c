// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <linux/errno.h>

#include "ax_diag.h"
#include <bcd.h>

#include "ax_rtc.h"

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

#define __bf_shf(x) (__builtin_ffsll(x) - 1)
#define FIELD_PREP(mask, val) \
	(((val) << __bf_shf(mask)) & (mask))
#define FIELD_GET(mask, reg) \
	(((reg) & (mask)) >> __bf_shf(mask))

static void cdns_rtc_set_enabled(bool enabled)
{
    void __iomem *base = (void __iomem *)AX3000_CSR_BASE_ADRS_RTC;
	u32 reg = enabled ? 0x0 : CDNS_RTC_CTLR_TIME_CAL;

	writel(reg, base + CDNS_RTC_CTLR);
}

static bool cdns_rtc_get_enabled(void)
{
    void __iomem *base = (void __iomem *)AX3000_CSR_BASE_ADRS_RTC;
	return !(readl(base + CDNS_RTC_CTLR) & CDNS_RTC_CTLR_TIME_CAL);
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

static int cdns_rtc_set_time(const struct rtc_time *tm) 
{
    void __iomem *base = (void __iomem *)AX3000_CSR_BASE_ADRS_RTC;
	u32 timr, calr, stsr;
	int ret = -EIO;
	int year = tm->tm_year + 1900;
	int tries;

	cdns_rtc_set_enabled(false);

	timr = cdns_rtc_time2reg(tm);

	calr = FIELD_PREP(CDNS_RTC_CAL_D, bin2bcd(tm->tm_mday))
	     | FIELD_PREP(CDNS_RTC_CAL_M, bin2bcd(tm->tm_mon + 1))
	     | FIELD_PREP(CDNS_RTC_CAL_Y, bin2bcd(year % 100))
	     | FIELD_PREP(CDNS_RTC_CAL_C, bin2bcd(year / 100))
	     | FIELD_PREP(CDNS_RTC_CAL_DAY, tm->tm_wday + 1);

	/* Update registers, check valid flags */
	for (tries = 0; tries < CDNS_RTC_MAX_REGS_TRIES; tries++) {
		writel(timr, base + CDNS_RTC_TIMR);
		writel(calr, base + CDNS_RTC_CALR);
		stsr = readl(base + CDNS_RTC_STSR);

		if ((stsr & CDNS_RTC_STSR_VT_VC) == CDNS_RTC_STSR_VT_VC) {
			ret = 0;
			break;
		}
	}

	cdns_rtc_set_enabled(true);
	return ret;
}

static int cdns_rtc_get_time(struct rtc_time *tm) 
{
	u32 reg;
    void __iomem *base = (void __iomem *)AX3000_CSR_BASE_ADRS_RTC;
	
	if (!cdns_rtc_get_enabled()) {
		cdns_rtc_set_enabled(true);
	}

	reg = readl(base + CDNS_RTC_TIMR);
	cdns_rtc_reg2time(reg, tm);

	reg = readl(base + CDNS_RTC_CALR);
	tm->tm_mday = bcd2bin(FIELD_GET(CDNS_RTC_CAL_D, reg));
	tm->tm_mon  = bcd2bin(FIELD_GET(CDNS_RTC_CAL_M, reg)) - 1;
	tm->tm_year = bcd2bin(FIELD_GET(CDNS_RTC_CAL_Y, reg))
		    + bcd2bin(FIELD_GET(CDNS_RTC_CAL_C, reg)) * 100 - 1900;
	tm->tm_wday = bcd2bin(FIELD_GET(CDNS_RTC_CAL_DAY, reg)) - 1;

	cdns_rtc_set_enabled(true);
	return 0;
}

static int cdns_rtc_init(void) 
{
    void __iomem *base = (void __iomem *)AX3000_CSR_BASE_ADRS_RTC;

	writel(0x0, base + CDNS_RTC_HMR);
	writel(CDNS_RTC_KRTCR_KRTC, base + CDNS_RTC_KRTCR);
	
	cdns_rtc_set_enabled(true);

    return 0;
}

struct ax_rtc_ops rtc = {
    .init = cdns_rtc_init,
    .set = cdns_rtc_set_time,
    .get = cdns_rtc_get_time,
};
