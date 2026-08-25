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

#include "ax_i3c.h"


static int do_ax_i3c_init(struct cmd_tbl *cmdtp, int flag,
                          int argc, char *const argv[])
{
    u8 bus;
    int ret;

    if (argc != 2)
        return CMD_RET_USAGE;

    bus = hextoul(argv[1], NULL);

    ret = ax_i3c_bus_init(bus);
    if (ret) {
        printf("I3C: bus init failed: bus=%u ret=%d\n",
               bus, ret);
        return CMD_RET_FAILURE;
    }

    printf("I3C: bus %u initialized\n", bus);

    return CMD_RET_SUCCESS;
}

static int do_ax_i3c_scan(struct cmd_tbl *cmdtp, int flag,
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

    printf("Scanning legacy I2C devices on I3C bus...\n");

    /*
     * Legacy I2C 7-bit address range.
     */
    for (addr = 0; addr < 128; addr++) {
        ax_i3c_attach_dev(bus, addr, 11);
        ret = ax_i3c_read(bus, addr, 0, 1, &val);

        if (ret == 0) {
            printf("0x%02x ", addr);
            found++;

            if ((found % 8) == 0)
                printf("\n");
        }
    }

    if (found % 8)
        printf("\n");

    printf("Found %d legacy I2C device(s)\n", found);

    return CMD_RET_SUCCESS;
}

static int do_ax_i3c_daa(struct cmd_tbl *cmdtp, int flag,
                         int argc, char *const argv[])
{
    int ret;
    unsigned int bus;
    char *endp;

    if (argc != 2)
        return CMD_RET_USAGE;

    bus = simple_strtol(argv[1], &endp, 10);

    ret = ax_i3c_do_daa(bus);
    if (ret) {
        printf("I3C: DAA failed: ret=%d\n", ret);
        return CMD_RET_FAILURE;
    }

    printf("I3C: DAA completed successfully\n");

    return CMD_RET_SUCCESS;
}

static int do_ax_i3c_read(struct cmd_tbl *cmdtp, int flag,
                          int argc, char *const argv[])
{
    uint slv_addr;
    uint reg;
    uint len;
    u8 *buf;
    uint i;
    unsigned int bus;
    char *endp;
    int ret;

    if (argc != 5)
        return CMD_RET_USAGE;

    bus = simple_strtol(argv[1], &endp, 10);
    slv_addr = hextoul(argv[2], NULL);
    reg      = hextoul(argv[3], NULL);
    len      = hextoul(argv[4], NULL);

    if (slv_addr > 0x7f) {
        printf("Invalid I3C slave address: 0x%x\n", slv_addr);
        return CMD_RET_USAGE;
    }

    if (len == 0) {
        printf("Length must be > 0\n");
        return CMD_RET_USAGE;
    }

    buf = malloc(len);
    if (!buf) {
        printf("I3C: cannot allocate %u bytes\n", len);
        return CMD_RET_FAILURE;
    }

    memset(buf, 0, len);

    ret = ax_i3c_read(bus, slv_addr, reg, len, buf);
    if (ret) {
        printf("I3C%d READ failed: addr=0x%02x reg=0x%x "
               "len=%u ret=%d\n",
               bus, slv_addr, reg, len, ret);

        free(buf);
        return CMD_RET_FAILURE;
    }

    printf("I3C%d READ: addr=0x%02x reg=0x%x len=%u\n",
           bus, slv_addr, reg, len);

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

static int do_ax_i3c_write(struct cmd_tbl *cmdtp, int flag,
                           int argc, char *const argv[])
{
    uint slv_addr;
    uint reg;
    uint len;
    u8 *buf;
    uint i;
    int ret;
    unsigned int bus;
    char *endp;

    if (argc < 5)
        return CMD_RET_USAGE;

    bus = simple_strtol(argv[1], &endp, 10);
    slv_addr = hextoul(argv[2], NULL);
    reg      = hextoul(argv[3], NULL);

    if (slv_addr > 0x7f) {
        printf("Invalid I3C slave address: 0x%x\n", slv_addr);
        return CMD_RET_USAGE;
    }

    len = argc - 3;

    buf = malloc(len);
    if (!buf) {
        printf("I3C: cannot allocate %u bytes\n", len);
        return CMD_RET_FAILURE;
    }

    for (i = 0; i < len; i++)
        buf[i] = hextoul(argv[i + 3], NULL);

    ret = ax_i3c_write(bus, slv_addr, reg, len, buf);
    if (ret) {
        printf("I3C WRITE failed: addr=0x%02x reg=0x%x "
               "len=%u ret=%d\n",
               slv_addr, reg, len, ret);

        free(buf);
        return CMD_RET_FAILURE;
    }

    printf("I3C WRITE: addr=0x%02x reg=0x%x len=%u\n",
           slv_addr, reg, len);

    printf("Data: ");
    for (i = 0; i < len; i++)
        printf("%02x ", buf[i]);
    printf("\n");

    free(buf);

    return CMD_RET_SUCCESS;
}

static int do_ax_i3c(struct cmd_tbl *cmdtp, int flag,
                     int argc, char *const argv[])
{
    if (argc < 2)
        return CMD_RET_USAGE;

    if (!strcmp(argv[1], "init"))
        return do_ax_i3c_init(cmdtp, flag,
                              argc - 1, &argv[1]);

    if (!strcmp(argv[1], "scan"))
        return do_ax_i3c_scan(cmdtp, flag,
                              argc - 1, &argv[1]);

    if (!strcmp(argv[1], "daa"))
        return do_ax_i3c_daa(cmdtp, flag,
                             argc - 1, &argv[1]);

    if (!strcmp(argv[1], "read"))
        return do_ax_i3c_read(cmdtp, flag,
                              argc - 1, &argv[1]);

    if (!strcmp(argv[1], "write"))
        return do_ax_i3c_write(cmdtp, flag,
                               argc - 1, &argv[1]);

    return CMD_RET_USAGE;
}


U_BOOT_CMD(
    ax_i3c, 16, 1, do_ax_i3c,
    "Axiado bare-metal I3C test",
    "init <bus>\n"
    "    - initialize I3C bus\n"
    "scan\n"
    "    - scan legacy I2C devices on the I3C bus\n"
    "daa\n"
    "    - perform Dynamic Address Assignment\n"
    "read <addr> <reg> <len>\n"
    "    - read data from I3C device\n"
    "write <addr> <reg> <data> [data ...]\n"
    "    - write data to I3C device\n"
);
