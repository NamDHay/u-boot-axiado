// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

/*
 * Diagnostics support
 */
#include <command.h>
#include "ax_diag.h"
#include <linux/string.h>
#include <log.h>

int do_ax_diag(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int i, ret, fails = 0;

	if (argc == 1) {
		/* List test info */
		printf("%s - perform axiado board diagnostics\n", argv[0]);
		puts("Available hardware tests:\n");
		diag_info(NULL);
		puts("Use 'ax_diag [<test1> [<test2> ...]]' to get more info.\n");
		puts("Use 'ax_diag run [<test1> [<test2> ...]]' to run tests.\n");
		puts("Use 'ax_diag status' to show the result of the last run.\n");
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "run")) {
		if (argc == 2) {
			fails = diag_run(NULL, 0);
		} else {
			for (i = 2; i < argc; i++) {
				ret = diag_run(argv[i], 0);
				if (ret < 0)
					printf("%s - no such test\n", argv[i]);
				else
					fails += ret;
			}
		}
		diag_run_end();
		diag_status();
		return fails ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "status"))
		return diag_status() ? CMD_RET_FAILURE : CMD_RET_SUCCESS;

	if (!strcmp(argv[1], "clear"))
		return diag_clear() ? CMD_RET_FAILURE : CMD_RET_SUCCESS;

	for (i = 1; i < argc; i++) {
		if (diag_info(argv[i]) != 0)
			printf("%s - no such test\n", argv[i]);
	}

	return CMD_RET_SUCCESS;
}
/***************************************************/

U_BOOT_CMD(
	ax_diag,	CONFIG_SYS_MAXARGS,	0,	do_ax_diag,
	"perform axiado board diagnostics",
	"    - print list of available tests\n"
	"ax_diag [test1 [test2]]\n"
	"         - print information about specified tests\n"
	"ax_diag run - run all available tests\n"
	"ax_diag run [test1 [test2]]\n"
	"         - run specified tests\n"
	"ax_diag status\n"
	"         - show per-testcase status of the last run; if the board\n"
	"           crashed/hung during a test, shows which test/step it was\n"
	"ax_diag clear\n"
	"         - clear the stored results"
);
