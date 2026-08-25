// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __PINMUX_H
#define __PINMUX_H

struct ax_pinmux_ops {
    int (*set_state)(const char* function);
};

extern struct ax_pinmux_ops pinmux;

static inline int ax_pinmux_set_state(const char* function) {
    if (pinmux.set_state == NULL) return -ENOSYS;

    return pinmux.set_state(function);
}

#endif /* __PINMUX_H */
