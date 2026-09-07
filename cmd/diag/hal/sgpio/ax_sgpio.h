// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __SGPIO_H
#define __SGPIO_H

struct ax_sgpio_ops {
    int (*request)(u8 bus, u16 total_pin);
    int (*set_value)(u16 pin, u8 value);
    int (*get_value)(u16 pin);
};

extern struct ax_sgpio_ops sgpio;

static inline int ax_sgpio_request(u8 bus, u16 total_pin) {  
    if (sgpio.request == NULL) return -ENOSYS;

    return sgpio.request(bus, total_pin); 
}

static inline int ax_sgpio_set_value(u16 pin, u8 value) {
    if (sgpio.set_value == NULL) return -ENOSYS;

    return sgpio.set_value(pin, value);
}

static inline int ax_sgpio_get_value(u16 pin) {
    if (sgpio.get_value == NULL) return -ENOSYS;

    return sgpio.get_value(pin);
}

#endif /* __SGPIO_H */
