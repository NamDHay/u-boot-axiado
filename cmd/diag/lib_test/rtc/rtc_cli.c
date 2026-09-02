#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>
#include <asm/io.h>
#include <string.h>

#include "ax_rtc.h"
#include "rtc_cli.h"

static int do_rtc_set(int argc, char * const argv[])
{
    struct rtc_time tm;
    int ret;

    if (argc != 7) {
        printf("Usage:\n");
        printf("rtc set [n] <year> <month> <day> <hour> <min> <sec>\n");
        return CMD_RET_USAGE;
    }

    tm.tm_year = simple_strtoul(argv[1], NULL, 10) - 1900;
    tm.tm_mon  = simple_strtoul(argv[2], NULL, 10) - 1;
    tm.tm_mday = simple_strtoul(argv[3], NULL, 10);
    tm.tm_hour = simple_strtoul(argv[4], NULL, 10);
    tm.tm_min  = simple_strtoul(argv[5], NULL, 10);
    tm.tm_sec  = simple_strtoul(argv[6], NULL, 10);

    printf("%04d-%02d-%02d %02d:%02d:%02d\n",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec);

    ret = ax_rtc_set(&tm);
    if (ret) {
        printf("RTC write failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    return CMD_RET_SUCCESS;
}

static int do_rtc_get(int argc, char * const argv[]) 
{
    struct rtc_time tm;
    int ret;

    ret = ax_rtc_get(&tm);
    if (ret) {
        printf("RTC read failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    printf("%04d-%02d-%02d %02d:%02d:%02d\n",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec);

    return CMD_RET_SUCCESS;
}

int do_ax_rtc(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[]) {
    if (argc < 2)
        return CMD_RET_USAGE;

    argc--;
    argv++;

    ax_rtc_init();

    if (!strcmp(argv[0], "set"))
        return do_rtc_set(argc , argv);

    if (!strcmp(argv[0], "get"))
        return do_rtc_get(argc , argv);

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
        ax_rtc,	10,	0,	do_ax_rtc,
        "perform axiado rtc diagnostics",
        "set [yy mm dd hr min sec]  - set time for rtc device\n"
        "get                      	- show time of rtc device\n"
        );
