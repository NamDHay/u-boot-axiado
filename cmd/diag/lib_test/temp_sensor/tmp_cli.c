// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <command.h>
#include <dm.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <malloc.h>

#include "ax_i2c.h"
#include "ax_i3c.h"

#define TMP75_I2C_SPEED 100000
#define TMP75_REG_TEMP 0x0

enum ax_tmp_bus_type { 
    AX_TMP_BUS_I2C = 0, 
    AX_TMP_BUS_I3C, 
};

struct ax_tmp_dev { 
    enum ax_tmp_bus_type type; 
    uint bus; 
    uint addr; 
};

static int ax_tmp_read(struct ax_tmp_dev *dev, 
                        uint reg, uint len, u8 *buf)
{
    int ret;

    if (!dev || !buf)
        return -EINVAL;

    switch (dev->type) {
        case AX_TMP_BUS_I2C:
            ret = ax_i2c_read(dev->bus, dev->addr,
                    reg, len, buf);
            break;
        case AX_TMP_BUS_I3C:
            ret = ax_i3c_read(dev->bus, dev->addr,
                    reg, len, buf);
            break;
        default:
            return -EINVAL;
    }
    return ret;
}

static int ax_tmp_init_bus(struct ax_tmp_dev *dev)
{
    int ret;

    if (!dev)
        return -EINVAL;

    switch (dev->type) {
        case AX_TMP_BUS_I2C:
            ret = ax_i2c_set_bus_speed(dev->bus, TMP75_I2C_SPEED);
            break;
        case AX_TMP_BUS_I3C:
            ret = ax_i3c_bus_init(dev->bus);
            ax_i3c_attach_dev(dev->bus, dev->addr, 11);
            break;
        default:
            return -EINVAL;
    }
    return ret;
}

static int ax_tmp75_get_temp(struct ax_tmp_dev *dev,
                                int *temp_mC)
{
    int ret;
    u8 buf[2];
    u16 raw;
    s16 temp_raw;

    if (!dev || !temp_mC)
        return -EINVAL;

    ret = ax_tmp_read(dev, TMP75_REG_TEMP, sizeof(buf), buf);
    if (ret) {
        printf("TMP75: failed to read temperature "
               "(bus=%u addr=0x%x ret=%d)\n", 
                dev->bus, dev->addr, ret);
        return ret;
    }

    raw = (u16)(buf[0] << 8) | buf[1];
    temp_raw = (s16)raw >> 4;
    *temp_mC = temp_raw * 625 / 10;

    return 0;
}

static int do_ax_tmp(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    char *endp;
    struct ax_tmp_dev tmp;
    int temp_mC;
    int ret;

    if (argc < 5)
        return CMD_RET_USAGE;

    if (!strncmp(argv[2], "i2c", 3)) {
        tmp.type = AX_TMP_BUS_I2C;
    } else if (!strncmp(argv[2], "i3c", 3)) {
        tmp.type = AX_TMP_BUS_I3C;
    } else {
        printf("Mode not found\n");
        return CMD_RET_USAGE;
    }
    tmp.bus = simple_strtol(argv[3], &endp, 16);
    tmp.addr = simple_strtol(argv[4], &endp, 16);

    ret = ax_tmp_init_bus(&tmp);
    if (ret) {
        printf("TMP75 initialized failed\n");
        return CMD_RET_FAILURE;
    }
    
    ret = ax_tmp75_get_temp(&tmp, &temp_mC);
    if (ret) {
        return CMD_RET_FAILURE;
    }

    if (temp_mC < 0) {
        printf("TMP75(0x%02X) Temperature : -%d.%03d C\n", tmp.addr,
                            (-temp_mC) / 1000, (-temp_mC) % 1000);
    } else {
        printf("TMP75(0x%02X) Temperature : %d.%03d C\n", tmp.addr, 
                            temp_mC / 1000, temp_mC % 1000);
    }
    
    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
        ax_tmp, 16, 1, do_ax_tmp,
        "Axiado bare-metal TMP75 diagnostics test",
        "read <i2c|i3c> <bus> <address>\n"
        "    - read TMP75 temperature\n"
        );
