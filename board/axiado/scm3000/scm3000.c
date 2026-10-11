// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2021-2026 Axiado Corporation (or its affiliates).
 */

#include <config.h>
#include <dm.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <asm/spin_table.h>
#include <asm/system.h>
#include <fdt_support.h>
#include <asm/cache.h>
#include <env.h>
#include <command.h>
#include <irq_func.h>
#include <asm/gic-v3.h>

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region axiado_ax3000_mem_map[] = {
	{
		.virt = 0x3C000000UL,
		.phys = 0x3C000000UL,
		.size = SZ_1G,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
		PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x400000000UL,
		.phys = 0x400000000UL,
		.size = SZ_1G,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
		PTE_BLOCK_INNER_SHARE
	}, {
		0,
	}
};


struct mm_region *mem_map = axiado_ax3000_mem_map;

#ifdef CONFIG_MISC_INIT_R

/**
 * @brief Configure Board Specific parts
 *
 * @param void
 *
 * @return int
 */
int misc_init_r(void)
{
	int ret = 0;
	/* ret = run_command("setmac", 0); */
	return ret;
}
#endif

/*
 * Accept any FIT configuration name - the board loads a single FIT image
 * and the first matching config is used.
 */
int board_fit_config_name_match(const char *name)
{
	return 0;
}

/**
 * @brief Board-specific FDT fixup to restore correct cpu-release-addr
 *
 * arch_fixup_fdt() calls spin_table_update_dt() which overwrites
 * cpu-release-addr with U-Boot's internal address. This function
 * restores the correct address (0x3c0013a0) from the device tree
 * source file, which matches what the kernel expects.
 *
 * @param blob: FDT blob
 * @param bd: Board data pointer
 * @return 0 on success
 */
int ft_board_setup(void *blob, struct bd_info *bd)
{
	int cpus_offset, offset;
	const char *prop;
	int ret;
	u64 cpu_release_addr = (u64)&spin_table_cpu_release_addr - gd->reloc_off;

	cpus_offset = fdt_path_offset(blob, "/cpus");
	if (cpus_offset < 0)
		return 0;

	for (offset = fdt_first_subnode(blob, cpus_offset); offset >= 0;
	     offset = fdt_next_subnode(blob, offset)) {
		prop = fdt_getprop(blob, offset, "device_type", NULL);
		if (!prop || strcmp(prop, "cpu"))
			continue;

		prop = fdt_getprop(blob, offset, "enable-method", NULL);
		if (!prop || strcmp(prop, "spin-table"))
			continue;

		ret = fdt_setprop_u64(blob, offset, "cpu-release-addr",
				      cpu_release_addr);
		if (ret) {
			printf("WARNING: Failed to restore cpu-release-addr\n");
			return ret;
		}
	}

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((void *)CFG_SYS_SDRAM_BASE,
				    CFG_SYS_SDRAM_SIZE);
	return 0;
}

/**
 * @brief Set up DRAM bank sizes for device tree fixup
 *
 * - Region 1: 0x3C000000, size 0x44000000 (1GB + 64MB)
 * - Region 2: 0x400000000, size 0x80000000 (2GB)
 *
 * @return 0 on success
 */
int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CFG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = CFG_SYS_SDRAM_SIZE;
	return 0;
}

void raw_write_cntfrq_el0(u32 cntfrq_el0)
{
	__asm__ __volatile__("msr CNTFRQ_EL0, %0\n\t" : : "r" (cntfrq_el0) : "memory");
}

int timer_init(void)
{
	u32 div0_val = 0, div1_val = 0;
	u64 counter_frq = 0;
	/* Check if PLL is locked and calculate the timer counter */
	if((readl(CLKRST_CPU_PLL_STS_ADRS_OFFSET)) & CPU_PLL_STS_LOCK){
		u32 reg_val = readl(CLKRST_CPU_PLL_POSTDIV_REG); /* Read the CPU frq regiter */
		div0_val = ((reg_val >> REG_CPU_PLL_POSTDIV_FLD_POSTDIV0_0_LSB) & (POST_DIV_BITS_MASK));
		div1_val = ((reg_val >> REG_CPU_PLL_POSTDIV_FLD_POSTDIV1_0_LSB) & (POST_DIV_BITS_MASK));
		counter_frq = (AX_PLL_CLK_4000MHZ / ((div0_val + 1) * (div1_val + 1))); /* Calculate the counter frq value */
		if (current_el() == 3)
			raw_write_cntfrq_el0(counter_frq);

	} else {
		/**
		* Control should not reach here...
		* If PLL is bypassed then recalculate the timer counter based on default frequency
		* TODO: KWS-5412-Notify system manager that system failed to boot.
		*/
		return -1;
	}
	writel(SYS_TIMER_ENABLE, SYS_TIMER_CTRL);
	return 0;
}

#ifdef CONFIG_GIC_V3_ITS

int ls_gic_rd_tables_init(void)
{
    int ret;
    struct udevice *dev;
    fdt_addr_t gicd_base, gicr_base;

    ret = uclass_first_device_err(UCLASS_IRQ, &dev);
    if (ret)
        return ret;

    gicd_base = dev_read_addr_index(dev, 0);
    gicr_base = dev_read_addr_index(dev, 1);

    ret = gicv3_cpu_init();
    if (ret)
        debug("%s: failed to init gic\n", __func__);

    return ret;
}
#endif



int board_init(void)
{
    ls_gic_rd_tables_init();
    enable_interrupts();
    return 0;
}

void reset_cpu(void)
{
}

void enable_caches(void)
{
    /* icache_enable(); */
    /* dcache_enable(); */
}

