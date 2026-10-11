// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>

#include "ax_diag.h"
#include "ax_ltpi.h"

#define AX_LTPI_READ_ADDRESS    (AX3000_MEM_BASE_ADRS_DDR_LO + 0x20000000)
#define AX_LTPI_WRITE_ADDRESS   (AX3000_MEM_BASE_ADRS_DDR_LO + 0x10000000)
#define AX_LTPI_TIMEOUT_MS      5000

static int do_ax_ltpi(struct cmd_tbl *cmdtp, int flag, int argc,
        char *const argv[])
{
    int ret;

    if (argc < 2)
        return CMD_RET_USAGE;

    if (!strcmp(argv[1], "init")) {
        ret = ax_ltpi_init();
        if (ret) {
            return CMD_RET_FAILURE;
        }

        ret = ax_ltpi_check_link(AX_LTPI_TIMEOUT_MS);
        if (ret) {
            return CMD_RET_FAILURE;
        }
    } else if (!strcmp(argv[1], "free")) {
        ax_ltpi_reset();
    } else if (!strcmp(argv[1], "set")) {
        if (!strcmp(argv[2], "gpio")) {
            u32 pin, value;

            pin = dectoul(argv[3], NULL);
            value = dectoul(argv[4], NULL);

            ax_ltpi_gpio_set_value(pin, value);
            printf("LTPI GPIO_%d value %d\n", pin, value);
        }
    } else if (!strcmp(argv[1], "get")) {
        if (!strcmp(argv[2], "gpio")) {
            u32 pin, value;

            pin = dectoul(argv[3], NULL);

            value = ax_ltpi_gpio_get_value(pin);

            printf("LTPI GPIO_%d value %d\n", pin, value);
        }
    } else {
        return CMD_RET_USAGE;
    }

    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(ax_ltpi, CONFIG_SYS_MAXARGS, 0, do_ax_ltpi,
        "Axiado bare-metal LTPI diagnostics test",
        "init - initialize LTPI\n"
        "free - Deinitialize LTPI\n"
        "set - set ltpi output data\n"
        "\t- [gpio] <pin> <value> - Set output pin\n"
        "get - get ltpi input data\n"
        "\t- [gpio] <pin> - Get input pin\n"
        );
