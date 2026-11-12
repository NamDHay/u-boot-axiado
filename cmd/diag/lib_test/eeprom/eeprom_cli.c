// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <command.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <malloc.h>
#include <console.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "ax_i2c.h"
#include "ax_i3c.h"

#define EEPROM_I2C_SPEED        100000
#define EEPROM_MAX_RW_SIZE      256

/*
 * Typical EEPROM write cycle time.
 * This can be replaced by ACK polling later.
 */
#define EEPROM_WRITE_DELAY_MS   10

enum ax_eeprom_bus_type {
    AX_EEPROM_BUS_I2C = 0,
    AX_EEPROM_BUS_I3C,
};

struct ax_eeprom_dev {
    enum ax_eeprom_bus_type type;

    uint bus;
    uint addr;
};

static int ax_eeprom_read(struct ax_eeprom_dev *dev,
                          uint reg, uint len, u8 *buf)
{
    if (!dev || !buf)
        return -EINVAL;

    if (!len)
        return -EINVAL;

    switch (dev->type) {
    case AX_EEPROM_BUS_I2C:
        return ax_i2c_read(dev->bus, dev->addr,
                           reg, len,
                           buf);

    case AX_EEPROM_BUS_I3C:
        return ax_i3c_read(dev->bus, dev->addr,
                           reg, len,
                           buf);

    default:
        return -EINVAL;
    }
}

static int ax_eeprom_write(struct ax_eeprom_dev *dev,
                           uint reg, uint len, const u8 *buf)
{
    if (!dev || !buf)
        return -EINVAL;

    if (!len)
        return -EINVAL;

    switch (dev->type) {
    case AX_EEPROM_BUS_I2C:
        return ax_i2c_write(dev->bus, dev->addr,
                            reg, len,
                            (u8 *)buf);

    case AX_EEPROM_BUS_I3C:
        return ax_i3c_write(dev->bus, dev->addr,
                            reg, len,
                            (u8 *)buf);

    default:
        return -EINVAL;
    }
}

static int ax_eeprom_init_bus(struct ax_eeprom_dev *dev)
{
    int ret;

    if (!dev)
        return -EINVAL;

    switch (dev->type) {
        case AX_EEPROM_BUS_I2C:
            ret = ax_i2c_set_bus_speed(dev->bus, EEPROM_I2C_SPEED);
            if (ret)
                return ret;

            break;
        case AX_EEPROM_BUS_I3C:
            ret = ax_i3c_bus_init(dev->bus);
            if (ret)
                return ret;

            ret = ax_i3c_attach_dev(dev->bus, dev->addr, 11);
            if (ret)
                return ret;

            break;
        default:
            return -EINVAL;

    }

    return 0;
}

static void ax_eeprom_print_buffer(uint offset,
                                   const u8 *buf,
                                   uint len)
{
    uint i;

    for (i = 0; i < len; i++) {

        if ((i % 16) == 0)
            printf("%08x: ", offset + i);

        printf("%02x ", buf[i]);

        if ((i % 16) == 15 || i == len - 1)
            printf("\n");
    }
}

static int ax_eeprom_cmd_read(struct ax_eeprom_dev *dev,
                              uint offset,
                              uint len)
{
    u8 *buf;
    int ret;

    if (!len || len > EEPROM_MAX_RW_SIZE) {
        printf("Invalid read length: %u\n", len);
        return -EINVAL;
    }

    buf = malloc(len);
    if (!buf)
        return -ENOMEM;

    memset(buf, 0, len);

    ret = ax_eeprom_read(dev,
                         offset,
                         len,
                         buf);

    if (ret) {
        printf("EEPROM read failed: %d\n", ret);
        goto out;
    }

    ax_eeprom_print_buffer(offset,
                           buf,
                           len);

out:
    free(buf);

    return ret;
}

static int ax_eeprom_cmd_write(struct ax_eeprom_dev *dev,
                               uint offset,
                               int argc,
                               char *const argv[])
{
    u8 *buf;
    int len;
    int i;
    int ret;

    len = argc;

    if (!len)
        return -EINVAL;

    if (len > EEPROM_MAX_RW_SIZE) {
        printf("Write size too large: %d\n", len);
        return -EINVAL;
    }

    buf = malloc(len);
    if (!buf)
        return -ENOMEM;

    for (i = 0; i < len; i++)
        buf[i] = simple_strtoul(argv[i], NULL, 16);

    ret = ax_eeprom_write(dev,
                          offset,
                          len,
                          buf);

    if (ret) {
        printf("EEPROM write failed: %d\n", ret);
        goto out;
    }

    /*
     * EEPROM requires internal write cycle.
     * Usually 5~10 ms depending on device.
     */
    if (dev->type == AX_EEPROM_BUS_I2C)
        mdelay(EEPROM_WRITE_DELAY_MS);

    printf("EEPROM write success\n");

out:
    free(buf);

    return ret;
}

/*
 * memtest limits:
 * - The I2C HAL sends a single byte register address, so only the first
 *   256 bytes behind one slave address are reachable.
 * - The Cadence I2C FIFO is 16 bytes (1 byte reg + data), so transfers are
 *   split into chunks of at most EEPROM_MT_CHUNK bytes, never crossing an
 *   EEPROM page boundary.
 */
#define EEPROM_MT_ADDR_SPACE    0x100
#define EEPROM_MT_CHUNK         8
#define EEPROM_MT_DEF_PAGE      8

enum {
    EEPROM_MT_PAT_00 = 0,
    EEPROM_MT_PAT_FF,
    EEPROM_MT_PAT_55,
    EEPROM_MT_PAT_AA,
    EEPROM_MT_PAT_ADDR,     /* byte = offset */
    EEPROM_MT_PAT_NADDR,    /* byte = ~offset */
    EEPROM_MT_PAT_COUNT,
};

static const char *const eeprom_mt_pat_name[EEPROM_MT_PAT_COUNT] = {
    "0x00", "0xff", "0x55", "0xaa", "address", "~address",
};

static u8 eeprom_mt_pattern(int pat, uint off)
{
    switch (pat) {
    case EEPROM_MT_PAT_00:    return 0x00;
    case EEPROM_MT_PAT_FF:    return 0xff;
    case EEPROM_MT_PAT_55:    return 0x55;
    case EEPROM_MT_PAT_AA:    return 0xaa;
    case EEPROM_MT_PAT_ADDR:  return (u8)off;
    case EEPROM_MT_PAT_NADDR: return (u8)~off;
    default:                  return 0;
    }
}

/* Length of the next transfer at @off: <= chunk, never crosses a page. */
static uint eeprom_mt_xfer_len(uint off, uint end, uint page)
{
    uint chunk = min_t(uint, page, EEPROM_MT_CHUNK);
    uint page_left = page - (off % page);
    uint n = min3(chunk, page_left, end - off);

    return n;
}

static int eeprom_mt_write_range(struct ax_eeprom_dev *dev, uint offset,
                                 uint len, uint page, const u8 *buf)
{
    uint off = offset, end = offset + len, n;
    int ret;

    while (off < end) {
        n = eeprom_mt_xfer_len(off, end, page);
        ret = ax_eeprom_write(dev, off, n, buf + (off - offset));
        if (ret) {
            printf("  write failed at 0x%02x (len %u): %d\n", off, n, ret);
            return ret;
        }
        /* Wait for the internal write cycle */
        mdelay(EEPROM_WRITE_DELAY_MS);
        off += n;
    }

    return 0;
}

static int eeprom_mt_read_range(struct ax_eeprom_dev *dev, uint offset,
                                uint len, u8 *buf)
{
    uint off = offset, end = offset + len, n;
    int ret;

    while (off < end) {
        n = min_t(uint, EEPROM_MT_CHUNK, end - off);
        ret = ax_eeprom_read(dev, off, n, buf + (off - offset));
        if (ret) {
            printf("  read failed at 0x%02x (len %u): %d\n", off, n, ret);
            return ret;
        }
        off += n;
    }

    return 0;
}

/*
 * Non-destructive EEPROM memory test:
 *   backup -> for each pattern: write, read back, compare -> restore.
 */
static int ax_eeprom_cmd_memtest(struct ax_eeprom_dev *dev, uint offset,
                                 uint len, uint loops, uint page)
{
    u8 *backup, *wbuf, *rbuf;
    uint i, loop, errs = 0, errs_pat;
    int pat, ret, rret;

    if (!len || offset + len > EEPROM_MT_ADDR_SPACE) {
        printf("Invalid range 0x%x+0x%x (max 0x%x)\n",
               offset, len, EEPROM_MT_ADDR_SPACE);
        return -EINVAL;
    }

    if (!page || page > EEPROM_MT_ADDR_SPACE || (page & (page - 1))) {
        printf("Invalid page size %u (power of 2, <= %u)\n",
               page, EEPROM_MT_ADDR_SPACE);
        return -EINVAL;
    }

    backup = malloc(len);
    wbuf = malloc(len);
    rbuf = malloc(len);
    if (!backup || !wbuf || !rbuf) {
        ret = -ENOMEM;
        goto out_free;
    }

    printf("EEPROM memtest: %s bus %u addr 0x%02x range 0x%02x-0x%02x, "
           "page %u, %u loop(s)\n",
           dev->type == AX_EEPROM_BUS_I2C ? "i2c" : "i3c",
           dev->bus, dev->addr, offset, offset + len - 1, page, loops);

    /* Backup original content */
    ret = eeprom_mt_read_range(dev, offset, len, backup);
    if (ret) {
        printf("Backup read failed: %d\n", ret);
        goto out_free;
    }

    for (loop = 0; loop < loops; loop++) {
        for (pat = 0; pat < EEPROM_MT_PAT_COUNT; pat++) {
            if (ctrlc()) {
                printf("\nAborted by user\n");
                ret = -EINTR;
                goto out_restore;
            }

            for (i = 0; i < len; i++)
                wbuf[i] = eeprom_mt_pattern(pat, offset + i);

            printf("  loop %u pattern %-8s: ", loop, eeprom_mt_pat_name[pat]);

            ret = eeprom_mt_write_range(dev, offset, len, page, wbuf);
            if (ret)
                goto out_restore;

            memset(rbuf, ~wbuf[0], len);
            ret = eeprom_mt_read_range(dev, offset, len, rbuf);
            if (ret)
                goto out_restore;

            errs_pat = 0;
            for (i = 0; i < len; i++) {
                if (rbuf[i] == wbuf[i])
                    continue;
                if (errs_pat < 8)
                    printf("\n    mismatch @0x%02x: wrote 0x%02x read 0x%02x",
                           offset + i, wbuf[i], rbuf[i]);
                errs_pat++;
            }

            if (errs_pat) {
                printf("\n    FAIL (%u byte(s) mismatch)\n", errs_pat);
                errs += errs_pat;
            } else {
                printf("OK\n");
            }
        }
    }

out_restore:
    /* Always restore original content */
    rret = eeprom_mt_write_range(dev, offset, len, page, backup);
    if (!rret)
        rret = eeprom_mt_read_range(dev, offset, len, rbuf);
    if (!rret && memcmp(rbuf, backup, len))
        rret = -EIO;
    if (rret)
        printf("WARNING: restoring original EEPROM content failed: %d\n",
               rret);
    else
        printf("Original content restored\n");

    if (!ret && errs)
        ret = -EIO;
    if (!ret)
        ret = rret;

    printf("EEPROM memtest %s (%u error(s))\n", ret ? "FAILED" : "PASSED",
           errs);

out_free:
    free(backup);
    free(wbuf);
    free(rbuf);

    return ret;
}

static int do_ax_eeprom(struct cmd_tbl *cmdtp,
                        int flag,
                        int argc,
                        char *const argv[])
{
    struct ax_eeprom_dev dev;
    const char *cmd;
    char *endp;
    uint offset;
    uint len;
    int ret;

    if (argc < 6)
        return CMD_RET_USAGE;

    cmd = argv[1];

    if (!strncmp(argv[2], "i2c", 3)) {
        dev.type = AX_EEPROM_BUS_I2C;
    } else if (!strncmp(argv[2], "i3c", 3)) {
        dev.type = AX_EEPROM_BUS_I3C;
    } else {
        printf("Mode not found\n");
        return CMD_RET_USAGE;
    }

    dev.bus = simple_strtoul(argv[3], &endp, 10);
    dev.addr = simple_strtoul(argv[4], &endp, 16);
    offset = simple_strtoul(argv[5], &endp, 16);

    ret = ax_eeprom_init_bus(&dev);
    if (ret) {
        printf("EEPROM bus initialization failed: %d\n",
               ret);
        return CMD_RET_FAILURE;
    }

    if (!strcmp(cmd, "read") ||
        !strcmp(cmd, "dump")) {

        if (argc < 7)
            return CMD_RET_USAGE;

        len = simple_strtoul(argv[6], &endp, 16);

        if (*endp) {
            printf("Invalid length: %s\n", argv[6]);
            return CMD_RET_USAGE;
        }

        ret = ax_eeprom_cmd_read(&dev,
                                 offset,
                                 len);

        if (ret)
            return CMD_RET_FAILURE;

        return CMD_RET_SUCCESS;
    }

    if (!strcmp(cmd, "write")) {

        if (argc < 7)
            return CMD_RET_USAGE;

        ret = ax_eeprom_cmd_write(&dev,
                                  offset,
                                  argc - 6,
                                  &argv[6]);

        if (ret)
            return CMD_RET_FAILURE;

        return CMD_RET_SUCCESS;
    }

    if (!strcmp(cmd, "memtest")) {
        uint loops = 1;
        uint page = EEPROM_MT_DEF_PAGE;

        if (argc < 7)
            return CMD_RET_USAGE;

        len = simple_strtoul(argv[6], &endp, 16);
        if (*endp) {
            printf("Invalid length: %s\n", argv[6]);
            return CMD_RET_USAGE;
        }

        if (argc > 7)
            loops = simple_strtoul(argv[7], NULL, 10);
        if (argc > 8)
            page = simple_strtoul(argv[8], NULL, 10);
        if (!loops)
            loops = 1;

        ret = ax_eeprom_cmd_memtest(&dev, offset, len, loops, page);

        return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
    }

    printf("Unknown command: %s\n", cmd);

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
    ax_eeprom,
    32,
    1,
    do_ax_eeprom,
    "Axiado EEPROM diagnostics test",
    "read <i2c|i3c> <bus> <addr> <offset> <len>\n"
    "    - Read EEPROM data\n"
    "\n"
    "ax_eeprom dump <i2c|i3c> <bus> <addr> <offset> <len>\n"
    "    - Dump EEPROM data\n"
    "\n"
    "ax_eeprom write <i2c|i3c> <bus> <addr> <offset> <data...>\n"
    "    - Write EEPROM data\n"
    "\n"
    "ax_eeprom memtest <i2c|i3c> <bus> <addr> <offset> <len> [loops] [page]\n"
    "    - Non-destructive memory test (backup, write/verify patterns\n"
    "      0x00/0xff/0x55/0xaa/addr/~addr, restore). offset/len in hex,\n"
    "      offset+len <= 0x100; loops (default 1) and page size in bytes\n"
    "      (default 8) in decimal\n"
);
