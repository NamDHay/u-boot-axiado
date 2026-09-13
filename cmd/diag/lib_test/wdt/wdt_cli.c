// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <command.h>
#include <dm.h>
#include <ax_wdt.h>

static int do_ax_wdt_start(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
	int ret;
	u64 timeout;

	if (argc < 2)
		return CMD_RET_USAGE;

	timeout = simple_strtoull(argv[1], NULL, 0);

	ret = ax_wdt_start(timeout);
	if (ret == -ENOSYS) {
		printf("Starting watchdog timer not supported.\n");
		return CMD_RET_FAILURE;
	} else if (ret) {
		printf("Starting watchdog timer failed (%d)\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_ax_wdt_stop(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
	int ret;

	ret = ax_wdt_stop();
	if (ret == -ENOSYS) {
		printf("Stopping watchdog timer not supported.\n");
		return CMD_RET_FAILURE;
	} else if (ret) {
		printf("Stopping watchdog timer failed (%d)\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_ax_wdt_reset(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
	int ret;

	ret = ax_wdt_reset();
	if (ret == -ENOSYS) {
		printf("Resetting watchdog timer not supported.\n");
		return CMD_RET_FAILURE;
	} else if (ret) {
		printf("Resetting watchdog timer failed (%d)\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_ax_wdt_expire(struct cmd_tbl *cmdtp, int flag,
        int argc, char *const argv[])
{
	int ret;

	ret = ax_wdt_expire_now();
	if (ret == -ENOSYS) {
		printf("Expiring watchdog timer not supported.\n");
		return CMD_RET_FAILURE;
	} else if (ret) {
		printf("Expiring watchdog timer failed (%d)\n", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_LONGHELP(ax_wdt,
	"ax_wdt start <timeout ms> - start watchdog timer\n"
	"ax_wdt stop - stop watchdog timer\n"
	"ax_wdt reset - reset watchdog timer\n"
	"ax_wdt expire - expire watchdog timer immediately\n");

U_BOOT_CMD_WITH_SUBCMDS(ax_wdt, "Axiado bare-metal WatchDog Timer diagnostics test", ax_wdt_help_text,
	U_BOOT_SUBCMD_MKENT(start, 3, 1, do_ax_wdt_start),
	U_BOOT_SUBCMD_MKENT(stop, 1, 1, do_ax_wdt_stop),
	U_BOOT_SUBCMD_MKENT(reset, 1, 1, do_ax_wdt_reset),
	U_BOOT_SUBCMD_MKENT(expire, 2, 1, do_ax_wdt_expire));
