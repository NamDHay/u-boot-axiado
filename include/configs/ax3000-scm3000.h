/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2021-2026 Axiado Corporation (or its affiliates).
 */

#ifndef __AX3000_SCM3000_H
#define __AX3000_SCM3000_H

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

/* DRAM: 2 GB at 0x80000000 */
#define CFG_SYS_SDRAM_BASE	0x3C000000
#define CFG_SYS_SDRAM_SIZE	SZ_1G
#define CFG_SYS_INIT_SP_ADDR	(CFG_SYS_SDRAM_BASE + SZ_1M)

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

#endif /* __AX3000_SCM3000_H */
