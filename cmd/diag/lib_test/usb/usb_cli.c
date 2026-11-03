// SPDX-License-Identifier: GPL-2.0+
#include <dm.h>
#include <command.h>
#include <mapmem.h>
#include <console.h>
#include <time.h>
#include <linux/errno.h>
#include "hal/usb/host/ax_usb.h"
#include "ax_pcie.h"
#include "ax_pcie_conf.h"

#define USB_TEST_DUMP_SIZE  64U
#define USB_SPEED_DURATION_S  60U
#define USB_SPEED_REPORT_MS   1000U

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

static u8 usb_pattern_byte(u64 lba, u32 offset)
{
	return (u8)((offset * 37U + (u32)lba +
		     (offset >> 8) + 0x5aU) & 0xffU);
}

static void usb_fill_pattern(u8 *buf, ulong bytes, u64 lba)
{
	ulong i;

	for (i = 0; i < bytes; i++)
		buf[i] = usb_pattern_byte(lba, i);
}

static int usb_compare_pattern(const u8 *buf, ulong bytes,
			       u64 lba, u32 blksz)
{
	ulong i;

	for (i = 0; i < bytes; i++) {
		u8 expected = usb_pattern_byte(lba, i);

		if (buf[i] != expected) {
			printf("COMPARE FAIL: offset=0x%lx "
			       "expected=0x%02x actual=0x%02x\n",
			       i, expected, buf[i]);
			printf("Failing LBA: %llu\n",
			       lba + i / blksz);
			return -EIO;
		}
	}

	return 0;
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

static int do_compare(bool write, char *const argv[])
{
	int dev = hextoul(argv[0], NULL);
	ulong addr = hextoul(argv[1], NULL);
	u64 lba = hextoul(argv[2], NULL);
	u32 cnt = hextoul(argv[3], NULL);
	struct ax_usb_info in;
	void *buf;
	ulong bytes;
	int ret = CMD_RET_FAILURE;

	if (!cnt) {
		printf("USB COMPARE: block count must be > 0\n");
		return CMD_RET_USAGE;
	}

	if (ax_usb_dev_info(dev, &in)) {
		printf("bad device %d\n", dev);
		return CMD_RET_FAILURE;
	}

	if (!in.blksz || (u64)cnt * in.blksz > ULONG_MAX) {
		printf("USB COMPARE: invalid transfer size\n");
		return CMD_RET_FAILURE;
	}

	bytes = (ulong)cnt * in.blksz;

	printf("USB COMPARE: LBA=%llu count=%u bytes=%lu\n",
	       lba, cnt, bytes);

	if (write)
		printf("WARNING: overwriting existing USB data\n");

	buf = map_sysmem(addr, bytes);
	if (!buf) {
		printf("USB COMPARE: map_sysmem failed\n");
		return CMD_RET_FAILURE;
	}

	if (write) {
		usb_fill_pattern(buf, bytes, lba);

		ret = ax_usb_write(dev, lba, cnt, buf);
		if (ret) {
			printf("USB COMPARE: WRITE failed: %d\n", ret);
			goto out;
		}
	}

	memset(buf, 0, bytes);

	ret = ax_usb_read(dev, lba, cnt, buf);
	if (ret) {
		printf("USB COMPARE: READ failed: %d\n", ret);
		goto out;
	}

    ret = usb_compare_pattern(buf, bytes, lba, in.blksz);
	if (ret) {
		printf("USB COMPARE: FAILED\n");
		goto out;
	}

	printf("USB COMPARE PASS: %lu bytes verified\n", bytes);

out:
	unmap_sysmem(buf);

	return ret ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static void usb_print_speed(const char *name, u64 bytes,
			    ulong elapsed_ms)
{
	u64 rate;

	if (!elapsed_ms) {
		printf("%s: elapsed time below timer resolution\n", name);
		return;
	}

	/* Decimal MB/s */
	rate = bytes / elapsed_ms;

	printf("%s: %llu.%03llu MB/s, %llu bytes in %lu ms\n",
	       name,
	       rate / 1000ULL, rate % 1000ULL,
	       bytes, elapsed_ms);
}

static int do_speed(bool write, char *const argv[])
{
	int dev = hextoul(argv[0], NULL);
	ulong addr = hextoul(argv[1], NULL);
	u64 lba = hextoul(argv[2], NULL);
	u32 cnt = hextoul(argv[3], NULL);
	struct ax_usb_info in;
	void *buf;
	ulong bytes;
	ulong start, report_start, io_start;
	ulong elapsed, io_elapsed;
	u64 total_bytes = 0;
	u64 interval_bytes = 0;
	u64 io_time_ms = 0;
	bool interrupted = false;
	int ret;

	if (!cnt) {
		printf("USB SPEED: block count must be > 0\n");
		return CMD_RET_USAGE;
	}

	if (ax_usb_dev_info(dev, &in)) {
		printf("bad device %d\n", dev);
		return CMD_RET_FAILURE;
	}

	if (!in.blksz || (u64)cnt * in.blksz > ULONG_MAX) {
		printf("USB SPEED: invalid transfer size\n");
		return CMD_RET_FAILURE;
	}

	bytes = (ulong)cnt * in.blksz;

	buf = map_sysmem(addr, bytes);
	if (!buf) {
		printf("USB SPEED: map_sysmem failed\n");
		return CMD_RET_FAILURE;
	}

	if (write)
		usb_fill_pattern(buf, bytes, lba);

	printf("\nUSB %s SPEED TEST\n", write ? "WRITE" : "READ");
	printf("Device=%d LBA=%llu blocks=%u block_size=%u\n",
	       dev, lba, cnt, in.blksz);
	printf("Transfer size: %lu bytes\n", bytes);
	printf("Duration: %u seconds maximum\n",
	       USB_SPEED_DURATION_S);

	if (write)
		printf("WARNING: overwrites the selected USB data range\n");

	printf("Press Ctrl-C to stop\n\n");

	start = get_timer(0);
	report_start = start;

	while (get_timer(start) < USB_SPEED_DURATION_S * 1000UL) {
		if (ctrlc()) {
			interrupted = true;
			break;
		}

		io_start = get_timer(0);

		if (write)
			ret = ax_usb_write(dev, lba, cnt, buf);
		else
			ret = ax_usb_read(dev, lba, cnt, buf);

		io_elapsed = get_timer(io_start);
		io_time_ms += io_elapsed;

		if (ret) {
			printf("\nUSB %s failed: %d\n",
			       write ? "WRITE" : "READ", ret);
			unmap_sysmem(buf);
			return CMD_RET_FAILURE;
		}

		total_bytes += bytes;
		interval_bytes += bytes;

		elapsed = get_timer(start);

		if (elapsed - report_start >= USB_SPEED_REPORT_MS) {
			ulong interval_ms = elapsed - report_start;

			usb_print_speed(write ? "WRITE" : "READ",
					interval_bytes, interval_ms);

			interval_bytes = 0;
			report_start = elapsed;
		}
	}

	elapsed = get_timer(start);

	printf("\n--- USB SPEED SUMMARY ---\n");
	printf("Status: %s\n",
	       interrupted ? "Stopped by Ctrl-C" : "Duration completed");
	printf("Elapsed: %lu.%03lu seconds\n",
	       elapsed / 1000UL, elapsed % 1000UL);
	printf("Total bytes: %llu\n", total_bytes);

	/* Overall throughput includes command-loop overhead. */
	usb_print_speed(write ? "WRITE" : "READ",
			total_bytes, elapsed);

	printf("Accumulated I/O time: %lu ms\n", io_time_ms);

	unmap_sysmem(buf);

	return CMD_RET_SUCCESS;
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
	if (!strcmp(argv[1], "compare") && argc == 6)
		return do_compare(true, &argv[2]);
	if (!strcmp(argv[1], "speed") && argc == 6)
		return do_speed(true, &argv[2]);
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
	   "ax_usb compare <dev> <lba> <cnt>\n"
       "    - Write, read back and compare every byte (DESTRUCTIVE)\n"
       "ax_usb speed <lba> <blocks> [duration_s]\n"
       "    - Continuous USB read/write speed test (default 60s)\n"
       "    - duration_s=0 runs until Ctrl-C (DESTRUCTIVE)\n"
	   "ax_usb stop");
