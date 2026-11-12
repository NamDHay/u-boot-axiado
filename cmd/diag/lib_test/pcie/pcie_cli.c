// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */
#include <command.h>
#include <dm.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <display_options.h>
#include <console.h>
#include <linux/sizes.h>

#include "ax_pcie.h"
#include "ax_pcie_conf.h"

#define AX_PCIE_MODE_EP		0
#define AX_PCIE_MODE_RC		1

static struct ax_pcie_msi msi;

static int ax_pcie_parse_bdf(const char *str, u8 *bus, u8 *dev, u8 *func)
{
    char *end;
    ulong val;

    /* Bus */
    val = simple_strtoul(str, &end, 16);
    if (*end != '.')
        return -EINVAL;

    if (val > 0xff)
        return -EINVAL;

    *bus = val;

    /* Device */
    str = end + 1;
    val = simple_strtoul(str, &end, 16);
    if (*end != '.')
        return -EINVAL;

    if (val > 0x1f)
        return -EINVAL;

    *dev = val;

    /* Function */
    str = end + 1;
    val = simple_strtoul(str, &end, 16);

    if (*end != '\0')
        return -EINVAL;

    if (val > 0x7)
        return -EINVAL;

    *func = val;

    return 0;
}

static void pcie_cli_msi_handle(void *arg)
{
    struct ax_pcie_msi *msi = arg;
    printf("%s - Receive MSI%d: \n\taddr = 0x%llx, \n\tdata = 0x%x\n", 
            __func__, msi->vector, msi->address, msi->data);
}

/*
 * ax_pcie init <port> <speed>
 */
static int ax_pcie_cmd_init(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int ret;
    unsigned int port;
    unsigned int mode;
    unsigned int speed;
    u8 dev, func;
    char *endp;

    if (argc < 3)
        return CMD_RET_USAGE;

    port = simple_strtol(argv[1], &endp, 16);
    speed = simple_strtol(argv[2], &endp, 16);
    mode = AX_PCIE_MODE_RC;

    ret = ax_pcie_init(port, mode, speed);
    if (ret) {
        printf("PCIe_x%d init failed: %d\n", port, ret);
        return CMD_RET_FAILURE;
    }

    printf("PCIe_x%d init successed\n", port);

    for (dev = 0; dev < PCI_MAX_PCI_DEVICES; dev++) {
        for (func = 0; func < PCI_MAX_PCI_FUNCTIONS; func++) {
            ret = ax_pcie_init_ep_bars(port, 1, dev, func);
            if (ret)
                continue;

            ret = ax_pcie_ep_enable_aer(port, 1, dev, func);
            if (ret) {
                printf("Failed to enable AER: %d\n", ret);
                return CMD_RET_FAILURE;
            }
            printf("AER enable\n");

            ax_pcie_print_header(port, 1, dev, func);

            printf("\nPCIe BAR initialization for %02x:%02x.%x\n",
                    1, dev, func);
        }
    }

    return CMD_RET_SUCCESS;
}

/*
 * ax_pcie link <port>
 */
static int ax_pcie_cmd_link(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int ret;
    u8 bus, dev, func;
    unsigned int port;
    char *endp;

    if (argc < 3)
        return CMD_RET_USAGE;

    port = simple_strtol(argv[1], &endp, 16);

    ret = ax_pcie_parse_bdf(argv[2], &bus, &dev, &func);
    if (ret) {
        printf("Invalid BDF: %s\n", argv[2]);
        return CMD_RET_USAGE;
    }

    ret = ax_pcie_init_ep_bars(port, bus, dev, func);
    if (ret) {
        printf("Failed to initialize endpoint BARs: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    ax_pcie_print_header(port, bus, dev, func);

    return CMD_RET_SUCCESS;
}

static int ax_pcie_cmd_cfg_read(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int port;
    u8 bus, dev, func;
    u16 offset;
    u32 val;
    int ret;

    if (argc != 4)
        return CMD_RET_USAGE;

    port = simple_strtoul(argv[1], NULL, 0);

    ret = ax_pcie_parse_bdf(argv[2], &bus, &dev, &func);
    if (ret) {
        printf("Invalid BDF: %s\n", argv[2]);
        return CMD_RET_USAGE;
    }

    offset = simple_strtoul(argv[3], NULL, 0);

    ret = ax_pcie_cfg_read(port, bus, dev, func, offset, &val);
    if (ret) {
        printf("PCIe%d cfg_read %02x.%x.%x offset 0x%04x failed: %d\n",
                port, bus, dev, func, offset, ret);
        return CMD_RET_FAILURE;
    }

    printf("PCIe%d %02x.%x.%x [0x%04x] = 0x%08x\n",
            port, bus, dev, func, offset, val);

    return CMD_RET_SUCCESS;
}

static int ax_pcie_cmd_cfg_write(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int port;
    u8 bus, dev, func;
    u16 offset;
    u32 val;
    int ret;

    if (argc != 5)
        return CMD_RET_USAGE;

    port = simple_strtoul(argv[1], NULL, 0);

    ret = ax_pcie_parse_bdf(argv[2], &bus, &dev, &func);
    if (ret) {
        printf("Invalid BDF: %s\n", argv[2]);
        return CMD_RET_USAGE;
    }

    offset = simple_strtoul(argv[3], NULL, 0);
    val = simple_strtoul(argv[4], NULL, 0);

    ret = ax_pcie_cfg_write(port, bus, dev, func, offset, val);
    if (ret) {
        printf("PCIe%d cfg_write %02x.%x.%x offset 0x%04x failed: %d\n",
                port, bus, dev, func, offset, ret);
        return CMD_RET_FAILURE;
    }

    printf("PCIe%d %02x.%x.%x [0x%04x] <= 0x%08x\n",
            port, bus, dev, func, offset, val);

    return CMD_RET_SUCCESS;
}

static int ax_pcie_cmd_mem_read(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    u8 port;
    u64 offset;
    size_t len;
    void *buf;
    int ret;

    if (argc != 4)
        return CMD_RET_USAGE;

    port = simple_strtoul(argv[1], NULL, 0);
    offset = simple_strtoull(argv[2], NULL, 0);
    len = simple_strtoul(argv[3], NULL, 0);

    if (len == 0) {
        printf("Invalid length: 0\n");
        return CMD_RET_USAGE;
    }

    buf = malloc(len);
    if (!buf) {
        printf("Failed to allocate %zu bytes\n", len);
        return CMD_RET_FAILURE;
    }

    memset(buf, 0, len);

    ret = ax_pcie_mem_read(port, offset, buf, len);
    if (ret) {
        printf("PCIe%d mem_read offset 0x%llx len 0x%zx failed: %d\n",
                port, offset, len, ret);
        free(buf);
        return CMD_RET_FAILURE;
    }

    printf("PCIe%d MEM[0x%llx] len 0x%zx:\n",
            port, offset, len);

    print_buffer((ulong)offset, buf, 1, len, 16);

    free(buf);

    return CMD_RET_SUCCESS;
}

static int ax_pcie_cmd_mem_write(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    u8 port;
    u64 offset;
    u64 value;
    size_t len = sizeof(u32);
    void *buf;
    int ret;

    if (argc != 4 && argc != 5)
        return CMD_RET_USAGE;

    port = simple_strtoul(argv[1], NULL, 0);
    offset = simple_strtoull(argv[2], NULL, 0);
    value = simple_strtoull(argv[3], NULL, 0);

    if (argc == 5)
        len = simple_strtoul(argv[4], NULL, 0);

    if (len == 0 || len > sizeof(value)) {
        printf("Invalid length: %zu (valid range: 1-%zu)\n",
                len, sizeof(value));
        return CMD_RET_USAGE;
    }

    buf = calloc(1, len);
    if (!buf) {
        printf("Failed to allocate %zu bytes\n", len);
        return CMD_RET_FAILURE;
    }

    memcpy(buf, &value, len);

    ret = ax_pcie_mem_write(port, offset, buf, len);
    if (ret) {
        printf("PCIe%d mem_write offset 0x%llx len 0x%zx "
                "value 0x%llx failed: %d\n",
                port, offset, len, value, ret);
        free(buf);
        return CMD_RET_FAILURE;
    }

    printf("PCIe%d MEM[0x%llx] <= 0x%0*llx (%zu bytes)\n",
            port, offset,
            (int)(len * 2), value, len);

    free(buf);

    return CMD_RET_SUCCESS;
}

/*
 * ax_pcie header <port> <bus>.<dev>.<func>
 */
static int ax_pcie_cmd_header(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int port;
    u8 bus, dev, func;
    int ret;

    if (argc != 3)
        return CMD_RET_USAGE;

    port = simple_strtoul(argv[1], NULL, 0);

    ret = ax_pcie_parse_bdf(argv[2], &bus, &dev, &func);
    if (ret) {
        printf("Invalid BDF: %s\n", argv[2]);
        return CMD_RET_USAGE;
    }

    ax_pcie_print_header(port, bus, dev, func);

    return CMD_RET_SUCCESS;
}

/*
 * ax_pcie cap <port> <bus>.<dev>.<func>
 */
static int ax_pcie_cmd_cap(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int port;
    u8 bus, dev, func;
    int ret;

    if (argc != 3)
        return CMD_RET_USAGE;

    port = simple_strtoul(argv[1], NULL, 0);

    ret = ax_pcie_parse_bdf(argv[2], &bus, &dev, &func);
    if (ret) {
        printf("Invalid BDF: %s\n", argv[2]);
        return CMD_RET_USAGE;
    }

    ax_pcie_print_capabilities(port, bus, dev, func);

    return CMD_RET_SUCCESS;
}

/*
 * ax_pcie debug [-v|-vv] <port> <bus>.<dev>.<num>
 */
static int ax_pcie_cmd_debug(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int port;
    u8 bus, dev, func;
    int ret;

    if (argc != 4)
        return CMD_RET_USAGE;

    port = simple_strtoul(argv[2], NULL, 0);
    ret = ax_pcie_parse_bdf(argv[3], &bus, &dev, &func);

    ax_pcie_debug_print(port);

    if(argc > 2) {
        if(!strcmp(argv[1], "-v"))
            ax_pcie_print_header(port, bus, dev, func);

        if(!strcmp(argv[1], "-vv")) {
            ax_pcie_print_header(port, bus, dev, func);
            ax_pcie_print_capabilities(port, bus, dev, func);
        }
    }

    return CMD_RET_SUCCESS;
}

/*
 * ax_pcie msi [enable|disable|status] <port> <bus>.<dev>.<func>
 */
static int ax_pcie_cmd_msi(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int port;
    u8 bus, dev, func;
    int ret;

    if (argc != 4)
        return CMD_RET_USAGE;

    port = simple_strtoul(argv[2], NULL, 0);
    ret = ax_pcie_parse_bdf(argv[3], &bus, &dev, &func);
    if (ret) {
        printf("Invalid BDF: %s\n", argv[3]);
        return CMD_RET_USAGE;
    }

    if (!strncmp(argv[1], "s", 1)) {
        ret = ax_pcie_ep_msi_status(port, bus, dev, func);
        if (ret)
            return CMD_RET_FAILURE;

        return CMD_RET_SUCCESS;
    }

    if (!strncmp(argv[1], "e", 1)) {
        printf("PCIe MSI enable: %02x:%02x.%x\n",
                bus, dev, func);

        ret = ax_pcie_ep_disable_msi(port, bus, dev, func);
        if (ret) {
            printf("Failed to disable MSI: %d\n", ret);
            return CMD_RET_FAILURE;
        }

        printf("MSI disabled before program address and data\n");

        /*
         * Program MSI Message Address/Data.
         */
        msi.handler = pcie_cli_msi_handle;
        msi.arg = &msi;
        msi.nr_vectors = 16;
        ret = ax_pcie_ep_program_msi(port, 1, dev, func, &msi);
        if (ret) {
            printf("Failed to program MSI: %d\n", ret);
            return CMD_RET_FAILURE;
        }

        /*
         * Enable MSI.
         */
        ret = ax_pcie_ep_enable_msi(port, bus, dev, func);
        if (ret) {
            printf("Failed to enable MSI: %d\n", ret);
            return CMD_RET_FAILURE;
        }

        printf("MSI enable\n");

        return CMD_RET_SUCCESS;
    }

    if (!strncmp(argv[1], "d", 1)) {
        printf("PCIe MSI disable: %02x:%02x.%x\n",
                bus, dev, func);

        ret = ax_pcie_ep_disable_msi(port, bus, dev, func);
        if (ret) {
            printf("Failed to disable MSI: %d\n", ret);
            return CMD_RET_FAILURE;
        }

        printf("MSI disabled\n");

        /* Hand this function's RP vectors back for the next enable */
        ax_pcie_ep_release_msi(port, bus, dev, func);

        return CMD_RET_SUCCESS;
    }

    return CMD_RET_USAGE;
}

/*
 * ---------------------------------------------------------------------------
 * EP BAR read/write test
 * ---------------------------------------------------------------------------
 *
 * Every memory BAR of every endpoint function is accessed from the root
 * port through the outbound window.  On the EP side each BAR is translated
 * (inbound ATU) to its own DDR buffer, so:
 *
 *  1. Data bus test   - walking 1/0 and 0x55/0xAA patterns on word 0,
 *                       catches stuck / shorted data bits.
 *  2. Byte lane test  - byte writes, 32-bit read back,
 *                       catches broken byte enables.
 *  3. Address test    - fill ALL BARs of ALL functions with an address
 *                       unique pattern, then verify all of them (and again
 *                       with the inverted pattern).  Because every BAR is
 *                       written before any is checked, two BARs/functions
 *                       translated to the same DDR, or a wrong/short
 *                       inbound window, show up as mismatches.
 *
 * The test is destructive for the EP DDR behind the BARs.
 */

#define AX_PCIE_BT_MAX_BARS	64
#define AX_PCIE_BT_CHUNK	SZ_4K
#define AX_PCIE_BT_MAX_ERRS	8
#define AX_PCIE_BT_NBARS	6

struct ax_pcie_bt_bar {
    u8 bus;
    u8 dev;
    u8 func;
    u8 index;
    bool is_64bit;
    bool prefetch;
    u64 addr;	/* PCI bus address programmed in the BAR */
    u64 size;	/* BAR size */
    u64 len;	/* bytes under test */
    u64 off;	/* offset for ax_pcie_mem_read/write */
    u32 tag;	/* per-BAR pattern salt */
    u32 errors;
};

static struct ax_pcie_bt_bar ax_pcie_bt_bars[AX_PCIE_BT_MAX_BARS];

/*
 * Convert a PCI bus address into the offset ax_pcie_mem_read/write expect
 * (relative to the first outbound region's CPU address).
 */
static int ax_pcie_bt_bus_to_off(u8 port, u64 bus_addr, u64 len, u64 *off)
{
    struct ax_pci_region *regions;
    int cnt, i, ret;
    u64 phys0;

    ret = ax_pcie_get_regions(port, &regions, &cnt);
    if (ret)
        return ret;

    if (!cnt)
        return -ENODEV;

    phys0 = (u64)regions[0].phys_start;

    for (i = 0; i < cnt; i++) {
        u64 base = (u64)regions[i].bus_start;
        u64 size = regions[i].size;

        if (!size || bus_addr < base || bus_addr - base + len > size)
            continue;

        *off = (bus_addr - base) + ((u64)regions[i].phys_start - phys0);
        return 0;
    }

    return -ERANGE;
}

static inline u32 ax_pcie_bt_pattern(const struct ax_pcie_bt_bar *b,
        u64 off, u32 seed)
{
    u64 a = b->addr + off;

    return ((u32)a ^ (u32)(a >> 32) ^ b->tag) ^ seed;
}

static void ax_pcie_bt_report(struct ax_pcie_bt_bar *b, const char *test,
        u64 off, u32 exp, u32 got)
{
    if (b->errors++ < AX_PCIE_BT_MAX_ERRS)
        printf("    [%02x:%02x.%x BAR%d] %s: bus 0x%llx exp 0x%08x got 0x%08x (xor 0x%08x)\n",
                b->bus, b->dev, b->func, b->index, test,
                b->addr + off, exp, got, exp ^ got);
}

static int ax_pcie_bt_wr32(u8 port, struct ax_pcie_bt_bar *b, u64 off, u32 v)
{
    return ax_pcie_mem_write(port, b->off + off, &v, sizeof(v));
}

static int ax_pcie_bt_rd32(u8 port, struct ax_pcie_bt_bar *b, u64 off, u32 *v)
{
    return ax_pcie_mem_read(port, b->off + off, v, sizeof(*v));
}

/* Walking 1/0 + fixed patterns on the first word of the BAR */
static int ax_pcie_bt_data_bus(u8 port, struct ax_pcie_bt_bar *b)
{
    static const u32 fixed[] = {
        0x00000000, 0xffffffff, 0x55555555, 0xaaaaaaaa,
        0x5a5aa5a5, 0xa5a55a5a, 0x01234567, 0xfedcba98,
    };
    u32 exp, got;
    int i, ret;

    for (i = 0; i < 64 + (int)ARRAY_SIZE(fixed); i++) {
        if (i < 32)
            exp = BIT(i);
        else if (i < 64)
            exp = ~BIT(i - 32);
        else
            exp = fixed[i - 64];

        ret = ax_pcie_bt_wr32(port, b, 0, exp);
        if (!ret)
            ret = ax_pcie_bt_rd32(port, b, 0, &got);
        if (ret)
            return ret;

        if (got != exp)
            ax_pcie_bt_report(b, "data-bus", 0, exp, got);
    }

    return 0;
}

/* Byte writes on the last word under test, 32-bit read back */
static int ax_pcie_bt_byte_lane(u8 port, struct ax_pcie_bt_bar *b)
{
    u64 off = b->len - sizeof(u32);
    u32 exp = 0, got;
    u8 v;
    int i, ret;

    ret = ax_pcie_bt_wr32(port, b, off, 0);
    if (ret)
        return ret;

    for (i = 0; i < 4; i++) {
        v = 0x11 * (i + 1) ^ b->tag;
        ret = ax_pcie_mem_write(port, b->off + off + i, &v, 1);
        if (ret)
            return ret;

        exp |= (u32)v << (i * 8);

        ret = ax_pcie_bt_rd32(port, b, off, &got);
        if (ret)
            return ret;

        if (got != exp)
            ax_pcie_bt_report(b, "byte-lane", off, exp, got);
    }

    return 0;
}

static int ax_pcie_bt_fill(u8 port, struct ax_pcie_bt_bar *b, u32 seed,
        u32 *buf)
{
    u64 pos, n, i;
    int ret;

    for (pos = 0; pos < b->len; pos += n) {
        n = min_t(u64, AX_PCIE_BT_CHUNK, b->len - pos);

        for (i = 0; i < n / 4; i++)
            buf[i] = ax_pcie_bt_pattern(b, pos + i * 4, seed);

        ret = ax_pcie_mem_write(port, b->off + pos, buf, n);
        if (ret)
            return ret;

        if (ctrlc())
            return -EINTR;
    }

    return 0;
}

static int ax_pcie_bt_verify(u8 port, struct ax_pcie_bt_bar *b, u32 seed,
        u32 *buf)
{
    u64 pos, n, i;
    u32 exp;
    int ret;

    for (pos = 0; pos < b->len; pos += n) {
        n = min_t(u64, AX_PCIE_BT_CHUNK, b->len - pos);

        ret = ax_pcie_mem_read(port, b->off + pos, buf, n);
        if (ret)
            return ret;

        for (i = 0; i < n / 4; i++) {
            exp = ax_pcie_bt_pattern(b, pos + i * 4, seed);
            if (buf[i] != exp)
                ax_pcie_bt_report(b, seed ? "address~" : "address",
                        pos + i * 4, exp, buf[i]);
        }

        if (ctrlc())
            return -EINTR;
    }

    return 0;
}

/*
 * Collect the memory BARs of one function.  Returns the number of entries
 * added, or a negative errno.
 */
static int ax_pcie_bt_collect(u8 port, u8 bus, u8 dev, u8 func,
        u64 max_len, struct ax_pcie_bt_bar *tbl, int used)
{
    struct ax_pcie_bar bar;
    struct ax_pcie_bt_bar *b;
    u16 command;
    int added = 0;
    int i, j, ret;

    ret = ax_pcie_cfg_read16(port, bus, dev, func, PCI_CFG_COMMAND, &command);
    if (ret)
        return ret;

    if (!(command & PCI_COMMAND_MEMORY))
        printf("  %02x:%02x.%x: Memory Space disabled, run 'ax_pcie init' first\n",
                bus, dev, func);

    for (i = 0; i < AX_PCIE_BT_NBARS; i += ret) {
        ret = ax_pcie_get_ep_bar(port, bus, dev, func, i, &bar);
        if (ret < 0) {
            if (ret == -EINVAL && i > 0)	/* bridge: only 2 BARs */
                break;
            printf("  %02x:%02x.%x BAR%d: read failed: %d\n",
                    bus, dev, func, i, ret);
            return ret;
        }

        if (!bar.size || bar.is_io)
            continue;

        if (!bar.addr) {
            printf("  %02x:%02x.%x BAR%d: size 0x%llx not assigned, skipped\n",
                    bus, dev, func, i, bar.size);
            continue;
        }

        /* Same window already listed (e.g. ECAM aliasing of dev numbers) */
        for (j = 0; j < used + added; j++)
            if (tbl[j].addr == bar.addr && tbl[j].size == bar.size)
                break;
        if (j < used + added) {
            printf("  %02x:%02x.%x BAR%d: same window as %02x:%02x.%x BAR%d, skipped\n",
                    bus, dev, func, i, tbl[j].bus, tbl[j].dev,
                    tbl[j].func, tbl[j].index);
            continue;
        }

        if (used + added >= AX_PCIE_BT_MAX_BARS) {
            printf("  too many BARs, max %d\n", AX_PCIE_BT_MAX_BARS);
            return -ENOSPC;
        }

        b = &tbl[used + added];
        memset(b, 0, sizeof(*b));
        b->bus = bus;
        b->dev = dev;
        b->func = func;
        b->index = i;
        b->is_64bit = bar.is_64bit;
        b->prefetch = bar.prefetch;
        b->addr = bar.addr;
        b->size = bar.size;
        b->len = (max_len && max_len < bar.size) ? max_len : bar.size;
        b->len &= ~(u64)3;
        b->tag = ((bus << 16) | (dev << 11) | (func << 8) | i) * 0x9e3779b9;

        if (b->len < sizeof(u32)) {
            printf("  %02x:%02x.%x BAR%d: too small, skipped\n",
                    bus, dev, func, i);
            continue;
        }

        if (ax_pcie_bt_bus_to_off(port, b->addr, b->len, &b->off)) {
            printf("  %02x:%02x.%x BAR%d: bus 0x%llx+0x%llx outside outbound window, skipped\n",
                    bus, dev, func, i, b->addr, b->len);
            continue;
        }

        printf("  %02x:%02x.%x BAR%d: bus 0x%010llx size 0x%llx %s%s, testing 0x%llx bytes\n",
                bus, dev, func, i, b->addr, b->size,
                b->is_64bit ? "64-bit" : "32-bit",
                b->prefetch ? " pref" : "", b->len);
        added++;
    }

    return added;
}

/*
 * ax_pcie bar_test <port> [<bus>.<dev>.<func>] [len]
 */
static int ax_pcie_cmd_bar_test(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    struct ax_pcie_bt_bar *tbl = ax_pcie_bt_bars;
    u8 port, bus = 1, dev, func;
    u8 dev_lo = 0, dev_hi = PCI_MAX_PCI_DEVICES - 1;
    u8 func_lo = 0, func_hi = PCI_MAX_PCI_FUNCTIONS - 1;
    u64 max_len = 0;
    u32 id, total_err = 0;
    u32 *buf;
    int nfunc = 0, n = 0;
    int i, ret;

    if (argc < 2 || argc > 4)
        return CMD_RET_USAGE;

    port = simple_strtoul(argv[1], NULL, 0);

    if (argc >= 3 && strcmp(argv[2], "all")) {
        ret = ax_pcie_parse_bdf(argv[2], &bus, &dev, &func);
        if (ret) {
            printf("Invalid BDF: %s\n", argv[2]);
            return CMD_RET_USAGE;
        }
        dev_lo = dev_hi = dev;
        func_lo = func_hi = func;
    }

    if (argc == 4)
        max_len = simple_strtoull(argv[3], NULL, 0);

    printf("PCIe%d BAR test, scanning bus %02x\n", port, bus);

    for (dev = dev_lo; dev <= dev_hi; dev++) {
        for (func = func_lo; func <= func_hi; func++) {
            ret = ax_pcie_cfg_read32(port, bus, dev, func,
                    PCI_CFG_VENDOR_ID, &id);
            if (ret || (id & 0xffff) == 0xffff || (id & 0xffff) == 0)
                continue;

            printf("  %02x:%02x.%x [%04x:%04x]\n",
                    bus, dev, func, id & 0xffff, id >> 16);
            nfunc++;

            ret = ax_pcie_bt_collect(port, bus, dev, func, max_len, tbl, n);
            if (ret < 0)
                return CMD_RET_FAILURE;
            n += ret;
        }
    }

    if (!nfunc) {
        printf("No function found on bus %02x\n", bus);
        return CMD_RET_FAILURE;
    }

    if (!n) {
        printf("No memory BAR to test\n");
        return CMD_RET_FAILURE;
    }

    buf = malloc(AX_PCIE_BT_CHUNK);
    if (!buf)
        return CMD_RET_FAILURE;

    /* 1/2: per-BAR data bus and byte lane */
    printf("\n[1/3] Data bus test\n");
    for (i = 0; i < n; i++) {
        ret = ax_pcie_bt_data_bus(port, &tbl[i]);
        if (ret)
            goto out;
    }

    printf("[2/3] Byte lane test\n");
    for (i = 0; i < n; i++) {
        ret = ax_pcie_bt_byte_lane(port, &tbl[i]);
        if (ret)
            goto out;
    }

    /* 3: fill everything, then verify everything; true then inverted */
    printf("[3/3] Address test (%d BARs, %d functions)\n", n, nfunc);
    for (u32 seed = 0; ; seed = ~0U) {
        for (i = 0; i < n; i++) {
            ret = ax_pcie_bt_fill(port, &tbl[i], seed, buf);
            if (ret)
                goto out;
        }

        for (i = 0; i < n; i++) {
            ret = ax_pcie_bt_verify(port, &tbl[i], seed, buf);
            if (ret)
                goto out;
        }

        if (seed)
            break;
    }

    ret = 0;

out:
    free(buf);

    if (ret == -EINTR) {
        printf("\nInterrupted\n");
        return CMD_RET_FAILURE;
    }

    if (ret)
        printf("\nPCIe access failed: %d\n", ret);

    printf("\n  BDF      BAR  Bus address         Size        Result\n");
    for (i = 0; i < n; i++) {
        printf("  %02x:%02x.%x  %d    0x%016llx  0x%08llx  ",
                tbl[i].bus, tbl[i].dev, tbl[i].func, tbl[i].index,
                tbl[i].addr, tbl[i].size);
        if (tbl[i].errors)
            printf("FAIL (%u errors)\n", tbl[i].errors);
        else
            printf("PASS\n");
        total_err += tbl[i].errors;
    }

    if (ret || total_err) {
        printf("PCIe%d BAR test FAILED\n", port);
        return CMD_RET_FAILURE;
    }

    printf("PCIe%d BAR test PASSED\n", port);

    return CMD_RET_SUCCESS;
}

U_BOOT_LONGHELP(ax_pcie,
        "init <port> <speed>\n"
        " - Initialize PCIe controller\n"
        "\n"
        "link <port>\n"
        " - Start PCIe link training\n"
        "\n"
        "header <port> <bus>.<dev>.<func>\n"
        " - Print PCI configuration header\n"
        "\n"
        "cap <port> <bus>.<dev>.<func>\n"
        " - Print PCI capabilities\n"
        "\n"
        "cfg_read <port> <bus>.<dev>.<func> <offset>\n"
        " - Read PCIe configuration space\n"
        "\n"
        "cfg_write <port> <bus>.<dev>.<func> <offset> <value>\n"
        " - Write PCIe configuration space\n"
        "\n"
        "mem_read <port> <addr> <len>\n"
        " - Read PCIe memory\n"
        "\n"
        "mem_write <port> <addr> <value> <len>\n"
        " - Write PCIe memory"
        "\n"
        "bar_test <port> [<bus>.<dev>.<func>|all] [len]\n"
        " - R/W test all memory BARs of all EP functions (default bus 1, all\n"
        "   dev/func). len: bytes per BAR (default whole BAR). Destructive.\n"
        "\n"
        "msi <enable|disable|status> <port> <bus>.<dev>.<func>\n"
        " - Config EP MSI"
        "\n"
        "debug [-v|-vv] <port> <bus>.<dev>.<num>\n"
        " - Print PCIe config"
);

U_BOOT_CMD_WITH_SUBCMDS(ax_pcie, "Axiado bare-metal PCIe diagnostics test", ax_pcie_help_text,
        U_BOOT_SUBCMD_MKENT(init, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_init),
        U_BOOT_SUBCMD_MKENT(link, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_link),
        U_BOOT_SUBCMD_MKENT(header, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_header),
        U_BOOT_SUBCMD_MKENT(cap, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_cap),
        U_BOOT_SUBCMD_MKENT(cfg_read, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_cfg_read),
        U_BOOT_SUBCMD_MKENT(cfg_write, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_cfg_write),
        U_BOOT_SUBCMD_MKENT(mem_read, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_mem_read),
        U_BOOT_SUBCMD_MKENT(mem_write, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_mem_write),
        U_BOOT_SUBCMD_MKENT(bar_test, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_bar_test),
        U_BOOT_SUBCMD_MKENT(msi, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_msi),
        U_BOOT_SUBCMD_MKENT(debug, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_debug));
