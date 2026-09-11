// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <malloc.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "ax_diag.h"
#include "ax_spi.h"
#include "ax_flash.h"

#define SPI_FLASH_TIMEOUT		1000
#define SPI_FLASH_SPEED 		5000000

#define TX_FIFO_SZ 0x4 /**< size of max bytes on TX fifo register */
#define RX_FIFO_SZ 0x4 /**< size of max bytes on RX fifo register */
#define SPI_TX_BLOCK	  0x0100 /** < max bytes to send at once is 256 bytes */
#define SPI_RX_BLOCK	  0x0100 /** < max bytes to receive at once is 256 bytes */

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif /* #ifndef MIN */

typedef int offset_t;

static int ax_flash_read_status(struct nor_flash_ctrl_t *flash, u8 cmd, u8 mask,
        bool *result)
{
    int ret;
    u8 src[TX_FIFO_SZ] = { 0x0, 0x0, 0x0, 0x0 };
    u8 dst[RX_FIFO_SZ] = { 0x0, 0x0, 0x0, 0x0 };

    if (!flash)
        return -EINVAL;

    src[RW_COMMAND_OFFSET] = cmd;

    ret = ax_spi_xfer(flash->bus, 4, src, 4, dst);
    if (ret)
        return ret;

    *result = (mask != (dst[RW_COMMAND_OFFSET - 1] & mask)) ? false : true;

    return 0;
}

static int ax_flash_wait_ready(struct nor_flash_ctrl_t *flash)
{
    int timeout = SPI_FLASH_TIMEOUT;
    bool result = false;

    if (!flash)
        return -EINVAL;

    while (timeout--) {
        ax_flash_read_status(flash, NOR_FLASH_CMD_RDSR,
                NOR_FLASH_STATUS_WIP, &result);

        if (result == false)
            return 0;

        mdelay(10);
    }

    return -ETIMEDOUT;
}

static int ax_flash_send_opcode(struct nor_flash_ctrl_t *flash, u8 cmd)
{
    int ret;
    if (!flash)
        return -EINVAL;

    u8 src[4] = { 0x0, 0x0, 0x0, 0x0 };

    src[0] = cmd;

    ret = ax_spi_xfer(flash->bus, 
            4,
            src, 0, NULL);    
    if (ret) {
        printf("CMD: 0x%02x xfer failed, status: 0x%x\r\n", cmd, ret);
        return ret;
    }

    return 0;
}

int ax_flash_set_adrs_mode(struct nor_flash_ctrl_t *flash)
{
    int ret;
    u8 cmd = 0x0;
    u8 mask = 0x0;

    if (!flash)
        return -EINVAL;

    if (NOR_FLASH_JEDEC_ID_MICRON == flash->manufacturer_id) {
        ret = ax_flash_send_opcode(flash, NOR_FLASH_CMD_WREN);
        if (ret)
            return -EINVAL;
    }
    /* Enter 4 Byte addressing mode */
    printf("Enter 4 Byte addressing mode\n");
    ret = ax_flash_send_opcode(flash, NOR_FLASH_CMD_4BA);
    if (ret) {
        printf("Addressing mode set failed, status: = 0x%x\n", ret);
        return -EINVAL;
    }

    /* Check setting status */
    switch (flash->manufacturer_id) {
        case NOR_FLASH_JEDEC_ID_MICRON:
            /* cmd = NOR_FLASH_CMD_FLSR; */
            /* mask = NOR_FLASH_STATUS_4B; */
            printf("Test not support Micron flash\n");
            break;

        case NOR_FLASH_JEDEC_ID_MACRONIX:
            /* cmd = NOR_FLASH_CMD_RDCR; */
            /* mask = NOR_FLASH_4BYTE_CF_MASK; */
            printf("Test not support Macronic flash\n");
            break;

        case NOR_FLASH_JEDEC_ID_WINBOND:
            cmd = NOR_FLASH_CMD_RDSR3;
            mask = NOR_FLASH_4BYTE_SR3_MASK;
            break;

        default:
            return -EINVAL;
    }

    bool result = false;
    ret = ax_flash_read_status(flash, cmd, mask, &result);
    if (ret)
        return -EACCES;

    if (result) {
        printf("Flash in 4B mode\n");
        flash->addr_mode_4_byte = true;
    } else {
        printf("Flash in 3B mode\n");
        flash->addr_mode_4_byte = false;
        return -EACCES;
    }

    return 0;
}


int ax_flash_init(struct nor_flash_ctrl_t *flash)
{
    int ret;

    if (!flash)
        return -EINVAL;

    ret = ax_spi_init(flash->bus);
    if (ret) {
        printf("SPI bus %u init failed: %d\n", flash->bus, ret);
        return ret;
    }

    ret = ax_spi_set_speed(flash->bus, SPI_FLASH_SPEED);
    if (ret) {
        printf("SPI bus %u set speed failed: %d\n", flash->bus, ret);
        return ret;
    }

    ret = ax_spi_set_cs(flash->bus, flash->chipsel);
    if (ret) {
        printf("SPI bus %u select slave %d failed: %d\n", flash->bus, flash->chipsel, ret);
        return ret;
    }

    ret = ax_spi_set_mode(flash->bus, SPI_MODE_0);
    if (ret) {
        printf("SPI bus %u set mode failed: %d\n", flash->bus, ret);
        return ret;
    }

    printf("SPI bus %u initialized\n", flash->bus);

    return 0;
}

int ax_flash_read_id(struct nor_flash_ctrl_t *flash)
{
    u8 src[TX_FIFO_SZ] = { 0x0, 0x0, 0x0, 0x0 };
    u8 dst[RX_FIFO_SZ] = { 0x0, 0x0, 0x0, 0x0 };
    u8 len;
    int ret;

    if (!flash)
        return -EINVAL;

    src[NOR_FLASH_MID_LEN] = NOR_FLASH_CMD_RDID;
    len = NOR_FLASH_MID_LEN + 1;

    ret = ax_spi_xfer(flash->bus, len, src, len, dst);
    if (ret) {
        printf("Read ID failed, status: 0x%x\n", ret);
        return ret;
    }

    flash->manufacturer_id = dst[NOR_FLASH_JEDEC_ID];
    flash->memory_type = dst[NOR_FLASH_JEDEC_ID_MEMORY_TYPE];
    flash->capacity_id = dst[NOR_FLASH_JEDEC_ID_MEMORY_DENSITY];

    return 0;
}

static int __hal_flash_read(struct nor_flash_ctrl_t *flash, offset_t offset,
        void *dest, size_t size)
{
    uint8_t txbuf[SPI_TX_BUF_SIZE] = { 0 };
    size_t tx_len = 0;
    int ret;

    if (flash->addr_mode_4_byte) {
        /* Prepare for command */
        txbuf[WE_4B_CMD_OFFSET] = NOR_FLASH_CMD_READ_FAST_4B;
        txbuf[WE_4B_DATA_OFFSET_1] = ((offset & 0xFFFFFF00) >> 24);
        txbuf[WE_4B_DATA_OFFSET_2] = ((offset & 0xFFFF00) >> 16);
        txbuf[WE_4B_DATA_OFFSET_3] = ((offset & 0xFF00) >> 8);
        txbuf[WE_4B_DATA_OFFSET_4] = (offset & 0xFF);
        /* 
         * In Fast read there is 1 dummy cycle before data is received, 5bytes for command and address and 2 dummy bytes
         * total len = 5(cmd +address) + 1 (fast read dummy cycle) + 2 bytes (dummy due to endianness)
         */
        tx_len = (NOR_FLASH_4B_ADDR_LEN + NOR_FLASH_4B_ADDR_LEN);
    } else {
        txbuf[RW_COMMAND_OFFSET] = NOR_FLASH_CMD_FASTREAD;
        txbuf[RW_DATA_OFFSET_1] = ((offset & 0xFFFF00) >> 16);
        txbuf[RW_DATA_OFFSET_2] = ((offset & 0xFF00) >> 8);
        txbuf[RW_DATA_OFFSET_3] = (offset & 0xFF);
        /* 
         * In Fast read there is 1 dummy cycle before data is received, 5bytes for command and address and 2 dummy bytes
         * total len = 4(cmd +address) + 1 (fast read dummy cycle) + 3 bytes (dummy due to endianness)
         */
        tx_len = (NOR_FLASH_3B_ADDR_LEN + 1);
    }

    /* Send READ command */
    ret = ax_spi_xfer(flash->bus, tx_len, txbuf, size, dest);
    if (ret) {
        printf("%s - read failed\n", __func__);
        return ret;
    }

    return 0;
}

int ax_flash_read(struct nor_flash_ctrl_t *flash, u32 addr,
        void *buf, size_t len)
{
    int ret;
    if (!buf || !len || !flash)
        return -EINVAL;

    while (len > 0) {
        /*
         * Don't read more than RX_BLOCK at a time.
         * because the lower layers like txrx will reject it
         * 6 comes from 1 byte (cmd) + 4 byte (addr) + 1 wait cycle(dummy)
         */

        size_t to_read = MIN(len, (SPI_RX_BLOCK - 8));
        ret = __hal_flash_read(flash, addr, buf, to_read); 
        if (ret)
            return ret;

        len -= to_read;
        buf = (u8 *)buf + to_read;
        addr += to_read;
    }

    return 0;
}

static int __hal_flash_write(struct nor_flash_ctrl_t *flash, offset_t offset,
        const void *src, size_t size)
{
    uint8_t txbuf[SPI_TX_BLOCK] = { 0 };
    size_t tx_len = 0;
    int ret;

    if (flash->addr_mode_4_byte) {
        txbuf[WE_4B_CMD_OFFSET] = NOR_FLASH_CMD_PP_4B;
        txbuf[WE_4B_DATA_OFFSET_1] = ((offset & 0xFFFFFF00) >> 24);
        txbuf[WE_4B_DATA_OFFSET_2] = ((offset & 0xFFFF00) >> 16);
        txbuf[WE_4B_DATA_OFFSET_3] = ((offset & 0xFF00) >> 8);
        txbuf[WE_4B_DATA_OFFSET_4] = (offset & 0xFF);

        /* 
         * 5 bytes for command and address and 3 dummy bytes at the start
         * total len = 5(cmd + address) + 3 bytes (dummy due to endianness)
         */
        tx_len = NOR_FLASH_4B_ADDR_LEN + NOR_FLASH_4B_ADDR_LEN;
    } else {
        txbuf[WE_3B_CMD_OFFSET] = NOR_FLASH_CMD_PP;
        txbuf[WE_3B_DATA_OFFSET_1] = ((offset & 0xFFFF00) >> 16);
        txbuf[WE_3B_DATA_OFFSET_2] = ((offset & 0xFF00) >> 8);
        txbuf[WE_3B_DATA_OFFSET_3] = (offset & 0xFF);

        /* 
         * 3 bytes for command and 1 byte for address
         * total len = 4(cmd + address)
         */
        tx_len = NOR_FLASH_3B_ADDR_LEN + 1;
    }

    memcpy(&txbuf[tx_len], src, size);

    /* Send command*/
    ret = ax_spi_xfer(flash->bus, tx_len + size, txbuf, 0, NULL);
    if (ret)
        return ret;

    ret = ax_flash_wait_ready(flash);
    if (ret)
        return ret;

    return 0;
}

int ax_flash_write(struct nor_flash_ctrl_t *flash, u32 addr,
        const void *buf, size_t len)
{
    int ret;

    if (!flash || !len || !buf)
        return -EINVAL;

    if ((addr < 0) || ((len + addr) > flash->flash_size)) 
        return -EINVAL;

    if (addr % flash->page_size != 0) 
        return -EINVAL;

    printf("Initiating NOR flash write of %ld bytes at offset %u.\r\n", len, addr);
    while (len > 0) {
        /* Don't write more than a TX_BLOCK at a time. */
        size_t to_write = MIN(len, (SPI_TX_BLOCK - 8));

        /* Don't write across a page boundary */
        if ((((uint32_t)addr + to_write - 1U) / flash->page_size) !=
                ((uint32_t)addr / flash->page_size)) {
            to_write = flash->page_size - (addr % flash->page_size);
        }

        /* Before issuing erase command, need to latch the write enable bit */
        ret = ax_flash_send_opcode(flash, NOR_FLASH_CMD_WREN);
        if (ret) {
            printf("WREN failed, status: = 0x%x\r\n", ret);
            return ret;
        }

        ret = __hal_flash_write(flash, addr, buf, to_write);
        if (ret)
            return ret;

        len -= to_write;
        buf = (const uint8_t *)buf + to_write;
        addr += to_write;
    };

    return 0;
}

static int __hal_flash_erase(struct nor_flash_ctrl_t *flash, u8 cmd, offset_t offset)
{
    int ret;
    uint8_t txbuf[SPI_TX_BUF_SIZE] = { 0 };
    size_t tx_len = 0;

    if (flash->addr_mode_4_byte) {
        /* Prepare for command */
        txbuf[WE_4B_CMD_OFFSET] = cmd;
        txbuf[WE_4B_DATA_OFFSET_1] = ((offset & 0xFFFFFF00) >> 24);
        txbuf[WE_4B_DATA_OFFSET_2] = ((offset & 0xFFFF00) >> 16);
        txbuf[WE_4B_DATA_OFFSET_3] = ((offset & 0xFF00) >> 8);
        txbuf[WE_4B_DATA_OFFSET_4] = (offset & 0xFF);

        /* 
         * 5 bytes for command and address and 3 dummy bytes at the start
         * total len = 5(cmd + address) + 3 bytes (dummy due to endianness)
         */
        tx_len = (NOR_FLASH_4B_ADDR_LEN + NOR_FLASH_4B_ADDR_LEN);
    } else {
        txbuf[WE_3B_CMD_OFFSET] = cmd;
        txbuf[WE_3B_DATA_OFFSET_1] = ((offset & 0xFFFF00) >> 16);
        txbuf[WE_3B_DATA_OFFSET_2] = ((offset & 0xFF00) >> 8);
        txbuf[WE_3B_DATA_OFFSET_3] = (offset & 0xFF);
        /* 
         * 3 bytes for command and 1 byte for address
         * total len = 4(cmd + address)
         */
        tx_len = (NOR_FLASH_3B_ADDR_LEN + 1);
    }

    /* Send command*/
    ret = ax_spi_xfer(flash->bus, tx_len, txbuf, 0, NULL);
    if (ret)
        return ret;

    /* Check flash status by reading the status register and polling for WIP bit */
    ret = ax_flash_wait_ready(flash);
    if (ret)
        return ret;

    return 0;
}

static bool __hal_flash_is_erase_possible(size_t capacity, int offset,
        size_t erase_size)
{
    return (erase_size >= capacity && (offset & (capacity - 1)) == 0);
}


int ax_flash_erase(struct nor_flash_ctrl_t *flash, u32 addr,
        size_t len)
{
    int ret;

    if (!flash)
        return -EINVAL;

    if ((addr < 0) || ((len != flash->flash_size) && ((len + addr) > flash->flash_size))) 
        return -EINVAL;

    /* address must be sector-aligned */
    if (!NOR_FLASH_IS_SECTOR_ALIGNED(addr)) {
        printf("Address must be sector-aligned\n");
        return -EINVAL;
    }

    /* size must be a multiple of sectors */
    if ((len % NOR_FLASH_SECTOR_SIZE) != 0) {
        printf("Size %ld is not multiple of sectors\n", len);
        return -EINVAL;
    }

    if (len == flash->flash_size) {
        /* Before issuing erase command, need to latch the write enable bit */
        ret = ax_flash_send_opcode(flash, NOR_FLASH_CMD_WREN);
        if (ret) {
            printf("WREN failed, status: = 0x%x\n", ret);
            return ret;
        }

        /* erase the whole chip */
        ret = ax_flash_send_opcode(flash, NOR_FLASH_CMD_CE);
        if (ret) {
            printf("Initiating Flash Erase failed, status: 0x%x\n", ret);
            return ret;
        }

        ret = ax_flash_wait_ready(flash);
        if (ret)
            return ret;
    } else {
        while (len > 0) {
            /* Before issuing erase command, need to latch the write enable bit */
            ret = ax_flash_send_opcode(flash, NOR_FLASH_CMD_WREN);
            if (ret) {
                printf("WREN failed, status: = 0x%x\n", ret);
                return ret;
            }

            if (__hal_flash_is_erase_possible(NOR_FLASH_BLOCK_SIZE, addr, len)) {
                printf("Erasing the block at %d, size passed = %zu\n", addr, len);
                ret = __hal_flash_erase(flash, NOR_FLASH_CMD_BE_4B, addr);
                if (ret) {
                    printf("Erase operation failed <0x%x>\n", ret);
                    return ret;
                }
                addr += NOR_FLASH_BLOCK_SIZE;
                len -= NOR_FLASH_BLOCK_SIZE;
            } else if (__hal_flash_is_erase_possible(NOR_FLASH_SECTOR_SIZE, addr, len)) {
                printf("Erasing the sector at %d, size passed = %zu\r\n", addr, len);
                ret = __hal_flash_erase(flash, NOR_FLASH_CMD_SE_4B, addr);
                if (ret) {
                    printf("Erase operation failed <0x%x>\n", ret);
                    return ret;
                }
                addr += NOR_FLASH_SECTOR_SIZE;
                len -= NOR_FLASH_SECTOR_SIZE;
            } else {
                printf("Can't erase %zu at 0x%lx", len, (long)addr);
            }
        }
    }

    printf("Erase Completed\r\n");

    return 0;
}

int ax_flash_memtest(struct nor_flash_ctrl_t *flash, u32 flash_addr,
        size_t len)
{
    u8 *read_buf;
    u8 *write_buf;
    size_t i;
    int ret;

    read_buf = (u8 *)FLASH_WRITE_IMAGE_READ_ADDRESS;
    write_buf = (u8 *)FLASH_READ_IMAGE_WRITE_ADDRESS;

    if (!len)
        return -EINVAL;

    printf("Flash address : 0x%08x\n", flash_addr);
    printf("Length        : 0x%zx (%zu bytes)\n",
            len, len);

    printf("Write buffer  : %p\n", write_buf);
    printf("Read buffer   : %p\n", read_buf);

    /*
     * Generate test pattern
     */
    printf("Generate test pattern...\n");

    for (i = 0; i < len; i++)
        write_buf[i] = (u8)(i & 0xff);

    /*
     * Erase flash
     */
    uint32_t sector_count = (len / NOR_FLASH_SECTOR_SIZE);
    sector_count = (len % NOR_FLASH_SECTOR_SIZE) ?
        (sector_count + 1) :
        sector_count;

    uint32_t block_count = (len / NOR_FLASH_BLOCK_SIZE);
    block_count = (len % NOR_FLASH_BLOCK_SIZE) ? (block_count + 1) :
        block_count;
    printf("Erase flash...\n");
    ret = ax_flash_erase(flash, flash_addr, sector_count * NOR_FLASH_SECTOR_SIZE);
    if (ret) {
        printf("Erase failed: %d\n", ret);
        return ret;
    }

    /*
     * Program flash
     */
    printf("Write flash...\n");

    ret = ax_flash_write(flash, flash_addr,
            write_buf, len);
    if (ret) {
        printf("Write failed: %d\n", ret);
        return ret;
    }

    /*
     * Clear read buffer
     */
    memset(read_buf, 0, len);

    /*
     * Read flash
     */
    printf("Read flash...\n");

    ret = ax_flash_read(flash, flash_addr,
            read_buf + flash_addr, len);
    if (ret) {
        printf("Read failed: %d\n", ret);
        return ret;
    }

    /*
     * Compare
     */
    printf("Compare...\n");

    for (i = 0; i < len; i++) {
        if (write_buf[i] != read_buf[i]) {
            printf("MEMTEST FAILED\n");
            printf("Offset : 0x%zx\n", i);
            printf("Flash  : 0x%08x\n",
                    flash_addr + (u32)i);
            printf("Write  : 0x%02x\n",
                    write_buf[i]);
            printf("Read   : 0x%02x\n",
                    read_buf[i]);

            return -EIO;
        }
    }

    printf("MEMTEST PASSED\n");

    return 0;
}
