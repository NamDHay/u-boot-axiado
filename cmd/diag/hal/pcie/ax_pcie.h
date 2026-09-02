// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __PCIE_H
#define __PCIE_H

struct ax_pcie_bar {
	u64 addr;
	u64 size;
	bool is_io;
	bool is_64bit;
	bool prefetch;
	int index;
};

struct ax_pcie_ops {
    int (*init)(u8 port, u8 controller_mode, u8 speed);
    int (*reset)(u8 port);

    int (*cfg_read)(int port, u8 bus, u8 dev, u8 func,
                    u16 offset, u32 *val);

    int (*cfg_write)(int port, u8 bus, u8 dev, u8 func,
                     u16 offset, u32 val);

    int (*mem_read)(u8 port, u64 offset, void *buf, size_t len);
    int (*mem_write)(u8 port, u64 offset, const void *buf, size_t len);
};

extern struct ax_pcie_ops pcie;

static inline int ax_pcie_init(u8 port, u8 controller_mode, u8 speed) {  
    if (pcie.init == NULL) return -ENOSYS;

    return pcie.init(port, controller_mode, speed); 
}

static inline int ax_pcie_cfg_read(int port, u8 bus, u8 dev, u8 func,
                    u16 offset, u32 *val) {  
    if (pcie.cfg_read == NULL) return -ENOSYS;

    return pcie.cfg_read(port, bus, dev, func, offset, val); 
}

static inline int ax_pcie_cfg_write(int port, u8 bus, u8 dev, u8 func,
                    u16 offset, u32 val) {  
    if (pcie.cfg_write == NULL) return -ENOSYS;

    return pcie.cfg_write(port, bus, dev, func, offset, val); 
}

static inline int ax_pcie_mem_read(u8 port, u64 offset, void *buf, size_t len) {  
    if (pcie.mem_read == NULL) return -ENOSYS;

    return pcie.mem_read(port, offset, buf, len); 
}

static inline int ax_pcie_mem_write(u8 port, u64 offset, const void *buf, size_t len) {  
    if (pcie.mem_write == NULL) return -ENOSYS;

    return pcie.mem_write(port, offset, buf, len); 
}

#endif /* __PCIE_H */
