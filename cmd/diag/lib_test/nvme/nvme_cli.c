#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <string.h>

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
#define NVME_TEST_BUF_ADDR   0x90010000ULL
#define NVME_TEST_BUF_SIZE   (256U * 1024U)
#define NVME_TEST_DUMP_SIZE  64U

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
);
