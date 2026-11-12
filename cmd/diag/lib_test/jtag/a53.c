// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * Cortex-A53 (ARMv8-A) core 0 over the Host JTAG DAP.
 * Debug + CTI behind the PERIPH APB-AP 0x2000 (a53_core_N.jlinkscript:
 * core N debug 0x810000 + N * 0x100000, CTI 0x820000 + N * 0x100000);
 * memory (md/mw/load) goes through the AXI MEM-AP 0x4000.
 * Halt/run/step/regs are the shared ARMv8 external-debug code (armv8_dbg.c).
 */

#include <linux/types.h>

#include "armv8_dbg.h"
#include "jtag_dap.h"
#include "jtag_target.h"

#define JTAG_A53_AP		0x2000U
#define A53_DBG_BASE		0x810000U
#define A53_CTI_BASE		0x820000U

/* Core 0 only for now; other cores = new descriptors + targets */
static const struct armv8_core a53_core0 = {
	.name		= "A53 core0",
	.ap		= JTAG_A53_AP,
	.dbg		= A53_DBG_BASE,
	.cti		= A53_CTI_BASE,
	.aarch64	= true,
};

static int a53_probe(void)
{
	return armv8_probe(&a53_core0);
}

static int a53_halt(void)
{
	return armv8_halt(&a53_core0);
}

static int a53_run(void)
{
	return armv8_run(&a53_core0);
}

static int a53_step(void)
{
	return armv8_step(&a53_core0);
}

static int a53_state(void)
{
	return armv8_state(&a53_core0);
}

static int a53_regs(void)
{
	return armv8_regs(&a53_core0);
}

static int a53_reg_read(u32 n, u64 *v)
{
	return armv8_reg_read(&a53_core0, n, v);
}

static int a53_reg_write(u32 n, u64 v)
{
	return armv8_reg_write(&a53_core0, n, v);
}

/* md/mw/load use the AXI MEM-AP; writes (image loads) need the core halted */
static int a53_mem_check(bool write)
{
	int ret;

	if (write) {
		ret = armv8_require_halt(&a53_core0);
		if (ret)
			return ret;
	}
	return mem_ap_open(JTAG_AXI_MEM_AP);
}

const struct ax_jtag_target ax_jtag_target_a53 = {
	.name		= "a53",
	.desc		= "Cortex-A53 core0 via APB-AP 0x2000",
	.ap		= JTAG_A53_AP,
	.mem_ap		= JTAG_AXI_MEM_AP,
	.reg64		= true,
	.probe		= a53_probe,
	.halt		= a53_halt,
	.run		= a53_run,
	.step		= a53_step,
	.state		= a53_state,
	.regs		= a53_regs,
	.reg_read	= a53_reg_read,
	.reg_write	= a53_reg_write,
	.mem_check	= a53_mem_check,
};
