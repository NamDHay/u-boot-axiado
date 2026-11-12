/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * Debug-target descriptor used by the ax_jtag command.  Each core that
 * can be debugged over the Host JTAG DAP (m0, r52, a53, a35, r4 ...)
 * provides one of these in its own file and is listed in ax_jtag_targets[]
 * in jtag_cli.c.  Any op may be NULL; the CLI then prints "not supported".
 */

#ifndef __AX_JTAG_TARGET_H
#define __AX_JTAG_TARGET_H

#include <linux/types.h>

struct ax_jtag_target {
	const char *name;	/* CLI name: "ax_jtag <name> ..." */
	const char *desc;
	u32 ap;			/* AP holding the core's debug registers */
	u32 mem_ap;		/* MEM-AP for md/mw/load/verify (0 = use ap) */
	bool reg64;		/* registers may be 64-bit (print as %016llx) */

	int (*probe)(void);			/* open AP, print IDs */
	int (*halt)(void);
	int (*run)(void);
	int (*step)(void);
	int (*state)(void);			/* print debug status */
	int (*regs)(void);			/* dump core registers */
	int (*reg_read)(u32 n, u64 *v);
	int (*reg_write)(u32 n, u64 v);
	int (*reset)(bool halt);		/* system reset, optionally halt at reset vector */
	int (*mem_check)(bool write);		/* optional gate before md/mw */
};

extern const struct ax_jtag_target ax_jtag_target_m0;
extern const struct ax_jtag_target ax_jtag_target_r52;
extern const struct ax_jtag_target ax_jtag_target_a53;

/* Target-side APs shared by several cores */
#define JTAG_AXI_MEM_AP		0x4000U	/* system memory (SRAM/DDR) */

#endif /* __AX_JTAG_TARGET_H */
