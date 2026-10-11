// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <div64.h>
#include <irq_func.h>

#include "ax_diag.h"
#include "ax_wdt.h"

#define A53_CORE0_WDT_BASE_ADDR 0x48014200
#define A53_CORE1_WDT_BASE_ADDR 0x48014240
#define A53_CORE2_WDT_BASE_ADDR 0x48014280
#define A53_CORE3_WDT_BASE_ADDR 0x480142C0

#define A53_CORE0_WDT_IRQ       223 
#define A53_CORE1_WDT_IRQ       224 
#define A53_CORE2_WDT_IRQ       225 
#define A53_CORE3_WDT_IRQ       226 

#define CONFIG_WDT_CLK          100000000
#define TIME_PER_TICK 10 /* 10ns */

#define AX_WDT_ENA_REG          0x0
#define AX_WDT_SEED_REG	        0x4
#define AX_WDT_RESTART_REG      0x8
#define AX_WDT_STATUS_REG       0xC

#define AX_WDT_SEED				 0x1FFFF
#define AX_MAX_WDT_SEED			 0x3FFFFF
#define MAX_SEED_VAL	 AX_MAX_WDT_SEED
#define DEFAULT_SEED_VAL AX_WDT_SEED

struct wdt_config_t {
    void __iomem *base;
    uint32_t irq_num;
    bool rst;
};

static struct wdt_config_t wdt_config[CONFIG_NR_CPUS] = {
    { .base = (void __iomem *)A53_CORE0_WDT_BASE_ADDR, .irq_num = A53_CORE0_WDT_IRQ, .rst = false },
    { .base = (void __iomem *)A53_CORE1_WDT_BASE_ADDR, .irq_num = A53_CORE1_WDT_IRQ, .rst = false },
    { .base = (void __iomem *)A53_CORE2_WDT_BASE_ADDR, .irq_num = A53_CORE2_WDT_IRQ, .rst = false },
    { .base = (void __iomem *)A53_CORE3_WDT_BASE_ADDR, .irq_num = A53_CORE3_WDT_IRQ, .rst = false },
};

static int axiado_wdt_start(u64 timeout);
static int axiado_wdt_stop(void);
static int axiado_wdt_reset(void);

#define MPIDR_AFF0_MASK     0xff
static uint32_t aarch64_get_coreid(void)
{
	u64 mpidr;

	asm volatile(
		"mrs %0, mpidr_el1"
		: "=r"(mpidr));

	return mpidr & MPIDR_AFF0_MASK;
}

void axiado_wdt_interrupt_handler(void *arg)
{
    struct wdt_config_t *wdt = arg;

    if (!wdt)
        return;

    printf("WDT Interrupt Trigger\n");
    if (wdt->rst)
        axiado_wdt_reset();
}

/**
 * axiado_wdt_start - Enable and start the watchdog.
 *
 * @timeout: Timeout value
 *
 * Return: Upon success 0, failure -1.
 */
static int axiado_wdt_start(u64 timeout)
{
    uint32_t cpu = aarch64_get_coreid();
    struct wdt_config_t *wdt = &wdt_config[cpu];
    u64 count;

    irq_install_handler(IRQ_VEC(wdt->irq_num, IRQ_TRIGGER_EDGE), 
            axiado_wdt_interrupt_handler, &wdt);

    /* Make sure the WDT is disabled before configuring. */
    writel(0x0, wdt->base + AX_WDT_ENA_REG);

    count = (timeout * 1000000) / TIME_PER_TICK ; /* Timeout value in ns */
    count = count >> 12;

    if (count > AX_MAX_WDT_SEED)
        count = AX_MAX_WDT_SEED;

    printf("%s: timeout %lldms\n", __func__, timeout);
    writel(count, wdt->base + AX_WDT_SEED_REG);

	/* Set the Enable Bit for WDT */
    writel(0x1, wdt->base + AX_WDT_ENA_REG);

    /* Need to do this, else the WDT will not start */
    writel(0x1, wdt->base + AX_WDT_RESTART_REG);

    return 0;
}

/**
 * axiado_wdt_stop - Stop the watchdog.
 *
 * @dev: Watchdog device
 *
 * Read the contents of the Enable register, clear the WDEN bit in the register
 *
 * Return: Always 0
 */
static int axiado_wdt_stop(void)
{
    uint32_t cpu = aarch64_get_coreid();
    struct wdt_config_t *wdt = &wdt_config[cpu];

    writel(0x0, wdt->base + AX_WDT_ENA_REG);

    return 0;
}

/**
 * axiado_wdt_reset - Reload the watchdog timer (i.e. pat the watchdog).
 *
 * Write the restart bit to the restart register.
 *
 * Return: Always 0
 */
static int axiado_wdt_reset(void)
{
    uint32_t cpu = aarch64_get_coreid();
    struct wdt_config_t *wdt = &wdt_config[cpu];

    writel(0x1, wdt->base + AX_WDT_RESTART_REG);

    return 0;
}

struct ax_wdt_ops wdt = {
    .start = axiado_wdt_start,
    .stop = axiado_wdt_stop,
    .reset = axiado_wdt_reset,
};
