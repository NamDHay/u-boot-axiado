// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */
#include <command.h>
#include <dm.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <stdio.h>

#include "ax_hdmi.h"

static int ax_hdmi_cmd_bist(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int ret;
    printf("Start HDMI BIST test\n");
    ret = ax_hdmi_bist();
    if (ret) 
        printf("FAILED\n");
    else
        printf("PASSED\n");

    printf("Start HDMI VIDEO BIST test\n");
    ret = ax_hdmi_vbist();
    if (ret) 
        printf("FAILED\n");
    else
        printf("PASSED\n");
    return CMD_RET_SUCCESS;
}

static int do_ax_hdmi(struct cmd_tbl *cmdtp,
        int flag,
        int argc,
        char *const argv[])
{
    if (argc < 2)
        return CMD_RET_USAGE;

    if (!strcmp(argv[1], "bist")) {
        return ax_hdmi_cmd_bist(cmdtp, flag, argc - 1, &argv[1]);
    }

    if (!strcmp(argv[1], "reset"))
        ax_hdmi_reset();


    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
        ax_hdmi,
        8,
        1,
        do_ax_hdmi,
        "Axiado HDMI diagnostic command",
        "bist\n"
        "    - Running HDMI BIST and VIDEO BIST test\n"
        "\n"
        "reset\n"
        "    - Reset HDMI controller\n"
        "\n"
);
