// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <command.h>
#include <dm.h>
#include <errno.h>
#include <time.h>
#include <console.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <string.h>

#include "ax_diag.h"
#include "ax_pcie.h"
#include "ax_nvme.h"

/*
 * NVMe test buffer.
 *
 * Must be:
 * - Reserved from other firmware components.
 * - Accessible by CPU and NVMe PCIe DMA.
 * - Identity-mapped: CPU address == PCIe bus address.
 *
 * Existing NVMe queues occupy 0x90000000 - 0x90005fff.
 */
#define NVME_TEST_BUF_ADDR   (AX3000_MEM_BASE_ADRS_DDR_LO + 0x40000000UL + 0x10000)
#define NVME_TEST_BUF_SIZE   (256U * 1024U)
#define NVME_TEST_DUMP_SIZE  64U

#define NVME_SPEED_DEFAULT_DURATION  60U
#define NVME_SPEED_REPORT_MS         1000U

static struct ax_nvme_dev ndev;
static bool nvme_initialized;

/* -------------------------------------------------------
 * Helpers
 * ------------------------------------------------------- */

static int nvme_parse_u64(const char *s, u64 *value)
{
    char *endp;
    unsigned long long v;

    if (!s || !*s || !value)
        return -EINVAL;

    v = simple_strtoull(s, &endp, 0);
    if (*endp != '\0')
        return -EINVAL;

    *value = (u64)v;
    return 0;
}

static int nvme_check_io(u64 lba, u32 blocks, u32 *bytes)
{
    u64 len;
    u32 block_size;

    if (!nvme_initialized) {
        printf("NVMe: run init first\n");
        return -ENODEV;
    }

    if (!blocks)
        return -EINVAL;

    if (ndev.lba_shift < 9 || ndev.lba_shift > 16)
        return -EINVAL;

    block_size = 1U << ndev.lba_shift;

    if (blocks > NVME_TEST_BUF_SIZE / block_size) {
        printf("NVMe: test buffer limit exceeded\n");
        return -EINVAL;
    }

    len = (u64)blocks * block_size;

    if (lba >= ndev.nsze || len / block_size != blocks ||
        blocks > ndev.nsze - lba) {
        printf("NVMe: LBA range out of bounds\n");
        return -ERANGE;
    }

    if (bytes)
        *bytes = (u32)len;

    return 0;
}

/*
 * Deterministic test pattern.
 * The same LBA and byte offset always produce the same data.
 */
static u8 nvme_pattern_byte(u64 lba, u32 offset)
{
    return (u8)((offset * 37U + (u32)lba +
                 (offset >> 8) + 0x5aU) & 0xffU);
}

static void nvme_fill_pattern(u8 *buf, u32 bytes, u64 lba)
{
    u32 i;

    for (i = 0; i < bytes; i++)
        buf[i] = nvme_pattern_byte(lba, i);
}

static int nvme_compare_pattern(const u8 *buf, u32 bytes, u64 lba)
{
    u32 i;

    for (i = 0; i < bytes; i++) {
        u8 expected = nvme_pattern_byte(lba, i);

        if (buf[i] != expected) {
            printf("COMPARE FAIL: offset=0x%x "
                   "expected=0x%02x actual=0x%02x\n",
                   i, expected, buf[i]);
            printf("LBA=%llu, LBA offset=%u\n",
                   lba, i >> ndev.lba_shift);
            return -EIO;
        }
    }

    return 0;
}

static u32 nvme_checksum(const u8 *buf, u32 bytes)
{
    u32 i;
    u32 sum = 0;

    for (i = 0; i < bytes; i++)
        sum = (sum << 5) - sum + buf[i];

    return sum;
}

/* -------------------------------------------------------
 * init
 * ------------------------------------------------------- */

static int ax_nvme_cmd_init(struct cmd_tbl *cmdtp, int flag,
                            int argc, char *const argv[])
{
    u64 port;
    int ret;

    if (argc < 3)
        return CMD_RET_USAGE;

    if (nvme_parse_u64(argv[2], &port) ||
        port > 0xff)
        return CMD_RET_USAGE;

    nvme_initialized = false;

    ret = ax_nvme_probe(&ndev, (u8)port, 1, 0, 0);
    if (ret) {
        printf("NVMe_%llu init failed: %d\n", port, ret);
        return CMD_RET_FAILURE;
    }

    nvme_initialized = true;

    printf("NVMe_%llu init success\n", port);
    printf("Namespace: %u\n", ndev.nsid);
    printf("Capacity: %llu blocks\n", ndev.nsze);
    printf("Block size: %u bytes\n", 1U << ndev.lba_shift);
    printf("Test buffer: 0x%llx, size %u bytes\n",
           (u64)NVME_TEST_BUF_ADDR, NVME_TEST_BUF_SIZE);

    return CMD_RET_SUCCESS;
}

/* -------------------------------------------------------
 * read <lba> <blocks>
 * ------------------------------------------------------- */

static int ax_nvme_cmd_read(struct cmd_tbl *cmdtp, int flag,
                            int argc, char *const argv[])
{
    u64 lba;
    u64 blocks64;
    u8 *buf = (u8 *)(ulong)NVME_TEST_BUF_ADDR;
    u32 blocks, bytes, i;
    int ret;

    if (argc != 4)
        return CMD_RET_USAGE;

    if (nvme_parse_u64(argv[2], &lba) ||
        nvme_parse_u64(argv[3], &blocks64) ||
        blocks64 > 0xffffffffULL)
        return CMD_RET_USAGE;

    blocks = (u32)blocks64;

    ret = nvme_check_io(lba, blocks, &bytes);
    if (ret)
        return CMD_RET_FAILURE;

    printf("NVMe READ: LBA=%llu blocks=%u bytes=%u\n",
           lba, blocks, bytes);

    ret = ax_nvme_read(&ndev, ndev.nsid, lba, blocks,
                       buf, NVME_TEST_BUF_ADDR);
    if (ret) {
        printf("NVMe READ failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    printf("Read success, checksum=0x%08x\n",
           nvme_checksum(buf, bytes));

    printf("First %u bytes:\n",
           bytes < NVME_TEST_DUMP_SIZE ?
           bytes : NVME_TEST_DUMP_SIZE);

    for (i = 0; i < bytes && i < NVME_TEST_DUMP_SIZE; i++) {
        if ((i & 0x0f) == 0)
            printf("%08x: ", i);

        printf("%02x ", buf[i]);

        if ((i & 0x0f) == 0x0f || i == bytes - 1)
            printf("\n");
    }

    return CMD_RET_SUCCESS;
}

/* -------------------------------------------------------
 * write <lba> <blocks>
 *
 * Destructive: writes a deterministic test pattern.
 * ------------------------------------------------------- */

static int ax_nvme_cmd_write(struct cmd_tbl *cmdtp, int flag,
                             int argc, char *const argv[])
{
    u64 lba;
    u64 blocks64;
    u8 *buf = (u8 *)(ulong)NVME_TEST_BUF_ADDR;
    u32 blocks, bytes;
    int ret;

    if (argc != 4)
        return CMD_RET_USAGE;

    if (nvme_parse_u64(argv[2], &lba) ||
        nvme_parse_u64(argv[3], &blocks64) ||
        blocks64 > 0xffffffffULL)
        return CMD_RET_USAGE;

    blocks = (u32)blocks64;

    ret = nvme_check_io(lba, blocks, &bytes);
    if (ret)
        return CMD_RET_FAILURE;

    printf("NVMe WRITE: LBA=%llu blocks=%u bytes=%u\n",
           lba, blocks, bytes);
    printf("WARNING: existing data in this range will be overwritten\n");

    nvme_fill_pattern(buf, bytes, lba);

    ret = ax_nvme_write(&ndev, ndev.nsid, lba, blocks,
                        buf, NVME_TEST_BUF_ADDR);
    if (ret) {
        printf("NVMe WRITE failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    printf("Write success, pattern checksum=0x%08x\n",
           nvme_checksum(buf, bytes));

    return CMD_RET_SUCCESS;
}

/* -------------------------------------------------------
 * compare <lba> <blocks>
 *
 * Destructive:
 *   1. Write pattern
 *   2. Read the same LBAs
 *   3. Compare every byte
 * ------------------------------------------------------- */

static int ax_nvme_cmd_compare(struct cmd_tbl *cmdtp, int flag,
                               int argc, char *const argv[])
{
    u64 lba;
    u64 blocks64;
    u8 *buf = (u8 *)(ulong)NVME_TEST_BUF_ADDR;
    u32 blocks, bytes;
    int ret;

    if (argc != 4)
        return CMD_RET_USAGE;

    if (nvme_parse_u64(argv[2], &lba) ||
        nvme_parse_u64(argv[3], &blocks64) ||
        blocks64 > 0xffffffffULL)
        return CMD_RET_USAGE;

    blocks = (u32)blocks64;

    ret = nvme_check_io(lba, blocks, &bytes);
    if (ret)
        return CMD_RET_FAILURE;

    printf("NVMe COMPARE: LBA=%llu blocks=%u bytes=%u\n",
           lba, blocks, bytes);
    printf("WARNING: this test overwrites existing SSD data\n");

    /* Generate expected contents and write to the SSD. */
    nvme_fill_pattern(buf, bytes, lba);

    ret = ax_nvme_write(&ndev, ndev.nsid, lba, blocks,
                        buf, NVME_TEST_BUF_ADDR);
    if (ret) {
        printf("NVMe COMPARE: WRITE failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    /*
     * Clear the CPU buffer before reading.
     * ax_nvme_read() handles DMA cache maintenance.
     */
    memset(buf, 0, bytes);

    ret = ax_nvme_read(&ndev, ndev.nsid, lba, blocks,
                       buf, NVME_TEST_BUF_ADDR);
    if (ret) {
        printf("NVMe COMPARE: READ failed: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    ret = nvme_compare_pattern(buf, bytes, lba);
    if (ret) {
        printf("NVMe COMPARE FAILED\n");
        return CMD_RET_FAILURE;
    }

    printf("NVMe COMPARE PASS: %u bytes verified\n", bytes);
    printf("Read checksum=0x%08x\n",
           nvme_checksum(buf, bytes));

    return CMD_RET_SUCCESS;
}

/* -------------------------------------------------------
 * speed <lba> <blocks> [duration_s]
 *
 * Default duration: 60 seconds.
 * duration_s = 0: run until Ctrl-C.
 *
 * Destructive: repeatedly writes the selected LBA range.
 * ------------------------------------------------------- */

static void nvme_print_speed(const char *name, u64 total_bytes,
                             u64 elapsed_ms)
{
    u64 speed_kbs;

    if (!elapsed_ms) {
        printf("%s: elapsed time below timer resolution\n", name);
        return;
    }

    /*
     * MB/s = bytes / (seconds * 1,000,000).
     * Use integer arithmetic to avoid floating-point dependencies.
     */
    speed_kbs = total_bytes / elapsed_ms;

    printf("%s: %llu bytes, %llu ms, ",
           name, total_bytes, elapsed_ms);
    printf("%llu.%03llu MB/s\n",
           speed_kbs / 1000ULL,
           speed_kbs % 1000ULL);
}

static int ax_nvme_cmd_speed(struct cmd_tbl *cmdtp, int flag,
                             int argc, char *const argv[])
{
    u64 lba, blocks64;
    u64 total_write_bytes = 0;
    u64 total_read_bytes = 0;
    u64 write_time_ms = 0;
    u64 read_time_ms = 0;
    u64 last_write_bytes = 0;
    u64 last_read_bytes = 0;
    u64 last_write_time_ms = 0;
    u64 last_read_time_ms = 0;
    u32 blocks, bytes;
    u32 duration_s = NVME_SPEED_DEFAULT_DURATION;
    u8 *buf = (u8 *)(ulong)NVME_TEST_BUF_ADDR;
    ulong test_start, report_start, io_start, elapsed;
    ulong io_elapsed;
    int ret;
    bool interrupted = false;

    if (argc != 4 && argc != 5)
        return CMD_RET_USAGE;

    if (nvme_parse_u64(argv[2], &lba) ||
        nvme_parse_u64(argv[3], &blocks64) ||
        blocks64 > 0xffffffffULL)
        return CMD_RET_USAGE;

    if (argc == 5) {
        u64 duration64;

        if (nvme_parse_u64(argv[4], &duration64) ||
            duration64 > 0xffffffffULL)
            return CMD_RET_USAGE;

        duration_s = (u32)duration64;
    }

    blocks = (u32)blocks64;

    ret = nvme_check_io(lba, blocks, &bytes);
    if (ret)
        return CMD_RET_FAILURE;

    if (ndev.max_xfer_bytes &&
        bytes > ndev.max_xfer_bytes) {
        printf("NVMe speed: transfer exceeds controller limit\n");
        return CMD_RET_FAILURE;
    }

    printf("\nNVMe CONTINUOUS SPEED TEST\n");
    printf("LBA=%llu blocks=%u bytes=%u\n", lba, blocks, bytes);
    printf("Duration: %s\n",
           duration_s ? "limited" : "until Ctrl-C");
    if (duration_s)
        printf("Time limit: %u seconds\n", duration_s);

    printf("WARNING: overwrites the selected LBA range\n");
    printf("Press Ctrl-C to stop\n\n");

    /* Generate the pattern outside the timed I/O loop. */
    nvme_fill_pattern(buf, bytes, lba);

    test_start = get_timer(0);
    report_start = test_start;

    while (1) {
        /*
         * Ctrl-C is checked between synchronous NVMe commands.
         * It cannot abort a command already blocked in the driver.
         */
        if (ctrlc()) {
            interrupted = true;
            break;
        }

        elapsed = get_timer(test_start);

        if (duration_s && elapsed >= (ulong)duration_s * 1000UL)
            break;

        /* Write one transfer. */
        io_start = get_timer(0);

        ret = ax_nvme_write(&ndev, ndev.nsid, lba, blocks,
                            buf, NVME_TEST_BUF_ADDR);

        io_elapsed = get_timer(io_start);
        write_time_ms += io_elapsed;

        if (ret) {
            printf("\nNVMe speed: WRITE failed: %d\n", ret);
            return CMD_RET_FAILURE;
        }

        total_write_bytes += bytes;

        if (ctrlc()) {
            interrupted = true;
            break;
        }

        /* Read one transfer. */
        io_start = get_timer(0);

        ret = ax_nvme_read(&ndev, ndev.nsid, lba, blocks,
                           buf, NVME_TEST_BUF_ADDR);

        io_elapsed = get_timer(io_start);
        read_time_ms += io_elapsed;

        if (ret) {
            printf("\nNVMe speed: READ failed: %d\n", ret);
            return CMD_RET_FAILURE;
        }

        total_read_bytes += bytes;

        /* Verify data outside the measured read interval. */
        ret = nvme_compare_pattern(buf, bytes, lba);
        if (ret) {
            printf("\nNVMe speed: data verification FAILED\n");
            return CMD_RET_FAILURE;
        }

        elapsed = get_timer(test_start);

        if (elapsed - report_start >= NVME_SPEED_REPORT_MS) {
            u64 delta_write = total_write_bytes - last_write_bytes;
            u64 delta_read = total_read_bytes - last_read_bytes;
            u64 delta_write_ms = write_time_ms - last_write_time_ms;
            u64 delta_read_ms = read_time_ms - last_read_time_ms;

            printf("[%lu s] ", elapsed / 1000UL);

            if (delta_write_ms)
                printf("Write=%llu.%03llu MB/s ",
                       (delta_write / delta_write_ms) / 1000ULL,
                       (delta_write / delta_write_ms) % 1000ULL);
            else
                printf("Write=N/A ");

            if (delta_read_ms)
                printf("Read=%llu.%03llu MB/s",
                       (delta_read / delta_read_ms) / 1000ULL,
                       (delta_read / delta_read_ms) % 1000ULL);
            else
                printf("Read=N/A");

            printf("\n");

            last_write_bytes = total_write_bytes;
            last_read_bytes = total_read_bytes;
            last_write_time_ms = write_time_ms;
            last_read_time_ms = read_time_ms;
            report_start = elapsed;
        }

        if (duration_s &&
            elapsed >= (ulong)duration_s * 1000UL)
            break;
    }

    elapsed = get_timer(test_start);

    printf("\n--- NVMe SPEED TEST SUMMARY ---\n");
    printf("Status: %s\n", interrupted ? "Stopped by Ctrl-C" :
           "Duration completed");
    printf("Elapsed: %lu.%03lu seconds\n",
           elapsed / 1000UL, elapsed % 1000UL);
    printf("Write total: %llu bytes\n", total_write_bytes);
    printf("Read total : %llu bytes\n", total_read_bytes);

    /*
     * These rates use accumulated NVMe call time, excluding
     * pattern verification and console output.
     */
    nvme_print_speed("WRITE", total_write_bytes, write_time_ms);
    nvme_print_speed("READ ", total_read_bytes, read_time_ms);

    printf("Data integrity: PASS\n");
    printf("Final checksum: 0x%08x\n",
           nvme_checksum(buf, bytes));

    return CMD_RET_SUCCESS;
}

/* -------------------------------------------------------
 * Command dispatcher
 * ------------------------------------------------------- */

int do_ax_nvme(struct cmd_tbl *cmdtp, int flag,
               int argc, char *const argv[])
{
    if (argc < 2)
        return CMD_RET_USAGE;

    if (!strcmp(argv[1], "init"))
        return ax_nvme_cmd_init(cmdtp, flag, argc, argv);

    if (!strcmp(argv[1], "read"))
        return ax_nvme_cmd_read(cmdtp, flag, argc, argv);

    if (!strcmp(argv[1], "write"))
        return ax_nvme_cmd_write(cmdtp, flag, argc, argv);

    if (!strcmp(argv[1], "compare"))
        return ax_nvme_cmd_compare(cmdtp, flag, argc, argv);

    if (!strcmp(argv[1], "speed"))
        return ax_nvme_cmd_speed(cmdtp, flag, argc, argv);

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
        ax_nvme, 8, 1, do_ax_nvme,
        "Axiado NVMe diagnostic commands",
        "init <port> [gen]\n"
        "    - Initialize NVMe controller and I/O queue\n"
        "ax_nvme read <lba> <blocks>\n"
        "    - Read blocks and display checksum/data\n"
        "ax_nvme write <lba> <blocks>\n"
        "    - Write deterministic test pattern (DESTRUCTIVE)\n"
        "ax_nvme compare <lba> <blocks>\n"
        "    - Write, read back and compare every byte (DESTRUCTIVE)\n"
        "ax_nvme speed <lba> <blocks> [duration_s]\n"
        "    - Continuous NVMe read/write speed test (default 60s)\n"
        "    - duration_s=0 runs until Ctrl-C (DESTRUCTIVE)\n"
        );
