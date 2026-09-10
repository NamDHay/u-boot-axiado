// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __FLASH_H
#define __FLASH_H

#include "ax_diag.h"
#include <linux/types.h>

/*
 * SIZE of the transmit buffer includes offset in big-endian format.
 */
#define SPI_TX_BUF_SIZE 8

/** Read size threshold for printing % complete */
#define FLASH_READ_THRESHOLD	 (1024 * 1024)
#define FLASH_WRITE_THRESHOLD (1024 * 1024)

/* Indicates that an access command includes bytes for the address.
 * If not provided the opcode is not followed by address bytes.
 */
#define NOR_FLASH_ADDR_PROVIDED BIT(0)

/* NOR Flash Slave device can handle the following cmds */
#define NOR_FLASH_CMD_WRSR		   0x01 /**< Write status register */
#define NOR_FLASH_CMD_RDSR		   0x05 /**< Read status register */
#define NOR_FLASH_CMD_RDSCUR	   0x2B /**< Read security register Macronix flash */
#define NOR_FLASH_CMD_FLSR		   0x70 /**< Read flag status register */
#define NOR_FLASH_CMD_WRSR2		   0x31 /**< Write status register 2 */
#define NOR_FLASH_CMD_RDSR2		   0x35 /**< Read status register 2 */
#define NOR_FLASH_CMD_RDSR3		   0x15 /**< Read status register 3 */
#define NOR_FLASH_CMD_WRSR3		   0x11 /**< Write status register 3 */
#define NOR_FLASH_CMD_READ		   0x03 /**< Read data */
#define NOR_FLASH_CMD_FASTREAD	   0x0B /**< Fast Read */
#define NOR_FLASH_CMD_DREAD		   0x3B /**< Read data (1-1-2) */
#define NOR_FLASH_CMD_QREAD		   0x6B /**< Read data (1-1-4) */
#define NOR_FLASH_CMD_4READ		   0xEB /**< Read data (1-4-4) */
#define NOR_FLASH_CMD_WREN		   0x06 /**< Write enable */
#define NOR_FLASH_CMD_WRDI		   0x04 /**< Write disable */
#define NOR_FLASH_CMD_PP		   0x02 /**< Page program */
#define NOR_FLASH_CMD_PP_1_1_4	   0x32 /**< Quad Page program (1-1-4) */
#define NOR_FLASH_CMD_PP_1_4_4	   0x38 /**< Quad Page program (1-4-4) */
#define NOR_FLASH_CMD_RDCR		   0x15 /**< Read control register */
#define NOR_FLASH_CMD_SE		   0x20 /**< Sector erase */
#define NOR_FLASH_CMD_SE_4B		   0x21 /**< 4-Byte Sector erase */
#define NOR_FLASH_CMD_BE_32K	   0x52 /**< Block erase 32KB */
#define NOR_FLASH_CMD_BE		   0xD8 /**< Block erase */
#define NOR_FLASH_CMD_BE_4B		   0xDC /**< 4-Byte Block erase */
#define NOR_FLASH_CMD_CE		   0xC7 /**< Chip erase */
#define NOR_FLASH_CMD_RDID		   0x9F /**< Read JEDEC ID. Can be either 9E or 9F */
#define NOR_FLASH_CMD_RDSFDP	   0x5A /**< read SDDP table */
#define NOR_FLASH_CMD_ULBPR		   0x98 /**< Global Block Protection Unlock */
#define NOR_FLASH_CMD_4BA		   0xB7 /**< Enter 4-Byte Address Mode */
#define NOR_FLASH_CMD_4BE		   0xE9 /**< Exit 4-Byte Address Mode */
#define NOR_FLASH_CMD_DPD		   0xB9 /**< Deep Power Down */
#define NOR_FLASH_CMD_RDPD		   0xAB /**< Release from Deep Power Down */
#define NOR_FLASH_CMD_WR_CFGREG2   0x72 /**< Write config register 2 */
#define NOR_FLASH_CMD_RD_CFGREG2   0x71 /**< Read config register 2 */
#define NOR_FLASH_CMD_RESET_EN	   0x66 /**< Reset Enable */
#define NOR_FLASH_CMD_RESET_MEM	   0x99 /**< Reset Memory */
#define NOR_FLASH_CMD_BULKE		   0x60 /**< Bulk Erase */
#define NOR_FLASH_CMD_PP_4B		   0x12 /**< Page Program 4 Byte Address */
#define NOR_FLASH_CMD_READ_4B	   0x13 /**< Read data with 4 Byte address */
#define NOR_FLASH_CMD_READ_FAST_4B 0x0C /**< Fast Read 4 Byte Address */
#define NOR_FLASH_CMD_WB_ENTER_QPI 0x38 /**< Winbond specific enter QPI */
#define NOR_FLASH_CMD_WB_EXIT_QPI  0xff /**< Winbond specific exit QPI */

/* JEDEC ID LIST */
#define NOR_FLASH_JEDEC_ID_MICRON	0x20
#define NOR_FLASH_JEDEC_ID_MACRONIX 0xc2
#define NOR_FLASH_JEDEC_ID_WINBOND	0xef
/* Add New JEDEC ID Above this */
#define NOR_FLASH_INVALID_JEDEC_ID 0xFF

#define NOR_FLASH_MID_LEN  3 /**< 3 byte JEDEC id field returned by Read ID cmd */
#define NOR_FLASH_MRETRIES 0xFF /**< Maximum number wait for flash ready */

/* SIZE/DUMMY Byte Macro for commands */
#define NOR_FLASH_CMD_ONLY_OPCODE_SIZE	  1 /* Number of bytes to send in only tx commands */
#define NOR_FLASH_CMD_ONLY_OPCODE_DUMMY	  0 /* Number of dummy bytes to send in only tx commands */
#define NOR_FLASH_CMD_READ_4B_STATUS_SIZE 2

#define NOR_FLASH_READ_ID_DUMMY_BYTE 1
#define RW_COMMAND_OFFSET			 0x3 /* Command Position in the transfer */
#define RW_DATA_OFFSET_1			 0x2 /* Offset byte */
#define RW_DATA_OFFSET_2			 0x1 /* Offset byte */
#define RW_DATA_OFFSET_3			 0x0 /* Offset byte */
#define RW_DATA_OFFSET_4			 0x7 /* Offset byte */
#define WE_4B_CMD_OFFSET			 0x0 /* Command offset for erase/write command */
#define WE_4B_DATA_OFFSET_1			 0x7 /* Address byte 1 for erase/write command */
#define WE_4B_DATA_OFFSET_2			 0x6 /* Address byte 2 for erase/write command */
#define WE_4B_DATA_OFFSET_3			 0x5 /* Address byte 3 for erase/write command */
#define WE_4B_DATA_OFFSET_4			 0x4 /* Address byte 4 for erase/write command */
#define WE_3B_CMD_OFFSET			 0x3 /* Command offset for erase/write command */
#define WE_3B_DATA_OFFSET_1			 0x2 /* Address byte 1 for erase/write command */
#define WE_3B_DATA_OFFSET_2			 0x1 /* Address byte 2 for erase/write command */
#define WE_3B_DATA_OFFSET_3			 0x0 /* Address byte 3 for erase/write command */

#define NOR_FLASH_JEDEC_ID				  0x02 /* JEDEC ID Position */
#define NOR_FLASH_JEDEC_ID_MEMORY_TYPE	  0x01 /* Memory type position */
#define NOR_FLASH_JEDEC_ID_MEMORY_DENSITY 0x00 /* Memory density position */

/* Flash define */
#define NOR_FLASH_STATUS	0x02 /* Flash Status */
#define NOR_FLASH_DONE		0x01 /* Flash  */

#define FLASH_WRITE_IMAGE_READ_ADDRESS      (AX3000_MEM_BASE_ADRS_DDR_LO + 0x10000000)
#define FLASH_READ_IMAGE_WRITE_ADDRESS      (AX3000_MEM_BASE_ADRS_DDR_LO + 0x20000000)

#define NOR_FLASH_PAGE_SIZE		 0x0100U
#define NOR_FLASH_SECTOR_SIZE	 0x1000U
#define NOR_FLASH_BLOCK_SIZE_32K 0x8000U
#define NOR_FLASH_BLOCK_SIZE	 0x10000U

#define NOR_FLASH_3B_ADDR_LEN 3 /**< 24 bit addressing mode offset length in bytes */
#define NOR_FLASH_4B_ADDR_LEN 4 /**< 32 bit addressing mode offset length in bytes */
/* ===========================================*/

/* Status register bits */
#define NOR_FLASH_STATUS_WIP BIT(0) /**< Write in progress */
#define NOR_FLASH_STATUS_WEL BIT(1) /**< Write enable latch */
#define NOR_FLASH_STATUS_QE	 BIT(6) /**< Enable quad mode */


#define FLASH_CMD_MIN_ARGC			  4 /**< Minimum command line arguments for the cmd */
#define FLASH_CMD_MAX_ARGC			  6 /**< Maximum command line arguments for the cmd */
#define FLASH_SUB_COMMAND_POS		  1 /**< Flash subcommand position */
#define FLASH_SPI_INSTANCE_ADDR_POS	  2 /**< SPI instance address position */
#define FLASH_SPI_CS_POS			  3 /**< Chip Select position*/
#define FLASH_READ_WRITE_OFF_ADDR_POS 4 /**< Flash read/write offset address position */
#define FLASH_TRANS_LEN_POS			  5 /**< Flash read/write length position */
#define FLASH_MAX_SPI_INSTANCE		  5 /**< Flash maximum SPI instance */
#define MAX_READ_OFFSET				  0x8000000 /**< Maximum read offset to check bounds */
#define FLASH_MAX_CHIPSELECT		  4 /**< Flash maximum chip select */
#define FLASH_READ_CHUNK_SIZE		  0x40000 /**< Maximum read size for verification */

/* status register-3 winbond specific */
#define NOR_FLASH_4BYTE_SR3_MASK 0x01

#define NOR_FLASH_IS_SECTOR_ALIGNED(_ofs) IS_ALIGNED(_ofs, 0x1000)

/**
 * @brief flash device instance to operate one NOR Flash
 */
struct nor_flash_ctrl_t {
	uint16_t page_size; /**< no of bytes per page; usually 256 bytes */
	uint16_t sector_size; /**< Smallest sector erase unit */
	uint32_t flash_size; /**< flash size, in bytes */
	uint32_t num_blocks; /**< total number of blocks in the flash device */
	uint32_t block_size; /**< Largest natural erase unit */
	uint32_t num_sector; /**< total number of sectors in the device */
	uint16_t sub_block_size; /**< sub block erase size */
    uint8_t manufacturer_id;
	uint8_t memory_type;
	uint8_t capacity_id;
	uint8_t bus;
	uint8_t chipsel;
	bool addr_mode_4_byte; /**< 4-byte addressability mode */
};

int ax_flash_init(struct nor_flash_ctrl_t *flash);
int ax_flash_read_id(struct nor_flash_ctrl_t *flash);

int ax_flash_set_adrs_mode(struct nor_flash_ctrl_t *flash);

int ax_flash_read(struct nor_flash_ctrl_t *flash, u32 addr,
		  void *buf, size_t len);

int ax_flash_write(struct nor_flash_ctrl_t *flash, u32 addr,
		   const void *buf, size_t len);

int ax_flash_erase(struct nor_flash_ctrl_t *flash, u32 addr,
        size_t len);

int ax_flash_memtest(struct nor_flash_ctrl_t *flash, u32 flash_addr,
		     size_t len);

#endif /* __FLASH_H */
