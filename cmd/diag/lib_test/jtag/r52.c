// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * Cortex-R52 (ARMv8-R, AArch32) core 0 over the Host JTAG DAP.
 * Debug + CTI behind the SVM APB-AP 0x1C000 (svm_r52.jlinkscript);
 * memory (md/mw/load) goes through the AXI MEM-AP 0x4000.
 * Halt/run/step/regs are the shared ARMv8 external-debug code (armv8_dbg.c).
 */

#include <linux/types.h>

#include "armv8_dbg.h"
#include "jtag_dap.h"
#include "jtag_target.h"

#define JTAG_R52_AP		0x1C000U
#define R52_DBG_BASE		0x810000U
#define R52_CTI_BASE		0x820000U

/* Core 0 only for now; other cores = new descriptors + targets */
static const struct armv8_core r52_core0 = {
	.name		= "R52 core0",
	.ap		= JTAG_R52_AP,
	.dbg		= R52_DBG_BASE,
	.cti		= R52_CTI_BASE,
	.aarch64	= false,
};

static int r52_probe(void)
{
	return armv8_probe(&r52_core0);
}

static int r52_halt(void)
{
	return armv8_halt(&r52_core0);
}

static int r52_run(void)
{
	return armv8_run(&r52_core0);
}

static int r52_step(void)
{
	return armv8_step(&r52_core0);
}

static int r52_state(void)
{
	return armv8_state(&r52_core0);
}

static int r52_regs(void)
{
	return armv8_regs(&r52_core0);
}

static int r52_reg_read(u32 n, u64 *v)
{
	return armv8_reg_read(&r52_core0, n, v);
}

static int r52_reg_write(u32 n, u64 v)
{
	return armv8_reg_write(&r52_core0, n, v);
}

/* md/mw/load use the AXI MEM-AP; writes (image loads) need the core halted */
static int r52_mem_check(bool write)
{
	int ret;

	if (write) {
		ret = armv8_require_halt(&r52_core0);
		if (ret)
			return ret;
	}
	return mem_ap_open(JTAG_AXI_MEM_AP);
}

const struct ax_jtag_target ax_jtag_target_r52 = {
	.name		= "r52",
	.desc		= "Cortex-R52 core0 via APB-AP 0x1C000",
	.ap		= JTAG_R52_AP,
	.mem_ap		= JTAG_AXI_MEM_AP,
	.reg64		= false,
	.probe		= r52_probe,
	.halt		= r52_halt,
	.run		= r52_run,
	.step		= r52_step,
	.state		= r52_state,
	.regs		= r52_regs,
	.reg_read	= r52_reg_read,
	.reg_write	= r52_reg_write,
	.mem_check	= r52_mem_check,
};
