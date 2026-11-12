// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __DIAG_H
#define __DIAG_H

#ifdef CONFIG_AXIADO_AX3005
#include "hal/base/Ultra/ax3000_base_adrs.h"
#include "hal/base/Ultra/ax3000_slo_base_adrs.h"
#define CONFIG_NR_CPUS  4
#elif defined(CONFIG_AXIADO_AX3000) || defined(CONFIG_AXIADO_MK)
/* mk = AX3000 silicon with U-Boot in OCM: same (A0) register map */
#include "hal/base/A0/ax3000_base_adrs.h"
#include "hal/base/A0/ax3000_slo_base_adrs.h"
#define CONFIG_NR_CPUS  4
#endif

#define DIAG_MAX_TEST   32
#define DIAG_MAX_GROUP  32

/*
 * Status of a single testcase.
 *
 * DIAG_ST_RUNNING is written (and flushed to DRAM) right before the command
 * is executed. If U-Boot crashes or hangs and the board is reset, the entry
 * stays RUNNING in the persistent record and is reported as DIAG_ST_CRASH
 * by the next 'ax_diag status' / 'ax_diag run'.
 */
enum diag_status {
    DIAG_ST_NOT_RUN = 0,
    DIAG_ST_RUNNING,
    DIAG_ST_PASS,
    DIAG_ST_FAIL,       /* command returned non-zero */
    DIAG_ST_TIMEOUT,    /* command returned but took longer than timeout */
    DIAG_ST_SKIPPED,    /* not run (currently unused: a failure only skips
                         * the remaining sub-commands of its own testcase) */
    DIAG_ST_CRASH,      /* board reset while the test was running */
    DIAG_ST_ABORTED,    /* stopped by the user with Ctrl+C */
    DIAG_ST_COUNT,
};

int diag_info(char *name);
int diag_run(char *name, unsigned long testid);
void diag_run_end(void);
int diag_status(void);
int diag_clear(void);
int diag_log(char *fmt, ...);

struct testcase {
    const char *cmd;    /* one or more commands separated by ';' */
    int timeout;        /* ms, 0 = no limit */
    int status;         /* enum diag_status */
    int ret;            /* return code of the failing sub-command */
    int step;           /* index of the ';' sub-command last executed */
    unsigned long elapsed; /* ms */
};

struct diag_test {
    const char *name;
    const char *desc;
    int test_num;
    struct testcase test[DIAG_MAX_TEST];
};

#endif /* __DIAG_H */
