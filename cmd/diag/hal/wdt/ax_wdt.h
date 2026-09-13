// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __WDT_H
#define __WDT_H

struct ax_wdt_ops {
    int (*start)(u64 timeout);
    int (*stop)(void);
    int (*reset)(void);
    int (*expire_now)(void);
};

extern struct ax_wdt_ops wdt;

static inline int ax_wdt_start(u64 timeout) {
    if (wdt.start == NULL) return -ENOSYS;

    return wdt.start(timeout);
}

static inline int ax_wdt_stop(void) {
    if (wdt.stop == NULL) return -ENOSYS;

    return wdt.stop();
}

static inline int ax_wdt_reset(void) {
    if (wdt.reset == NULL) return -ENOSYS;

    return wdt.reset();
}

static inline int ax_wdt_expire_now(void) {
    if (wdt.expire_now == NULL) return -ENOSYS;

    return wdt.expire_now();
}

#endif /* __WDT_H */
