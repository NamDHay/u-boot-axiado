// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * Cortex-M0 (ARMv6-M) debug target over the Host JTAG DAP.
 * The M0 sits behind AHB-AP 0x6000; all debug goes through the
 * System Control Space debug registers (DHCSR/DCRSR/DCRDR/DEMCR).
 */

#include <stdio.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/types.h>

#include "jtag_dap.h"
#include "jtag_target.h"

#define JTAG_M0_AP		0x6000U

/* ARMv6-M System Control Space */
#define M0_CPUID		0xE000ED00U
#define M0_AIRCR		0xE000ED0CU
#define M0_DFSR			0xE000ED30U
#define M0_DHCSR		0xE000EDF0U
#define M0_DCRSR		0xE000EDF4U
#define M0_DCRDR		0xE000EDF8U
#define M0_DEMCR		0xE000EDFCU

/* DHCSR */
#define M0_DBGKEY		0xA05F0000U
#define M0_C_DEBUGEN		BIT(0)
#define M0_C_HALT		BIT(1)
#define M0_C_STEP		BIT(2)
#define M0_C_MASKINTS		BIT(3)
#define M0_S_REGRDY		BIT(16)
#define M0_S_HALT		BIT(17)
#define M0_S_SLEEP		BIT(18)
#define M0_S_LOCKUP		BIT(19)
#define M0_S_RETIRE_ST		BIT(24)
#define M0_S_RESET_ST		BIT(25)

/* DCRSR */
#define M0_DCRSR_REGWNR		BIT(16)
#define M0_REG_MAX		20U

/* DEMCR / AIRCR */
#define M0_DEMCR_VC_CORERESET	BIT(0)
#define M0_DEMCR_DWTENA		BIT(24)
#define M0_AIRCR_SYSRESETREQ	0x05FA0004U

#define M0_POLL			100

static const char *const m0_reg_name[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc",
	"xpsr", "msp", "psp", NULL, "ctrl/primask",
};

static int m0_rd(u32 addr, u32 *v)
{
	return mem_read32(JTAG_M0_AP, addr, v);
}

static int m0_wr(u32 addr, u32 v)
{
	return mem_write32(JTAG_M0_AP, addr, v);
}

static int m0_open(void)
{
	return mem_ap_open(JTAG_M0_AP);
}

/* Poll DHCSR until (DHCSR & mask) == want; *v gets the last value */
static int m0_wait_dhcsr(u32 mask, u32 want, u32 *v)
{
	int i, ret = -ETIMEDOUT;

	for (i = 0; i < M0_POLL; i++) {
		ret = m0_rd(M0_DHCSR, v);
		if (!ret && (*v & mask) == want)
			return 0;
	}
	return ret ? ret : -ETIMEDOUT;
}

static int m0_require_halt(void)
{
	u32 v;
	int ret;

	ret = m0_rd(M0_DHCSR, &v);
	if (ret)
		return ret;
	if (!(v & M0_S_HALT)) {
		printf("M0 not halted (DHCSR 0x%08x) - run 'ax_jtag m0 halt' first\n", v);
		return -EBUSY;
	}
	return 0;
}

static int m0_probe(void)
{
	u32 cpuid, dhcsr;
	int ret;

	ret = m0_open();
	if (ret)
		return ret;

	ret = m0_rd(M0_CPUID, &cpuid);
	if (!ret)
		ret = m0_rd(M0_DHCSR, &dhcsr);
	if (ret) {
		printf("M0 SCS read failed (ack %u)\n", dap_last_ack());
		return ret;
	}

	printf("M0 CPUID   0x%08x (impl 0x%02x part 0x%03x r%up%u)%s\n", cpuid,
	       cpuid >> 24, (cpuid >> 4) & 0xFFF, (cpuid >> 20) & 0xF, cpuid & 0xF,
	       ((cpuid >> 4) & 0xFFF) == 0xC20 ? " Cortex-M0" :
	       ((cpuid >> 4) & 0xFFF) == 0xC60 ? " Cortex-M0+" : "");
	printf("M0 DHCSR   0x%08x\n", dhcsr);
	return 0;
}

static int m0_halt(void)
{
	u32 v = 0;
	int ret;

	ret = m0_open();
	if (!ret)
		ret = m0_wr(M0_DHCSR, M0_DBGKEY | M0_C_HALT | M0_C_DEBUGEN);
	if (!ret)
		ret = m0_wait_dhcsr(M0_S_HALT, M0_S_HALT, &v);

	printf("M0 halt %s (DHCSR 0x%08x)\n", ret ? "FAILED" : "OK", v);
	return ret;
}

static int m0_run(void)
{
	u32 v = 0;
	int ret;

	ret = m0_open();
	if (!ret)
		ret = m0_wr(M0_DHCSR, M0_DBGKEY | M0_C_DEBUGEN);
	if (!ret)
		ret = m0_wait_dhcsr(M0_S_HALT, 0, &v);

	printf("M0 run %s (DHCSR 0x%08x)\n", ret ? "FAILED" : "OK", v);
	return ret;
}

static int m0_step(void)
{
	u32 v = 0, pc = 0;
	int ret;

	ret = m0_open();
	if (!ret)
		ret = m0_require_halt();
	if (ret)
		return ret;

	/* Single step with interrupts masked, then wait for the re-halt */
	ret = m0_wr(M0_DHCSR, M0_DBGKEY | M0_C_STEP | M0_C_MASKINTS | M0_C_DEBUGEN);
	if (!ret)
		ret = m0_wait_dhcsr(M0_S_HALT, M0_S_HALT, &v);
	if (!ret)
		ret = mem_write32(JTAG_M0_AP, M0_DCRSR, 15U);
	if (!ret)
		ret = m0_wait_dhcsr(M0_S_REGRDY, M0_S_REGRDY, &v);
	if (!ret)
		ret = m0_rd(M0_DCRDR, &pc);

	printf("M0 step %s (DHCSR 0x%08x PC 0x%08x)\n", ret ? "FAILED" : "OK", v, pc);
	return ret;
}

static int m0_state(void)
{
	u32 dhcsr, dfsr, demcr;
	int ret;

	ret = m0_open();
	if (!ret)
		ret = m0_rd(M0_DHCSR, &dhcsr);
	if (!ret)
		ret = m0_rd(M0_DFSR, &dfsr);
	if (!ret)
		ret = m0_rd(M0_DEMCR, &demcr);
	if (ret) {
		printf("M0 state read failed (ack %u)\n", dap_last_ack());
		return ret;
	}

	printf("DHCSR 0x%08x: %s%s%s%s%s%s\n", dhcsr,
	       dhcsr & M0_S_HALT ? "HALTED" : "RUNNING",
	       dhcsr & M0_C_DEBUGEN ? " DEBUGEN" : "",
	       dhcsr & M0_S_SLEEP ? " SLEEP" : "",
	       dhcsr & M0_S_LOCKUP ? " LOCKUP" : "",
	       dhcsr & M0_S_RETIRE_ST ? " RETIRED" : "",
	       dhcsr & M0_S_RESET_ST ? " RESET_SEEN" : "");
	printf("DFSR  0x%08x: %s%s%s%s%s\n", dfsr,
	       dfsr & BIT(0) ? " HALTED" : "", dfsr & BIT(1) ? " BKPT" : "",
	       dfsr & BIT(2) ? " DWTTRAP" : "", dfsr & BIT(3) ? " VCATCH" : "",
	       dfsr & BIT(4) ? " EXTERNAL" : "");
	printf("DEMCR 0x%08x\n", demcr);
	return 0;
}

static int m0_reg_read(u32 n, u64 *v)
{
	u32 d, r;
	int ret;

	if (n > M0_REG_MAX || !m0_reg_name[n])
		return -EINVAL;

	ret = m0_open();
	if (!ret)
		ret = m0_require_halt();
	if (!ret)
		ret = m0_wr(M0_DCRSR, n);
	if (!ret)
		ret = m0_wait_dhcsr(M0_S_REGRDY, M0_S_REGRDY, &d);
	if (!ret)
		ret = m0_rd(M0_DCRDR, &r);
	if (!ret)
		*v = r;
	return ret;
}

static int m0_reg_write(u32 n, u64 val)
{
	u32 d;
	int ret;

	if (n > M0_REG_MAX || !m0_reg_name[n])
		return -EINVAL;

	ret = m0_open();
	if (!ret)
		ret = m0_require_halt();
	if (!ret)
		ret = m0_wr(M0_DCRDR, (u32)val);
	if (!ret)
		ret = m0_wr(M0_DCRSR, n | M0_DCRSR_REGWNR);
	if (!ret)
		ret = m0_wait_dhcsr(M0_S_REGRDY, M0_S_REGRDY, &d);
	return ret;
}

static int m0_regs(void)
{
	u32 n;
	u64 v;
	int ret;

	ret = m0_open();
	if (!ret)
		ret = m0_require_halt();
	if (ret)
		return ret;

	for (n = 0; n <= M0_REG_MAX; n++) {
		if (!m0_reg_name[n])
			continue;
		ret = m0_reg_read(n, &v);
		if (ret) {
			printf("%-12s read failed (%d)\n", m0_reg_name[n], ret);
			return ret;
		}
		printf("%-12s 0x%08x\n", m0_reg_name[n], (u32)v);
	}
	return 0;
}

static int m0_reset(bool halt)
{
	u32 demcr, v = 0;
	int ret;

	ret = m0_open();
	if (!ret)
		ret = m0_rd(M0_DEMCR, &demcr);
	if (ret)
		return ret;

	if (halt) {
		/* Vector catch on core reset needs C_DEBUGEN set */
		ret = m0_wr(M0_DHCSR, M0_DBGKEY | M0_C_DEBUGEN);
		if (!ret)
			ret = m0_wr(M0_DEMCR, demcr | M0_DEMCR_VC_CORERESET);
	} else {
		ret = m0_wr(M0_DEMCR, demcr & ~M0_DEMCR_VC_CORERESET);
	}
	if (ret)
		return ret;

	/*
	 * SYSRESETREQ: the AP write may not get an OK ACK if the reset
	 * hits the debug logic, so do not treat that as fatal.
	 */
	if (m0_wr(M0_AIRCR, M0_AIRCR_SYSRESETREQ))
		printf("AIRCR write ack %u (may be expected during reset)\n", dap_last_ack());

	if (halt) {
		ret = m0_wait_dhcsr(M0_S_HALT, M0_S_HALT, &v);
		printf("M0 reset-halt %s (DHCSR 0x%08x)\n", ret ? "FAILED" : "OK", v);
		/* Drop the vector catch so the next reset runs normally */
		if (!ret)
			ret = m0_wr(M0_DEMCR, demcr & ~M0_DEMCR_VC_CORERESET);
	} else {
		printf("M0 reset requested\n");
	}
	return ret;
}

/* Reference flow: memory writes into the M0 side need the M0 halted. */
static int m0_mem_check(bool write)
{
	int ret = m0_open();

	if (ret || !write)
		return ret;
	return m0_require_halt();
}

const struct ax_jtag_target ax_jtag_target_m0 = {
	.name		= "m0",
	.desc		= "Cortex-M0 via AHB-AP 0x6000",
	.ap		= JTAG_M0_AP,
	.probe		= m0_probe,
	.halt		= m0_halt,
	.run		= m0_run,
	.step		= m0_step,
	.state		= m0_state,
	.regs		= m0_regs,
	.reg_read	= m0_reg_read,
	.reg_write	= m0_reg_write,
	.reset		= m0_reset,
	.mem_check	= m0_mem_check,
};
