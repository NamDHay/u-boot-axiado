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

#include "ax_spi.h"

#define AX_SPI_MAX_DATA_LEN	256

static int ax_spi_parse_data(int argc, char *const argv[],
			     u8 *buf, int max_len)
{
	int i;
	int len = 0;
	ulong value;

	for (i = 0; i < argc; i++) {
		if (len >= max_len)
			return -ENOMEM;

		value = hextoul(argv[i], NULL);

		if (value > 0xff)
			return -EINVAL;

		buf[len++] = (u8)value;
	}

	return len;
}

static void ax_spi_dump_data(const u8 *buf, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if ((i % 16) == 0)
			printf("%04x: ", i);

		printf("%02x ", buf[i]);

		if ((i % 16) == 15 || i == len - 1)
			printf("\n");
	}
}

static int do_ax_spi_setup(struct cmd_tbl *cmdtp, int flag,
			   int argc, char *const argv[])
{
	unsigned int bus;
	unsigned int cs;
	unsigned int max_hz;
	unsigned int mode;
	int ret;

	if (argc != 5)
		return CMD_RET_USAGE;

	bus = simple_strtoul(argv[1], NULL, 0);
	cs = simple_strtoul(argv[2], NULL, 0);
	max_hz = simple_strtoul(argv[3], NULL, 0);
	mode = simple_strtoul(argv[4], NULL, 0);

	ret = ax_spi_setup(bus, cs, max_hz, mode);
	if (ret) {
		printf("SPI setup failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("SPI setup successful\n");
	printf("  Bus    : %u\n", bus);
	printf("  CS     : %u\n", cs);
	printf("  Speed  : %u Hz\n", max_hz);
	printf("  Mode   : %u\n", mode);

	return CMD_RET_SUCCESS;
}

static int do_ax_spi_write(struct cmd_tbl *cmdtp, int flag,
			   int argc, char *const argv[])
{
	unsigned int bus;
	u8 tx_buf[AX_SPI_MAX_DATA_LEN];
	int len;
	int ret;

	if (argc < 3)
		return CMD_RET_USAGE;

	bus = simple_strtoul(argv[1], NULL, 0);

	len = ax_spi_parse_data(argc - 2, &argv[2],
				tx_buf, AX_SPI_MAX_DATA_LEN);
	if (len < 0) {
		printf("Invalid SPI data\n");
		return CMD_RET_FAILURE;
	}

	ret = ax_spi_xfer(bus, len * 8, tx_buf, NULL);
	if (ret) {
		printf("SPI write failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("SPI write successful (%d bytes)\n", len);

	return CMD_RET_SUCCESS;
}

static int do_ax_spi_read(struct cmd_tbl *cmdtp, int flag,
			  int argc, char *const argv[])
{
	unsigned int bus;
	unsigned int bitlen;
	unsigned int bytes;
	u8 *rx_buf;
	int ret;

	if (argc != 3)
		return CMD_RET_USAGE;

	bus = simple_strtoul(argv[1], NULL, 0);
	bitlen = simple_strtoul(argv[2], NULL, 0);

	if (!bitlen) {
		printf("Invalid bit length\n");
		return CMD_RET_FAILURE;
	}

	bytes = DIV_ROUND_UP(bitlen, 8);

	if (bytes > AX_SPI_MAX_DATA_LEN) {
		printf("Maximum transfer size is %d bytes\n",
		       AX_SPI_MAX_DATA_LEN);
		return CMD_RET_FAILURE;
	}

	rx_buf = calloc(bytes, sizeof(*rx_buf));
	if (!rx_buf)
		return CMD_RET_FAILURE;

	ret = ax_spi_xfer(bus, bitlen, NULL, rx_buf);
	if (ret) {
		printf("SPI read failed: %d\n", ret);
		free(rx_buf);
		return CMD_RET_FAILURE;
	}

	printf("SPI read (%u bits / %u bytes):\n",
	       bitlen, bytes);
	ax_spi_dump_data(rx_buf, bytes);

	free(rx_buf);

	return CMD_RET_SUCCESS;
}

static int do_ax_spi_xfer(struct cmd_tbl *cmdtp, int flag,
			  int argc, char *const argv[])
{
	unsigned int bus;
	u8 tx_buf[AX_SPI_MAX_DATA_LEN];
	u8 rx_buf[AX_SPI_MAX_DATA_LEN];
	int len;
	int ret;

	if (argc < 3)
		return CMD_RET_USAGE;

	bus = simple_strtoul(argv[1], NULL, 0);

	len = ax_spi_parse_data(argc - 2, &argv[2],
				tx_buf, AX_SPI_MAX_DATA_LEN);
	if (len < 0) {
		printf("Invalid SPI data\n");
		return CMD_RET_FAILURE;
	}

	memset(rx_buf, 0, sizeof(rx_buf));

	ret = ax_spi_xfer(bus, len, tx_buf, rx_buf);
	if (ret) {
		printf("SPI transfer failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("SPI TX:\n");
	ax_spi_dump_data(tx_buf, len);

	printf("SPI RX:\n");
	ax_spi_dump_data(rx_buf, len);

	return CMD_RET_SUCCESS;
}

static int do_ax_spi(struct cmd_tbl *cmdtp, int flag,
		     int argc, char *const argv[])
{
	const char *cmd;

	if (argc < 2)
		return CMD_RET_USAGE;

	cmd = argv[1];

	if (!strcmp(cmd, "setup"))
		return do_ax_spi_setup(cmdtp, flag,
				       argc - 1, &argv[1]);

	if (!strcmp(cmd, "write"))
		return do_ax_spi_write(cmdtp, flag,
				       argc - 1, &argv[1]);

	if (!strcmp(cmd, "read"))
		return do_ax_spi_read(cmdtp, flag,
				      argc - 1, &argv[1]);

	if (!strcmp(cmd, "xfer"))
		return do_ax_spi_xfer(cmdtp, flag,
				      argc - 1, &argv[1]);

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	ax_spi, CONFIG_SYS_MAXARGS, 1, do_ax_spi,
	"Axiado SPI diagnostic command",
	"setup <bus> <cs> <max_hz> <mode>\n"
	"    - Configure SPI controller\n"
	"ax_spi write <bus> <byte0> [byte1 ...]\n"
	"    - Write data to SPI device\n"
	"ax_spi read <bus> <bitlen>\n"
	"    - Read data from SPI device\n"
	"ax_spi xfer <bus> <byte0> [byte1 ...]\n"
	"    - Full duplex SPI transfer"
);
