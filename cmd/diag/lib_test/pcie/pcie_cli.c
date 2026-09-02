// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */
#include <command.h>
#include <dm.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/types.h>
#include <stdio.h>

#include "ax_pcie.h"
#include "ax_pcie_conf.h"

#define AX_PCIE_MODE_EP		0
#define AX_PCIE_MODE_RC		1

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

/*
 * ax_pcie init <port> RP/EP <speed>
 */
static int ax_pcie_cmd_init(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int ret;
    unsigned int port;
    unsigned int mode;
    unsigned int speed;
    char *endp;

    if (argc < 4)
        return CMD_RET_USAGE;

    if (!strncmp(argv[2], "rp", 2)) {
        mode = AX_PCIE_MODE_RC;
    } else if (!strncmp(argv[2], "ep", 2)) {
        mode = AX_PCIE_MODE_EP;
    } else {
        printf("Mode not found\n");
        return CMD_RET_FAILURE;
    }
    port = simple_strtol(argv[1], &endp, 16);
    speed = simple_strtol(argv[3], &endp, 16);

    ret = ax_pcie_init(port, mode, speed);
    if (ret) {
        printf("PCIe_x%d init failed: %d\n", port, ret);
        return CMD_RET_FAILURE;
    }

    printf("PCIe_x%d init successed\n", port);

    return CMD_RET_SUCCESS;
}

/*
 * ax_pcie enum <port>
 */
static int ax_pcie_cmd_enum(struct cmd_tbl *cmdtp, int flag,
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

    ret = ax_pcie_init_ep_bars(port, 1, 0, 0);
    if (ret) {
        printf("Failed to initialize endpoint BARs: %d\n", ret);
        return CMD_RET_FAILURE;
    }

    ax_pcie_print_header(port, 1, 0, 0);

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

static int do_ax_pcie(struct cmd_tbl *cmdtp,
        int flag,
        int argc,
        char *const argv[])
{
    if (argc < 3)
        return CMD_RET_USAGE;

    if (!strcmp(argv[1], "init"))
        return ax_pcie_cmd_init(cmdtp, flag, argc - 1, &argv[1]);

    if (!strcmp(argv[1], "enum"))
        return ax_pcie_cmd_enum(cmdtp, flag, argc - 1, &argv[1]);

    if (!strcmp(argv[1], "header"))
        return ax_pcie_cmd_header(cmdtp, flag,
                argc - 1, &argv[1]);

    if (!strcmp(argv[1], "cap"))
        return ax_pcie_cmd_cap(cmdtp, flag,
                argc - 1, &argv[1]);

    if (!strcmp(argv[1], "cfg_read"))
        return ax_pcie_cmd_cfg_read(cmdtp, flag, argc - 1, &argv[1]);

    if (!strcmp(argv[1], "cfg_write"))
        return ax_pcie_cmd_cfg_write(cmdtp, flag, argc - 1, &argv[1]);

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
        ax_pcie,
        8,
        1,
        do_ax_pcie,
        "Axiado PCIe diagnostic command",
        "init <port> <RP|EP> <speed>\n"
        "    - Initialize PCIe controller\n"
        "\n"
        "ax_pcie enum <port>\n"
        "    - Start PCIe link training\n"
        "\n"
        "ax_pcie header <port> <bus>.<dev>.<func>\n"
        "    - Print PCI configuration header\n"
        "\n"
        "ax_pcie cap <port> <bus>.<dev>.<func>\n"
        "    - Print PCI capabilities\n"
        "\n"
        "ax_pcie cfg_read <port> <bus>.<dev>.<func> <offset>\n"
        "    - Read PCIe configuration space\n"
        "\n"
        "ax_pcie cfg_write <port> <bus>.<dev>.<func> <offset> <value>\n"
        "    - Write PCIe configuration space\n"
        "\n"
        "ax_pcie mem_read <port> <addr> <len>\n"
        "    - Read PCIe memory\n"
        "\n"
        "ax_pcie mem_write <port> <addr> <value> <len>\n"
        "    - Write PCIe memory"
);
