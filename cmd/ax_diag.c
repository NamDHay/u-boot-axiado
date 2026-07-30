// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

/*
 * Diagnostics support
 */
#include <command.h>
#include "diag/diag.h"
#include <linux/string.h>
#include <log.h>

int do_diag(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned int i;

	if (argc == 1 || strcmp (argv[1], "run") != 0) {
		/* List test info */
		if (argc == 1) {
			puts ("Available hardware tests:\n");
			diag_info (NULL);
			puts ("Use 'diag [<test1> [<test2> ...]]'"
					" to get more info.\n");
			puts ("Use 'diag run [<test1> [<test2> ...]]'"
					" to run tests.\n");
		} else {
			for (i = 1; i < argc; i++) {
			    if (diag_info (argv[i]) != 0)
				printf ("%s - no such test\n", argv[i]);
			}
		}
	} else {
		/* Run tests */
		if (argc == 2) {
            diag_run (NULL, 0);
        } else {
            for (i = 2; i < argc; i++) {
                if (diag_run (argv[i], 0) != 0)
                    printf ("%s - unable to execute the test\n",
                            argv[i]);
            }
        }
    }

    return 0;
}
/***************************************************/

U_BOOT_CMD(
        diag,	CONFIG_SYS_MAXARGS,	0,	do_diag,
        "perform board diagnostics",
        "    - print list of available tests\n"
        "ax_diag [test1 [test2]]\n"
        "         - print information about specified tests\n"
        "ax_diag run - run all available tests\n"
        "ax_diag run [test1 [test2]]\n"
        "         - run specified tests"
        );
