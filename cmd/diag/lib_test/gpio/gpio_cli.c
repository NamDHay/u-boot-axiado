// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <command.h>
#include <dm.h>
#include <errno.h>
#include <vsprintf.h>
#include <linux/delay.h>
#include <linux/printk.h>

#include "ax_diag.h"
#include "ax_gpio.h"

#define AX_GPIO_MAX_PIN     256     /* 8 banks x 32 pins (cdns_gpio.c) */
#define AX_GPIO_SETTLE_US   10      /* output -> input register settle time */

enum ax_gpio_op {
    AX_GPIO_OP_INPUT,
    AX_GPIO_OP_SET,
    AX_GPIO_OP_CLEAR,
    AX_GPIO_OP_TOGGLE,
};

static int ax_gpio_parse_op(const char *s, enum ax_gpio_op *op)
{
    if (!strcmp(s, "input"))
        *op = AX_GPIO_OP_INPUT;
    else if (!strcmp(s, "set"))
        *op = AX_GPIO_OP_SET;
    else if (!strcmp(s, "clear"))
        *op = AX_GPIO_OP_CLEAR;
    else if (!strcmp(s, "toggle"))
        *op = AX_GPIO_OP_TOGGLE;
    else
        return -EINVAL;

    return 0;
}

/*
 * Drive @value on @pin, then read the pin back through the input register.
 * The value is written before switching to output so the pin never drives
 * a stale OUTPUT_VALUE.
 */
static int ax_gpio_drive_check(unsigned int pin, int value)
{
    int ret, rb;

    ret = ax_gpio_set_value(pin, value);
    if (ret) {
        printf("gpio: pin %u set value failed: %d\n", pin, ret);
        return ret;
    }

    ret = ax_gpio_set_direction(pin, 0);
    if (ret) {
        printf("gpio: pin %u set output failed: %d\n", pin, ret);
        return ret;
    }

    udelay(AX_GPIO_SETTLE_US);

    rb = ax_gpio_get_value(pin);
    if (rb < 0) {
        printf("gpio: pin %u read-back failed: %d\n", pin, rb);
        return rb;
    }

    printf("gpio: pin %u drive %d read-back %d\n", pin, value, rb);
    if (rb != value) {
        printf("gpio: pin %u FAIL: read-back mismatch\n", pin);
        return -EIO;
    }

    return 0;
}

int do_ax_gpio(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    enum ax_gpio_op op;
    unsigned long pin;
    char *endp;
    int value, ret;

    if (argc != 3)
        return CMD_RET_USAGE;

    if (ax_gpio_parse_op(argv[1], &op))
        return CMD_RET_USAGE;

    pin = simple_strtoul(argv[2], &endp, 10);
    if (*argv[2] == '\0' || *endp != '\0' || pin >= AX_GPIO_MAX_PIN) {
        printf("gpio: invalid pin '%s' (0..%d)\n", argv[2],
               AX_GPIO_MAX_PIN - 1);
        return CMD_RET_USAGE;
    }

    ret = ax_gpio_request(pin, "gpio");
    if (ret) {
        printf("gpio: pin %lu request failed: %d\n", pin, ret);
        return CMD_RET_FAILURE;
    }

    switch (op) {
    case AX_GPIO_OP_INPUT:
        ret = ax_gpio_set_direction(pin, 1);
        if (ret) {
            printf("gpio: pin %lu set input failed: %d\n", pin, ret);
            break;
        }
        value = ax_gpio_get_value(pin);
        if (value < 0) {
            printf("gpio: pin %lu read failed: %d\n", pin, value);
            ret = value;
            break;
        }
        printf("gpio: pin %lu value is %d\n", pin, value);
        break;

    case AX_GPIO_OP_SET:
        ret = ax_gpio_drive_check(pin, 1);
        break;

    case AX_GPIO_OP_CLEAR:
        ret = ax_gpio_drive_check(pin, 0);
        break;

    case AX_GPIO_OP_TOGGLE:
        /*
         * Read the current pin level without changing direction: the
         * input register reflects the pin in output mode too. Switching
         * to input would release the line and read back a floating level.
         */
        value = ax_gpio_get_value(pin);
        if (value < 0) {
            printf("gpio: pin %lu read failed: %d\n", pin, value);
            ret = value;
            break;
        }
        ret = ax_gpio_drive_check(pin, !value);
        break;
    }

    return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

U_BOOT_CMD(
        ax_gpio,	3,	0,	do_ax_gpio,
        "Axiado bare-metal GPIO diagnostics test",
        "<input|set|clear|toggle> <pin>\n"
        "    input  - configure as input and print the level\n"
        "    set    - drive 1 and verify the pin reads back 1\n"
        "    clear  - drive 0 and verify the pin reads back 0\n"
        "    toggle - drive the inverse of the current level and verify\n"
        "    pin    - 0..255 (bank = pin / 32)"
        );
