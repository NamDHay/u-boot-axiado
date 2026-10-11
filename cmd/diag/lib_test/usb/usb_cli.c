// SPDX-License-Identifier: GPL-2.0+
#include <dm.h>
#include <command.h>
#include <mapmem.h>
#include "usb_cli.h"
#include "hal/usb/ax_usb.h"
#include "ax_pcie.h"
#include "ax_pcie_conf.h"

static const char *const spd_str[] = { "?", "FS", "LS", "HS", "SS", "SS+" };

static int do_info(void)
{
	struct ax_usb_info in;
	int i, n = ax_usb_dev_count();

	if (!n) {
		printf("no USB storage device\n");
		return CMD_RET_FAILURE;
	}
	for (i = 0; i < n; i++) {
		if (ax_usb_dev_info(i, &in))
			continue;
		printf("dev %d: %s %s  port %u  %s  %llu blocks x %u B (%llu MiB)\n",
		       i, in.vendor, in.product, in.port,
		       spd_str[in.speed < 6 ? in.speed : 0], in.blocks, in.blksz,
		       (in.blocks * in.blksz) >> 20);
	}
	return CMD_RET_SUCCESS;
}

static int do_rw(bool write, char *const argv[])
{
	int dev = hextoul(argv[0], NULL);
	ulong addr = hextoul(argv[1], NULL);
	u64 lba = hextoul(argv[2], NULL);
	u32 cnt = hextoul(argv[3], NULL);
	struct ax_usb_info in;
	void *buf;
	int ret;

	if (ax_usb_dev_info(dev, &in)) {
		printf("bad device %d\n", dev);
		return CMD_RET_FAILURE;
	}
	buf = map_sysmem(addr, (ulong)cnt * in.blksz);
	ret = write ? ax_usb_write(dev, lba, cnt, buf) : ax_usb_read(dev, lba, cnt, buf);
	unmap_sysmem(buf);

	printf("%s %u blocks @ lba %llu: %s (%d)\n", write ? "write" : "read",
	       cnt, lba, ret ? "FAILED" : "OK", ret);
	return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_ax_usb(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "init")) {
		if (argc > 2 && !strcmp(argv[2], "mmio")) {
			if (argc < 4)
				return CMD_RET_USAGE;
			ret = ax_usb_init_mmio(hextoul(argv[3], NULL),
					       argc > 4 ? hextoul(argv[4], NULL) : 0);
		} else {
			int ai = (argc > 2 && !strcmp(argv[2], "pcie")) ? 3 : 2;
			u8 port = argc > ai ? hextoul(argv[ai], NULL) : 0;

			ret = ax_usb_init(port);
		}
		if (ret < 0) {
			printf("axusb init failed: %d\n", ret);
			return CMD_RET_FAILURE;
		}
		printf("%d storage device(s)\n", ret);
		return CMD_RET_SUCCESS;
	}
	if (!strcmp(argv[1], "info"))
		return do_info();
	if (!strcmp(argv[1], "stop")) {
		ax_usb_stop();
		return CMD_RET_SUCCESS;
	}
	if (!strcmp(argv[1], "read") && argc == 6)
		return do_rw(false, &argv[2]);
	if (!strcmp(argv[1], "write") && argc == 6)
		return do_rw(true, &argv[2]);
	if (!strcmp(argv[1], "erase") && argc == 5) {
		int dev = hextoul(argv[2], NULL);
		u64 lba = hextoul(argv[3], NULL);
		u32 cnt = hextoul(argv[4], NULL);

		ret = ax_usb_erase(dev, lba, cnt);
		printf("erase %u blocks @ lba %llu: %s (%d)\n", cnt, lba,
		       ret ? "FAILED" : "OK", ret);
		return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
	}
	return CMD_RET_USAGE;
}

U_BOOT_CMD(ax_usb, 6, 0, do_ax_usb,
	   "xHCI mass-storage diag",
	   "init pcie [port]            - via PCIe RP: probe card, start xHCI, enumerate\n"
	   "ax_usb init mmio <base>     - local MMIO, no PCIe\n"
	   "ax_usb info                       - list storage devices\n"
	   "ax_usb read  <dev> <addr> <lba> <cnt>\n"
	   "ax_usb write <dev> <addr> <lba> <cnt>\n"
	   "ax_usb erase <dev> <lba> <cnt>    - writes zeroes\n"
	   "ax_usb stop");
