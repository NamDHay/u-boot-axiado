// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __GPIO_H
#define __GPIO_H

struct ax_gpio_ops {
    int (*request)(unsigned pin, const char *label);
    int (*set_direction)(unsigned pin, unsigned dir);
    int (*set_value)(unsigned pin, unsigned value);
    int (*get_value)(unsigned pin);
    int (*free)(unsigned pin);
};

extern struct ax_gpio_ops gpio;

static inline int ax_gpio_request(unsigned pin, const char *label) {  
    if (gpio.request == NULL) return -ENOSYS;

    return gpio.request(pin, label); 
}

static inline int ax_gpio_set_direction(unsigned pin, unsigned dir) {
    if (gpio.set_direction == NULL) return -ENOSYS;

    return gpio.set_direction(pin, dir);
}

static inline int ax_gpio_set_value(unsigned pin, unsigned value) {
    if (gpio.set_value == NULL) return -ENOSYS;

    return gpio.set_value(pin, value);
}

static inline int ax_gpio_get_value(unsigned pin) {
    if (gpio.get_value == NULL) return -ENOSYS;

    return gpio.get_value(pin);
}

static inline int ax_gpio_free(unsigned pin) {
    if (gpio.free == NULL) return -ENOSYS;

    return gpio.free(pin);
}

#endif /* __GPIO_H */
