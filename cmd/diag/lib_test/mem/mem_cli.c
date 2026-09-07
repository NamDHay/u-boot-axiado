// SPDX-License-Identifier: GPL-2.0+
/*
 * DDR memory stress test for U-Boot
 *
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 *
 * Supported tests:
 *   - Fixed patterns
 *   - Checkerboard
 *   - Address bus test
 *   - Walking 1
 *   - Walking 0
 *   - Random / inverse random
 *   - March C-
 *
 */

#include <command.h>
#include <dm.h>
#include <console.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/printk.h>
#include <malloc.h>
#include <asm/cache.h>
#include <rand.h>

#define DDRTEST_DEFAULT_SEED       0x12345678

#define DDRTEST_PATTERN_0          0x00000000
#define DDRTEST_PATTERN_1          0xffffffff
#define DDRTEST_PATTERN_A          0xaaaaaaaa
#define DDRTEST_PATTERN_5          0x55555555

#define DDRTEST_MAX_ERRORS         32

#define DDRTEST_PROGRESS_STEP      5

struct ddrtest_ctx {
	ulong start;
	ulong size;
	ulong end;

	u64 words;

	u32 iteration;
	u32 iterations;

	u32 seed;

	u32 errors;
	u32 max_errors;

	bool aborted;
};

static u32 ddrtest_errors;

/* ------------------------------------------------------------- */
/* Helpers                                                       */
/* ------------------------------------------------------------- */

static inline u32 ddrtest_read(volatile u32 *addr)
{
	u32 value;

	/*
	 * Read directly through volatile pointer.
	 *
	 * Cache maintenance is handled at test boundaries.
	 */
	value = *addr;

	return value;
}

static inline void ddrtest_write(volatile u32 *addr, u32 value)
{
	*addr = value;
}

static bool ddrtest_abort(struct ddrtest_ctx *ctx)
{
	if (ctrlc()) {
		printf("\nDDR test aborted by user\n");
		ctx->aborted = true;
		return true;
	}

	return false;
}

static void ddrtest_progress(struct ddrtest_ctx *ctx,
			     u64 current,
			     u64 total)
{
	static int last_percent = -1;
	int percent;

	if (!total)
		return;

	percent = (int)((current * 100) / total);

	if (percent >= last_percent + DDRTEST_PROGRESS_STEP ||
	    percent == 100) {
		printf("\r    Progress: %3d%%", percent);
		last_percent = percent;
	}

	if (percent == 100)
		printf("\n");
}

static int ddrtest_compare(struct ddrtest_ctx *ctx,
			   ulong addr,
			   u32 expected,
			   u32 actual)
{
	if (expected == actual)
		return 0;

	ctx->errors++;
	ddrtest_errors++;

	printf("\n");
	printf("DDR TEST ERROR #%u\n", ctx->errors);
	printf("  Address : 0x%08lx\n", addr);
	printf("  Expected: 0x%08x\n", expected);
	printf("  Actual  : 0x%08x\n", actual);
	printf("  XOR     : 0x%08x\n", expected ^ actual);
	printf("  Bit diff: %u bits\n",
	       hweight32(expected ^ actual));

	if (ctx->errors >= ctx->max_errors) {
		printf("Maximum error limit reached\n");
		return -EIO;
	}

	return 0;
}

/* ------------------------------------------------------------- */
/* Fixed Pattern                                                  */
/* ------------------------------------------------------------- */

static int ddrtest_fixed_pattern(struct ddrtest_ctx *ctx,
				 u32 pattern)
{
	volatile u32 *p;
	u64 i;

	printf("  Pattern 0x%08x\n", pattern);

	p = (volatile u32 *)ctx->start;

	printf("    Write...\n");

	for (i = 0; i < ctx->words; i++) {
		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;

			ddrtest_progress(ctx, i, ctx->words);
		}

		ddrtest_write(&p[i], pattern);
	}

	printf("    Read/compare...\n");

	for (i = 0; i < ctx->words; i++) {
		u32 actual;

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;

			ddrtest_progress(ctx, i, ctx->words);
		}

		actual = ddrtest_read(&p[i]);

		if (ddrtest_compare(ctx,
				    ctx->start + i * sizeof(u32),
				    pattern,
				    actual))
			return -EIO;
	}

	printf("    PASS\n");

	return 0;
}

/* ------------------------------------------------------------- */
/* Checkerboard                                                   */
/* ------------------------------------------------------------- */

static int ddrtest_checkerboard(struct ddrtest_ctx *ctx)
{
	volatile u32 *p;
	u64 i;

	printf("  Checkerboard\n");

	p = (volatile u32 *)ctx->start;

	printf("    Write...\n");

	for (i = 0; i < ctx->words; i++) {
		u32 pattern;

		pattern = (i & 1) ?
			DDRTEST_PATTERN_A :
			DDRTEST_PATTERN_5;

		ddrtest_write(&p[i], pattern);
	}

	printf("    Read/compare...\n");

	for (i = 0; i < ctx->words; i++) {
		u32 expected;
		u32 actual;

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}

		expected = (i & 1) ?
			DDRTEST_PATTERN_A :
			DDRTEST_PATTERN_5;

		actual = ddrtest_read(&p[i]);

		if (ddrtest_compare(ctx,
				    ctx->start + i * sizeof(u32),
				    expected,
				    actual))
			return -EIO;
	}

	printf("    PASS\n");

	return 0;
}

/* ------------------------------------------------------------- */
/* Address test                                                   */
/* ------------------------------------------------------------- */

static int ddrtest_address(struct ddrtest_ctx *ctx)
{
	volatile u32 *p;
	u64 i;

	printf("  Address bus test\n");

	p = (volatile u32 *)ctx->start;

	printf("    Write address...\n");

	for (i = 0; i < ctx->words; i++) {
		u32 value;

		value = (u32)(ctx->start + i * sizeof(u32));

		ddrtest_write(&p[i], value);

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}
	}

	printf("    Read address...\n");

	for (i = 0; i < ctx->words; i++) {
		u32 expected;
		u32 actual;

		expected = (u32)(ctx->start + i * sizeof(u32));
		actual = ddrtest_read(&p[i]);

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}

		if (ddrtest_compare(ctx,
				    ctx->start + i * sizeof(u32),
				    expected,
				    actual))
			return -EIO;
	}

	printf("    PASS\n");

	return 0;
}

/* ------------------------------------------------------------- */
/* Walking 1                                                       */
/* ------------------------------------------------------------- */

static int ddrtest_walking1(struct ddrtest_ctx *ctx)
{
	volatile u32 *p;
	u32 bit;
	u64 i;

	printf("  Walking 1\n");

	p = (volatile u32 *)ctx->start;

	for (bit = 0; bit < 32; bit++) {
		u32 pattern = BIT(bit);

		printf("    bit %u: 0x%08x\n", bit, pattern);

		for (i = 0; i < ctx->words; i++)
			ddrtest_write(&p[i], pattern);

		for (i = 0; i < ctx->words; i++) {
			u32 actual;

			if ((i & 0xffff) == 0) {
				if (ddrtest_abort(ctx))
					return -ECANCELED;
			}

			actual = ddrtest_read(&p[i]);

			if (ddrtest_compare(ctx,
					    ctx->start +
					    i * sizeof(u32),
					    pattern,
					    actual))
				return -EIO;
		}
	}

	printf("    PASS\n");

	return 0;
}

/* ------------------------------------------------------------- */
/* Walking 0                                                       */
/* ------------------------------------------------------------- */

static int ddrtest_walking0(struct ddrtest_ctx *ctx)
{
	volatile u32 *p;
	u32 bit;
	u64 i;

	printf("  Walking 0\n");

	p = (volatile u32 *)ctx->start;

	for (bit = 0; bit < 32; bit++) {
		u32 pattern = ~BIT(bit);

		printf("    bit %u: 0x%08x\n", bit, pattern);

		for (i = 0; i < ctx->words; i++)
			ddrtest_write(&p[i], pattern);

		for (i = 0; i < ctx->words; i++) {
			u32 actual;

			if ((i & 0xffff) == 0) {
				if (ddrtest_abort(ctx))
					return -ECANCELED;
			}

			actual = ddrtest_read(&p[i]);

			if (ddrtest_compare(ctx,
					    ctx->start +
					    i * sizeof(u32),
					    pattern,
					    actual))
				return -EIO;
		}
	}

	printf("    PASS\n");

	return 0;
}

/* ------------------------------------------------------------- */
/* Random test                                                     */
/* ------------------------------------------------------------- */

static u32 ddrtest_prng(u32 *state)
{
	u32 x;

	x = *state;

	/*
	 * xorshift32
	 */
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;

	*state = x;

	return x;
}

static int ddrtest_random(struct ddrtest_ctx *ctx)
{
	volatile u32 *p;
	u32 seed;
	u64 i;

	printf("  Random test\n");
	printf("    Seed: 0x%08x\n", ctx->seed);

	p = (volatile u32 *)ctx->start;
	seed = ctx->seed;

	printf("    Write random...\n");

	for (i = 0; i < ctx->words; i++) {
		u32 value;

		value = ddrtest_prng(&seed);

		ddrtest_write(&p[i], value);

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}
	}

	printf("    Read random...\n");

	seed = ctx->seed;

	for (i = 0; i < ctx->words; i++) {
		u32 expected;
		u32 actual;

		expected = ddrtest_prng(&seed);
		actual = ddrtest_read(&p[i]);

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}

		if (ddrtest_compare(ctx,
				    ctx->start +
				    i * sizeof(u32),
				    expected,
				    actual))
			return -EIO;
	}

	printf("    PASS\n");

	return 0;
}

/* ------------------------------------------------------------- */
/* Random inverse                                                  */
/* ------------------------------------------------------------- */

static int ddrtest_random_inverse(struct ddrtest_ctx *ctx)
{
	volatile u32 *p;
	u32 seed;
	u64 i;

	printf("  Random inverse test\n");
	printf("    Seed: 0x%08x\n", ctx->seed);

	p = (volatile u32 *)ctx->start;
	seed = ctx->seed;

	for (i = 0; i < ctx->words; i++) {
		u32 value;

		value = ~ddrtest_prng(&seed);

		ddrtest_write(&p[i], value);

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}
	}

	seed = ctx->seed;

	for (i = 0; i < ctx->words; i++) {
		u32 expected;
		u32 actual;

		expected = ~ddrtest_prng(&seed);
		actual = ddrtest_read(&p[i]);

		if (ddrtest_compare(ctx,
				    ctx->start +
				    i * sizeof(u32),
				    expected,
				    actual))
			return -EIO;
	}

	printf("    PASS\n");

	return 0;
}

/* ------------------------------------------------------------- */
/* March C-                                                        */
/* ------------------------------------------------------------- */

static int ddrtest_march_cminus(struct ddrtest_ctx *ctx)
{
	volatile u32 *p;
	u64 i;

	printf("  March C-\n");

	p = (volatile u32 *)ctx->start;

	/*
	 * Step 1:
	 * ↑ (w0)
	 */
	printf("    1/6: ↑ w0\n");

	for (i = 0; i < ctx->words; i++)
		ddrtest_write(&p[i], 0);

	/*
	 * Step 2:
	 * ↑ (r0, w1)
	 */
	printf("    2/6: ↑ r0/w1\n");

	for (i = 0; i < ctx->words; i++) {
		u32 actual;

		actual = ddrtest_read(&p[i]);

		if (ddrtest_compare(ctx,
				    ctx->start +
				    i * sizeof(u32),
				    0,
				    actual))
			return -EIO;

		ddrtest_write(&p[i], 0xffffffff);

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}
	}

	/*
	 * Step 3:
	 * ↑ (r1, w0)
	 */
	printf("    3/6: ↑ r1/w0\n");

	for (i = 0; i < ctx->words; i++) {
		u32 actual;

		actual = ddrtest_read(&p[i]);

		if (ddrtest_compare(ctx,
				    ctx->start +
				    i * sizeof(u32),
				    0xffffffff,
				    actual))
			return -EIO;

		ddrtest_write(&p[i], 0);

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}
	}

	/*
	 * Step 4:
	 * ↓ (r0, w1)
	 */
	printf("    4/6: ↓ r0/w1\n");

	for (i = ctx->words; i > 0; i--) {
		u64 index = i - 1;
		u32 actual;

		actual = ddrtest_read(&p[index]);

		if (ddrtest_compare(ctx,
				    ctx->start +
				    index * sizeof(u32),
				    0,
				    actual))
			return -EIO;

		ddrtest_write(&p[index], 0xffffffff);

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}
	}

	/*
	 * Step 5:
	 * ↓ (r1, w0)
	 */
	printf("    5/6: ↓ r1/w0\n");

	for (i = ctx->words; i > 0; i--) {
		u64 index = i - 1;
		u32 actual;

		actual = ddrtest_read(&p[index]);

		if (ddrtest_compare(ctx,
				    ctx->start +
				    index * sizeof(u32),
				    0xffffffff,
				    actual))
			return -EIO;

		ddrtest_write(&p[index], 0);

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}
	}

	/*
	 * Step 6:
	 * ↓ (r0)
	 */
	printf("    6/6: ↓ r0\n");

	for (i = ctx->words; i > 0; i--) {
		u64 index = i - 1;
		u32 actual;

		actual = ddrtest_read(&p[index]);

		if (ddrtest_compare(ctx,
				    ctx->start +
				    index * sizeof(u32),
				    0,
				    actual))
			return -EIO;

		if ((i & 0xffff) == 0) {
			if (ddrtest_abort(ctx))
				return -ECANCELED;
		}
	}

	printf("    PASS\n");

	return 0;
}

/* ------------------------------------------------------------- */
/* One complete iteration                                          */
/* ------------------------------------------------------------- */

static int ddrtest_iteration(struct ddrtest_ctx *ctx)
{
	int ret;

	printf("\n");
	printf("========================================\n");
	printf("Iteration %u/%u\n",
	       ctx->iteration,
	       ctx->iterations);
	printf("========================================\n");

	ret = ddrtest_fixed_pattern(ctx, DDRTEST_PATTERN_0);
	if (ret)
		return ret;

	ret = ddrtest_fixed_pattern(ctx, DDRTEST_PATTERN_1);
	if (ret)
		return ret;

	ret = ddrtest_fixed_pattern(ctx, DDRTEST_PATTERN_A);
	if (ret)
		return ret;

	ret = ddrtest_fixed_pattern(ctx, DDRTEST_PATTERN_5);
	if (ret)
		return ret;

	ret = ddrtest_checkerboard(ctx);
	if (ret)
		return ret;

	ret = ddrtest_address(ctx);
	if (ret)
		return ret;

	ret = ddrtest_walking1(ctx);
	if (ret)
		return ret;

	ret = ddrtest_walking0(ctx);
	if (ret)
		return ret;

	ret = ddrtest_random(ctx);
	if (ret)
		return ret;

	ret = ddrtest_random_inverse(ctx);
	if (ret)
		return ret;

	ret = ddrtest_march_cminus(ctx);
	if (ret)
		return ret;

	return 0;
}

/* ------------------------------------------------------------- */
/* Command                                                        */
/* ------------------------------------------------------------- */

static int do_ax_memtest(struct cmd_tbl *cmdtp,
		      int flag,
		      int argc,
		      char *const argv[])
{
	struct ddrtest_ctx ctx;
	int ret;
    char *endp;

	if (argc != 4)
		return CMD_RET_USAGE;

	memset(&ctx, 0, sizeof(ctx));

	ctx.start = simple_strtol(argv[1], &endp, 16);
	ctx.size = simple_strtol(argv[2], &endp, 16);
	ctx.iterations = simple_strtol(argv[3], &endp, 16);

	ctx.max_errors = DDRTEST_MAX_ERRORS;
	ctx.seed = DDRTEST_DEFAULT_SEED;

	if (ctx.size < sizeof(u32)) {
		printf("ERROR: size must be >= 4 bytes\n");
		return CMD_RET_FAILURE;
	}

	if (ctx.start & 0x3) {
		printf("ERROR: start address must be 4-byte aligned\n");
		return CMD_RET_FAILURE;
	}

	if (ctx.size & 0x3) {
		printf("ERROR: size must be 4-byte aligned\n");
		return CMD_RET_FAILURE;
	}

	if (!ctx.iterations) {
		printf("ERROR: iterations must be > 0\n");
		return CMD_RET_FAILURE;
	}

	ctx.end = ctx.start + ctx.size - 1;
	ctx.words = ctx.size / sizeof(u32);

	if (ctx.end < ctx.start) {
		printf("ERROR: address overflow\n");
		return CMD_RET_FAILURE;
	}

	printf("\n");
	printf("====================================================\n");
	printf("                 U-BOOT DDR TEST\n");
	printf("====================================================\n");
	printf("Start       : 0x%08lx\n", ctx.start);
	printf("End         : 0x%08lx\n", ctx.end);
	printf("Size        : 0x%08lx (%lu MB)\n",
	       ctx.size,
	       ctx.size / 1024 / 1024);
	printf("Words       : %llu\n",
	       ctx.words);
	printf("Iterations  : %u\n",
	       ctx.iterations);
	printf("Seed        : 0x%08x\n",
	       ctx.seed);
	printf("Max errors  : %u\n",
	       ctx.max_errors);
	printf("====================================================\n");

	/*
	 * Important:
	 *
	 * The test area MUST NOT overlap:
	 *
	 *   - U-Boot image
	 *   - stack
	 *   - malloc pool
	 *   - global data
	 *   - DTB
	 *   - loaded kernel
	 *   - initrd
	 *   - other reserved memory
	 */

	printf("\nWARNING:\n");
	printf("The selected DDR region will be destroyed.\n");
	printf("Make sure it does not contain U-Boot/runtime data.\n\n");

	if (ctrlc()) {
		printf("Aborted\n");
		return CMD_RET_FAILURE;
	}

	ddrtest_errors = 0;

	for (ctx.iteration = 1;
	     ctx.iteration <= ctx.iterations;
	     ctx.iteration++) {

		ret = ddrtest_iteration(&ctx);

		if (ret) {
			printf("\n");
			printf("====================================================\n");
			printf("DDR TEST FAILED\n");
			printf("Iteration : %u\n", ctx.iteration);
			printf("Errors    : %u\n", ctx.errors);
			printf("====================================================\n");

			return CMD_RET_FAILURE;
		}

		printf("\nIteration %u: PASS\n",
		       ctx.iteration);
	}

	printf("\n");
	printf("====================================================\n");
	printf("              DDR TEST PASSED\n");
	printf("====================================================\n");
	printf("Iterations : %u\n", ctx.iterations);
	printf("Errors     : %u\n", ctx.errors);
	printf("====================================================\n");

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	ax_memtest, 4, 1, do_ax_memtest,
    "Axiado bare-metal DDR memory stress test",
	"<start> <size> <iterations>\n"
	"    - Run DDR memory stress test\n"
	"    - Fixed patterns\n"
	"    - Checkerboard\n"
	"    - Address test\n"
	"    - Walking 1/0\n"
	"    - Random/inverse random\n"
	"    - March C-\n"
	"\n"
	"Example:\n"
	"    ax_memtest 0x80000000 0x10000000 10\n"
);
