// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include "ax_diag.h"
#include "command.h"
#include <config.h>
#include <log.h>
#include <linux/string.h>
#include <linux/libfdt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <asm/cache.h>
#include <cpu_func.h>
#include <mapmem.h>
#include <time.h>
#include <u-boot/crc.h>
#include <console.h>
#if IS_ENABLED(CONFIG_CMD_AX_DIAG_ABORT)
#include <cyclic.h>
#include <setjmp.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;
#endif

struct diag_test diag_list[] = {
#ifdef CONFIG_CMD_AX_MEM
    {
        "mem",
        "mem diagnostic test",
        .test = {
            { "ax_memtest 0x40200000 0x100000 1", 5000000, 0 },
            { "ax_memtest 0x90000000 0x10000000 1", 5000000, 0 }
        }
    },
#endif

#ifdef CONFIG_CMD_AX_GPIO
    {
        "gpio",
        "gpio diagnostic test",
        .test = {
            { "ax_gpio set 6", 100, 0 },
            { "ax_gpio set 30", 100, 0 },
            { "ax_gpio set 31", 100, 0 },
            { "ax_gpio set 126", 100, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_ADC
    {
        "adc",
        "adc diagnostic test",
        .test = {
            { "ax_adc single 0 0 10000", 100, 0 },
            { "ax_adc single 1 0 10000", 100, 0 },
            { "ax_adc single 2 0 10000", 100, 0 },
            { "ax_adc single 3 0 10000", 100, 0 },
            { "ax_adc single 4 0 10000", 100, 0 },
            { "ax_adc single 5 0 10000", 100, 0 },
            { "ax_adc single 6 0 10000", 100, 0 },
            { "ax_adc single 7 0 10000", 100, 0 },
        },
    },
#endif

#ifdef CONFIG_CMD_AX_EMMC
    {
        "emmc",
        "emmc diagnostic test",
        .test = {
            { "ax_mmc init 2 2 2; ax_mmc memtest 0 100", 5000, 0 },
        },
    },
#endif

#ifdef CONFIG_CMD_AX_HDMI
    {
        "hdmi",
        "hdmi diagnostic test",
        .test = {
            { "ax_hdmi bist", 5000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_I2C
    {
        "i2c",
        "i2c diagnostic test",
        .test = {
#ifdef CONFIG_CMD_AX_VSENS
            { "ax_ina read i2c 16 0x41", 5000, 0 },
            { "ax_ina read i2c 16 0x42", 5000, 0 },
            { "ax_ina read i2c 16 0x43", 5000, 0 },
            { "ax_ina read i2c 16 0x48", 5000, 0 },
            { "ax_ina read i2c 16 0x49", 5000, 0 },
            { "ax_ina read i2c 16 0x4a", 5000, 0 },
            { "ax_ina read i2c 16 0x4b", 5000, 0 },
            { "ax_ina read i2c 16 0x4c", 5000, 0 },
#endif
#ifdef CONFIG_CMD_AX_TSENS
            { "ax_temp read i2c 16 0x4e", 5000, 0 },
            { "ax_temp read i2c 16 0x4f", 5000, 0 },
#endif
#ifdef CONFIG_CMD_AX_EEPROM
            { "ax_eeprom memtest i2c 16 0x50 0 100", 5000, 0 },
#endif
        }
    },
#endif

#ifdef CONFIG_CMD_AX_I3C
    {
        "i3c",
        "i3c diagnostic test",
        .test = {
#ifdef CONFIG_CMD_AX_VSENS
            /* { "ax_ina read i2c 16 0x41", 5000, 0 }, */
            /* { "ax_ina read i2c 16 0x42", 5000, 0 }, */
            /* { "ax_ina read i2c 16 0x43", 5000, 0 }, */
            /* { "ax_ina read i2c 16 0x48", 5000, 0 }, */
            /* { "ax_ina read i2c 16 0x49", 5000, 0 }, */
            /* { "ax_ina read i2c 16 0x4a", 5000, 0 }, */
            /* { "ax_ina read i2c 16 0x4b", 5000, 0 }, */
            /* { "ax_ina read i2c 16 0x4c", 5000, 0 }, */
#endif
#ifdef CONFIG_CMD_AX_TSENS
            /* { "ax_temp read i2c 16 0x4e", 5000, 0 }, */
            /* { "ax_temp read i2c 16 0x4f", 5000, 0 }, */
#endif
#ifdef CONFIG_CMD_AX_EEPROM
            /* { "ax_eeprom memtest i2c 16 0x50 0 100", 5000, 0 }, */
#endif
        }
    },
#endif

#ifdef CONFIG_CMD_AX_LTPI
    {
        "ltpi",
        "ltpi diagnostic test",
        .test = {
            { "ax_ltpi init; ax_ltpi set gpio 0 1; ax_ltpi get gpio 0", 10000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_UART
    {
        "uart",
        "uart diagnostic test",
        .test = {
            { "ax_uart ext_lb 0", 1000, 0 },
            { "ax_uart ext_lb 1", 1000, 0 },
            { "ax_uart ext_lb 2", 1000, 0 },
            { "ax_uart ext_lb 4", 1000, 0 },
            { "ax_uart ext_lb 5", 1000, 0 },
            { "ax_uart ext_lb 6", 1000, 0 },
            { "ax_uart ext_lb 7", 1000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_PCIE
    {
        "pcie",
        "pcie diagnostic test",
        .test = {
            { "ax_pcie init 1 4", 10000, 0 },
            { "ax_pcie init 2 4", 10000, 0 },

            /* { "ax_gpio input 34; ax_pcie init 1 4", 10000, 0 }, */
            /* { "ax_gpio clear 203; ax_gpio input 37; ax_pcie init 2 4", 10000, 0 }, */
#ifdef CONFIG_CMD_AX_NVME
            { "ax_nvme init 1; ax_nvme compare 0 100", 10000, 0 },
            { "ax_nvme init 2; ax_nvme compare 0 100", 10000, 0 },
#endif
        }
    },
#endif

#ifdef CONFIG_CMD_AX_PWM
    {
        "pwm",
        "pwm diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_RMII
    {
        "rmii",
        "rmii diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_RTC
    {
        "rtc",
        "rtc diagnostic test",
        .test = {
            /* { "ax_rtc get", 1000, 0 }, */
        }
    },
#endif

#ifdef CONFIG_CMD_AX_SGMII
    {
        "sgmii",
        "sgmii diagnostic test",
        .test = {
        }
    },
#endif

#ifdef CONFIG_CMD_AX_SGPIO
    {
        "sgpio",
        "sgpio diagnostic test",
        .test = {
            { "ax_sgpio request 0 512; ax_sgpio request 1 512", 1000, 0 },
            { "ax_sgpio ext_lb", 1000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_SPI
    {
        "spi",
        "spi diagnostic test",
        .test = {
#ifdef CONFIG_CMD_AX_FLASH
            { "ax_flash init 0 0; ax_flash memtest 0 0 0x1000", 2000, 0 },
            { "ax_flash init 0 1; ax_flash memtest 0 0 0x1000", 2000, 0 },
#endif
        }
    },
#endif

#ifdef CONFIG_CMD_AX_USB
    {
        "usb",
        "usb diagnostic test",
        .test = {
            { "ax_usb stop; ax_usb init mmio 0x41000000; ax_usb info; ax_usb compare 0 0xa0000000 0 1000", 7000, 0 },
            { "ax_usb stop; ax_usb init mmio 0x41400000; ax_usb info; ax_usb compare 0 0xa0000000 0 1000", 7000, 0 },
            { "ax_usb stop; ax_usb init mmio 0x41500000; ax_usb info; ax_usb compare 0 0xa0000000 0 1000", 7000, 0 },
        }
    },
#endif

#ifdef CONFIG_CMD_AX_XGMII
    {
        "xgmii",
        "xgmii diagnostic test",
        .test = {
        }
    },
#endif
};

unsigned int diag_list_size = ARRAY_SIZE(diag_list);
static bool g_diag_run_started;

/* ------------------------------------------------------------------------ */
/* Persistent result record                                                 */
/* ------------------------------------------------------------------------ */

/*
 * The record lives at a fixed DRAM address (CONFIG_CMD_AX_DIAG_RECORD_ADDR)
 * that U-Boot does not clear on boot. Every state change is written back
 * from the D-cache to DRAM before the test command runs, so after a crash,
 * hang + manual reset, or watchdog reset the last RUNNING entry tells
 * exactly which test (and which ';' sub-command) was executing.
 */
#define DIAG_REC_MAGIC      0x47414944  /* "DIAG" */
#define DIAG_REC_VERSION    1

struct diag_rec_tc {
    u8 status;
    u8 step;
    u16 rsvd;
    s32 ret;
    u32 elapsed;
};

struct diag_record {
    u32 magic;
    u32 version;
    u32 layout;         /* crc32 of the test table, detects FW change */
    u32 run_seq;        /* incremented on every 'ax_diag run' */
    u32 active;         /* 1 while a test command is executing */
    s32 cur_group;
    s32 cur_test;
    s32 cur_step;
    struct diag_rec_tc tc[DIAG_MAX_GROUP][DIAG_MAX_TEST];
    u32 crc;
};

static const char *const diag_st_name[DIAG_ST_COUNT] = {
    [DIAG_ST_NOT_RUN] = "NOT RUN",
    [DIAG_ST_RUNNING] = "RUNNING",
    [DIAG_ST_PASS]    = "PASS",
    [DIAG_ST_FAIL]    = "FAIL",
    [DIAG_ST_TIMEOUT] = "TIMEOUT",
    [DIAG_ST_SKIPPED] = "SKIPPED",
    [DIAG_ST_CRASH]   = "CRASH",
    [DIAG_ST_ABORTED] = "ABORTED",
};

static const char *diag_st_str(int st)
{
    if (st < 0 || st >= DIAG_ST_COUNT)
        return "?";
    return diag_st_name[st];
}

static struct diag_record *diag_rec(void)
{
    return map_sysmem(CONFIG_CMD_AX_DIAG_RECORD_ADDR,
                      sizeof(struct diag_record));
}

static u32 diag_layout_id(void)
{
    u32 crc = 0;
    unsigned int g, t;

    for (g = 0; g < diag_list_size; g++) {
        crc = crc32(crc, (const u8 *)diag_list[g].name,
                    strlen(diag_list[g].name) + 1);
        for (t = 0; t < DIAG_MAX_TEST && diag_list[g].test[t].cmd; t++)
            crc = crc32(crc, (const u8 *)diag_list[g].test[t].cmd,
                        strlen(diag_list[g].test[t].cmd) + 1);
    }

    return crc;
}

static u32 diag_rec_crc(struct diag_record *r)
{
    return crc32(0, (const u8 *)r, offsetof(struct diag_record, crc));
}

static void diag_rec_commit(struct diag_record *r)
{
    ulong start = (ulong)r & ~(ulong)(ARCH_DMA_MINALIGN - 1);
    ulong end = ALIGN((ulong)r + sizeof(*r), ARCH_DMA_MINALIGN);

    r->crc = diag_rec_crc(r);
    /* Make sure the record reaches DRAM before a possible crash */
    flush_dcache_range(start, end);
}

static bool diag_rec_valid(struct diag_record *r)
{
    return r->magic == DIAG_REC_MAGIC &&
           r->version == DIAG_REC_VERSION &&
           r->layout == diag_layout_id() &&
           r->crc == diag_rec_crc(r);
}

static void diag_rec_reset(struct diag_record *r)
{
    memset(r, 0, sizeof(*r));
    r->magic = DIAG_REC_MAGIC;
    r->version = DIAG_REC_VERSION;
    r->layout = diag_layout_id();
    r->cur_group = -1;
    r->cur_test = -1;
    r->cur_step = -1;
    diag_rec_commit(r);
}

/* Return a valid record, creating a fresh one if needed. */
static struct diag_record *diag_rec_get(void)
{
    struct diag_record *r = diag_rec();

    if (!diag_rec_valid(r))
        diag_rec_reset(r);

    return r;
}

/* Copy the step-th ';' separated sub-command of cmd into buf (trimmed). */
static char *diag_sub_cmd(const char *cmd, int step, char *buf, size_t len)
{
    const char *p = cmd, *e;
    int i = 0;

    buf[0] = '\0';
    while (p) {
        while (*p == ' ' || *p == '\t')
            p++;
        e = strchr(p, ';');
        if (*p && *p != ';') {
            if (i == step) {
                size_t n = e ? (size_t)(e - p) : strlen(p);

                if (n >= len)
                    n = len - 1;
                memcpy(buf, p, n);
                buf[n] = '\0';
                while (n && (buf[n - 1] == ' ' || buf[n - 1] == '\t'))
                    buf[--n] = '\0';
                return buf;
            }
            i++;
        }
        p = e ? e + 1 : NULL;
    }

    return buf;
}

/*
 * If the previous run was interrupted (board reset while a test was
 * RUNNING) convert that entry to CRASH. Returns true if a crash was found.
 */
static bool diag_rec_check_crash(struct diag_record *r)
{
    struct diag_rec_tc *e;
    char sub[CONFIG_SYS_CBSIZE];
    int g = r->cur_group, t = r->cur_test;

    if (!r->active)
        return false;

    r->active = 0;
    if (g < 0 || g >= (int)diag_list_size || t < 0 || t >= DIAG_MAX_TEST) {
        diag_rec_commit(r);
        return false;
    }

    e = &r->tc[g][t];
    e->status = DIAG_ST_CRASH;
    diag_rec_commit(r);

    printf("\n*** ax_diag: previous run #%u was interrupted (crash/hang/reset)\n",
           r->run_seq);
    printf("***   group : %s\n", diag_list[g].name);
    printf("***   test  : #%d \"%s\"\n", t, diag_list[g].test[t].cmd);
    printf("***   step  : #%d \"%s\"\n", r->cur_step,
           diag_sub_cmd(diag_list[g].test[t].cmd, r->cur_step, sub, sizeof(sub)));
    printf("***   time in test before reset: >= %u ms\n\n", e->elapsed);

    return true;
}

static void diag_rec_update(struct diag_record *r, int g, int t,
                            struct testcase *tc, bool active)
{
    struct diag_rec_tc *e;

    if (g < 0 || g >= DIAG_MAX_GROUP || t < 0 || t >= DIAG_MAX_TEST)
        return;

    e = &r->tc[g][t];
    e->status = tc->status;
    e->step = tc->step;
    e->ret = tc->ret;
    e->elapsed = tc->elapsed;

    r->active = active;
    r->cur_group = g;
    r->cur_test = t;
    r->cur_step = tc->step;
    diag_rec_commit(r);
}

/* ------------------------------------------------------------------------ */
/* Info                                                                     */
/* ------------------------------------------------------------------------ */

static int diag_info_single(struct diag_test *test, int full)
{
    int i;
    printf("%s- %s\n", test->name, test->desc);
    if (full) {
        for (i = 0; i < DIAG_MAX_TEST && test->test[i].cmd != NULL; i++) {
            printf("\t%2d: %s (timeout %d ms)\n", i, test->test[i].cmd,
                   test->test[i].timeout);
        }
    }

    return 0;
}

int diag_info(char *name)
{
    unsigned int i;

    if (name == NULL) {
        for (i = 0; i < diag_list_size; i++)
            diag_info_single(diag_list + i, 0);

        return 0;
    } else {
        for (i = 0; i < diag_list_size; i++) {
            if (strcmp(diag_list[i].name, name) == 0)
                break;
        }

        if (i < diag_list_size)
            return diag_info_single(diag_list + i, 1);
        else
            return -1;
    }
}

/* ------------------------------------------------------------------------ */
/* Run                                                                      */
/* ------------------------------------------------------------------------ */

/*
 * Abort guard
 *
 * Test commands cannot be killed in U-Boot, and most IP drivers never look
 * at Ctrl+C. But nearly every wait loop calls udelay()/mdelay(), which call
 * schedule() -> cyclic_run(). A cyclic hook there checks for Ctrl+C and for
 * the testcase timeout, and if one fires it longjmp()s back to diag_exec(),
 * abandoning the command without any change in the drivers.
 */
enum diag_abort {
    DIAG_ABORT_NONE = 0,
    DIAG_ABORT_CTRLC,
    DIAG_ABORT_TIMEOUT,
};

#if IS_ENABLED(CONFIG_CMD_AX_DIAG_ABORT)
#define DIAG_GUARD_PERIOD_US    10000   /* poll every 10 ms */

static struct {
    struct cyclic_info cyclic;
    jmp_buf jb;
    bool registered;
    volatile bool armed;    /* jb is valid, a command is running */
    ulong start;            /* get_timer() base of the testcase */
    int timeout;            /* ms, 0 = no limit */
} diag_guard;

static void diag_guard_func(struct cyclic_info *c)
{
    int why = DIAG_ABORT_NONE;

    if (!diag_guard.armed)
        return;

    if (ctrlc())
        why = DIAG_ABORT_CTRLC;
    else if (diag_guard.timeout > 0 &&
             get_timer(diag_guard.start) > (ulong)diag_guard.timeout)
        why = DIAG_ABORT_TIMEOUT;

    if (why == DIAG_ABORT_NONE)
        return;

    diag_guard.armed = false;
    /* We leave cyclic_run() without returning: drop its recursion guard */
    gd->flags &= ~GD_FLG_CYCLIC_RUNNING;
    longjmp(diag_guard.jb, why);
}

static void diag_guard_start(void)
{
    if (diag_guard.registered)
        return;
    cyclic_register(&diag_guard.cyclic, diag_guard_func,
                    DIAG_GUARD_PERIOD_US, "ax_diag");
    diag_guard.registered = true;
}

static void diag_guard_stop(void)
{
    if (!diag_guard.registered)
        return;
    diag_guard.armed = false;
    cyclic_unregister(&diag_guard.cyclic);
    diag_guard.registered = false;
}

/* Run one sub-command; *abort says if/why it was cut short. */
static int diag_exec(const char *cmd, ulong start, int timeout, int *abort)
{
    int ret;

    *abort = DIAG_ABORT_NONE;
    diag_guard.start = start;
    diag_guard.timeout = timeout;
    clear_ctrlc();

    ret = setjmp(diag_guard.jb);
    if (ret) {
        /* Came back from diag_guard_func() */
        *abort = ret;
        clear_ctrlc();
        return 0;
    }

    diag_guard.armed = true;
    ret = run_command(cmd, 0);
    diag_guard.armed = false;

    /* The command saw the Ctrl+C itself and returned early */
    if (had_ctrlc())
        *abort = DIAG_ABORT_CTRLC;
    clear_ctrlc();

    return ret;
}
#else
static void diag_guard_start(void) {}
static void diag_guard_stop(void) {}

static int diag_exec(const char *cmd, ulong start, int timeout, int *abort)
{
    int ret;

    clear_ctrlc();
    ret = run_command(cmd, 0);
    *abort = had_ctrlc() ? DIAG_ABORT_CTRLC : DIAG_ABORT_NONE;
    clear_ctrlc();

    return ret;
}
#endif

/* Run one testcase, each ';' sub-command separately so a crash is pinned
 * down to the exact sub-command. */
static int diag_run_tc(struct diag_record *r, struct diag_test *ip,
                       int g, int t)
{
    struct testcase *tc = &ip->test[t];
    char sub[CONFIG_SYS_CBSIZE];
    ulong start = get_timer(0);
    int abort = DIAG_ABORT_NONE;
    int step;

    tc->ret = 0;
    tc->elapsed = 0;

    for (step = 0; ; step++) {
        if (!*diag_sub_cmd(tc->cmd, step, sub, sizeof(sub)))
            break;

        tc->step = step;
        tc->status = DIAG_ST_RUNNING;
        tc->elapsed = get_timer(start);
        diag_rec_update(r, g, t, tc, true);

        printf("\n[diag] %s #%d.%d: %s\n", ip->name, t, step, sub);
        tc->ret = diag_exec(sub, start, tc->timeout, &abort);
        if (abort == DIAG_ABORT_CTRLC)
            printf("\n[diag] %s #%d.%d: aborted by Ctrl+C\n",
                   ip->name, t, step);
        else if (abort == DIAG_ABORT_TIMEOUT)
            printf("\n[diag] %s #%d.%d: aborted, timeout %d ms exceeded\n",
                   ip->name, t, step, tc->timeout);

        if (tc->ret || abort) {
            /* Abort the rest of this line, move on to the next testcase */
            while (*diag_sub_cmd(tc->cmd, ++step, sub, sizeof(sub)))
                printf("[diag] %s #%d.%d: skipped: %s\n", ip->name, t,
                       step, sub);
            break;
        }
    }

    tc->elapsed = get_timer(start);
    if (abort == DIAG_ABORT_CTRLC)
        tc->status = DIAG_ST_ABORTED;
    else if (abort == DIAG_ABORT_TIMEOUT)
        tc->status = DIAG_ST_TIMEOUT;
    else if (tc->ret)
        tc->status = DIAG_ST_FAIL;
    else if (tc->timeout > 0 && tc->elapsed > (ulong)tc->timeout)
        tc->status = DIAG_ST_TIMEOUT;
    else
        tc->status = DIAG_ST_PASS;

    diag_rec_update(r, g, t, tc, false);

    printf("[diag] %s #%d: %s (%lu ms", ip->name, t,
           diag_st_str(tc->status), tc->elapsed);
    if (tc->status == DIAG_ST_FAIL)
        printf(", ret %d at step %d", tc->ret, tc->step);
    else if (tc->status == DIAG_ST_TIMEOUT)
        printf(", limit %d ms at step %d", tc->timeout, tc->step);
    else if (tc->status == DIAG_ST_ABORTED)
        printf(", Ctrl+C at step %d", tc->step);
    printf(")\n");

    return tc->status == DIAG_ST_PASS ? 0 : 1;
}

/* Returns the number of testcases that did not pass. */
static int diag_run_single(struct diag_record *r, int g)
{
    struct diag_test *ip = &diag_list[g];
    int i, fails = 0;

    /* Clear previous result of this group */
    for (i = 0; i < DIAG_MAX_TEST; i++) {
        ip->test[i].status = DIAG_ST_NOT_RUN;
        ip->test[i].ret = 0;
        ip->test[i].step = 0;
        ip->test[i].elapsed = 0;
        if (g < DIAG_MAX_GROUP)
            memset(&r->tc[g][i], 0, sizeof(r->tc[g][i]));
    }
    diag_rec_commit(r);

    printf("\n================ diag: %s ================\n", ip->name);
    /*
     * A failure only aborts the remaining ';' sub-commands of that testcase
     * (see diag_run_tc); the other testcases of the group still run.
     */
    for (i = 0; i < DIAG_MAX_TEST && ip->test[i].cmd != NULL; i++)
        fails += diag_run_tc(r, ip, g, i);

    return fails;
}

static void diag_run_begin(struct diag_record *r)
{
    if (g_diag_run_started)
        return;
    g_diag_run_started = true;
    diag_guard_start();
    diag_rec_check_crash(r);
    r->run_seq++;
    diag_rec_commit(r);
}

/*
 * name == NULL: run all groups.
 * Returns -ENOENT if no such test, otherwise the number of failed testcases.
 */
int diag_run(char *name, unsigned long testid)
{
    struct diag_record *r = diag_rec_get();
    unsigned int i;
    int fails = 0;

    if (name == NULL) {
        diag_run_begin(r);
        for (i = 0; i < diag_list_size; i++)
            fails += diag_run_single(r, i);
        return fails;
    }

    for (i = 0; i < diag_list_size; i++) {
        if (strcmp(diag_list[i].name, name) == 0)
            break;
    }

    if (i >= diag_list_size)
        return -ENOENT;

    diag_run_begin(r);
    return diag_run_single(r, i);
}

/* Mark the end of an 'ax_diag run' invocation (one or more groups). */
void diag_run_end(void)
{
    diag_guard_stop();
    g_diag_run_started = false;
}

/* ------------------------------------------------------------------------ */
/* Status report                                                            */
/* ------------------------------------------------------------------------ */

int diag_status(void)
{
    struct diag_record *r = diag_rec();
    unsigned int cnt[DIAG_ST_COUNT] = { 0 };
    char sub[CONFIG_SYS_CBSIZE];
    unsigned int g;
    int t;

    if (!diag_rec_valid(r)) {
        puts("ax_diag: no valid result record (never run, cleared, "
             "power-cycled or firmware changed)\n");
        return -ENOENT;
    }

    diag_rec_check_crash(r);

    printf("\n======================= AX DIAG STATUS (run #%u) =======================\n",
           r->run_seq);
    /*
     * Tree view:
     *   group
     *   |-- #0  PASS                120 ms  <command>
     *   |-- #1  FAIL[1]              35 ms  <command>
     *   |   `-- step 1: "<sub-command>"
     *   `-- #2  PASS                 98 ms  <command>
     */
    for (g = 0; g < diag_list_size && g < DIAG_MAX_GROUP; g++) {
        struct diag_test *ip = &diag_list[g];
        bool any = false;
        int n;

        for (n = 0; n < DIAG_MAX_TEST && ip->test[n].cmd; n++) {
            struct diag_rec_tc *e = &r->tc[g][n];

            cnt[e->status < DIAG_ST_COUNT ? e->status : DIAG_ST_NOT_RUN]++;
            if (e->status != DIAG_ST_NOT_RUN)
                any = true;
        }
        if (!any)
            continue;

        printf("\n%s\n", ip->name);
        for (t = 0; t < n; t++) {
            struct diag_rec_tc *e = &r->tc[g][t];
            bool last = (t == n - 1);
            char st[24];

            /* Status with error code appended, e.g. "FAIL[108]" */
            if (e->ret)
                snprintf(st, sizeof(st), "%s[%d]",
                         diag_st_str(e->status), e->ret);
            else
                snprintf(st, sizeof(st), "%s", diag_st_str(e->status));

            printf("%s #%-2d %-14s", last ? "`--" : "|--", t, st);
            if (e->status == DIAG_ST_CRASH || e->status == DIAG_ST_SKIPPED ||
                e->status == DIAG_ST_NOT_RUN)
                printf(" %8s     ", "-");
            else
                printf(" %8u ms  ", e->elapsed);
            printf("%s\n", ip->test[t].cmd);

            if (e->status == DIAG_ST_FAIL || e->status == DIAG_ST_CRASH ||
                e->status == DIAG_ST_TIMEOUT || e->status == DIAG_ST_ABORTED)
                printf("%s   `-- step %d: \"%s\"\n", last ? " " : "|",
                       e->step,
                       diag_sub_cmd(ip->test[t].cmd, e->step, sub, sizeof(sub)));
        }
    }
    puts("\n");

    puts("-------------------------------------------------------------------------\n");
    printf("PASS %u  FAIL %u  TIMEOUT %u  CRASH %u  ABORTED %u  SKIPPED %u  NOT RUN %u\n",
           cnt[DIAG_ST_PASS], cnt[DIAG_ST_FAIL], cnt[DIAG_ST_TIMEOUT],
           cnt[DIAG_ST_CRASH], cnt[DIAG_ST_ABORTED], cnt[DIAG_ST_SKIPPED],
           cnt[DIAG_ST_NOT_RUN]);

    return cnt[DIAG_ST_FAIL] + cnt[DIAG_ST_TIMEOUT] + cnt[DIAG_ST_CRASH] +
           cnt[DIAG_ST_ABORTED];
}

int diag_clear(void)
{
    diag_rec_reset(diag_rec());
    puts("ax_diag: result record cleared\n");
    return 0;
}

int diag_log(char *fmt, ...)
{
    return 0;
}
