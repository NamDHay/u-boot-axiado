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
        "adc diagnostic test",
        "ax_adc",
        "This test verifies the adc operation.",
        {
            "ax_adc start 8" ,
            "ax_adc multi 8" ,
            NULL ,
        },
    },
#endif

#ifdef CONFIG_CMD_AX_EMMC
    {
        "emmc diagnostic test",
        "ax_emmc",
        "This test verifies the emmc operation.",
        {
            "ax_adc start 8" ,
            "ax_adc multi 8" ,
            NULL ,
        },
    },
#endif

#ifdef CONFIG_CMD_AX_GPIO
    {
        "gpio diagnostic test",
        "ax_gpio",
        "This test verifies the gpio operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_HDMI
    {
        "hdmi diagnostic test",
        "ax_hdmi",
        "This test verifies the hdmi operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_I2C
    {
        "i2c diagnostic test",
        "ax_i2c",
        "This test verifies the i2c operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_I3C
    {
        "i3c diagnostic test",
        "ax_i3c",
        "This test verifies the i3c operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_LTPI
    {
        "ltpi diagnostic test",
        "ax_ltpi",
        "This test verifies the ltpi operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_UART
    {
        "uart diagnostic test",
        "ax_uart",
        "This test verifies the uart operation.",
        {
            "ax_uart ext_lb 0",
            "ax_uart ext_lb 1",
            "ax_uart ext_lb 2",
            "ax_uart ext_lb 4",
            "ax_uart ext_lb 5",
            "ax_uart ext_lb 6",
            "ax_uart ext_lb 7",
            "ax_uart ext_lb 8",
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_MEM
    {
        "mem diagnostic test",
        "ax_mem",
        "This test verifies the mem operation.",
        {
            "ax_memtest 0x90000000 0x10000000 1",
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_PCIE
    {
        "pcie diagnostic test",
        "ax_pcie",
        "This test verifies the pcie operation.",
        {
            "ax_pcie init 0 rp 4",
            "ax_pcie init 1 rp 4",
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_PWM
    {
        "pwm diagnostic test",
        "ax_pwm",
        "This test verifies the pwm operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_RMII
    {
        "rmii diagnostic test",
        "ax_rmii",
        "This test verifies the rmii operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_RTC
    {
        "rtc diagnostic test",
        "ax_rtc",
        "This test verifies the rtc operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_SGMII
    {
        "sgmii diagnostic test",
        "ax_sgmii",
        "This test verifies the sgmii operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_SGPIO
    {
        "sgpio diagnostic test",
        "ax_sgpio",
        "This test verifies the sgpio operation.",
        {
            "ax_sgpio request 0 512",
            "ax_sgpio request 1 512",
            "ax_sgpio ext_lb",
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_SPI
    {
        "spi diagnostic test",
        "ax_spi",
        "This test verifies the spi operation.",
        {
#ifdef CONFIG_CMD_AX_FLASH
            "ax_flash init 0 0",
            "ax_flash memtest 0 0 0x1000",
            "ax_flash init 0 1",
            "ax_flash memtest 0 0 0x1000",
#endif
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_USB
    {
        "usb diagnostic test",
        "ax_usb",
        "This test verifies the usb operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_XGMII
    {
        "xgmii diagnostic test",
        "ax_xgmii",
        "This test verifies the xgmii operation.",
        {
            NULL
        }
    },
#endif

#ifdef CONFIG_CMD_AX_WDT
    {
        "wdt diagnostic test",
        "ax_wdt",
        "This test verifies the wdt operation.",
        {
            "ax_wdt start 5000" ,
            NULL
        }
    },
#endif
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

static int diag_run_single(struct diag_test *ip, unsigned long testid) 
{
    int i;
    int ret;

    for (i = 0; ip->testcase[i] != NULL; i++) {
        printf("\n\n\n\nRunning: \"%s\"\n", ip->testcase[i]);
        ret = run_command(ip->testcase[i], 0);
        if (ret) {
            printf("\nError: Command failed with exit code %d. Halting sequence.\n",
                    ret);
            return ret;
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
