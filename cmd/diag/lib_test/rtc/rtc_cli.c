#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>
#include <asm/io.h>
#include <string.h>
#include <console.h>
#include <rtc.h>
#include <time.h>
#include <vsprintf.h>
#include <linux/delay.h>

#include "ax_rtc.h"
#include "rtc_cli.h"

static void rtc_hal_set_wday(struct rtc_time *hal);

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
    rtc_hal_set_wday(&tm);  /* was left uninitialised -> garbage weekday */

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

/*
 * The HAL uses the Linux struct rtc_time convention (tm_year = years since
 * 1900, tm_mon = 0..11), but U-Boot's rtc_mktime()/rtc_calc_weekday() use
 * full year and 1..12. Convert before calling them.
 */
static void rtc_hal_to_ub(const struct rtc_time *hal, struct rtc_time *ub)
{
    *ub = *hal;
    ub->tm_year += 1900;
    ub->tm_mon  += 1;
}

static time64_t rtc_hal_mktime(const struct rtc_time *hal)
{
    struct rtc_time ub;

    rtc_hal_to_ub(hal, &ub);
    return rtc_mktime(&ub);
}

/* Fill tm_wday (0 = Sunday) so the RTC day-of-week register is valid */
static void rtc_hal_set_wday(struct rtc_time *hal)
{
    struct rtc_time ub;

    rtc_hal_to_ub(hal, &ub);
    rtc_calc_weekday(&ub);
    hal->tm_wday = ub.tm_wday;
}

static void rtc_print(const char *tag, const struct rtc_time *tm)
{
    printf("%-8s %04d-%02d-%02d %02d:%02d:%02d (wday %d)\n", tag,
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_wday);
}

/* Range-check a value read back from the RTC; returns 0 if sane */
static int rtc_check_fields(const struct rtc_time *tm)
{
    struct rtc_time ref = *tm;

    if (tm->tm_sec < 0 || tm->tm_sec > 59 ||
        tm->tm_min < 0 || tm->tm_min > 59 ||
        tm->tm_hour < 0 || tm->tm_hour > 23 ||
        tm->tm_mday < 1 || tm->tm_mday > 31 ||
        tm->tm_mon < 0 || tm->tm_mon > 11) {
        printf("FAIL: RTC returned an out-of-range field\n");
        return -1;
    }

    rtc_hal_set_wday(&ref);
    if (ref.tm_wday != tm->tm_wday) {
        printf("FAIL: weekday %d, expected %d\n", tm->tm_wday, ref.tm_wday);
        return -1;
    }

    return 0;
}

/*
 * ax_rtc test [wait_s [tol_s]]
 *
 * 1. Set 2026-12-31 23:59:50. With the default 15 s wait, the RTC must roll
 *    over seconds, minutes, hours, day, month and year.
 * 2. Read back straight away: it must be the set time (+ tol_s).
 * 3. Wait wait_s seconds, timed by the CPU timer (get_timer).
 * 4. Read again: the RTC must have advanced by the measured time, within
 *    tol_s, and all fields (including the weekday) must be valid.
 */
static int do_rtc_test(int argc, char * const argv[])
{
    struct rtc_time set_tm = {
        .tm_year = 2026 - 1900, .tm_mon = 12 - 1, .tm_mday = 31,
        .tm_hour = 23, .tm_min = 59, .tm_sec = 50,
    };
    struct rtc_time tm;
    ulong wait_s = 15, tol_s = 1;
    ulong start, elapsed_ms, last_print = 0;
    time64_t t_set, t_now;
    long delta, expect;
    int ret;

    if (argc > 3)
        return CMD_RET_USAGE;
    if (argc > 1)
        wait_s = simple_strtoul(argv[1], NULL, 10);
    if (argc > 2)
        tol_s = simple_strtoul(argv[2], NULL, 10);
    if (!wait_s)
        return CMD_RET_USAGE;

    rtc_hal_set_wday(&set_tm);
    t_set = rtc_hal_mktime(&set_tm);

    /* 1. set */
    rtc_print("set:", &set_tm);
    ret = ax_rtc_set(&set_tm);
    if (ret) {
        printf("FAIL: RTC write failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }
    start = get_timer(0);

    /* 2. immediate read-back */
    ret = ax_rtc_get(&tm);
    if (ret) {
        printf("FAIL: RTC read failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }
    rtc_print("readback:", &tm);
    if (rtc_check_fields(&tm))
        return CMD_RET_FAILURE;
    delta = (long)(rtc_hal_mktime(&tm) - t_set);
    if (delta < 0 || delta > (long)tol_s) {
        printf("FAIL: read-back is %ld s from the set time (tol %lu s)\n",
               delta, tol_s);
        return CMD_RET_FAILURE;
    }

    /* 3. wait (mdelay keeps the ax_diag abort/timeout hook working) */
    printf("waiting %lu s ...\n", wait_s);
    while ((elapsed_ms = get_timer(start)) < wait_s * 1000) {
        if (ctrlc()) {
            puts("aborted\n");
            return CMD_RET_FAILURE;
        }
        if (elapsed_ms - last_print >= 5000) {
            last_print = elapsed_ms;
            if (!ax_rtc_get(&tm))
                rtc_print("  now:", &tm);
        }
        mdelay(10);
    }

    /* 4. re-check */
    ret = ax_rtc_get(&tm);
    elapsed_ms = get_timer(start);
    if (ret) {
        printf("FAIL: RTC read failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }
    rtc_print("after:", &tm);
    if (rtc_check_fields(&tm))
        return CMD_RET_FAILURE;

    t_now = rtc_hal_mktime(&tm);
    delta = (long)(t_now - t_set);
    expect = elapsed_ms / 1000;
    printf("RTC advanced %ld s, CPU timer %lu ms (tol %lu s)\n",
           delta, elapsed_ms, tol_s);

    if (delta < expect - (long)tol_s || delta > expect + (long)tol_s) {
        printf("FAIL: RTC drift %+ld s\n", delta - expect);
        return CMD_RET_FAILURE;
    }

    puts("PASS\n");
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

    if (!strcmp(argv[0], "test"))
        return do_rtc_test(argc, argv);

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
        ax_rtc,	10,	0,	do_ax_rtc,
        "Axiado bare-metal RTC diagnostics test",
        "set <yyyy> <mm> <dd> <hr> <min> <sec> - set time for rtc device\n"
        "ax_rtc get                       - show time of rtc device\n"
        "ax_rtc test [wait_s [tol_s]]     - set 2026-12-31 23:59:50, wait\n"
        "     wait_s (default 15) and check the RTC advanced by the elapsed\n"
        "     time within tol_s (default 1), incl. year rollover\n"
        );
