// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <command.h>
#include <dm.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <malloc.h>

#include "ax_sgpio.h"

#define AX_SGPIO_MAX_DATA_LEN 16

static int ax_sgpio_parse_data(int argc, char *const argv[],
			     u32 *buf, int max_len)
{
	int i;
	int len = 0;
	u32 value;

	for (i = 0; i < argc; i++) {
		if (len >= max_len)
			return -ENOMEM;

		value = hextoul(argv[i], NULL);

		buf[len++] = (u32)value;
	}

	return len;
}

static void ax_sgpio_dump_data(const u32 *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if ((i % 4) == 0)
			printf("%08x: ", i * 4);

		printf("%08x ", buf[i]);

		if ((i % 4) == 3 || i == len - 1)
			printf("\n");
	}
}

static int do_ax_sgpio_request(struct cmd_tbl *cmdtp, int flag,
			   int argc, char *const argv[])
{
    u8 bus;
    u16 ngpios;
    int ret;

	if (argc != 3)
		return CMD_RET_USAGE;

	bus = simple_strtoul(argv[1], NULL, 0);
	ngpios = simple_strtoul(argv[2], NULL, 0);

    ret = ax_sgpio_request(bus, ngpios);
	if (ret) {
		printf("SGPIO setup failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_ax_sgpio_put(struct cmd_tbl *cmdtp, int flag,
			   int argc, char *const argv[])
{
    u8 bus;
    u32 len;
    u32 tx_buf[AX_SGPIO_MAX_DATA_LEN];
    int ret;

	if (argc < 3)
		return CMD_RET_USAGE;

	bus = simple_strtoul(argv[1], NULL, 0);
	len = ax_sgpio_parse_data(argc - 2, &argv[2],
                    tx_buf, AX_SGPIO_MAX_DATA_LEN);

    if (len < 0) {
        printf("Invalid SGPIO data\n");
        return CMD_RET_FAILURE;
    }

    ret = ax_sgpio_set_value(bus, tx_buf, len);
	if (ret) {
		printf("SGPIO put failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_ax_sgpio_get(struct cmd_tbl *cmdtp, int flag,
			   int argc, char *const argv[])
{
    u8 bus;
    u32 len;
    u32 bytes;
    u32 *rxbuf;
    int ret;

	if (argc != 3)
		return CMD_RET_USAGE;

	bus = simple_strtoul(argv[1], NULL, 0);
	bytes = simple_strtoul(argv[2], NULL, 0);
    len = bytes / 4;

    if (!len) {
        printf("Invalid length\n");
        return CMD_RET_FAILURE;
    }

    rxbuf = calloc(len, sizeof(*rxbuf));
    if (!rxbuf)
        return CMD_RET_FAILURE;

    ret = ax_sgpio_get_value(bus, rxbuf, len);
	if (ret) {
        printf("SGPIO get failed: %d\n", ret);
        return CMD_RET_FAILURE;
	}

	printf("SGPIO read (%u bytes):\n",
	       len * 4);
	ax_sgpio_dump_data(rxbuf, len);

    free(rxbuf);

	return CMD_RET_SUCCESS;
}

static int do_ax_sgpio_ext_lb(struct cmd_tbl *cmdtp, int flag,
			   int argc, char *const argv[])
{
    u32 len;
    u32 *txbuf;
    u32 *rxbuf;
    int ret;
    int i;
    int src = 0;
    int dst = 1;

    len = AX_SGPIO_MAX_DATA_LEN;
    txbuf = malloc(len * sizeof(uint32_t));
    rxbuf = malloc(len * sizeof(uint32_t));

    if (!txbuf && !rxbuf)
        return CMD_RET_FAILURE;

    for (i = 0; i < len; i++) {
        txbuf[i] = (u32)(0xAABBCCDD);
    }

    printf("SGPIO_%d Send pattern\n", src);
    ret = ax_sgpio_set_value(src, txbuf, len);
    if (ret) {
		printf("SGPIO put failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }
    ax_sgpio_dump_data(txbuf, len);

    printf("SGPIO_%d Receive pattern\n", src);
    ret = ax_sgpio_get_value(dst, rxbuf, len);
    if (ret) {
		printf("SGPIO get failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }
	ax_sgpio_dump_data(txbuf, len);

    for (i = 0; i < len; i++) {
        if (txbuf[i] != rxbuf[i]) {
            printf("Loopback test failed\n");
            printf("Write  : 0x%02x\n",
                    txbuf[i]);
            printf("Read   : 0x%02x\n",
                    rxbuf[i]);

            free(txbuf);
            free(rxbuf);

            return CMD_RET_FAILURE;
        }
    }

    free(txbuf);
    free(rxbuf);

    printf("SGPIO External Loopback Test Passed\n");

    return CMD_RET_SUCCESS;
}

static int do_ax_sgpio(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    const char *cmd;

    cmd = argv[1];

    if (!strcmp(cmd, "request"))
        return do_ax_sgpio_request(cmdtp, flag,
                argc - 1, &argv[1]);

    if (!strcmp(cmd, "put"))
        return do_ax_sgpio_put(cmdtp, flag,
                argc - 1, &argv[1]);

    if (!strcmp(cmd, "get"))
        return do_ax_sgpio_get(cmdtp, flag,
                argc - 1, &argv[1]);

    if (!strcmp(cmd, "ext_lb"))
        return do_ax_sgpio_ext_lb(cmdtp, flag,
                argc - 1, &argv[1]);

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
        ax_sgpio, CONFIG_SYS_MAXARGS, 1, do_ax_sgpio,
        "Axiado SGPIO diagnostic command",
        "ax_sgpio request <bus> <ngpios>\n"
        "    - Initalize SGPIO device\n"
        "ax_sgpio put <bus> <byte0> [byte1 ...]\n"
        "    - Write data to SGPIO device\n"
        "ax_sgpio get <bus> <len>\n"
        "    - Read data from SGPIO device\n"
        "ax_sgpio ext_lb\n"
        "    - External Loopback test"
        );

