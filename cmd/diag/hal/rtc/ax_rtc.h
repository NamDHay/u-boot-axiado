// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __RTC_H
#define __RTC_H

#include <rtc_def.h>

struct ax_rtc_ops {
    int (*init)(void);

    int (*set)(const struct rtc_time *tm);

    int (*get)(struct rtc_time *tm);
};
extern struct ax_rtc_ops rtc;

static inline int ax_rtc_init(void) {
    if (rtc.init == NULL) return -ENOSYS;

    return rtc.init();
}

static inline int ax_rtc_set(const struct rtc_time *tm) {
    if (rtc.set == NULL) return -ENOSYS;

    return rtc.set(tm);
}

static inline int ax_rtc_get(struct rtc_time *tm) {
    if (rtc.get == NULL) return -ENOSYS;

    return rtc.get(tm);
}

#endif /* __RTC_H */
