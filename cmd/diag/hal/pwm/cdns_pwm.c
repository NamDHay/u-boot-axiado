// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <stdbool.h>

#include "ax_diag.h"
#include "ax_pwm.h"
#include "ax_pinmux.h"

#define CONFIG_MAX_PWM  16

static const ulong pwm_base[CONFIG_MAX_PWM] = {
	AX3000_CSR_BASE_ADRS_PWM_0, AX3000_CSR_BASE_ADRS_PWM_1,
	AX3000_CSR_BASE_ADRS_PWM_2, AX3000_CSR_BASE_ADRS_PWM_3,
	AX3000_CSR_BASE_ADRS_PWM_4, AX3000_CSR_BASE_ADRS_PWM_5,
	AX3000_CSR_BASE_ADRS_PWM_6, AX3000_CSR_BASE_ADRS_PWM_7,
	AX3000_CSR_BASE_ADRS_PWM_8, AX3000_CSR_BASE_ADRS_PWM_9,
	AX3000_CSR_BASE_ADRS_PWM_10, AX3000_CSR_BASE_ADRS_PWM_11,
	AX3000_CSR_BASE_ADRS_PWM_12, AX3000_CSR_BASE_ADRS_PWM_13,
	AX3000_CSR_BASE_ADRS_PWM_14, AX3000_CSR_BASE_ADRS_PWM_15,
};

/* PWM Register offsets */
#define AX_PWM_CNTRL     0x0000
#define AX_PWM_PERIOD    0x0004
#define AX_PWM_HIGH      0x0008

/* PWM Channels */
#define AX_PWM_NUM 0x10

/* Period and Dutycycle Range */
#define PERIOD_MIN_VAL (-2)
#define PERIOD_MAX_VAL 0xFFFFFFFE
#define DUTYCYCLE_MIN_VAL (-1)
#define DUTYCYCLE_MAX_VAL 0xFFFFFFFD

/* Control Register Bits */
#define PWM_CTRL_ENABLE 0x1
#define PWM_CTRL_DISABLE 0x0

#define CONFIG_PWM_CLK  100000000

static int cdns_pwm_set_config(uint channel, uint period_ns,
        uint duty_ns) {
    char name[5];
    void __iomem *base = (void __iomem *)pwm_base[channel];

    sprintf(name, "PWM%d", channel);
    ax_pinmux_set_state(name);

    printf("axiado: Config %s %d/%d\n", name, duty_ns, period_ns);
    if ((period_ns <= PERIOD_MIN_VAL) || /* Checking period minimum value */
	    (period_ns >= PERIOD_MAX_VAL) || /* Checking period maximum value */
	    (duty_ns <=
	     DUTYCYCLE_MIN_VAL) || /* Checking dutycycle minimum value */
	    (duty_ns >=
	     DUTYCYCLE_MAX_VAL)) { /* Checking dutycyle maximum value */
		printf("axiado: PWM: Invalid period or dutycycle values\n");
		return -EINVAL;
	}

	/* Setting period */
	writel(period_ns, (base + AX_PWM_PERIOD));
	/* Setting duty cycle */
	writel(duty_ns, (base + AX_PWM_HIGH));

    return 0;
}

static int cdns_pwm_set_enable(uint channel, bool enable) {
    void __iomem *base = (void __iomem *)pwm_base[channel];

	printf("axiado: PWM: Enabling PWM\n");
    if (enable)
        writel(PWM_CTRL_ENABLE, (base + AX_PWM_CNTRL));
    else
        writel(PWM_CTRL_DISABLE, (base + AX_PWM_CNTRL));

    return 0;
}

struct ax_pwm_ops pwm = {
    .set_config = cdns_pwm_set_config,
    .set_enable = cdns_pwm_set_enable,
};
