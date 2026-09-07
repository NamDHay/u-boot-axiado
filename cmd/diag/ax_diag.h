// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __DIAG_H
#define __DIAG_H

#ifdef CONFIG_TARGET_SCM3005
#include "hal/base/Ultra/ax3000_base_adrs.h"
#include "hal/base/Ultra/ax3000_slo_base_adrs.h"
#elif CONFIG_TARGET_SCM3000
#elif CONFIG_TARGET_SCM1100
#endif

#define DIAG_MAX_TEST 32

int diag_info(char *name);
int diag_run(char *name, unsigned long testid);
int diag_log(char *fmt, ...);

struct diag_test {
    char *name;
    char *cmd;
    char *desc;
    int (*init) (void);
    int (*test) (unsigned long testid);
    void (*stat) (void);
};

#endif /* __DIAG_H */
