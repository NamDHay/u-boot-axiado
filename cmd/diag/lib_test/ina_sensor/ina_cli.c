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

#define INA232_I2C_SPEED 100000
#define INA232_VOL_REG 0x2

enum ax_ina_bus_type { 
    AX_INA_BUS_I2C = 0, 
    AX_INA_BUS_I3C, 
};

struct ax_ina_dev { 
    enum ax_ina_bus_type type; 
    uint bus; 
    uint addr; 
};

static int ax_ina_read(struct ax_ina_dev *dev, 
                        uint reg, uint len, u8 *buf)
{
    int ret;

    if (!dev || !buf)
        return -EINVAL;

    switch (dev->type) {
        case AX_INA_BUS_I2C:
            ret = ax_i2c_read(dev->bus, dev->addr,
                    reg, len, buf);
            break;
        case AX_INA_BUS_I3C:
            ret = ax_i3c_read(dev->bus, dev->addr,
                    reg, len, buf);
            break;
        default:
            return -EINVAL;
    }
    return ret;
}

static int ax_ina_init_bus(struct ax_ina_dev *dev)
{
    int ret;

    if (!dev)
        return -EINVAL;

    switch (dev->type) {
        case AX_INA_BUS_I2C:
            ret = ax_i2c_set_bus_speed(dev->bus, INA232_I2C_SPEED);
            break;
        case AX_INA_BUS_I3C:
            ret = ax_i3c_bus_init(dev->bus);
            ax_i3c_attach_dev(dev->bus, dev->addr, 11);
            break;
        default:
            return -EINVAL;
    }
    return ret;
}

static int ax_ina232_get_voltage(struct ax_ina_dev *dev,
                                int *mV)
{
    int ret;
    u8 buf[2];
    u16 raw;

    if (!dev || !mV)
        return -EINVAL;

    ret = ax_ina_read(dev, INA232_VOL_REG, sizeof(buf), buf);
    if (ret) {
        printf("INA232: failed to read voltage "
               "(bus=%u addr=0x%x ret=%d)\n", 
                dev->bus, dev->addr, ret);
        return ret;
    }

    raw = (u16)(buf[0] << 8) | buf[1];
    *mV = raw * 16 / 10;

    return 0;
}

static int do_ax_ina(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    char *endp;
    struct ax_ina_dev ina;
    int mV;
    int ret;

    if (argc < 5)
        return CMD_RET_USAGE;

    if (!strncmp(argv[2], "i2c", 3)) {
        ina.type = AX_INA_BUS_I2C;
    } else if (!strncmp(argv[2], "i3c", 3)) {
        ina.type = AX_INA_BUS_I3C;
    } else {
        printf("Mode not found\n");
        return CMD_RET_USAGE;
    }
    ina.bus = simple_strtol(argv[3], &endp, 10);
    ina.addr = simple_strtol(argv[4], &endp, 16);

    ret = ax_ina_init_bus(&ina);
    if (ret) {
        printf("ina75 initialized failed\n");
        return CMD_RET_FAILURE;
    }
    
    ret = ax_ina232_get_voltage(&ina, &mV);
    if (ret) {
        return CMD_RET_FAILURE;
    }

    if (mV < 0) {
        printf("INA232(0x%02X) Voltage : -%d.%03d C\n", ina.addr,
                            (-mV) / 1000, (-mV) % 1000);
    } else {
        printf("INA232(0x%02X) Voltage : %d.%03d C\n", ina.addr, 
                            mV / 1000, mV % 1000);
    }
    
    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
        ax_ina, 16, 1, do_ax_ina,
        "Axiado bare-metal INA232 diagnostics test",
        "read <i2c|i3c> <bus> <address>\n"
        "    - read INA232 Voltage\n"
        );

