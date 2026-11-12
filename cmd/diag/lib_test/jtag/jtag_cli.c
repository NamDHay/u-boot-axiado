// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * ax_jtag - Axiado Host JTAG diagnostics.
 *
 *   controller : enable / disable / freq / trst
 *   chain      : idcode (master TAP + DAP TAP), dap (DPIDR + power-up), apidr
 *   targets    : ax_jtag <target> <op> ...   (see ax_jtag_targets[])
 */

#include <command.h>
#include <console.h>
#include <asm/cache.h>
#include <cpu_func.h>
#include <env.h>
#include <mapmem.h>
#include <vsprintf.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "ax_diag.h"
#include "ax_jtag.h"
#include "jtag_dap.h"
#include "jtag_target.h"

/* Default DDR staging buffer for "load"/"verify" (fill it with tftp/fatload/loady) */
#define JTAG_LOAD_SRC_ADDR	(AX3000_MEM_BASE_ADRS_DDR_LO + 0x10000000UL)
#define JTAG_LOAD_CHUNK_WORDS	256U	/* 1 KB per block transfer */

/* Add new debug targets (r52, a53, a35, r4 ...) here */
static const struct ax_jtag_target *const ax_jtag_targets[] = {
	&ax_jtag_target_m0,
	&ax_jtag_target_r52,
	&ax_jtag_target_a53,
};

/* AP used for target memory access (md/mw/load/verify) */
static u32 target_mem_ap(const struct ax_jtag_target *t)
{
	return t->mem_ap ? t->mem_ap : t->ap;
}

static int jtag_ret(const char *what, int ret)
{
	if (ret == -ENOSYS) {
		printf("%s not supported\n", what);
		return CMD_RET_FAILURE;
	}
	if (ret) {
		printf("%s failed (%d)\n", what, ret);
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* Controller / chain                                                 */
/* ------------------------------------------------------------------ */

static int do_jtag_enable(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	u32 freq = argc > 1 ? simple_strtoul(argv[1], NULL, 0) : 0;

	return jtag_ret("enable", jtag_start(freq));
}

static int do_jtag_disable(struct cmd_tbl *cmdtp, int flag, int argc,
			   char *const argv[])
{
	return jtag_ret("disable", jtag_stop());
}

static int do_jtag_freq(struct cmd_tbl *cmdtp, int flag, int argc,
			char *const argv[])
{
	u32 freq;
	int ret;

	if (argc > 1) {
		ret = ax_jtag_freq_set(AX_JTAG_PORT, simple_strtoul(argv[1], NULL, 0));
		if (ret)
			return jtag_ret("freq set", ret);
	}

	ret = ax_jtag_freq_get(AX_JTAG_PORT, &freq);
	if (ret)
		return jtag_ret("freq get", ret);

	printf("TCK %u Hz\n", freq);
	return CMD_RET_SUCCESS;
}

static int do_jtag_trst(struct cmd_tbl *cmdtp, int flag, int argc,
			char *const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	dap_invalidate();
	return jtag_ret("trst", ax_jtag_trst_set(AX_JTAG_PORT,
					simple_strtoul(argv[1], NULL, 0)));
}

static int do_jtag_idcode(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	u32 freq = argc > 1 ? simple_strtoul(argv[1], NULL, 0) : 0;
	u32 master = 0, dap = 0;
	int ret;

	ret = jtag_start(freq);
	if (ret)
		return jtag_ret("enable", ret);

	ret = jtag_read_master_idcode(&master);
	if (ret)
		return jtag_ret("master IDCODE", ret);
	jtag_print_idcode("Master TAP", master);

	/* Re-start so the chain scan begins from a clean TLR */
	ret = jtag_start(0);
	if (!ret)
		ret = jtag_read_chain_idcode(&master, &dap);
	if (ret)
		return jtag_ret("DAP IDCODE", ret);
	jtag_print_idcode("DAP TAP", dap);

	return CMD_RET_SUCCESS;
}

static int do_jtag_dap(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	u32 v;
	int ret;

	ret = dap_open();
	if (ret)
		return jtag_ret("DAP open", ret);

	ret = dap_clear_sticky();
	if (!ret)
		ret = dap_dp_read(DP_CTRL_STAT, &v);
	if (ret)
		return jtag_ret("CTRL/STAT", ret);

	printf("CTRL/STAT  0x%08x after sticky clear\n", v);
	return CMD_RET_SUCCESS;
}

static int do_jtag_apidr(struct cmd_tbl *cmdtp, int flag, int argc,
			 char *const argv[])
{
	u32 ap, v;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;
	ap = simple_strtoul(argv[1], NULL, 16);

	if (!dap_is_open()) {
		ret = dap_open();
		if (ret)
			return jtag_ret("DAP open", ret);
	}

	ret = ap_read(ap, AP_IDR, &v);
	if (ret) {
		printf("AP 0x%x IDR read failed (ack %u)\n", ap, dap_last_ack());
		return CMD_RET_FAILURE;
	}
	printf("AP 0x%x IDR 0x%08x%s\n", ap, v, v ? "" : " (no AP)");
	return CMD_RET_SUCCESS;
}

static int do_jtag_targets(struct cmd_tbl *cmdtp, int flag, int argc,
			   char *const argv[])
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ax_jtag_targets); i++)
		printf("  %-6s AP 0x%05x mem AP 0x%05x  %s\n",
		       ax_jtag_targets[i]->name, ax_jtag_targets[i]->ap,
		       target_mem_ap(ax_jtag_targets[i]), ax_jtag_targets[i]->desc);
	return CMD_RET_SUCCESS;
}

/* ------------------------------------------------------------------ */
/* Target ops: ax_jtag <target> <op> [args]                           */
/* ------------------------------------------------------------------ */

static int target_simple(const char *what, int (*fn)(void))
{
	return jtag_ret(what, fn ? fn() : -ENOSYS);
}

static int target_reg(const struct ax_jtag_target *t, int argc,
		      char *const argv[])
{
	u32 n;
	u64 v;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;
	n = simple_strtoul(argv[1], NULL, 0);

	if (argc > 2) {
		v = simple_strtoull(argv[2], NULL, 16);
		ret = t->reg_write ? t->reg_write(n, v) : -ENOSYS;
		return jtag_ret("reg write", ret);
	}

	ret = t->reg_read ? t->reg_read(n, &v) : -ENOSYS;
	if (ret)
		return jtag_ret("reg read", ret);
	if (t->reg64)
		printf("reg %u = 0x%016llx\n", n, v);
	else
		printf("reg %u = 0x%08x\n", n, (u32)v);
	return CMD_RET_SUCCESS;
}

static int target_md(const struct ax_jtag_target *t, int argc,
		     char *const argv[])
{
	u32 addr, count, i, v;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;
	addr = simple_strtoul(argv[1], NULL, 16) & ~3U;
	count = argc > 2 ? simple_strtoul(argv[2], NULL, 0) : 1;

	ret = t->mem_check ? t->mem_check(false) : mem_ap_open(target_mem_ap(t));
	if (ret)
		return jtag_ret("md", ret);

	for (i = 0; i < count; i++, addr += 4) {
		if (!(i % 4))
			printf("%s%08x:", i ? "\n" : "", addr);
		ret = mem_read32(target_mem_ap(t), addr, &v);
		if (ret) {
			printf("\nread 0x%08x failed (ack %u)\n", addr, dap_last_ack());
			return CMD_RET_FAILURE;
		}
		printf(" %08x", v);
		if (ctrlc())
			break;
	}
	printf("\n");
	return CMD_RET_SUCCESS;
}

static int target_mw(const struct ax_jtag_target *t, int argc,
		     char *const argv[])
{
	u32 addr, val, count, i;
	int ret;

	if (argc < 3)
		return CMD_RET_USAGE;
	addr = simple_strtoul(argv[1], NULL, 16) & ~3U;
	val = simple_strtoul(argv[2], NULL, 16);
	count = argc > 3 ? simple_strtoul(argv[3], NULL, 0) : 1;

	ret = t->mem_check ? t->mem_check(true) : mem_ap_open(target_mem_ap(t));
	if (ret)
		return jtag_ret("mw", ret);

	for (i = 0; i < count; i++, addr += 4) {
		ret = mem_write32(target_mem_ap(t), addr, val);
		if (ret) {
			printf("write 0x%08x failed (ack %u)\n", addr, dap_last_ack());
			return CMD_RET_FAILURE;
		}
	}
	return CMD_RET_SUCCESS;
}

/*
 * <target> load|verify <dst_hex> [size] [src_hex]
 *   size defaults to $filesize, src to JTAG_LOAD_SRC_ADDR (DDR).
 *   load copies DDR -> target over the target's MEM-AP, verify compares.
 *   A trailing partial word is merged with the target's existing bytes.
 */
static int target_load(const struct ax_jtag_target *t, int argc,
		       char *const argv[], bool verify)
{
	const char *what = verify ? "verify" : "load";
	u32 chunk[JTAG_LOAD_CHUNK_WORDS], tgt[JTAG_LOAD_CHUNK_WORDS];
	u32 dst, size, done, n, words, i, errors = 0;
	ulong src;
	const u8 *p;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;
	dst = simple_strtoul(argv[1], NULL, 16);
	size = argc > 2 ? simple_strtoul(argv[2], NULL, 16) :
			  (u32)env_get_hex("filesize", 0);
	src = argc > 3 ? simple_strtoul(argv[3], NULL, 16) : JTAG_LOAD_SRC_ADDR;

	if (dst & 3U) {
		printf("%s: target address 0x%08x must be 4-byte aligned\n", what, dst);
		return CMD_RET_FAILURE;
	}
	if (!size) {
		printf("%s: size is 0 (pass <size> or set $filesize)\n", what);
		return CMD_RET_FAILURE;
	}

	ret = t->mem_check ? t->mem_check(!verify) : mem_ap_open(target_mem_ap(t));
	if (ret)
		return jtag_ret(what, ret);

	printf("%s %s: DDR 0x%08lx -> %s 0x%08x, %u bytes\n", t->name, what,
	       src, t->name, dst, size);

	p = map_sysmem(src, size);
	for (done = 0; done < size; done += n) {
		n = min(size - done, JTAG_LOAD_CHUNK_WORDS * 4U);
		words = DIV_ROUND_UP(n, 4U);

		memcpy(chunk, p + done, n);
		if (n & 3U) {
			/* Tail: keep the target bytes past the end of the image */
			u32 last, mask = (1U << ((n & 3U) * 8U)) - 1U;

			ret = mem_read_block(target_mem_ap(t), dst + done + (words - 1) * 4U, &last, 1);
			if (ret)
				break;
			chunk[words - 1] = (chunk[words - 1] & mask) | (last & ~mask);
		}

		if (!verify) {
			ret = mem_write_block(target_mem_ap(t), dst + done, chunk, words);
			if (ret)
				break;
		} else {
			ret = mem_read_block(target_mem_ap(t), dst + done, tgt, words);
			if (ret)
				break;
			for (i = 0; i < words; i++) {
				if (tgt[i] == chunk[i])
					continue;
				if (errors++ < 10)
					printf("  mismatch @0x%08x: target 0x%08x ddr 0x%08x\n",
					       dst + done + i * 4U, tgt[i], chunk[i]);
			}
		}

		printf("\r  %u / %u bytes", done + n, size);
		if (ctrlc()) {
			printf("\n%s interrupted\n", what);
			unmap_sysmem(p);
			return CMD_RET_FAILURE;
		}
	}
	printf("\n");
	unmap_sysmem(p);

	if (ret)
		return jtag_ret(what, ret);

	/* Push the staged image out of the A53 D-cache so DDR matches what was loaded */
	if (!verify)
		flush_cache(ALIGN_DOWN(src, ARCH_DMA_MINALIGN),
			    ALIGN(size + (src & (ARCH_DMA_MINALIGN - 1)),
				  ARCH_DMA_MINALIGN));

	if (verify && errors) {
		printf("verify FAILED: %u mismatching words\n", errors);
		return CMD_RET_FAILURE;
	}
	printf("%s OK\n", what);
	return CMD_RET_SUCCESS;
}

static int do_jtag_target(const struct ax_jtag_target *t, int argc,
			  char *const argv[])
{
	const char *op;

	if (argc < 1)
		return CMD_RET_USAGE;
	op = argv[0];

	if (!strcmp(op, "probe"))
		return target_simple("probe", t->probe);
	if (!strcmp(op, "halt"))
		return target_simple("halt", t->halt);
	if (!strcmp(op, "run"))
		return target_simple("run", t->run);
	if (!strcmp(op, "step"))
		return target_simple("step", t->step);
	if (!strcmp(op, "state"))
		return target_simple("state", t->state);
	if (!strcmp(op, "regs"))
		return target_simple("regs", t->regs);
	if (!strcmp(op, "reg"))
		return target_reg(t, argc, argv);
	if (!strcmp(op, "reset"))
		return jtag_ret("reset", t->reset ?
				t->reset(argc > 1 && !strcmp(argv[1], "halt")) : -ENOSYS);
	if (!strcmp(op, "md"))
		return target_md(t, argc, argv);
	if (!strcmp(op, "mw"))
		return target_mw(t, argc, argv);
	if (!strcmp(op, "load"))
		return target_load(t, argc, argv, false);
	if (!strcmp(op, "verify"))
		return target_load(t, argc, argv, true);

	return CMD_RET_USAGE;
}

static struct cmd_tbl ax_jtag_sub[] = {
	U_BOOT_CMD_MKENT(enable, 2, 0, do_jtag_enable, "", ""),
	U_BOOT_CMD_MKENT(disable, 1, 0, do_jtag_disable, "", ""),
	U_BOOT_CMD_MKENT(freq, 2, 0, do_jtag_freq, "", ""),
	U_BOOT_CMD_MKENT(trst, 2, 0, do_jtag_trst, "", ""),
	U_BOOT_CMD_MKENT(idcode, 2, 0, do_jtag_idcode, "", ""),
	U_BOOT_CMD_MKENT(dap, 1, 0, do_jtag_dap, "", ""),
	U_BOOT_CMD_MKENT(apidr, 2, 0, do_jtag_apidr, "", ""),
	U_BOOT_CMD_MKENT(targets, 1, 0, do_jtag_targets, "", ""),
};

static int do_ax_jtag(struct cmd_tbl *cmdtp, int flag, int argc,
		      char *const argv[])
{
	struct cmd_tbl *c;
	int i;

	if (argc < 2)
		return CMD_RET_USAGE;
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], ax_jtag_sub, ARRAY_SIZE(ax_jtag_sub));
	if (c)
		return c->cmd(c, flag, argc, argv);

	for (i = 0; i < ARRAY_SIZE(ax_jtag_targets); i++)
		if (!strcmp(argv[0], ax_jtag_targets[i]->name))
			return do_jtag_target(ax_jtag_targets[i], argc - 1, argv + 1);

	return CMD_RET_USAGE;
}

U_BOOT_LONGHELP(ax_jtag,
	"enable [tck_hz]      - enable Host JTAG (TRST + TLR -> RTI)\n"
	"ax_jtag disable              - disable Host JTAG\n"
	"ax_jtag freq [tck_hz]        - get / set TCK frequency\n"
	"ax_jtag trst <0|1>           - drive TRST (1 = assert)\n"
	"ax_jtag idcode [tck_hz]      - scan master TAP and DAP TAP IDCODE\n"
	"ax_jtag dap                  - open DAP: chain, DPIDR, power-up, CTRL/STAT\n"
	"ax_jtag apidr <ap_hex>       - read AP IDR (e.g. 6000 = M0 AHB-AP)\n"
	"ax_jtag targets              - list debug targets\n"
	"ax_jtag <target> probe       - open target AP, print IDs\n"
	"ax_jtag <target> halt|run|step|state|regs\n"
	"ax_jtag <target> reg <n> [val_hex]   - read / write core register n\n"
	"        (m0: 15=pc 16=xpsr; r52/a53 AArch32: 15=pc 16=cpsr;\n"
	"         a53 AArch64: 0-30=x0-x30 31=sp 32=pc 33=pstate)\n"
	"ax_jtag <target> reset [halt]        - system reset (optionally halt at reset)\n"
	"ax_jtag <target> md <addr> [count]   - read 32-bit words via target AP\n"
	"ax_jtag <target> mw <addr> <val> [count] - write 32-bit words via target AP\n"
	"ax_jtag <target> load <dst> [size] [src] - copy firmware from DDR to target\n"
	"        (size hex, default $filesize; src default DDR_LO + 0x10000000)\n"
	"ax_jtag <target> verify <dst> [size] [src] - compare target memory with DDR\n");

U_BOOT_CMD(ax_jtag, 6, 0, do_ax_jtag,
	   "Axiado Host JTAG diagnostics test", ax_jtag_help_text);
