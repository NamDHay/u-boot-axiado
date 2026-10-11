// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __LTPI_H
#define __LTPI_H

#include "ax_gpio.h"

struct ax_ltpi_data_req {
	uint64_t addr;
	uint32_t len;
	uint32_t flags;
	void *buf;
};

struct ax_ltpi_ops {
    int (*init)(void);
    void (*reset)(void);
    int (*check_link)(u32 timeout);

    /* GPIO */
    struct ax_gpio_ops gpio;

    /* I2C */
    int (*i2c_request)(u32 port);

    /* UART */
    int (*uart_request)(u32 port);

    /* OEM */
    int (*oem_send)(const void *buf, uint32_t len);
    int (*oem_recv)(void *buf, uint32_t len);

    /* Data Channel */
    int (*data_read)(struct ax_ltpi_data_req *req);
    int (*data_write)(struct ax_ltpi_data_req *req);
};

extern struct ax_ltpi_ops ltpi;

static inline int ax_ltpi_init(void) {  
    if (ltpi.init == NULL) return -ENOSYS;

    return ltpi.init(); 
}

static inline void ax_ltpi_reset(void) {  
    if (ltpi.reset == NULL) return ;

    ltpi.reset(); 
}

static inline int ax_ltpi_check_link(u32 timeout) {  
    if (ltpi.check_link == NULL) return -ENOSYS;

    return ltpi.check_link(timeout); 
}

static inline int ax_ltpi_gpio_set_value(unsigned pin, unsigned value)
{
    if (ltpi.gpio.set_value == NULL) return -ENOSYS;

    return ltpi.gpio.set_value(pin, value);
}

static inline int ax_ltpi_gpio_get_value(unsigned pin)
{
    if (ltpi.gpio.get_value == NULL) return -ENOSYS;

    return ltpi.gpio.get_value(pin);
}

static inline int ax_ltpi_i2c_request(u32 port) {  
    if (ltpi.i2c_request == NULL) return -ENOSYS;

    return ltpi.i2c_request(port); 
}

static inline int ax_ltpi_uart_request(u32 port) {  
    if (ltpi.uart_request == NULL) return -ENOSYS;

    return ltpi.uart_request(port); 
}

#endif /* __LTPI_H */
