// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <command.h>
#include <dm.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <malloc.h>

#include "ax_i2c.h"

static int do_ax_i2c_speed(struct cmd_tbl *cmdtp, int flag,
                           int argc, char *const argv[])
{
    unsigned int bus;
    unsigned int speed;
    int ret;
    char *endp;

    if (argc != 3)
        return CMD_RET_USAGE;

    bus = simple_strtol(argv[1], &endp, 10);
    speed = simple_strtol(argv[2], &endp, 10);

    ret = ax_i2c_set_bus_speed(bus, speed);
    if (ret) {
        printf("ax_i2c: failed to set speed %u Hz, ret=%d\n",
               speed, ret);
        return CMD_RET_FAILURE;
    }

    printf("I2C_%d speed = %u Hz\n", bus, speed);

    return CMD_RET_SUCCESS;
}

static int do_ax_i2c_scan(struct cmd_tbl *cmdtp, int flag,
                          int argc, char *const argv[])
{
    uint addr;
    int ret;
    int found = 0;
    unsigned int bus;
    char *endp;
    u8 val;

    if (argc != 2)
        return CMD_RET_USAGE;

    bus = simple_strtol(argv[1], &endp, 10);

    printf("Scanning I2C_%d bus...\n", bus);

    /*
     * 7-bit I2C address range:
     *
     * 0x00 - 0x02 : reserved
     * 0x03 - 0x77 : usable slave addresses
     * 0x78 - 0x7F : reserved
     */
    for (addr = 0; addr < 128; addr++) {
        ret = ax_i2c_read(bus, addr, 0x0, 0x1, &val);

        if (ret == 0) {
            printf("0x%02x ", addr);
            found++;

            /*
             * Keep output readable.
             */
            if ((found % 8) == 0)
                printf("\n");
        }
    }

    if (found % 8)
        printf("\n");

    printf("Found %d device(s)\n", found);

    return CMD_RET_SUCCESS;
}

static int do_ax_i2c_read(struct cmd_tbl *cmdtp, int flag,
                          int argc, char *const argv[])
{
    uint addr;
    uint reg;
    uint len;
    u8 *buf;
    int ret;
    uint i;

    unsigned int bus;
    char *endp;

    if (argc != 5)
        return CMD_RET_USAGE;

    bus  = simple_strtol(argv[1], &endp, 10);
    addr = hextoul(argv[2], NULL);
    reg  = hextoul(argv[3], NULL);
    len  = hextoul(argv[4], NULL);

    if (addr > 0x7f) {
        printf("Invalid I2C address: 0x%x\n", addr);
        return CMD_RET_USAGE;
    }

    if (len == 0) {
        printf("Length must be > 0\n");
        return CMD_RET_USAGE;
    }

    buf = malloc(len);
    if (!buf) {
        printf("Cannot allocate %u bytes\n", len);
        return CMD_RET_FAILURE;
    }

    memset(buf, 0, len);

    ret = ax_i2c_read(bus, addr, reg, len, buf);
    if (ret) {
        printf("I2C_%d read failed: bus_addr=0x%02x reg=0x%x "
               "len=%u ret=%d\n",
               bus, addr, reg, len, ret);
        free(buf);
        return CMD_RET_FAILURE;
    }

    printf("I2C_%d READ: addr=0x%02x reg=0x%x len=%u\n",
           bus, addr, reg, len);

    for (i = 0; i < len; i++) {
        if ((i % 16) == 0)
            printf("%04x: ", reg + i);

        printf("%02x ", buf[i]);

        if ((i % 16) == 15 || i == len - 1)
            printf("\n");
    }

    free(buf);

    return CMD_RET_SUCCESS;
}

static int do_ax_i2c_write(struct cmd_tbl *cmdtp, int flag,
                           int argc, char *const argv[])
{
    uint addr;
    uint reg;
    uint len;
    u8 *buf;
    int ret;
    uint i;
    unsigned int bus;
    char *endp;

    /*
     * argc:
     *
     * ax_i2c write <addr> <reg> <data> [data ...]
     *
     * argc >= 4
     */
    if (argc < 5)
        return CMD_RET_USAGE;

    bus  = simple_strtol(argv[1], &endp, 10);
    addr = hextoul(argv[2], NULL);
    reg  = hextoul(argv[3], NULL);

    if (addr > 0x7f) {
        printf("Invalid I2C address: 0x%x\n", addr);
        return CMD_RET_USAGE;
    }

    len = argc - 4;

    buf = malloc(len);
    if (!buf) {
        printf("Cannot allocate %u bytes\n", len);
        return CMD_RET_FAILURE;
    }

    for (i = 0; i < len; i++)
        buf[i] = hextoul(argv[i + 4], NULL);

    ret = ax_i2c_write(bus, addr, reg, len, buf);
    if (ret) {
        printf("I2C_%d write failed: bus_addr=0x%02x reg=0x%x "
               "len=%u ret=%d\n",
               bus, addr, reg, len, ret);

        free(buf);
        return CMD_RET_FAILURE;
    }

    printf("I2C_%d WRITE: addr=0x%02x reg=0x%x len=%u\n",
           bus, addr, reg, len);

    printf("Data: ");
    for (i = 0; i < len; i++)
        printf("%02x ", buf[i]);
    printf("\n");

    free(buf);

    return CMD_RET_SUCCESS;
}

static int do_ax_i2c(struct cmd_tbl *cmdtp, int flag,
                     int argc, char *const argv[])
{
    if (argc < 2)
        return CMD_RET_USAGE;

    if (!strcmp(argv[1], "speed"))
        return do_ax_i2c_speed(cmdtp, flag, argc - 1, &argv[1]);

    if (!strcmp(argv[1], "scan"))
        return do_ax_i2c_scan(cmdtp, flag, argc - 1, &argv[1]);

    if (!strcmp(argv[1], "read"))
        return do_ax_i2c_read(cmdtp, flag, argc - 1, &argv[1]);

    if (!strcmp(argv[1], "write"))
        return do_ax_i2c_write(cmdtp, flag, argc - 1, &argv[1]);

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
    ax_i2c, 16, 1, do_ax_i2c,
    "Axiado bare-metal I2C diagnostics test",
    "speed <bus> <hz>\n"
    "    - set I2C bus speed\n"
    "scan <bus>\n"
    "    - scan I2C slave addresses\n"
    "read <bus> <addr> <reg> <len>\n"
    "    - read data from I2C device\n"
    "write <bus> <addr> <reg> <data> [data ...]\n"
    "    - write data to I2C device\n"
);
