// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __EMMC_H
#define __EMMC_H

struct ax_emmc_ops {
};

extern struct ax_emmc_ops emmc;

static inline int ax_emmc_request(unsigned pin, const char *label) {  
    if (emmc.request == NULL) return -ENOSYS;

    return emmc.request(pin, label); 
}

#endif /* __EMMC_H */

