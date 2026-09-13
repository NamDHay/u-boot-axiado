// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __DIAG_H
#define __DIAG_H

#ifdef CONFIG_TARGET_SCM3005
#include "hal/base/Ultra/ax3000_base_adrs.h"
#include "hal/base/Ultra/ax3000_slo_base_adrs.h"
#define CONFIG_NR_CPUS  4
#elif CONFIG_TARGET_SCM3000
#include "hal/base/A0/ax3000_base_adrs.h"
#include "hal/base/A0/ax3000_slo_base_adrs.h"
#define CONFIG_NR_CPUS  4
#elif CONFIG_TARGET_SCM1100
#include "hal/base/mK/ax3000_base_adrs.h"
#include "hal/base/mK/ax3000_slo_base_adrs.h"
#define CONFIG_NR_CPUS  1
#endif

#define DIAG_MAX_TEST 32

int diag_info(char *name);
int diag_run(char *name, unsigned long testid);
int diag_log(char *fmt, ...);

struct diag_test {
    const char *name;
    const char *cmd;
    const char *desc;
    const char *testcase[DIAG_MAX_TEST];
};

#endif /* __DIAG_H */
