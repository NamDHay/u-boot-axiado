// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __SGPIO_H
#define __SGPIO_H

struct ax_sgpio_ops {
    int (*request)(u8 bus, u16 total_pin);
    int (*set_value)(u8 bus, const u32 *data_out_slice, size_t len);
    int (*get_value)(u8 bus, u32 *data_in_slice, size_t len);
};

extern struct ax_sgpio_ops sgpio;

static inline int ax_sgpio_request(u8 bus, u16 total_pin) {  
    if (sgpio.request == NULL) return -ENOSYS;

    return sgpio.request(bus, total_pin); 
}

static inline int ax_sgpio_set_value(u8 bus, const u32 *data_out_slice, size_t len) {
    if (sgpio.set_value == NULL) return -ENOSYS;

    return sgpio.set_value(bus, data_out_slice, len);
}

static inline int ax_sgpio_get_value(u8 bus, u32 *data_in_slice, size_t len) {
    if (sgpio.get_value == NULL) return -ENOSYS;

    return sgpio.get_value(bus, data_in_slice, len);
}

#endif /* __SGPIO_H */
