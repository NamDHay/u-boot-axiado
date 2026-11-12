/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2021-2026 Axiado Corporation (or its affiliates).
 *
 * mk: AX3000 (SCM3000 clone) U-Boot that runs entirely from the 1 MiB
 * A53 on-chip SRAM (OCM) at 0x80200000, with no relocation
 * (CONFIG_SKIP_RELOCATE). DDR is mapped but never used by U-Boot itself,
 * so the image also works before DDR is initialised.
 */

#ifndef __MK_H
#define __MK_H

#include <linux/sizes.h>

#define GICD_BASE		0x80300000
#define GICR_BASE		0x80380000

#define SYS_TIMER_CTRL   0x8A020000
#define SYS_TIMER_ENABLE	0x1
#define SYS_TIMER_DISABLE	0x0
#define CLKRST_CPU_PLL_POSTDIV_REG 0x8000000C
#define CLKRST_CPU_PLL_STS_ADRS_OFFSET 0x80000014
#define REG_CPU_PLL_POSTDIV_FLD_POSTDIV0_0_LSB 0
#define REG_CPU_PLL_POSTDIV_FLD_POSTDIV1_0_LSB 12
#define POST_DIV_BITS_MASK 0x7
#define CPU_PLL_STS_LOCK 0x1
#define AX_PLL_CLK_4000MHZ			4000000000

/*
 * A53 OCM (AX3000_MEM_BASE_ADRS_SRAM): 0x80200000 - 0x802FFFFF
 *
 *   0x80200000  U-Boot image (text/rodata/data, then rela.dyn + appended DTB)
 *               with .bss overlaying rela.dyn
 *      ...      final stack (grows down), fdt copy, gd, bd, malloc heap,
 *               MMU tables: placed by board_init_f() going down from ram_top
 *   0x802F0000  ram_top. Everything above it is never handed to U-Boot:
 *   0x802F0000  early (pre-board_init_r) stack + gd + malloc_f, 48 KiB
 *   0x802FC000  ax_diag persistent record (CONFIG_CMD_AX_DIAG_RECORD_ADDR)
 *   0x80300000  end of OCM
 *
 * Because the early area sits above ram_top, it never overlaps the final
 * reservations, so the layout keeps working as the image grows.
 * u-boot.lds asserts that the image leaves room below the heap.
 */
#define MK_OCM_BASE		0x80200000
#define MK_OCM_SIZE		SZ_1M
#define MK_OCM_RSVD_SIZE	SZ_64K
#define MK_OCM_DIAG_REC_SIZE	SZ_16K

#define CFG_SYS_SDRAM_BASE	MK_OCM_BASE
#define CFG_SYS_SDRAM_SIZE	(MK_OCM_SIZE - MK_OCM_RSVD_SIZE)

#define CFG_SYS_INIT_RAM_ADDR	(CFG_SYS_SDRAM_BASE + CFG_SYS_SDRAM_SIZE)
#define CFG_SYS_INIT_RAM_SIZE	(MK_OCM_RSVD_SIZE - MK_OCM_DIAG_REC_SIZE)

/* DDR, as on SCM3000: mapped and reported to Linux, not used by U-Boot */
#define MK_DDR0_BASE		0x3C000000ULL
#define MK_DDR0_SIZE		(SZ_1G + SZ_64M)
#define MK_DDR1_BASE		0x400000000ULL
#define MK_DDR1_SIZE		0x80000000ULL

#define CFG_SYS_MAXARGS		64
#define CFG_SYS_BARGSIZE	CFG_SYS_CBSIZE

#define CONFIG_ENV_OVERWRITE


#ifdef CONFIG_BOOTARGS
#undef CONFIG_BOOTARGS
#endif
#define CONFIG_BOOTARGS			"console=ttyPS3,115200 " \
					"maxcpus=4 nr_cpus=4 earlycon " \
					"hugepages=16 root=/dev/ram rw " \
					"phram.phram=ramrofs"

#define CFG_SYS_BAUDRATE_TABLE	\
	{ 4800, 9600, 19200, 38400, 57600, 115200 }

#endif /* __MK_H */
