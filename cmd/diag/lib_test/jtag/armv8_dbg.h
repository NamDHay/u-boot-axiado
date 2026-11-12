/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * ARMv8 external debug (ARMv8-A A53, ARMv8-R R52) over the Host JTAG DAP.
 * Debug registers are reached through an APB-AP: EDSCR/EDITR/DTR for
 * instruction execution in Debug state, CTI for halt/restart.
 * Each core is described by one struct armv8_core; r52.c / a53.c only
 * fill in descriptors and wrap these helpers into ax_jtag_target ops.
 */

#ifndef __AX_ARMV8_DBG_H
#define __AX_ARMV8_DBG_H

#include <linux/types.h>

struct armv8_core {
	const char *name;	/* for messages, e.g. "R52 core0" */
	u32 ap;			/* APB-AP holding the debug + CTI blocks */
	u32 dbg;		/* external debug register base */
	u32 cti;		/* CTI base for this core */
	bool aarch64;		/* core can be in AArch64 (A53); R52 is AArch32 only */
};

/* Register numbers used by reg_read/reg_write (depend on execution state) */
#define ARMV8_A32_PC		15U	/* DLR */
#define ARMV8_A32_CPSR		16U	/* DSPSR */
#define ARMV8_A32_REG_MAX	16U

#define ARMV8_A64_SP		31U
#define ARMV8_A64_PC		32U	/* DLR_EL0 */
#define ARMV8_A64_PSTATE	33U	/* DSPSR_EL0 */
#define ARMV8_A64_REG_MAX	33U

int armv8_probe(const struct armv8_core *c);
int armv8_halt(const struct armv8_core *c);
int armv8_run(const struct armv8_core *c);
int armv8_step(const struct armv8_core *c);
int armv8_state(const struct armv8_core *c);
int armv8_regs(const struct armv8_core *c);
int armv8_reg_read(const struct armv8_core *c, u32 n, u64 *v);
int armv8_reg_write(const struct armv8_core *c, u32 n, u64 v);
int armv8_require_halt(const struct armv8_core *c);

#endif /* __AX_ARMV8_DBG_H */
