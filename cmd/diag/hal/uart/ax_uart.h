// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __UART_H
#define __UART_H

struct ax_uart_ops {
    int (*init)(unsigned instance);
    void (*exit)(unsigned instance);

    int (*setbaud)(unsigned instance, int baud);
    int (*setconfig)(unsigned instance, uint serial_config);
    int (*getconfig)(unsigned instance, uint *serial_config);
    int (*getc)(unsigned instance);
    int (*putc)(unsigned instance, const char c);
    int (*puts)(unsigned instance, const char *s, size_t len);
    int (*clear)(unsigned instance);
    int (*loop)(unsigned instance, int on);
};

extern struct ax_uart_ops uart;

static inline int ax_uart_init(unsigned instance) {
    if (uart.init == NULL) return -ENOSYS;

    return uart.init(instance);
}

static inline void ax_uart_exit(unsigned instance) {
    if (uart.exit == NULL) return;

    return uart.exit(instance);
}

static inline int ax_uart_setbaud(unsigned instance, int baud) {
    if (uart.setbaud == NULL) return -ENOSYS;

    return uart.setbaud(instance, baud);
}

static inline int ax_uart_setconfig(unsigned instance, uint serial_config) {
    if (uart.setconfig == NULL) return -ENOSYS;

    return uart.setconfig(instance, serial_config);
}

static inline int ax_uart_getconfig(unsigned instance, uint *serial_config) {
    if (uart.getconfig == NULL) return -ENOSYS;

    return uart.getconfig(instance, serial_config);
}

static inline int ax_uart_getc(unsigned instance) {
    if (uart.getc == NULL) return -ENOSYS;

    return uart.getc(instance);
}

static inline int ax_uart_putc(unsigned instance, const char c) {
    if (uart.putc == NULL) return -ENOSYS;

    return uart.putc(instance, c);
}

static inline int ax_uart_puts(unsigned instance, const char *s, size_t len) {
    if (uart.puts == NULL) return -ENOSYS;

    return uart.puts(instance, s, len);
}

static inline int ax_uart_clear(unsigned instance) {
    if (uart.clear == NULL) return -ENOSYS;

    return uart.clear(instance);
}

static inline int ax_uart_loop(unsigned instance, int on) {
    if (uart.loop == NULL) return -ENOSYS;

    return uart.loop(instance, on);
}

#endif /* __UART_H */
