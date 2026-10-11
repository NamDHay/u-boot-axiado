// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <command.h>
#include <dm.h>
#include <errno.h>
#include <linux/printk.h>

#include "ax_diag.h"
#include "ax_mailbox.h"

#define AX_MBOX_READ_ADDRESS    (AX3000_MEM_BASE_ADRS_DDR_LO + 0x20000000)
#define AX_MBOX_WRITE_ADDRESS   (AX3000_MEM_BASE_ADRS_DDR_LO + 0x10000000)
#define AX_MBOX_TIMEOUT_MS      5000

static int do_ax_mailbox(struct cmd_tbl *cmdtp, int flag, int argc,
        char *const argv[])
{
    u32 channel;
    u32 *src, *dst;
    int ret;

    if (argc < 2)
        return CMD_RET_USAGE;

    if (!strcmp(argv[1], "init")) {
        ret = ax_mbox_request();
    } else if (!strcmp(argv[1], "free")) {
        ret = ax_mbox_free();
    } else if (!strcmp(argv[1], "start")) {
        channel = simple_strtoull(argv[2], NULL, 0);
        ret = ax_mbox_startup(channel);
    } else if (!strcmp(argv[1], "stop")) {
        channel = simple_strtoull(argv[2], NULL, 0);
        ret = ax_mbox_shutdown(channel);
    } else if (!strcmp(argv[1], "send")) {
        src = (void __iomem *)AX_MBOX_READ_ADDRESS;
        channel = simple_strtoull(argv[2], NULL, 0);
        ret = ax_mbox_send(channel, src);
    } else if (!strcmp(argv[1], "recv")) {
        dst = (void __iomem *)AX_MBOX_WRITE_ADDRESS;
        channel = simple_strtoull(argv[2], NULL, 0);
        ret = ax_mbox_recv(channel, dst, AX_MBOX_TIMEOUT_MS);
    } else {
        return CMD_RET_USAGE;
    }

    if (ret) {
        printf("error(%d)\n", ret);
        return CMD_RET_FAILURE;
    }

    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(ax_mbx, CONFIG_SYS_MAXARGS, 0, do_ax_mailbox,
        "Axiado bare-metal Mailbox diagnostics test",
        "init - initialize Mailbox\n"
        "free - Deinitialize Mailbox\n"
        "start <channel> - start mailbox channel\n"
        "stop <channel> - disable mailbox channel\n"
        "send <channel> - send mailbox from 0xA0000000\n"
        "recv <channel> - receive mailbox to 0x90000000\n"
);
