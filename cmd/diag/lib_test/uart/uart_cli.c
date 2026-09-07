#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>
#include <time.h>
#include "asm/io.h"

#include "ax_uart.h"
#include "uart_cli.h"

#define UART_TIMEOUT_MS  1000

static int ax_uart_put_string(int uart, const char *str)
{
    int i;
    int len = strlen(str);

    if (!len)
        return -EINVAL;

    printf("UART_%d TX [%d bytes]: \"%s\"\n", uart, len, str);

    for (i = 0; i < len; i++)
        ax_uart_putc(uart, str[i]);

    return 0;
}

static int ax_uart_get_string(int uart, int len)
{
    int i;
    int c;

    if (len <= 0)
        return -EINVAL;

    printf("UART_%d RX [%d bytes]: ", uart, len);

    for (i = 0; i < len; i++) {
        c = ax_uart_getc(uart);

        if (c < 0)
            return c;

        printf("%c", (char)c);
    }

    printf("\n");

    return 0;
}

int do_ax_uart(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]) {
    unsigned ins;
    int ret;
    char *endp;
    int i;

    if (argc < 3)
        return CMD_RET_USAGE;

    ins = simple_strtol(argv[2], &endp, 10);
    ret = ax_uart_init(ins);
    if (ret) {
        printf("UART: '%s' init failed\n", argv[2]);
        return -EINVAL;
    }

    if (!strcmp(argv[1], "baud") && argc == 4) {
        int baud;
        baud = simple_strtol(argv[3], &endp, 10);
        ret = ax_uart_setbaud(ins, baud);
        if (ret) {
            printf("UART: '%s' set baudrate\n", argv[2]);
            return -EINVAL;
        }

        printf("UART_%d: %d\n", ins, baud);
    } else if (!strcmp(argv[1], "put") && argc == 4) {
        if (argc != 4)
            return CMD_RET_USAGE;

        ret = ax_uart_put_string(ins, argv[3]);
        if (ret)
            return CMD_RET_FAILURE;

    } else if (!strcmp(argv[1], "get") && argc == 4) {
        int len;
        len = simple_strtol(argv[3], &endp, 0);
        if (*endp != '\0' || !len)
            return CMD_RET_USAGE;

        ret = ax_uart_get_string(ins, len);
        if (ret)
            return CMD_RET_FAILURE;

    } else if (!strcmp(argv[1], "ext_lb") && argc == 3) {
        int len = 64;
        int ret;
        int errors = 0;
        ulong start;
        u8 tx, rx;

        /* Flush stale RX data */
        while (ax_uart_getc(ins) != -EAGAIN)
            ;

        for (i = 0; i < len; i++) {
            tx = 0x0A + i;

            /* Wait until TX FIFO has space */
            start = get_timer(0);

            do {
                ret = ax_uart_putc(ins, tx);

                if (get_timer(start) >= UART_TIMEOUT_MS) {
                    pr_err("TX timeout: byte=%d data=0x%02x\n",
                            i, tx);
                    return CMD_RET_FAILURE;
                }
            } while (ret == -EAGAIN);

            if (ret < 0) {
                pr_err("TX error: byte=%d ret=%d\n", i, ret);
                return CMD_RET_FAILURE;
            }

            /* Wait for received data */
            start = get_timer(0);

            do {
                ret = ax_uart_getc(ins);

                if (get_timer(start) >= UART_TIMEOUT_MS) {
                    pr_err("RX timeout: byte=%d expected=0x%02x\n",
                            i, tx);
                    return CMD_RET_FAILURE;
                }
            } while (ret == -EAGAIN);

            if (ret < 0) {
                pr_err("RX error: byte=%d ret=%d\n", i, ret);
                return CMD_RET_FAILURE;
            }

            rx = ret & 0xff;

            if (tx != rx) {
                pr_err("Mismatch [%d]: Send 0x%02x | Recv 0x%02x\n",
                        i, tx, rx);
                errors++;
            }
        }

        if (errors) {
            pr_err("External loopback UART_%d FAILED: %d/%d errors\n",
                    ins, errors, len);
            return CMD_RET_FAILURE;
        }

        printf("External loopback UART_%d PASSED: %d bytes\n", ins, len);

    } else if (!strcmp(argv[1], "exit") && argc == 3) {
        ax_uart_exit(ins);
    } else {
        return CMD_RET_USAGE;
    }

    return 0;
}

U_BOOT_CMD(
        ax_uart,	CONFIG_SYS_MAXARGS,	0,	do_ax_uart,
        "Axiado bare-metal UART diagnostics test",
        "baud <instance> <baudrate>\n"
        "put <instance> <string>\n"
        "get <instance> <len>\n"
        "ext_lb <instance>\n"
        "exit <instance>\n"
        );
