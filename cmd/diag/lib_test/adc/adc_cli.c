#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>

#include "ax_adc.h"
#include "adc_cli.h"

int do_ax_adc(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]) 
{
    int i;
    int ret;
    char *endp;
    unsigned int data;
    unsigned int channel;

    if (argc < 2)
        return CMD_RET_USAGE;

    if (!strcmp(argv[1], "start")) {
        channel = simple_strtol(argv[2], &endp, 10);
        ret = ax_adc_start_channels(channel);
        if (ret < 0)
            return CMD_RET_FAILURE;

    } else if (!strcmp(argv[1], "single")) {
        channel = simple_strtol(argv[2], &endp, 10);

        ret = ax_adc_start_channels(channel);
        if (ret < 0)
            return CMD_RET_FAILURE;

        ret = ax_adc_channel_data(channel, &data);
        if (ret < 0)
            return CMD_RET_FAILURE;

        printf("Channel %d: %d\n", channel, data);

    } else if (!strcmp(argv[1], "multi")) {
        channel = simple_strtol(argv[2], &endp, 10);
        ret = ax_adc_start_channels(channel);
        if (ret < 0)
            return CMD_RET_FAILURE;

        for (i = 0; i < channel; i++) {
            ret = ax_adc_channel_data(i, &data);
            if (ret < 0)
                return CMD_RET_FAILURE;
            printf("Channel %d: %d\n", i, data);
        }

    } else {
        return CMD_RET_USAGE;
    }

    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
        ax_adc,	CONFIG_SYS_MAXARGS,	0,	do_ax_adc,
        "perform axiado adc diagnostics",
        "start <channel> - Start total channels\n"
        "single <channel> - Get single raw data of ADC device channel\n"
        "multi <total_channel> - Get <total_channel> raw data of ADC device channel\n"
        );

