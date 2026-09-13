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

#define CDNS_WDT_ZMR_REG        0x0
#define CDNS_WDT_CCR_REG	    0x4
#define CDNS_WDT_RESTART_REG    0x8
#define CDNS_WDT_STATUS_REG     0xC

#define CDNS_WDT_DEFAULT_TIMEOUT	10

/* Supports 1 - 516 sec */
#define CDNS_WDT_MIN_TIMEOUT		1
#define CDNS_WDT_MAX_TIMEOUT		516

/* Restart key */
#define CDNS_WDT_RESTART_KEY		0x00001999

/* Counter register access key */
#define CDNS_WDT_REGISTER_ACCESS_KEY	0x00920000

/* Counter value divisor */
#define CDNS_WDT_COUNTER_VALUE_DIVISOR	0x1000

/* Clock prescaler value and selection */
#define CDNS_WDT_PRESCALE_64		64
#define CDNS_WDT_PRESCALE_512		512
#define CDNS_WDT_PRESCALE_4096		4096
#define CDNS_WDT_PRESCALE_SELECT_64	1
#define CDNS_WDT_PRESCALE_SELECT_512	2
#define CDNS_WDT_PRESCALE_SELECT_4096	3

/* Input clock frequency */
#define CDNS_WDT_CLK_75MHZ	75000000

/* Counter maximum value */
#define CDNS_WDT_COUNTER_MAX	0xFFF

/*********************    Register Map    **********************************/

/*
 * Zero Mode Register - This register controls how the time out is indicated
 * and also contains the access code to allow writes to the register (0xABC).
 */
#define CDNS_WDT_ZMR_WDEN_MASK	0x00000001 /* Enable the WDT */
#define CDNS_WDT_ZMR_RSTEN_MASK	0x00000002 /* Enable the reset output */
#define CDNS_WDT_ZMR_IRQEN_MASK	0x00000004 /* Enable IRQ output */
#define CDNS_WDT_ZMR_RSTLEN_16	0x00000030 /* Reset pulse of 16 pclk cycles */
#define CDNS_WDT_ZMR_ZKEY_VAL	0x00ABC000 /* Access key, 0xABC << 12 */

/*
 * Counter Control register - This register controls how fast the timer runs
 * and the reset value and also contains the access code to allow writes to
 * the register.
 */
#define CDNS_WDT_CCR_CRV_MASK	0x00003FFC /* Counter reset value */

struct wdt_config_t {
    void __iomem *base;
    uint32_t irq_num;
    bool rst;
};

static struct wdt_config_t wdt_config[CONFIG_NR_CPUS] = {
    { .base = (void __iomem *)A53_CORE0_WDT_BASE_ADDR, .irq_num = A53_CORE0_WDT_IRQ, .rst = true },
    { .base = (void __iomem *)A53_CORE1_WDT_BASE_ADDR, .irq_num = A53_CORE1_WDT_IRQ, .rst = true },
    { .base = (void __iomem *)A53_CORE2_WDT_BASE_ADDR, .irq_num = A53_CORE2_WDT_IRQ, .rst = true },
    { .base = (void __iomem *)A53_CORE3_WDT_BASE_ADDR, .irq_num = A53_CORE3_WDT_IRQ, .rst = true },
};

static int cdns_wdt_start(u64 timeout);
static int cdns_wdt_stop(void);
static int cdns_wdt_reset(void);

#define MPIDR_AFF0_MASK     0xff
static uint32_t aarch64_get_coreid(void)
{
	u64 mpidr;

	asm volatile(
		"mrs %0, mpidr_el1"
		: "=r"(mpidr));

	return mpidr & MPIDR_AFF0_MASK;
}

void cdns_wdt_interrupt_handler(void *arg)
{
    struct wdt_config *wdt = arg;

    if (!wdt)
        return;

    printf("WDT Interrupt Trigger\n");
}

/**
 * cdns_wdt_start - Enable and start the watchdog.
 *
 * @timeout: Timeout value
 *
 * The counter value is calculated according to the formula:
 *		count = (timeout * clock) / prescaler + 1.
 *
 * The calculated count is divided by 0x1000 to obtain the field value
 * to write to counter control register.
 *
 * Clears the contents of prescaler and counter reset value. Sets the
 * prescaler to 4096 and the calculated count and access key
 * to write to CCR Register.
 *
 * Sets the WDT (WDEN bit) and either the Reset signal(RSTEN bit)
 * or Interrupt signal(IRQEN) with a specified cycles and the access
 * key to write to ZMR Register.
 *
 * Return: Upon success 0, failure -1.
 */
static int cdns_wdt_start(u64 timeout)
{
    uint32_t cpu = aarch64_get_coreid();
    struct wdt_config_t *wdt = &wdt_config[cpu];
	u32 count, prescaler, ctrl_clksel, data = 0;
	ulong clk_f;

    irq_install_handler(IRQ_VEC(wdt->irq_num, IRQ_TRIGGER_EDGE), 
            cdns_wdt_interrupt_handler, &wdt);

    clk_f = CONFIG_WDT_CLK;
	/* Calculate timeout in seconds and restrict to min and max value */
	do_div(timeout, 1000);
	timeout = max_t(u64, timeout, CDNS_WDT_MIN_TIMEOUT);
	timeout = min_t(u64, timeout, CDNS_WDT_MAX_TIMEOUT);

	printf("%s: CLK_FREQ %ld, timeout %lld\n", __func__, clk_f, timeout);

	if (clk_f <= CDNS_WDT_CLK_75MHZ) {
		prescaler = CDNS_WDT_PRESCALE_512;
		ctrl_clksel = CDNS_WDT_PRESCALE_SELECT_512;
	} else {
		prescaler = CDNS_WDT_PRESCALE_4096;
		ctrl_clksel = CDNS_WDT_PRESCALE_SELECT_4096;
	}

	/*
	 * Counter value divisor to obtain the value of
	 * counter reset to be written to control register.
	 */
	count = (timeout * (clk_f / prescaler)) /
		CDNS_WDT_COUNTER_VALUE_DIVISOR + 1;

	if (count > CDNS_WDT_COUNTER_MAX)
		count = CDNS_WDT_COUNTER_MAX;

    writel(CDNS_WDT_ZMR_ZKEY_VAL, wdt->base + CDNS_WDT_ZMR_REG);

	count = (count << 2) & CDNS_WDT_CCR_CRV_MASK;

	/* Write counter access key first to be able write to register */
	data = count | CDNS_WDT_REGISTER_ACCESS_KEY | ctrl_clksel;
    writel(data, wdt->base + CDNS_WDT_CCR_REG);

	data = CDNS_WDT_ZMR_WDEN_MASK | CDNS_WDT_ZMR_RSTLEN_16 |
		CDNS_WDT_ZMR_ZKEY_VAL;

	/* Reset on timeout if specified in device tree. */
	if (wdt->rst) {
		data |= CDNS_WDT_ZMR_RSTEN_MASK;
		data &= ~CDNS_WDT_ZMR_IRQEN_MASK;
	} else {
		data &= ~CDNS_WDT_ZMR_RSTEN_MASK;
		data |= CDNS_WDT_ZMR_IRQEN_MASK;
	}

    writel(data, wdt->base + CDNS_WDT_ZMR_REG);
    writel(CDNS_WDT_RESTART_KEY, wdt->base + CDNS_WDT_RESTART_REG);

	return 0;
}

/**
 * cdns_wdt_stop - Stop the watchdog.
 *
 * @dev: Watchdog device
 *
 * Read the contents of the ZMR register, clear the WDEN bit in the register
 * and set the access key for successful write.
 *
 * Return: Always 0
 */
static int cdns_wdt_stop(void)
{
    uint32_t cpu = aarch64_get_coreid();
    struct wdt_config_t *wdt = &wdt_config[cpu];

    writel(CDNS_WDT_ZMR_ZKEY_VAL & (~CDNS_WDT_ZMR_WDEN_MASK), 
            wdt->base + CDNS_WDT_ZMR_REG);

    return 0;
}

/**
 * cdns_wdt_reset - Reload the watchdog timer (i.e. pat the watchdog).
 *
 * Write the restart key value (0x00001999) to the restart register.
 *
 * Return: Always 0
 */
static int cdns_wdt_reset(void)
{
    uint32_t cpu = aarch64_get_coreid();
    struct wdt_config_t *wdt = &wdt_config[cpu];

    writel(CDNS_WDT_RESTART_KEY, wdt->base + CDNS_WDT_RESTART_REG);

    return 0;
}

/**
 * cdns_wdt_expire_now - Expire the watchdog.
 *
 * Access WDT and configure with minimal counter value to expire ASAP.
 * Expiration issues system reset. When DEBUG is enabled count should be
 * bigger to at least see debug message.
 *
 * Return: Always 0
 */
static int cdns_wdt_expire_now(void)
{
    uint32_t cpu = aarch64_get_coreid();
    struct wdt_config_t *wdt = &wdt_config[cpu];
	u32 data, count = 0;

	count = 0x40; /* Increase the value if you need more time */
	printf("%s: Expire wdt\n", __func__);

    writel(CDNS_WDT_ZMR_ZKEY_VAL, wdt->base + CDNS_WDT_ZMR_REG);

	count = (count << 2) & CDNS_WDT_CCR_CRV_MASK;

	/* Write counter access key first to be able write to register */
	data = count | CDNS_WDT_REGISTER_ACCESS_KEY;
    writel(data, wdt->base + CDNS_WDT_CCR_REG);

	data = CDNS_WDT_ZMR_WDEN_MASK |  CDNS_WDT_ZMR_RSTEN_MASK |
		CDNS_WDT_ZMR_ZKEY_VAL;

    writel(data, wdt->base + CDNS_WDT_ZMR_REG);
    writel(CDNS_WDT_RESTART_KEY, wdt->base + CDNS_WDT_RESTART_REG);

	return 0;
}

struct ax_wdt_ops wdt = {
    .start = cdns_wdt_start,
    .stop = cdns_wdt_stop,
    .reset = cdns_wdt_reset,
	.expire_now = cdns_wdt_expire_now,
};
