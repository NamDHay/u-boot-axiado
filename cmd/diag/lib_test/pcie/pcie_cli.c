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

#include "ax_pcie.h"

#define AX_PCIE_MODE_EP		0
#define AX_PCIE_MODE_RC		1

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

    if (!strncmp(argv[2], "RP", 2)) {
        mode = AX_PCIE_MODE_RC;
    } else if (!strncmp(argv[2], "EP", 2)) {
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
    unsigned int port;
    char *endp;

    port = simple_strtol(argv[1], &endp, 16);

    ret = ax_pcie_start_link(port);
    if (ret) {
        printf("PCIe_x%d link down: %d\n", port, ret);
        return CMD_RET_FAILURE;
    }

    printf("PCIe_x%d link up\n", port);

    return CMD_RET_SUCCESS;
}


/*
 * ax_pcie link_status <port>
 */
static int ax_pcie_cmd_link_status(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int ret;
    unsigned int port;
    char *endp;

    port = simple_strtol(argv[1], &endp, 16);

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

    if (!strcmp(argv[1], "link_status"))
        return ax_pcie_cmd_link_status(cmdtp, flag, argc - 1, &argv[1]);

    return CMD_RET_USAGE;
}

U_BOOT_CMD(
        ax_pcie,
        8,
        1,
        do_ax_pcie,
        "Axiado PCIe diagnostic command",
        "init <port>\n"
        "    - Initialize PCIe controller\n"
        "ax_pcie reset <port>\n"
        "    - Reset PCIe controller\n"
        "ax_pcie enum <port>\n"
        "    - Start PCIe link training\n"
        "ax_pcie cfg_read <port> <bus> <dev> <func> <offset>\n"
        "    - Read PCIe configuration space\n"
        "ax_pcie cfg_write <port> <bus> <dev> <func> <offset> <value>\n"
        "    - Write PCIe configuration space\n"
        "ax_pcie mem_read <port> <addr> <len>\n"
        "    - Read PCIe memory\n"
        "ax_pcie mem_write <port> <addr> <value> <len>\n"
        "    - Write PCIe memory\n"
        "ax_pcie link_status <port>\n"
"    - Get PCIe link status"
);
