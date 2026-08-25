// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __PCIE_H
#define __PCIE_H

struct ax_pcie_ops {
    int (*init)(u8 port, u8 controller_mode, u8 speed);
    int (*reset)(u8 port);

    int (*start_link)(u8 port);

    int (*cfg_read)(int port, u8 bus, u8 dev, u8 func,
                    u16 offset, u32 *val);

    int (*cfg_write)(int port, u8 bus, u8 dev, u8 func,
                     u16 offset, u32 val);

    int (*mem_read)(u64 addr, void *buf, size_t len);
    int (*mem_write)(u64 addr, const void *buf, size_t len);

    int (*link_status)(u8 port, u32 *status);
};

extern struct ax_pcie_ops pcie;

static inline int ax_pcie_init(u8 port, u8 controller_mode, u8 speed) {  
    if (pcie.init == NULL) return -ENOSYS;

    return pcie.init(port, controller_mode, speed); 
}

static inline int ax_pcie_start_link(u8 port) {  
    if (pcie.start_link == NULL) return -ENOSYS;

    return pcie.start_link(port); 
}

static inline int ax_pcie_link_status(u8 port) {  
    if (pcie.start_link == NULL) return -ENOSYS;

    return pcie.start_link(port); 
}

#endif /* __PCIE_H */

