// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __LTPI_H
#define __LTPI_H

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
	int (*gpio_request)(u32 port);

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

static inline int ax_ltpi_gpio_request(u32 port) {  
    if (ltpi.gpio_request == NULL) return -ENOSYS;

    return ltpi.gpio_request(port); 
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
