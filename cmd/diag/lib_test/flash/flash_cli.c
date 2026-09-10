// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <command.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include "ax_spi.h"
#include "ax_flash.h"

static void ax_flash_usage(void)
{
	printf("\n");
	printf("AX SPI Flash Command\n");
	printf("\n");

	printf("Usage:\n");
	printf("  ax_flash init <bus>\n");
	printf("  ax_flash id <bus>\n");

	printf("  ax_flash read <bus> <flash_addr> <len>\n");
	printf("  ax_flash write <bus> <flash_addr> <len>\n");
	printf("  ax_flash erase <bus> <flash_addr> <len>\n");

	printf("  ax_flash memtest <bus> <flash_addr> <len>\n");

	printf("\n");
	printf("Memory buffers:\n");
	printf("  Read buffer  : 0x%08x\n",
	       FLASH_WRITE_IMAGE_READ_ADDRESS);

	printf("  Write buffer : 0x%08x\n",
	       FLASH_READ_IMAGE_WRITE_ADDRESS);

	printf("\n");
}

static int do_ax_flash(struct cmd_tbl *cmdtp, int flag,
		       int argc, char *const argv[])
{
    struct nor_flash_ctrl_t flash;
	const char *cmd;
	u32 flash_addr;
	size_t len;
	int ret;

	if (argc < 2) {
		ax_flash_usage();
		return CMD_RET_USAGE;
	}

	cmd = argv[1];
    flash.bus = simple_strtoul(argv[2], NULL, 0);

    flash.num_blocks = 0x0;
    flash.num_sector = 0x0;
    flash.flash_size = 0x002020ca;
    flash.page_size = NOR_FLASH_PAGE_SIZE;
    flash.sector_size = NOR_FLASH_SECTOR_SIZE;
    flash.block_size = NOR_FLASH_BLOCK_SIZE;
    flash.sub_block_size = NOR_FLASH_BLOCK_SIZE_32K;
    flash.addr_mode_4_byte = true;

	/*
	 * ax_flash init <bus> <cs>
	 */
	if (!strcmp(cmd, "init")) {
		if (argc != 4)
			return CMD_RET_USAGE;

		flash.chipsel = simple_strtoul(argv[3], NULL, 0);

		ret = ax_flash_init(&flash);
		if (ret)
			return CMD_RET_FAILURE;

        ret = ax_spi_set_cs(flash.bus, flash.chipsel);
		if (ret) {
			printf("Set CS failed: %d\n", ret);
			return CMD_RET_FAILURE;
		}

		ret = ax_flash_read_id(&flash);
		if (ret) {
			printf("Read JEDEC ID failed: %d\n", ret);
			return CMD_RET_FAILURE;
		}

		printf("JEDEC ID: %02x %02x %02x\n",
		       flash.manufacturer_id,
		       flash.memory_type,
		       flash.capacity_id);

		/* Extract flash ID */
		if (flash.manufacturer_id == NOR_FLASH_JEDEC_ID_MICRON) {
			printf("Micron Flash\n");
		} else if (flash.manufacturer_id == NOR_FLASH_JEDEC_ID_MACRONIX) {
			printf("Macronix Flash\n");
		} else if (flash.manufacturer_id == NOR_FLASH_JEDEC_ID_WINBOND) {
			printf("Winbond Flash\n");
		} 
		else {
			printf("Unrecognized JEDEC id\n");
            return CMD_RET_FAILURE;
		}

        ret = ax_flash_set_adrs_mode(&flash);
		if (ret) {
			printf("Set flash to 4B mode failed: %d\n", ret);
			return CMD_RET_FAILURE;
		}

		return CMD_RET_SUCCESS;
	}

	/*
	 * ax_flash id <bus>
	 */
	if (!strcmp(cmd, "id")) {
		if (argc != 3)
			return CMD_RET_USAGE;

		ret = ax_flash_read_id(&flash);
		if (ret) {
			printf("Read JEDEC ID failed: %d\n", ret);
			return CMD_RET_FAILURE;
		}

		printf("JEDEC ID: %02x %02x %02x\n",
		       flash.manufacturer_id,
		       flash.memory_type,
		       flash.capacity_id);

		/* Extract flash ID */
		if (flash.manufacturer_id == NOR_FLASH_JEDEC_ID_MICRON) {
			printf("Micron Flash\n");
		} else if (flash.manufacturer_id == NOR_FLASH_JEDEC_ID_MACRONIX) {
			printf("Macronix Flash\n");
		} else if (flash.manufacturer_id == NOR_FLASH_JEDEC_ID_WINBOND) {
			printf("Winbond Flash\n");
		} 
		else {
			printf("Unrecognized JEDEC id\n");
            return CMD_RET_FAILURE;
		}

        ret = ax_flash_set_adrs_mode(&flash);
		if (ret) {
			printf("Set flash to 4B mode failed: %d\n", ret);
			return CMD_RET_FAILURE;
		}

		return CMD_RET_SUCCESS;
	}

	/*
	 * Commands below:
	 *
	 * ax_flash read    <bus> <addr> <len>
	 * ax_flash write   <bus> <addr> <len>
	 * ax_flash erase   <bus> <addr> <len>
	 * ax_flash memtest <bus> <addr> <len>
	 */

    ret = ax_flash_read_id(&flash);
    if (ret) {
        printf("Read JEDEC ID failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    if (!strcmp(cmd, "read") ||
            !strcmp(cmd, "write") ||
            !strcmp(cmd, "erase") ||
            !strcmp(cmd, "memtest")) {

        if (argc != 5)
            return CMD_RET_USAGE;

        flash_addr = simple_strtoul(argv[3], NULL, 0);

        len = simple_strtoul(argv[4], NULL, 0);

        if (!len) {
            printf("Invalid length\n");
            return CMD_RET_FAILURE;
        }
    } else {
        printf("Unknown command: %s\n", cmd);
        ax_flash_usage();

        return CMD_RET_USAGE;
    }

    /*
     * READ
     */
    if (!strcmp(cmd, "read")) {

        printf("Read flash 0x%08x -> RAM 0x%08x\n",
                flash_addr,
                FLASH_WRITE_IMAGE_READ_ADDRESS);

        ret = ax_flash_read(&flash,
                flash_addr,
                (void *)FLASH_WRITE_IMAGE_READ_ADDRESS,
                len);

        if (ret) {
            printf("Read failed: %d\n", ret);
            return CMD_RET_FAILURE;
        }

        printf("Read completed\n");

        return CMD_RET_SUCCESS;
    }

    /*
     * WRITE
     */
    if (!strcmp(cmd, "write")) {

        printf("Write RAM 0x%08x -> flash 0x%08x\n",
                FLASH_READ_IMAGE_WRITE_ADDRESS,
                flash_addr);

        ret = ax_flash_write(&flash,
                flash_addr,
                (void *)FLASH_READ_IMAGE_WRITE_ADDRESS,
                len);

        if (ret) {
            printf("Write failed: %d\n", ret);
            return CMD_RET_FAILURE;
        }

        printf("Write completed\n");

        return CMD_RET_SUCCESS;
    }

    /*
     * ERASE
     */
    if (!strcmp(cmd, "erase")) {

        printf("Erase flash address 0x%08x length 0x%zx\n",
                flash_addr, len);

        ret = ax_flash_erase(&flash,
                flash_addr,
                len);

        if (ret) {
            printf("Erase failed: %d\n", ret);
            return CMD_RET_FAILURE;
        }

        printf("Erase completed\n");

        return CMD_RET_SUCCESS;
    }

    /*
     * MEMTEST
     */
    if (!strcmp(cmd, "memtest")) {

        ret = ax_flash_memtest(&flash,
                flash_addr,
                len);

        if (ret) {
            printf("Memtest FAILED\n");
            return CMD_RET_FAILURE;
        }

        return CMD_RET_SUCCESS;
    }

    return CMD_RET_FAILURE;
}

U_BOOT_CMD(
        ax_flash,
        5,
        1,
        do_ax_flash,
        "AX SPI flash diagnostic command",
        "init <bus>\n"
        "    - initialize SPI flash\n"
        "ax_flash id <bus>\n"
        "    - read JEDEC ID\n"
        "ax_flash read <bus> <flash_addr> <len>\n"
        "    - read flash into 0x90000000\n"
        "ax_flash write <bus> <flash_addr> <len>\n"
        "    - write flash from 0xA0000000\n"
        "ax_flash erase <bus> <flash_addr> <len>\n"
        "    - erase flash sectors\n"
        "ax_flash memtest <bus> <flash_addr> <len>\n"
        "    - write/read/compare test"
        );
