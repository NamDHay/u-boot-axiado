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

#include "ax_pcie.h"
#include "ax_pcie_conf.h"
#include "axiado_pcie_ep.h"

#define AX_PCIE_MODE_EP		0

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
    char *endp;

    if (argc < 2)
        return CMD_RET_USAGE;
    port = simple_strtol(argv[1], &endp, 16);
    mode = AX_PCIE_MODE_EP;
    speed = 0x4;

    ret = ax_pcie_init(port, mode, speed);
    if (ret) {
        printf("PCIe_x%d init failed: %d\n", port, ret);
        return CMD_RET_FAILURE;
    }

    printf("PCIe_EP_x%d init successed\n", port);

    return CMD_RET_SUCCESS;
}

/*
 * ax_pcie msi <port> <speed>
 */
static int ax_pcie_cmd_trigger_msi(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
    int ret;
    unsigned int port;
    unsigned int pf_num;
    uint msi_id;
    char *endp;

    if (argc != 4)
        return CMD_RET_USAGE;

    port = simple_strtol(argv[1], &endp, 16);
    pf_num = simple_strtol(argv[2], &endp, 16);
    msi_id = simple_strtol(argv[3], &endp, 16);

    ret = ax_pcie_set_msi(port, pf_num, msi_id);
    if (ret)
        return CMD_RET_FAILURE;

    printf("PCIe_EP_x%d Func: %d send MSI successed\n", port, pf_num);

    return CMD_RET_SUCCESS;
}

U_BOOT_LONGHELP(ax_pcie_ep,
        "init <port>\n"
        " - Initialize PCIe as Endpoint\n"
        "msi <port> <func> <msi_id>\n"
        " - Trigger PCIe MSI Interrupt\n"
        );

U_BOOT_CMD_WITH_SUBCMDS(ax_pcie_ep, "Axiado bare-metal PCIe Endpoint diagnostics test", ax_pcie_ep_help_text,
        U_BOOT_SUBCMD_MKENT(init, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_init),
        U_BOOT_SUBCMD_MKENT(msi, CONFIG_SYS_MAXARGS, 1, ax_pcie_cmd_trigger_msi));
