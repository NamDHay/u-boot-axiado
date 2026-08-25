// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __I2C_H
#define __I2C_H

struct ax_i2c_ops {
    int (*set_bus_speed)(uint bus, unsigned int speed);
    int (*read)(uint bus, uint addr, uint reg, uint len, u8 *buf);
    int (*write)(uint bus, uint addr, uint reg, uint len, u8 *buf);
};

extern struct ax_i2c_ops i2c;

static inline int ax_i2c_set_bus_speed(uint bus, unsigned int speed) {  
    if (i2c.set_bus_speed == NULL) return -ENOSYS;

    return i2c.set_bus_speed(bus, speed); 
}

static inline int ax_i2c_read(uint bus, uint addr, uint reg, uint len, u8 *buf) {  
    if (i2c.read == NULL) return -ENOSYS;

    return i2c.read(bus, addr, reg, len, buf); 
}

static inline int ax_i2c_write(uint bus, uint addr, uint reg, uint len, u8 *buf) {  
    if (i2c.write == NULL) return -ENOSYS;

    return i2c.write(bus, addr, reg, len, buf); 
}

#endif /* __I2C_H */

