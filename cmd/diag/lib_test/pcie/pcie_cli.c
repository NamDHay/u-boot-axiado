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
                break;

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

        return CMD_RET_SUCCESS;
    }

    return CMD_RET_USAGE;
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
        U_BOOT_SUBCMD_MKENT(mem_write, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_mem_write),
        U_BOOT_SUBCMD_MKENT(msi, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_msi),
        U_BOOT_SUBCMD_MKENT(debug, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_debug));
