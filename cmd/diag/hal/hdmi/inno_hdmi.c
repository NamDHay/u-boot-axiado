// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <stdio.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <time.h>
#include <linux/delay.h>

#include "ax3000_base_adrs.h"
#include "ax3000_slo_base_adrs.h"

#include "ax_hdmi.h"
#include "inno_hdmi.h"

static int inno_hdmi_reset(void)
{
    void __iomem *base_addr = AX3000_CSR_BASE_ADRS_DSUB_HDMI;

	writel(HDMI_SYS_PWR_LOW, base_addr + HDMI_SYS_CTRL);
	mdelay(100);
	writel(HDMI_SYS_PWR_ON, base_addr + HDMI_SYS_CTRL);
    return 0;
}

static int inno_hdmi_vbist(void)
{
    void __iomem *base_addr = AX3000_CSR_BASE_ADRS_DSUB_HDMI;
    u32 timeout;

    /* turn on bias circuit*/
    writel(TURN_ON_BIAS_CKT, base_addr + HDMI_BIAS_CIRCUIT);
    /* turn on rxsense  detection  circuit */
    mdelay(100);
    writel(RX_SENSE_DET_YES, base_addr + HDMI_RX_SENSE);

    /* wait for RX sense detection result */
    timeout = 20;
    printf("Polling detect RX sense ");
    while (!readl(base_addr + HDMI_RX_SENSE_STATUS)) {
        printf(".");
        timeout--;
        if (!timeout) {
            pr_err(" timeout\n");
            pr_err("Please check the HDMI/DP cable and connector\r\n");
            return -ETIMEDOUT;
        }
        mdelay(500); // Polling until detection is successful
    }
    printf(" detected\n");

    // turn on pre-PLL (set 0x1a0[0] to 1’b0) and post-PLL (set 0x1aa[0] to 1’b0) with correct configuration.
    clrbits_le32(base_addr + PLL_PRE_REG, BIT(0));
    clrbits_le32(base_addr + PLL_POST_REG, BIT(0));

    timeout = 20;
    printf("Polling check Pre-PLL status ");
    while (!readl(base_addr + PLL_PRE_LOCK_STAT_REG)) {
        printf(".");
        timeout--;
        if (!timeout) {
            printf(" timeout. Status is 0x%08x\n",
                    readl(base_addr + PLL_PRE_LOCK_STAT_REG));
            return -ETIMEDOUT;
        }
        mdelay(500);
    }
    printf(" success\n");

    timeout = 20;
    printf("Polling check Post-PLL status ");
    while (!readl(base_addr + PLL_POST_LOCK_STAT_REG)) {
        printf(".");
        timeout--;
        if (!timeout) {
            printf(" timeout. status is 0x%08x\n",
                    readl(base_addr + PLL_POST_LOCK_STAT_REG));
            return -ETIMEDOUT;
        }
        mdelay(500);
    }
    printf(" success\n");

    /* turn on LDO */
    writel(0x7, base_addr + HDMI_LDO);

    /* turn on serializer */
    writel(0x71, base_addr + HDMI_SERIAL);

    /* enable BIST test */
    writel(0x0, base_addr + HDMI_VBIST);

    /* Send initialization command */
    writel(0x63, base_addr + HDMI_SYS_CTRL);

    /* power on  */
    writel(0x61, base_addr + HDMI_SYS_CTRL);

    /* Synchronization enable */
    writel(0x0, base_addr + HDMI_SYNC_REG);
    writel(0x1, base_addr + HDMI_SYNC_REG);

    /* turn on TMDS driver */
    writel(0x8f, base_addr + HDMI_TMDS_SYS_CTL);

    return 0;
}

static int inno_hdmi_bist(void)
{
    void __iomem *base_addr = AX3000_CSR_BASE_ADRS_DSUB_LOCAL;
    u32 reg = 0;
    u32 timeout = 0;

    /* Disable BIST */
    writel(0x0, base_addr + 0x40);
    /* Select BIST */
    writel(0x1, base_addr + 0x40);

    /* Assert reset */
    writel(0xAB3, base_addr + 0x44);

    //should delay > 2Tclk
    mdelay(1);

    /* Deassert reset */
    writel(0x0, base_addr + 0x44);

    //should delay > 20 Tclk
    mdelay(1);

    /* BIST Enable */
    writel(0x3, base_addr + 0x40);
    writel(0x1, base_addr + 0x40);

    /* Wait for DONE or timeout */
    timeout = 3;
    while (!(reg & (1 << 4)) && (!timeout)) { // cover 132k Tclk
        reg = readl(base_addr + 0x40);
        timeout--;
        mdelay(1000);
    }
    if (timeout == 0) {
        printf("HDMI BIST test failed\r\n");
        return -ETIMEDOUT;
    }

    /* Check PASS */
    if (!((reg & (1 << 4)) && (reg & (1 << 5))))
        return -EAGAIN;

    /* Exit BIST mode */
    writel(0x0, base_addr + 0x40);

    return 0;
}

struct ax_hdmi_ops hdmi = {
    .reset = inno_hdmi_reset,
    .bist = inno_hdmi_bist,
    .vbist = inno_hdmi_vbist,
};
