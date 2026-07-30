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
#include <env.h>
#include <command.h>
#include <net.h>
#include <irq_func.h>

DECLARE_GLOBAL_DATA_PTR;

static struct mm_region axiado_ax3005_mem_map[] = {
	{ /* Peripherals including UART */
	  .virt = 0x00000000UL,
	  .phys = 0x00000000UL,
	  .size = 0x80000000UL, /* 0 to 0x4A000000: peripherals */
	  .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) | PTE_BLOCK_NON_SHARE |
		   PTE_BLOCK_PXN | PTE_BLOCK_UXN },
	{ .virt = 0x80000000UL,
	  .phys = 0x80000000UL,
	  .size = 0x80000000UL,
	  .attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) | PTE_BLOCK_INNER_SHARE },
	{
	  .virt = 0x500000000UL,
	  .phys = 0x500000000UL,
	  .size = 0x280000000UL, /* 0x5_0000_0000 to 0x7_8000_0000: peripherals */
	  .attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRE) | PTE_BLOCK_NON_SHARE |
		   PTE_BLOCK_PXN | PTE_BLOCK_UXN },
    {
        0,
	}
};

struct mm_region *mem_map = axiado_ax3005_mem_map;

#ifdef CONFIG_MISC_INIT_R
/* Axiado MAC Updating */
#define AXIADO_OUI_B2				0xF9
#define MAX_MAC_CNT				5
#define MAC_BASE_OFFSET				0x400
#define MAC_CONFIG_BYTE_CNT 			0x400
#define AX3000_CSR_BASE_ADRS_HCP		0x43100000
#define R_MAC_0					0x00c
#define R_MAC_1					0x010


/**
 * @brief Program MAC addresses into HCP registers.
 *
 * @details
 * This function validates the base MAC address against the expected AXIADO OUI.
 * It then programs MAC addresses into hardware registers for enabled ports.
 *
 * @param mac   Pointer to base MAC address (6 bytes).
 * @param ports Bitmask indicating enabled MAC ports.
 *
 */
static void write_mac_to_hcp_regs(uint8_t *mac, uint32_t ports)
{
	uint32_t mac_base, mac0, mac1, new_mac = 0;
	uint8_t *c, i, mac_idx;

	if (AXIADO_OUI_B2 != mac[2]) {
		printf("MAC is not provisioned with AXIADO AX3005 OUI\n");
		return;
	}

	if (!ports) {
		printf("No MACs enabled\n");
		return;
	}

	/* for incrementing MAC LSB, accounting overflow */
	c = (uint8_t *)&new_mac;
	c[0] = mac[5];
	c[1] = mac[4];
	c[2] = mac[3];

	for (i = 0; i < MAX_MAC_CNT; i++) {
		/* ports: BIT0->MAC1 BIT1->MAC2 BIT2->MAC3 BIT3->MAC4 BIT4->MAC0 */
		if (ports & (0x1 << i)) {
			mac0 = ((uint32_t)c[2] << 24) | ((uint32_t)mac[2] << 16) | ((uint32_t)mac[1] << 8) |
				   ((uint32_t)mac[0] << 0);
			mac1 = ((uint32_t)c[0] << 8) | ((uint32_t)c[1] << 0);

			if (i == (MAX_MAC_CNT - 1))
				mac_idx = 0;
			else
				mac_idx = i + 1;
			mac_base = AX3000_CSR_BASE_ADRS_HCP + MAC_BASE_OFFSET + (mac_idx * MAC_CONFIG_BYTE_CNT);

			/* program the MAC regs */
			writel(mac0, mac_base + R_MAC_0);
			writel(mac1, mac_base + R_MAC_1);

			/* increment MAC LSB */
			new_mac++;
		}
	}
}

/**
 * @brief Command handler for setmac
 *
 * @param cmdtp: Command table entry
 * @param flag: Command flags
 * @param argc: Argument count
 * @param argv: Argument vector
 * @return 0 on success, non-zero on failure
 */
static int do_setmac(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    u8 mac[6];
	const char* mac_addr;

	mac_addr = env_get("ethaddr");
	if (!mac_addr) {
        printf("ethaddr not set\n");
        return -ENOMEM;
	}

	/* parse "xx:xx:xx:xx:xx:xx" into bytes */
	if (!eth_env_get_enetaddr("ethaddr", mac)) {
		printf("ethaddr is invalid: %s\n", mac_addr);
		return -EINVAL;;
	}

	if (!is_valid_ethaddr(mac)) {
        return -EINVAL;
	}

	printf("Set MAC address to be %s.\n", mac_addr);
	write_mac_to_hcp_regs(mac, 0x1);
	return 0;
}

U_BOOT_CMD(
	setmac, 1, 1, do_setmac,
	"Set MAC address",
	""
);
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

/*
 * ft_board_setup - restore cpu-release-addr after relocation
 *
 * arch_fixup_fdt() / spin_table_update_dt() overwrites cpu-release-addr
 * with U-Boot's relocated address.  Restore the pre-relocation physical
 * address so secondary cores spin on the correct location.
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

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CFG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = CFG_SYS_SDRAM_SIZE;
	return 0;
}

/*
 * timer_init - enable the AX3005 platform system timer
 *
 * CNTFRQ_EL0 is already set by arch/arm/cpu/armv8/start.S using
 * CONFIG_COUNTER_FREQUENCY from the defconfig.
 *
 * SYS_TIMER_CTRL (0x48016000) is the AX3005 system timer control
 * register — writing SYS_TIMER_ENABLE starts the counter that feeds
 * the ARM generic timer.  A proper DM timer driver should replace
 * this once the SoC timer binding is defined.
 */
int timer_init(void)
{
	writel(SYS_TIMER_ENABLE, SYS_TIMER_CTRL);
	return 0;
}

int board_init(void)
{
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

