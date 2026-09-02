// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include "ax_diag.h"
#include <config.h>
#include <log.h>
#include <linux/string.h>
#include <linux/libfdt.h>

struct diag_test diag_list[] = {
    {
        "adc diagnostic test",
        "adc",
        "This test verifies the adc operation.",
    },
    {
        "emmc diagnostic test",
        "emmc",
        "This test verifies the emmc operation.",
    },
    {
        "gpio diagnostic test",
        "gpio",
        "This test verifies the gpio operation.",
    },
    {
        "hdmi diagnostic test",
        "hdmi",
        "This test verifies the hdmi operation.",
    },
    {
        "i2c diagnostic test",
        "i2c",
        "This test verifies the i2c operation.",
    },
    {
        "i3c diagnostic test",
        "i3c",
        "This test verifies the i3c operation.",
    },
    {
        "ltpi diagnostic test",
        "ltpi",
        "This test verifies the ltpi operation.",
    },
    {
        "uart diagnostic test",
        "uart",
        "This test verifies the uart operation.",
    },
    {
        "mem diagnostic test",
        "mem",
        "This test verifies the mem operation.",
    },
    {
        "pcie diagnostic test",
        "pcie",
        "This test verifies the pcie operation.",
    },
    {
        "pwm diagnostic test",
        "pwm",
        "This test verifies the pwm operation.",
    },
    {
        "rmii diagnostic test",
        "rmii",
        "This test verifies the rmii operation.",
    },
    {
        "rtc diagnostic test",
        "rtc",
        "This test verifies the rtc operation.",
    },
    {
        "sgmii diagnostic test",
        "sgmii",
        "This test verifies the sgmii operation.",
    },
    {
        "sgpio diagnostic test",
        "sgpio",
        "This test verifies the sgpio operation.",
    },
    {
        "spi diagnostic test",
        "spi",
        "This test verifies the spi operation.",
    },
    {
        "usb diagnostic test",
        "usb",
        "This test verifies the usb operation.",
    },
    {
        "xgmii diagnostic test",
        "xgmii",
        "This test verifies the xgmii operation.",
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
