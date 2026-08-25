// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include "diag.h"
#include <config.h>
#include <log.h>
#include <linux/string.h>
#include <linux/libfdt.h>

extern int gpio_diag_init(void);

extern int gpio_diag_test(unsigned long testid);

extern void gpio_diag_stat(void);

struct diag_test diag_list[] = {
    {
        "adc diagnostic test",
        "adc",
        "This test verifies the adc operation.",
        /* &adc_diag_init, */
        /* &adc_diag_test, */
        /* &adc_diag_stat, */
    },
    {
        "emmc diagnostic test",
        "emmc",
        "This test verifies the emmc operation.",
        /* &emmc_diag_init, */
        /* &emmc_diag_test, */
        /* &emmc_diag_stat, */
    },
    {
        "gpio diagnostic test",
        "gpio",
        "This test verifies the gpio operation.",
        &gpio_diag_init,
        &gpio_diag_test,
        &gpio_diag_stat,
    },
    {
        "hdmi diagnostic test",
        "hdmi",
        "This test verifies the hdmi operation.",
        /* &hdmi_diag_init, */
        /* &hdmi_diag_test, */
        /* &hdmi_diag_stat, */
    },
    {
        "i2c diagnostic test",
        "i2c",
        "This test verifies the i2c operation.",
        /* &i2c_diag_init, */
        /* &i2c_diag_test, */
        /* &i2c_diag_stat, */
    },
    {
        "i3c diagnostic test",
        "i3c",
        "This test verifies the i3c operation.",
        /* &i3c_diag_init, */
        /* &i3c_diag_test, */
        /* &i3c_diag_stat, */
    },
    {
        "ltpi diagnostic test",
        "ltpi",
        "This test verifies the ltpi operation.",
        /* &ltpi_diag_init, */
        /* &ltpi_diag_test, */
        /* &ltpi_diag_stat, */
    },
    {
        "uart diagnostic test",
        "uart",
        "This test verifies the uart operation.",
        /* &uart_diag_init, */
        /* &uart_diag_test, */
        /* &uart_diag_stat, */
    },
    {
        "mem diagnostic test",
        "mem",
        "This test verifies the mem operation.",
        /* &mem_diag_init, */
        /* &mem_diag_test, */
        /* &mem_diag_stat, */
    },
    {
        "pcie diagnostic test",
        "pcie",
        "This test verifies the pcie operation.",
        /* &pcie_diag_init, */
        /* &pcie_diag_test, */
        /* &pcie_diag_stat, */
    },
    {
        "pwm diagnostic test",
        "pwm",
        "This test verifies the pwm operation.",
        /* &pwm_diag_init, */
        /* &pwm_diag_test, */
        /* &pwm_diag_stat, */
    },
    {
        "rmii diagnostic test",
        "rmii",
        "This test verifies the rmii operation.",
        /* &rmii_diag_init, */
        /* &rmii_diag_test, */
        /* &rmii_diag_stat, */
    },
    {
        "rtc diagnostic test",
        "rtc",
        "This test verifies the rtc operation.",
        /* &rtc_diag_init, */
        /* &rtc_diag_test, */
        /* &rtc_diag_stat, */
    },
    {
        "sgmii diagnostic test",
        "sgmii",
        "This test verifies the sgmii operation.",
        /* &sgmii_diag_init, */
        /* &sgmii_diag_test, */
        /* &sgmii_diag_stat, */
    },
    {
        "sgpio diagnostic test",
        "sgpio",
        "This test verifies the sgpio operation.",
        /* &sgpio_diag_init, */
        /* &sgpio_diag_test, */
        /* &sgpio_diag_stat, */
    },
    {
        "spi diagnostic test",
        "spi",
        "This test verifies the spi operation.",
        /* &spi_diag_init, */
        /* &spi_diag_test, */
        /* &spi_diag_stat, */
    },
    {
        "usb diagnostic test",
        "usb",
        "This test verifies the usb operation.",
        /* &usb_diag_init, */
        /* &usb_diag_test, */
        /* &usb_diag_stat, */
    },
    {
        "xgmii diagnostic test",
        "xgmii",
        "This test verifies the xgmii operation.",
        /* &xgmii_diag_init, */
        /* &xgmii_diag_test, */
        /* &xgmii_diag_stat, */
    },
};

unsigned int diag_list_size = ARRAY_SIZE(diag_list);

static int diag_info_single(struct diag_test *test, int full)
{
    if (full)
        printf("%s - %s\n"
            "  %s\n", test->cmd, test->name, test->desc);
    else
        printf("  %-15s - %s\n", test->cmd, test->name);

    return 0;
}

int diag_info(char *name)
{
    unsigned int i;

    if (name == NULL) {
		for (i = 0; i < diag_list_size; i++)
			diag_info_single(diag_list + i, 0);

        return 0;
    } else {
        for (i = 0; i < diag_list_size; i++) {
            if (strcmp(diag_list[i].cmd, name) == 0)
                break;
        }

        if (i < diag_list_size)
            return diag_info_single(diag_list + i, 1);
        else
            return -1;
    }
}

static int diag_run_single(struct diag_test *test, unsigned long testid) {
    int ret;

    ret = test->init();
    if (ret) {
        pr_err("%s init failed, %d\n", test->name, ret);
        goto log;
    }

    ret = test->test(testid);
    if (ret) {
        pr_err("%s case %ld failed, %d\n", test->name, testid, ret);
        goto log;
    }

log:
    test->stat();
    return ret;
}

int diag_run(char *name, unsigned long testid)
{
    unsigned int i;

    if (name == NULL) {
        return 0;
    } else {
        for (i = 0; i < diag_list_size; i++) {
            if (strcmp(diag_list[i].cmd, name) == 0)
                break;
        }

        if (i < diag_list_size)
            return diag_run_single(diag_list + i, 1);
        else
            return -1;
    }
}
int diag_log(char *fmt, ...)
{
    return 0;
}
