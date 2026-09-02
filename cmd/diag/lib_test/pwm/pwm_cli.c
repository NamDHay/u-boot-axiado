// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>

#include "ax_pwm.h"

static int do_ax_pwm(struct cmd_tbl *cmdtp, int flag, int argc,
        char *const argv[])
{
    u32 channel, pwm_enable, pwm_invert, period_ns = 0, duty_ns = 0;
    char *endp;
    int ret;

    if (argc < 4)
        return CMD_RET_USAGE;

    if (!strncmp(argv[1], "i", 1)) {
        channel = simple_strtol(argv[3], &endp, 10);
        pwm_invert = simple_strtol(argv[4], &endp, 10);
        ret = ax_pwm_set_invert(channel, pwm_invert);
    } else if (!strncmp(argv[1], "c", 1)) {
        channel = simple_strtol(argv[3], &endp, 10);
        period_ns = simple_strtol(argv[5], &endp, 10);
        duty_ns = simple_strtol(argv[6], &endp, 10);
        ret = ax_pwm_set_config(channel, period_ns, duty_ns);
    } else if (!strncmp(argv[1], "e", 1)) {
        channel = simple_strtol(argv[3], &endp, 10);
        pwm_enable = simple_strtol(argv[4], &endp, 10);
        ret = ax_pwm_set_enable(channel, pwm_enable);
    } else if (!strncmp(argv[1], "d", 1)) {
        channel = simple_strtol(argv[3], &endp, 10);
        ret = ax_pwm_set_enable(channel, 0);
    } else {
        return CMD_RET_USAGE;
    }

    if (ret) {
        printf("error(%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    return CMD_RET_USAGE;
}

U_BOOT_CMD(ax_pwm, CONFIG_SYS_MAXARGS, 0, do_ax_pwm,
        "perform axiado pwm diagnostics",
        "invert <channel> <polarity> - invert polarity\n"
        "config <channel> <period_ns> <duty_ns> - config PWM\n"
        "enable <channel> - enable PWM output\n"
        "disable <channel> - disable PWM output\n"
        "Note: All input values are in decimal");
