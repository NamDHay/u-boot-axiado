// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <command.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <malloc.h>

#include "ax_i2c.h"
#include "ax_i3c.h"

#define EEPROM_I2C_SPEED        100000
#define EEPROM_MAX_RW_SIZE      256

/*
 * Typical EEPROM write cycle time.
 * This can be replaced by ACK polling later.
 */
#define EEPROM_WRITE_DELAY_MS   10

enum ax_eeprom_bus_type {
    AX_EEPROM_BUS_I2C = 0,
    AX_EEPROM_BUS_I3C,
};

struct ax_eeprom_dev {
    enum ax_eeprom_bus_type type;

    uint bus;
    uint addr;
};

static int ax_eeprom_read(struct ax_eeprom_dev *dev,
                          uint reg, uint len, u8 *buf)
{
    if (!dev || !buf)
        return -EINVAL;

    if (!len)
        return -EINVAL;

    switch (dev->type) {
    case AX_EEPROM_BUS_I2C:
        return ax_i2c_read(dev->bus, dev->addr,
                           reg, len,
                           buf);

    case AX_EEPROM_BUS_I3C:
        return ax_i3c_read(dev->bus, dev->addr,
                           reg, len,
                           buf);

    default:
        return -EINVAL;
    }
}

static int ax_eeprom_write(struct ax_eeprom_dev *dev,
                           uint reg, uint len, const u8 *buf)
{
    if (!dev || !buf)
        return -EINVAL;

    if (!len)
        return -EINVAL;

    switch (dev->type) {
    case AX_EEPROM_BUS_I2C:
        return ax_i2c_write(dev->bus, dev->addr,
                            reg, len,
                            (u8 *)buf);

    case AX_EEPROM_BUS_I3C:
        return ax_i3c_write(dev->bus, dev->addr,
                            reg, len,
                            (u8 *)buf);

    default:
        return -EINVAL;
    }
}

static int ax_eeprom_init_bus(struct ax_eeprom_dev *dev)
{
    int ret;

    if (!dev)
        return -EINVAL;

    switch (dev->type) {
        case AX_EEPROM_BUS_I2C:
            ret = ax_i2c_set_bus_speed(dev->bus, EEPROM_I2C_SPEED);
            if (ret)
                return ret;

            break;
        case AX_EEPROM_BUS_I3C:
            ret = ax_i3c_bus_init(dev->bus);
            if (ret)
                return ret;

            ret = ax_i3c_attach_dev(dev->bus, dev->addr, 11);
            if (ret)
                return ret;

            break;
        default:
            return -EINVAL;

    }

    return 0;
}

static void ax_eeprom_print_buffer(uint offset,
                                   const u8 *buf,
                                   uint len)
{
    uint i;

    for (i = 0; i < len; i++) {

        if ((i % 16) == 0)
            printf("%08x: ", offset + i);

        printf("%02x ", buf[i]);

        if ((i % 16) == 15 || i == len - 1)
            printf("\n");
    }
}

static int ax_eeprom_cmd_read(struct ax_eeprom_dev *dev,
                              uint offset,
                              uint len)
{
    u8 *buf;
    int ret;

    if (!len || len > EEPROM_MAX_RW_SIZE) {
        printf("Invalid read length: %u\n", len);
        return -EINVAL;
    }

    buf = malloc(len);
    if (!buf)
        return -ENOMEM;

    memset(buf, 0, len);

    ret = ax_eeprom_read(dev,
                         offset,
                         len,
                         buf);

    if (ret) {
        printf("EEPROM read failed: %d\n", ret);
        goto out;
    }

    ax_eeprom_print_buffer(offset,
                           buf,
                           len);

out:
    free(buf);

    return ret;
}

static int ax_eeprom_cmd_write(struct ax_eeprom_dev *dev,
                               uint offset,
                               int argc,
                               char *const argv[])
{
    u8 *buf;
    int len;
    int i;
    int ret;

    len = argc;

    if (!len)
        return -EINVAL;

    if (len > EEPROM_MAX_RW_SIZE) {
        printf("Write size too large: %d\n", len);
        return -EINVAL;
    }

    buf = malloc(len);
    if (!buf)
        return -ENOMEM;

    for (i = 0; i < len; i++)
        buf[i] = simple_strtoul(argv[i], NULL, 16);

    ret = ax_eeprom_write(dev,
                          offset,
                          len,
                          buf);

    if (ret) {
        printf("EEPROM write failed: %d\n", ret);
        goto out;
    }

    /*
     * EEPROM requires internal write cycle.
     * Usually 5~10 ms depending on device.
     */
    if (dev->type == AX_EEPROM_BUS_I2C)
        mdelay(EEPROM_WRITE_DELAY_MS);

    printf("EEPROM write success\n");

out:
    free(buf);

    return ret;
}

static int do_ax_eeprom(struct cmd_tbl *cmdtp,
                        int flag,
                        int argc,
                        char *const argv[])
{
    struct ax_eeprom_dev dev;
    const char *cmd;
    char *endp;
    uint offset;
    uint len;
    int ret;

    if (argc < 6)
        return CMD_RET_USAGE;

    cmd = argv[1];

    if (!strncmp(argv[2], "i2c", 3)) {
        dev.type = AX_EEPROM_BUS_I2C;
    } else if (!strncmp(argv[2], "i3c", 3)) {
        dev.type = AX_EEPROM_BUS_I3C;
    } else {
        printf("Mode not found\n");
        return CMD_RET_USAGE;
    }

    dev.bus = simple_strtoul(argv[3], &endp, 10);
    dev.addr = simple_strtoul(argv[4], &endp, 16);
    offset = simple_strtoul(argv[5], &endp, 16);

    ret = ax_eeprom_init_bus(&dev);
    if (ret) {
        printf("EEPROM bus initialization failed: %d\n",
               ret);
        return CMD_RET_FAILURE;
    }

    if (!strcmp(cmd, "read") ||
        !strcmp(cmd, "dump")) {

        if (argc < 7)
            return CMD_RET_USAGE;

        len = simple_strtoul(argv[6], &endp, 16);

        if (*endp) {
            printf("Invalid length: %s\n", argv[6]);
            return CMD_RET_USAGE;
        }

        ret = ax_eeprom_cmd_read(&dev,
                                 offset,
                                 len);

        if (ret)
            return CMD_RET_FAILURE;

        return CMD_RET_SUCCESS;
    }

    if (!strcmp(cmd, "write")) {

        if (argc < 7)
            return CMD_RET_USAGE;

        ret = ax_eeprom_cmd_write(&dev,
                                  offset,
                                  argc - 6,
                                  &argv[6]);

        if (ret)
            return CMD_RET_FAILURE;

        return CMD_RET_SUCCESS;
    }

    printf("Unknown command: %s\n", cmd);

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
    ax_eeprom,
    32,
    1,
    do_ax_eeprom,
    "Axiado EEPROM diagnostics test",
    "read <i2c|i3c> <bus> <addr> <offset> <len>\n"
    "    - Read EEPROM data\n"
    "\n"
    "ax_eeprom dump <i2c|i3c> <bus> <addr> <offset> <len>\n"
    "    - Dump EEPROM data\n"
    "\n"
    "ax_eeprom write <i2c|i3c> <bus> <addr> <offset> <data...>\n"
    "    - Write EEPROM data\n"
);
