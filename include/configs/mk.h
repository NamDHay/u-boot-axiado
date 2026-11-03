/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2021-2026 Axiado Corporation (or its affiliates).
 */

#ifndef __MK_H
#define __MK_H

#include <linux/sizes.h>

#define GICD_BASE		0x80300000
#define GICR_BASE		0x80380000

#define SYS_TIMER_CTRL		0x80502000
#define SYS_TIMER_ENABLE	0x1
#define SYS_TIMER_DISABLE	0x0

/* DRAM: 2 GB at 0x80000000 */
#define CFG_SYS_SDRAM_BASE	0x80200000
#define CFG_SYS_SDRAM_SIZE	SZ_1M
// #define CFG_SYS_INIT_SP_ADDR	(CFG_SYS_SDRAM_BASE + SZ_1M)

#define CFG_SYS_MAXARGS		64
#define CFG_SYS_BARGSIZE	CFG_SYS_CBSIZE

#define CONFIG_ENV_OVERWRITE

#define CFG_SYS_BAUDRATE_TABLE	\
	{ 4800, 9600, 19200, 38400, 57600, 115200 }

#endif /* __MK_H */
