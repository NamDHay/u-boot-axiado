// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <command.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <time.h>
#include <malloc.h>
#include "asm/io.h"

#include <ax_usb.h>

/* legacy "init <port> <proto> <mode>" storage modes (were USB_*_MODE in uhc_cmd.h) */
#define AX_USB_MODE_NONE	0
#define AX_USB_MODE_INIT	1
#define AX_USB_MODE_READ	2	/* read AX_USB_LEGACY_BLKS blocks from LBA 0 */
#define AX_USB_MODE_WRITE	3	/* write pattern, read back, verify at LBA 0 */
#define AX_USB_MODE_ERASE	4	/* zero-fill AX_USB_LEGACY_BLKS blocks at LBA 0 */
#define AX_USB_LEGACY_BLKS	200
#define AX_USB_NUM_PORTS 6

static int cli_num(const char *s, u64 *out)
{
	char *endp;

	*out = simple_strtoull(s, &endp, 0);
	return (*s == '\0' || *endp != '\0') ? -1 : 0;
}

#define CLI_NUM(idx, var)							\
	do {									\
		if (cli_num(argv[idx], &(var))) {				\
			printf("Error: invalid number '%s'\n", argv[idx]);	\
			return CMD_RET_FAILURE;					\
		}								\
	} while (0)

static int cli_print_info(void)
{
	struct ax_usb_info i;

	if (ax_usb_get_info(&i)) {
		printf("no active USB session (use: usbval init <port> [proto])\n");
		return CMD_RET_FAILURE;
	}
	printf("Port     : %d (USB%d.0 root port)\n", i.port, i.protocol);
	printf("Speed    : %s\n", i.speed_str);
	printf("VID:PID  : %04x:%04x\n", i.vid, i.pid);
	printf("Class    : %02x/%02x/%02x\n", i.if_class, i.if_subclass, i.if_protocol);
	printf("Vendor   : %s\n", i.manufacturer);
	printf("Product  : %s\n", i.product);
	printf("Serial   : %s\n", i.serial);
	if (i.is_msc) {
		printf("SCSI     : %s %s %s\n", i.scsi_vendor, i.scsi_product, i.scsi_rev);
		printf("Capacity : %llu blocks x %u bytes (%llu MiB)\n",
		       (unsigned long long)i.blk_count, i.blk_size,
		       (unsigned long long)(i.blk_count * i.blk_size >> 20));
	}
	return CMD_RET_SUCCESS;
}

/* legacy init-time storage tests (baremetal crg_mass_storage_thread modes) */
static int cli_legacy_msc(u32 mode)
{
	struct ax_usb_info i;
	int ret = 0;

	if (ax_usb_get_info(&i) || !i.is_msc) {
		printf("device is not a mass-storage device\n");
		return -1;
	}

	switch (mode) {
	case AX_USB_MODE_READ: {
		void *buf = malloc(AX_USB_LEGACY_BLKS * i.blk_size);

		if (!buf)
			return -1;
		ret = ax_usb_msc_read(0, AX_USB_LEGACY_BLKS, buf);
		free(buf);
		break;
	}
	case AX_USB_MODE_WRITE:
		ret = ax_usb_msc_test(0, AX_USB_LEGACY_BLKS, 0xaaaa5566);
		break;
	case AX_USB_MODE_ERASE:
		ret = ax_usb_msc_erase(0, AX_USB_LEGACY_BLKS);
		break;
	default:
		break;
	}
	return ret;
}

static int do_msc(int argc, char *const argv[])
{
	u64 a, lba, cnt, pat = 0xaaaa5555;
	int ret;

	if (argc < 3)
		return CMD_RET_USAGE;

	if (!strcmp(argv[2], "info"))
		return cli_print_info();

	if (!strcmp(argv[2], "read") && argc == 6) {
		CLI_NUM(3, a);
		CLI_NUM(4, lba);
		CLI_NUM(5, cnt);
		ret = ax_usb_msc_read(lba, cnt, (void *)(uintptr_t)a);
		printf("read %llu blocks @ LBA %llu -> 0x%llx: %s\n",
		       (unsigned long long)cnt, (unsigned long long)lba,
		       (unsigned long long)a, ret ? "FAILED" : "OK");
	} else if (!strcmp(argv[2], "write") && argc == 6) {
		CLI_NUM(3, a);
		CLI_NUM(4, lba);
		CLI_NUM(5, cnt);
		ret = ax_usb_msc_write(lba, cnt, (const void *)(uintptr_t)a);
		printf("write %llu blocks @ LBA %llu <- 0x%llx: %s\n",
		       (unsigned long long)cnt, (unsigned long long)lba,
		       (unsigned long long)a, ret ? "FAILED" : "OK");
	} else if (!strcmp(argv[2], "erase") && argc == 5) {
		CLI_NUM(3, lba);
		CLI_NUM(4, cnt);
		ret = ax_usb_msc_erase(lba, cnt);
		printf("erase %llu blocks @ LBA %llu: %s\n", (unsigned long long)cnt,
		       (unsigned long long)lba, ret ? "FAILED" : "OK");
	} else if (!strcmp(argv[2], "test") && (argc == 5 || argc == 6)) {
		CLI_NUM(3, lba);
		CLI_NUM(4, cnt);
		if (argc == 6)
			CLI_NUM(5, pat);
		ret = ax_usb_msc_test(lba, cnt, (u32)pat);
		printf("write/read/verify %llu blocks @ LBA %llu: %s\n",
		       (unsigned long long)cnt, (unsigned long long)lba,
		       ret ? "FAILED" : "PASSED");
	} else {
		return CMD_RET_USAGE;
	}
	return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_ax_usb(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	u64 port, proto, mode;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "test_all") && argc == 2) {
		ret = ax_usb_test_all();
		printf(ret ? "USB TEST FAILED\n" : "USB TEST PASSED\n");
		return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "init") && argc >= 3 && argc <= 5) {
		CLI_NUM(2, port);
		proto = AX_USB_PROTO_2;
		mode = AX_USB_MODE_NONE;
		if (argc >= 4)
			CLI_NUM(3, proto);
		if (argc == 5)
			CLI_NUM(4, mode);
		if (proto != AX_USB_PROTO_2 && proto != AX_USB_PROTO_3) {
			printf("protocol must be 2 or 3 (hub modes are not supported)\n");
			return CMD_RET_FAILURE;
		}

		ret = ax_usb_init((int)port, (int)proto);
		if (!ret && mode >= AX_USB_MODE_READ)
			ret = cli_legacy_msc((u32)mode);
		if (!ret)
			cli_print_info();

		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "info") && argc == 2)
		return cli_print_info();

	if (!strcmp(argv[1], "msc"))
		return do_msc(argc, argv);

	if (!strcmp(argv[1], "stop")) {
		if (argc == 2) {
			ax_usb_stop();
		} else if (argc == 3) {
			CLI_NUM(2, port);
			ax_usb_stop_port((int)port);
		} else {
			return CMD_RET_USAGE;
		}
		return CMD_RET_SUCCESS;
	}

	if (!strcmp(argv[1], "reset") && argc == 3) {
		int first = 0, n = AX_USB_NUM_PORTS, idx;

		if (strcmp(argv[2], "all")) {
			CLI_NUM(2, port);
			first = (int)port;
			n = 1;
		}
		for (idx = first; idx < first + n; idx++) {
			ax_usb_port_reset(idx);
			mdelay(500);
		}
		return CMD_RET_SUCCESS;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	ax_usb, 8, 0, do_ax_usb,
    "Axiado bare-metal USB host (xHCI) diagnostics test",
	"test_all                                scan every USB port\n"
	"usbval init <port 0-5> [proto 2|3] [mode 0-4]\n"
	"                                        bring up port, enumerate device\n"
	"                                        mode: 2=read 3=write+verify 4=erase (LBA 0, 200 blks)\n"
	"usbval info                             show enumerated device\n"
	"usbval msc info\n"
	"usbval msc read  <ramaddr> <lba> <blks>\n"
	"usbval msc write <ramaddr> <lba> <blks> (destructive)\n"
	"usbval msc erase <lba> <blks>           (destructive, zero-fill)\n"
	"usbval msc test  <lba> <blks> [pattern] (destructive, write/read/compare)\n"
	"usbval stop [port]                      stop session / power a port down\n"
	"usbval reset <port|all>                 release controller + PHY resets\n"
);
