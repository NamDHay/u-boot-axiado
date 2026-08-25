// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __I3C_H
#define __I3C_H

struct ax_i3c_ops {
    int (*bus_init)(u8 bus);
    int (*do_daa)(u8 bus);
    int (*attach_dev)(u8 bus, u8 slv_addr, u8 slot);
    int (*read)(u8 bus, uint slv_addr, uint reg, uint len, u8 *buf);
    int (*write)(u8 bus, uint slv_addr, uint reg, uint len, u8 *buf);
};

extern struct ax_i3c_ops i3c;

static inline int ax_i3c_bus_init(u8 bus) {  
    if (i3c.bus_init == NULL) return -ENOSYS;

    return i3c.bus_init(bus); 
}

static inline int ax_i3c_do_daa(u8 bus) {  
    if (i3c.do_daa == NULL) return -ENOSYS;

    return i3c.do_daa(bus); 
}

static inline int ax_i3c_attach_dev(u8 bus, u8 slv_addr, u8 slot) {  
    if (i3c.attach_dev == NULL) return -ENOSYS;

    return i3c.attach_dev(bus, slv_addr, slot); 
}

static inline int ax_i3c_read(u8 bus, uint slv_addr, uint reg, uint len, u8 *buf) {  
    if (i3c.read == NULL) return -ENOSYS;

    return i3c.read(bus, slv_addr, reg, len, buf); 
}

static inline int ax_i3c_write(u8 bus, uint slv_addr, uint reg, uint len, u8 *buf) {  
    if (i3c.write == NULL) return -ENOSYS;

    return i3c.write(bus, slv_addr, reg, len, buf); 
}

#endif /* __I3C_H */


