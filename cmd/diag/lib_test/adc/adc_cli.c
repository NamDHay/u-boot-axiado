// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * U-Boot "ax_adc" command: SARADC diagnostics.
 */

#include <command.h>
#include <dm.h>
#include <errno.h>
#include <stdio.h>
#include <vsprintf.h>
#include <linux/bitops.h>
#include <linux/printk.h>

#include "ax_adc.h"

#define AX_ADC_NUM_CHANNELS	16

static int ax_adc_parse_uint(const char *str, int base, unsigned long *val)
{
	char *endp;

	if (!*str)
		return -EINVAL;

	*val = simple_strtoul(str, &endp, base);

	return *endp ? -EINVAL : 0;
}

/* Parse <total_channel> (1..16) and convert it to the mask of channels 0..n-1 */
static int ax_adc_parse_total(const char *str, unsigned int *total,
			      unsigned int *mask)
{
	unsigned long n;

	if (ax_adc_parse_uint(str, 10, &n))
		return -EINVAL;

	if (!n || n > AX_ADC_NUM_CHANNELS)
		return -ERANGE;

	*total = n;
	*mask = GENMASK(n - 1, 0);

	return 0;
}

static int do_ax_adc_start(int argc, char *const argv[])
{
	unsigned int total, mask;
	int ret;

	if (argc != 3)
		return CMD_RET_USAGE;

	if (ax_adc_parse_total(argv[2], &total, &mask)) {
		printf("Invalid total channel (1..%d): %s\n",
		       AX_ADC_NUM_CHANNELS, argv[2]);
		return CMD_RET_FAILURE;
	}

	ret = ax_adc_start_channels(mask);
	if (ret) {
		printf("Start channels failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("Started channels 0..%u\n", total - 1);

	return CMD_RET_SUCCESS;
}

static int do_ax_adc_single(int argc, char *const argv[])
{
	unsigned long channel, min, max;
	unsigned int data;
	int ret;

	if (argc != 5)
		return CMD_RET_USAGE;

	if (ax_adc_parse_uint(argv[2], 10, &channel) ||
	    channel >= AX_ADC_NUM_CHANNELS) {
		printf("Invalid channel (0..%d): %s\n",
		       AX_ADC_NUM_CHANNELS - 1, argv[2]);
		return CMD_RET_FAILURE;
	}

	/* min/max accept decimal or 0x-prefixed hex */
	if (ax_adc_parse_uint(argv[3], 0, &min) ||
	    ax_adc_parse_uint(argv[4], 0, &max) || min > max) {
		printf("Invalid range: %s %s\n", argv[3], argv[4]);
		return CMD_RET_FAILURE;
	}

	ret = ax_adc_start_channel(channel);
	if (ret) {
		printf("Start channel %lu failed: %d\n", channel, ret);
		return CMD_RET_FAILURE;
	}

	ret = ax_adc_channel_data(channel, &data);
	if (ret) {
		printf("Read channel %lu failed: %d\n", channel, ret);
		return CMD_RET_FAILURE;
	}

	printf("Channel %lu: 0x%x -> ", channel, data);
	if (data < min || data > max) {
		printf("Out of range\n");
		return CMD_RET_FAILURE;
	}
	printf("Valid\n");

	return CMD_RET_SUCCESS;
}

static int do_ax_adc_multi(int argc, char *const argv[])
{
	unsigned int data[AX_ADC_NUM_CHANNELS];
	unsigned int total, mask;
	unsigned int i;
	int ret;

	if (argc != 3)
		return CMD_RET_USAGE;

	if (ax_adc_parse_total(argv[2], &total, &mask)) {
		printf("Invalid total channel (1..%d): %s\n",
		       AX_ADC_NUM_CHANNELS, argv[2]);
		return CMD_RET_FAILURE;
	}

	ret = ax_adc_start_channels(mask);
	if (ret) {
		printf("Start channels failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	/* Results are packed in ascending channel order: data[i] = channel i */
	ret = ax_adc_channels_data(mask, data);
	if (ret) {
		printf("Read channels failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	for (i = 0; i < total; i++)
		printf("Channel %u: 0x%x\n", i, data[i]);

	return CMD_RET_SUCCESS;
}

static int do_ax_adc_stop(int argc, char *const argv[])
{
	int ret;

	if (argc != 2)
		return CMD_RET_USAGE;

	ret = ax_adc_stop();
	if (ret) {
		printf("Stop failed: %d\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

int do_ax_adc(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "start"))
		return do_ax_adc_start(argc, argv);
	else if (!strcmp(argv[1], "single"))
		return do_ax_adc_single(argc, argv);
	else if (!strcmp(argv[1], "multi"))
		return do_ax_adc_multi(argc, argv);
	else if (!strcmp(argv[1], "stop"))
		return do_ax_adc_stop(argc, argv);

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	ax_adc, 5, 0, do_ax_adc,
	"Axiado bare-metal SARADC diagnostics test",
	"start <total_channel> - Start channels 0..<total_channel>-1\n"
	"ax_adc single <channel> <min_val> <max_val> - Read one channel and check the raw value is within [min, max]\n"
	"ax_adc multi <total_channel> - Read raw data of channels 0..<total_channel>-1\n"
	"ax_adc stop - Power down the ADC\n"
	"  <channel> and <total_channel> are decimal; <min_val>/<max_val> accept decimal or 0x hex"
);
