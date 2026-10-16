// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include "ax_diag.h"
#include "command.h"
#include <config.h>
#include <log.h>
#include <linux/string.h>
#include <linux/libfdt.h>

struct diag_test diag_list[] = {
#ifdef CONFIG_CMD_AX_ADC
    {
        "adc",
        "adc diagnostic test",
        .test = {
            { "ax_adc single 0 0 10000", 1000, 0 },
            { "ax_adc single 1 0 10000", 1000, 0 },
            { "ax_adc single 2 0 10000", 1000, 0 },
            { "ax_adc single 3 0 10000", 1000, 0 },
            { "ax_adc single 4 0 10000", 1000, 0 },
            { "ax_adc single 5 0 10000", 1000, 0 },
            { "ax_adc single 6 0 10000", 1000, 0 },
            { "ax_adc single 7 0 10000", 1000, 0 },
        },
    },
#endif

#ifdef CONFIG_CMD_AX_EMMC
    {
        "emmc",
        "emmc diagnostic test",
        .test = {
            { "ax_mmc init 2 2 1", 1000, 0 },
            { "ax_mmc memtest 0 10", 1000, 0 },
        },
    },
#endif

#ifdef CONFIG_CMD_AX_GPIO
    {
        "gpio",
        "gpio diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_HDMI
    {
        "hdmi",
        "hdmi diagnostic test",
        .test = {
            { "ax_hdmi bist", 1000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_I2C
    {
        "i2c",
        "i2c diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_I3C
    {
        "i3c",
        "i3c diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_LTPI
    {
        "ltpi",
        "ltpi diagnostic test",
        .test = {
            { "ax_ltpi init", 1000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_UART
    {
        "uart",
        "uart diagnostic test",
        .test = {
            { "ax_uart ext_lb 0", 1000, 0 },
            { "ax_uart ext_lb 1", 1000, 0 },
            { "ax_uart ext_lb 2", 1000, 0 },
            { "ax_uart ext_lb 4", 1000, 0 },
            { "ax_uart ext_lb 5", 1000, 0 },
            { "ax_uart ext_lb 6", 1000, 0 },
            { "ax_uart ext_lb 7", 1000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_MEM
    {
        "mem",
        "mem diagnostic test",
        .test = {
            { "ax_memtest 0x90000000 0x10000000 1", 1000, 0 }
        }
    },
#endif

#ifdef CONFIG_CMD_AX_PCIE
    {
        "pcie",
        "pcie diagnostic test",
        .test = {
            { "ax_pcie init 0 4", 1000, 0 },
            { "ax_pcie init 1 4", 1000, 0 },
#ifdef CONFIG_CMD_AX_NVME
            { "ax_nvme init 1; ax_nvme compare 0 100", 1000, 0 },
            { "ax_nvme init 2; ax_nvme compare 0 100", 1000, 0 },
#endif
        }
    },
#endif

#ifdef CONFIG_CMD_AX_PWM
    {
        "pwm",
        "pwm diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_RMII
    {
        "rmii",
        "rmii diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_RTC
    {
        "rtc",
        "rtc diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_SGMII
    {
        "sgmii",
        "sgmii diagnostic test",
        test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_SGPIO
    {
        "sgpio",
        "sgpio diagnostic test",
        .test = {
            { "ax_sgpio request 0 512", 1000, 0 },
            { "ax_sgpio request 1 512", 1000, 0 },
            { "ax_sgpio ext_lb", 1000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_SPI
    {
        "spi",
        "spi diagnostic test",
        .test = {
#ifdef CONFIG_CMD_AX_FLASH
            { "ax_flash init 0 0", 1000, 0 },
            { "ax_flash memtest 0 0 0x1000", 1000, 0 },
            { "ax_flash init 0 1", 1000, 0 },
            { "ax_flash memtest 0 0 0x1000", 1000, 0 },
#endif
        }
    },
#endif

#ifdef CONFIG_CMD_AX_USB
    {
        "usb",
        "usb diagnostic test",
        .test = {
            { "ax_usb stop; ax_usb init mmio 0x41000000; ax_usb info; ax_usb read 0 0xa0000000 0 1", 1000, 0 },
            { "ax_usb stop; ax_usb init mmio 0x41400000; ax_usb info; ax_usb read 0 0xa0000000 0 1", 1000, 0 },
            { "ax_usb stop; ax_usb init mmio 0x41500000; ax_usb info; ax_usb read 0 0xa0000000 0 1", 1000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_XGMII
    {
        "xgmii",
        "xgmii diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_WDT
    {
        "wdt",
        "wdt diagnostic test",
        .test = {
        }
    },
#endif
};

unsigned int diag_list_size = ARRAY_SIZE(diag_list);

static int diag_info_single(struct diag_test *test, int full)
{
    int i;
    printf("%s- %s\n", test->name, test->desc);
    if (full) {
        for (i = 0; test->test[i].cmd != NULL; i++) {
            printf("\t -%s", test->test[i].cmd);
        }
    }

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
            if (strcmp(diag_list[i].name, name) == 0)
                break;
        }

        if (i < diag_list_size)
            return diag_info_single(diag_list + i, 1);
        else
            return -1;
    }
}

static int diag_run_single(struct diag_test *ip, unsigned long testid) 
{
    int i;

    for (i = 0; ip->test[i].cmd != NULL; i++) {
        printf("\n\n\n\nRunning: \"%s\"\n", ip->test[i].cmd);
        ip->test->status = run_command(ip->test[i].cmd, 0);
        if (ip->test[i].status) {
            printf("\nError: Command failed with exit code %d. Halting sequence.\n",
                    ip->test[i].status);
            return ip->test[i].status;
        }
    }
    printf("\n\n\n\n===================================================================\n");
    return 0;
}

int diag_run(char *name, unsigned long testid)
{
    unsigned int i;

    if (name == NULL) {
        return 0;
    } else {
        for (i = 0; i < diag_list_size; i++) {
            if (strcmp(diag_list[i].name, name) == 0)
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
