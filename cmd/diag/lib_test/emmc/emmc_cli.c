// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <command.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include "ax_emmc.h"

#define AX_MMC_READ_ADDR	0x90000000UL
#define AX_MMC_WRITE_ADDR	0xA0000000UL

#define AX_MMC_BLOCK_SIZE	512

#define AX_MMC_PATTERN_1	0xAAAAAAAA
#define AX_MMC_PATTERN_2	0x55555555
#define AX_MMC_PATTERN_3	0xFFFFFFFF
#define AX_MMC_PATTERN_4	0x00000000

static int do_ax_mmc_init(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int ret;
    u32 width;
    u32 speed;

    if (argc != 3)
        return CMD_RET_USAGE;

    width = hextoul(argv[1], NULL);
    speed = hextoul(argv[2], NULL);

    ret = ax_emmc_init();
    if (ret) {
        printf("eMMC init failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    ret = ax_emmc_set_bus_width((emmc_bus_width_t)width);
    if (ret) {
        printf("eMMC set bus width failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    ret = ax_emmc_set_speed((emmc_speed_mode_t)speed);
    if (ret) {
        printf("eMMC set speed failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    printf("eMMC initialized successfully\n");
    printf("Bus width: %u\n", width);
    printf("Speed mode: %u\n", speed);

    return CMD_RET_SUCCESS;
}

static int do_ax_mmc_reset(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int ret;

    if (argc != 1)
        return CMD_RET_USAGE;

    ret = ax_emmc_reset();
    if (ret) {
        printf("eMMC reset failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    printf("eMMC reset successfully\n");

    return CMD_RET_SUCCESS;
}

static int do_ax_mmc_read(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    u32 start;
    u32 blkcnt;
    u32 block_size;
    u64 total_size;
    void *dst;
    int ret;

    if (argc != 3)
        return CMD_RET_USAGE;

    start = hextoul(argv[1], NULL);
    blkcnt = hextoul(argv[2], NULL);

    if (!blkcnt)
        return CMD_RET_FAILURE;

    block_size = ax_emmc_get_block_size();
    if (!block_size)
        block_size = AX_MMC_BLOCK_SIZE;

    total_size = (u64)blkcnt * block_size;

    if (total_size > CONFIG_SYS_MAXARGS * 0x100000)
        printf("Warning: large transfer size: %llu bytes\n",
                total_size);

    dst = (void *)AX_MMC_READ_ADDR;

    printf("Read: start=%u, blocks=%u, dst=0x%lx\n",
            start, blkcnt, (ulong)dst);

    ret = ax_emmc_read(start, blkcnt, dst);
    if (ret) {
        printf("eMMC read failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    printf("eMMC read successfully\n");

    return CMD_RET_SUCCESS;
}

static int do_ax_mmc_write(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    u32 start;
    u32 blkcnt;
    u32 block_size;
    u64 total_size;
    const void *src;
    int ret;

    if (argc != 3)
        return CMD_RET_USAGE;

    start = hextoul(argv[1], NULL);
    blkcnt = hextoul(argv[2], NULL);

    if (!blkcnt)
        return CMD_RET_FAILURE;

    block_size = ax_emmc_get_block_size();
    if (!block_size)
        block_size = AX_MMC_BLOCK_SIZE;

    total_size = (u64)blkcnt * block_size;

    if (total_size > CONFIG_SYS_MAXARGS * 0x100000)
        printf("Warning: large transfer size: %llu bytes\n",
                total_size);

    src = (const void *)AX_MMC_WRITE_ADDR;

    printf("Write: start=%u, blocks=%u, src=0x%lx\n",
            start, blkcnt, (ulong)src);

    ret = ax_emmc_write(start, blkcnt, src);
    if (ret) {
        printf("eMMC write failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    printf("eMMC write successfully\n");

    return CMD_RET_SUCCESS;
}

static int do_ax_mmc_erase(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    u32 start;
    u32 blkcnt;
    int ret;

    if (argc != 3)
        return CMD_RET_USAGE;

    start = hextoul(argv[1], NULL);
    blkcnt = hextoul(argv[2], NULL);

    if (!blkcnt)
        return CMD_RET_FAILURE;

    printf("Erase: start=%u, blocks=%u\n", start, blkcnt);

    ret = ax_emmc_erase(start, blkcnt);
    if (ret) {
        printf("eMMC erase failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    printf("eMMC erase successfully\n");

    return CMD_RET_SUCCESS;
}

static int do_ax_mmc_memtest(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    u32 start;
    u32 blkcnt;
    u32 block_size;
    u32 total_words;
    u32 i;
    u32 *write_buf;
    u32 *read_buf;
    int ret;

    if (argc != 3)
        return CMD_RET_USAGE;

    start = hextoul(argv[1], NULL);
    blkcnt = hextoul(argv[2], NULL);

    if (!blkcnt)
        return CMD_RET_FAILURE;

    block_size = ax_emmc_get_block_size();
    if (!block_size)
        block_size = AX_MMC_BLOCK_SIZE;

    total_words = (blkcnt * block_size) / sizeof(u32);

    write_buf = (u32 *)AX_MMC_WRITE_ADDR;
    read_buf = (u32 *)AX_MMC_READ_ADDR;

    printf("eMMC memtest: start=%u, blocks=%u\n",
            start, blkcnt);

    printf("Write pattern 0x%08x\n", AX_MMC_PATTERN_1);

    for (i = 0; i < total_words; i++)
        write_buf[i] = AX_MMC_PATTERN_1;

    ret = ax_emmc_write(start, blkcnt, write_buf);
    if (ret) {
        printf("Memtest write failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    ret = ax_emmc_read(start, blkcnt, read_buf);
    if (ret) {
        printf("Memtest read failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    for (i = 0; i < total_words; i++) {
        if (read_buf[i] != AX_MMC_PATTERN_1) {
            printf("Memtest failed at word %u\n", i);
            printf("Expected: 0x%08x\n", AX_MMC_PATTERN_1);
            printf("Actual:   0x%08x\n", read_buf[i]);
            return CMD_RET_FAILURE;
        }
    }

    printf("eMMC memtest passed\n");

    return CMD_RET_SUCCESS;
}

static int do_ax_mmc_info(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    u32 block_size;
    u32 block_count;

    if (argc != 1)
        return CMD_RET_USAGE;

    block_size = ax_emmc_get_block_size();
    block_count = ax_emmc_get_block_count();

    printf("eMMC block size:  %u bytes\n", block_size);
    printf("eMMC block count: %u\n", block_count);

    printf("eMMC capacity: %llu MiB\n",
            (u64)block_size * block_count / (1024 * 1024));

    return CMD_RET_SUCCESS;
}

U_BOOT_LONGHELP(ax_mmc,
        "init <width> <speed>\n"
        "    - initialize eMMC device\n"
        "    - width:\n"
        "       0: EMMC_BUS_1BIT\n"
        "       1: EMMC_BUS_4BIT\n"
        "       2: EMMC_BUS_8BIT\n"
        "       3: EMMC_BUS_4BIT_DDR\n"
        "       4: EMMC_BUS_8BIT_DDR\n"
        "    - speed:\n"
        "       0: EMMC_MODE_NORMAL\n"
        "       1: EMMC_MODE_HIGH_SPEED\n"
        "       2: EMMC_MODE_HS200\n"
        "       3: EMMC_MODE_HS400\n"
        "       4: EMMC_MODE_DDR50\n"
        "reset\n"
        "    - reset eMMC controller\n"
        "read <start_block> <block_count>\n"
        "    - read eMMC into 0x90000000\n"
        "write <start_block> <block_count>\n"
        "    - write eMMC from 0xA0000000\n"
        "erase <start_block> <block_count>\n"
        "    - erase eMMC block\n"
        "memtest <start_block> <block_count>\n"
        "    - write/read/compare test\n"
        "info\n"
        "    - show eMMC information"
);

U_BOOT_CMD_WITH_SUBCMDS(ax_mmc, "Axiado bare-metal eMMC diagnostics test", ax_mmc_help_text,
        U_BOOT_SUBCMD_MKENT(init, CONFIG_SYS_MAXARGS, 1, do_ax_mmc_init),
        U_BOOT_SUBCMD_MKENT(reset, CONFIG_SYS_MAXARGS, 1, do_ax_mmc_reset),
        U_BOOT_SUBCMD_MKENT(read, CONFIG_SYS_MAXARGS, 1, do_ax_mmc_read),
        U_BOOT_SUBCMD_MKENT(write, CONFIG_SYS_MAXARGS, 1, do_ax_mmc_write),
        U_BOOT_SUBCMD_MKENT(erase, CONFIG_SYS_MAXARGS, 1, do_ax_mmc_erase),
        U_BOOT_SUBCMD_MKENT(memtest, CONFIG_SYS_MAXARGS, 1, do_ax_mmc_memtest),
        U_BOOT_SUBCMD_MKENT(info, CONFIG_SYS_MAXARGS, 1, do_ax_mmc_info));
