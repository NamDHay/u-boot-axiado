// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __MDIO_H
#define __MDIO_H

struct ax_mdio_ops {
	int (*read)(int addr, int devad, int reg);
	int (*write)(int addr, int devad, int reg, u16 val);
	int (*reset)(void);
};
extern struct ax_mdio_ops mdio;

static inline int ax_mdio_read(int addr, int devad, int reg) {  
    if (mdio.read == NULL) return -EIO;

    return mdio.read(addr, devad, reg); 
}

static inline int ax_mdio_write(int addr, int devad, int reg, u16 val) {  
    if (mdio.write == NULL) return -EIO;

    return mdio.write(addr, devad, reg, val); 
}

static inline int ax_mdio_reset(void) {  
    if (mdio.reset == NULL) return -EIO;

    return mdio.reset(); 
}

static inline int ax_mdio_set_lb(int addr);

#endif /* __MDIO_H */

