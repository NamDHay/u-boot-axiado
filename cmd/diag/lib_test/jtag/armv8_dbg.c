// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * ARMv8 external debug over the Host JTAG DAP (shared by r52.c, a53.c).
 * Ported from the bare-metal __r52_* helpers in ~/jtag/jtag_cmd.c and
 * extended with the AArch64 path for the A53.
 *
 * Halt/restart go through the core's CTI: channel 0 -> trigger out 0
 * (debug request), channel 1 -> trigger out 1 (restart), pulsed with
 * CTIAPPPULSE. In Debug state, instructions are fed through EDITR and
 * data moves through DBGDTRRX/DBGDTRTX.
 */

#include <stdio.h>
#include <vsprintf.h>
#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/types.h>

#include "armv8_dbg.h"
#include "jtag_dap.h"

/* External debug registers (offset from core debug base) */
#define ED_EDECR		0x024U
#define ED_DTRRX		0x080U	/* DBGDTRRX_EL0: debugger -> core */
#define ED_EDITR		0x084U
#define ED_EDSCR		0x088U
#define ED_DTRTX		0x08CU	/* DBGDTRTX_EL0: core -> debugger */
#define ED_EDRCR		0x090U
#define ED_OSLAR		0x300U
#define ED_EDPRCR		0x310U
#define ED_EDPRSR		0x314U
#define ED_MIDR			0xD00U
#define ED_LAR			0xFB0U
#define ED_DEVARCH		0xFBCU

#define CS_UNLOCK		0xC5ACCE55U

/* EDSCR */
#define EDSCR_STATUS_MASK	0x3FU
#define EDSCR_STATUS_RESTARTING	0x01U
#define EDSCR_STATUS_NON_DEBUG	0x02U
#define EDSCR_ERR		BIT(6)
#define EDSCR_EL(v)		(((v) >> 8) & 3U)
#define EDSCR_RW(v)		(((v) >> 10) & 0xFU)
#define EDSCR_HDE		BIT(14)
#define EDSCR_SDD		BIT(16)
#define EDSCR_INTDIS_MASK	(3U << 22)
#define EDSCR_ITE		BIT(24)
#define EDSCR_TXFULL		BIT(29)
#define EDSCR_RXFULL		BIT(30)

/* EDRCR */
#define EDRCR_CSE		BIT(2)	/* clear sticky error */
#define EDRCR_CSPA		BIT(3)	/* clear sticky pipeline advance */

/* EDECR */
#define EDECR_SS		BIT(2)	/* halting step enable */

/* EDPRSR */
#define EDPRSR_PU		BIT(0)
#define EDPRSR_SPD		BIT(1)
#define EDPRSR_R		BIT(2)
#define EDPRSR_HALTED		BIT(4)
#define EDPRSR_OSLK		BIT(5)
#define EDPRSR_DLK		BIT(6)
#define EDPRSR_SDR		BIT(11)

/* CTI (offset from CTI base) */
#define CTI_CONTROL		0x000U
#define CTI_INTACK		0x010U
#define CTI_APPPULSE		0x01CU
#define CTI_OUTEN(n)		(0x0A0U + 4U * (n))
#define CTI_TRIGOUTSTATUS	0x134U
#define CTI_GATE		0x140U

#define CTI_CH_HALT		BIT(0)	/* channel 0 -> trigger out 0 (EDBGRQ) */
#define CTI_CH_RESTART		BIT(1)	/* channel 1 -> trigger out 1 (restart) */
#define CTI_TRIG_HALT		BIT(0)
/* Value from the working reference flow; channels 0/1 pass to the CTM */
#define CTI_GATE_VAL		(CTI_CH_HALT | CTI_CH_RESTART)

/*
 * AArch32 (T32) opcodes. EDITR takes the first halfword in [15:0], so the
 * 32-bit T32 encoding is halfword-swapped when written (T32_ITR()).
 */
#define T32_ITR(op)		((((op) & 0xFFFFU) << 16) | ((op) >> 16))
#define T32_MCR_DTRTX(t)	(0xEE000E15U | ((t) << 12))	/* MCR p14,0,Rt,c0,c5,0 */
#define T32_MRC_DTRRX(t)	(0xEE100E15U | ((t) << 12))	/* MRC p14,0,Rt,c0,c5,0 */
#define T32_MRC_DSPSR(t)	(0xEE740F15U | ((t) << 12))	/* MRC p15,3,Rt,c4,c5,0 */
#define T32_MCR_DSPSR(t)	(0xEE640F15U | ((t) << 12))	/* MCR p15,3,Rt,c4,c5,0 */
#define T32_MRC_DLR(t)		(0xEE740F35U | ((t) << 12))	/* MRC p15,3,Rt,c4,c5,1 */
#define T32_MCR_DLR(t)		(0xEE640F35U | ((t) << 12))	/* MCR p15,3,Rt,c4,c5,1 */

/* AArch64 (A64) opcodes */
#define A64_MSR_DBGDTR(t)	(0xD5130400U | (t))	/* MSR DBGDTR_EL0, Xt  */
#define A64_MRS_DBGDTR(t)	(0xD5330400U | (t))	/* MRS Xt, DBGDTR_EL0  */
#define A64_MRS_DLR_X0		0xD53B4520U		/* MRS X0, DLR_EL0     */
#define A64_MSR_DLR_X0		0xD51B4520U		/* MSR DLR_EL0, X0     */
#define A64_MRS_DSPSR_X0	0xD53B4500U		/* MRS X0, DSPSR_EL0   */
#define A64_MSR_DSPSR_X0	0xD51B4500U		/* MSR DSPSR_EL0, X0   */
#define A64_MOV_X0_SP		0x910003E0U		/* ADD X0, SP, #0      */
#define A64_MOV_SP_X0		0x9100001FU		/* ADD SP, X0, #0      */

#define ARMV8_POLL		100

/* ------------------------------------------------------------------ */
/* Register access                                                    */
/* ------------------------------------------------------------------ */

static int dbg_rd(const struct armv8_core *c, u32 off, u32 *v)
{
	return mem_read32(c->ap, c->dbg + off, v);
}

static int dbg_wr(const struct armv8_core *c, u32 off, u32 v)
{
	return mem_write32(c->ap, c->dbg + off, v);
}

static int cti_rd(const struct armv8_core *c, u32 off, u32 *v)
{
	return mem_read32(c->ap, c->cti + off, v);
}

static int cti_wr(const struct armv8_core *c, u32 off, u32 v)
{
	return mem_write32(c->ap, c->cti + off, v);
}

static bool edscr_halted(u32 edscr)
{
	u32 s = edscr & EDSCR_STATUS_MASK;

	return s != EDSCR_STATUS_RESTARTING && s != EDSCR_STATUS_NON_DEBUG;
}

static const char *edscr_status_name(u32 edscr)
{
	switch (edscr & EDSCR_STATUS_MASK) {
	case 0x01: return "restarting";
	case 0x02: return "running (non-debug)";
	case 0x07: return "halted: breakpoint";
	case 0x13: return "halted: external debug request";
	case 0x1B: return "halted: step, normal";
	case 0x1F: return "halted: step, exclusive";
	case 0x23: return "halted: OS unlock catch";
	case 0x27: return "halted: reset catch";
	case 0x2B: return "halted: watchpoint";
	case 0x2F: return "halted: HLT instruction";
	case 0x33: return "halted: software access to debug register";
	case 0x37: return "halted: exception catch";
	case 0x3B: return "halted: step, no syndrome";
	default:   return "unknown";
	}
}

/* Poll EDSCR until (EDSCR & mask) == want */
static int edscr_wait(const struct armv8_core *c, u32 mask, u32 want, u32 *v)
{
	int i, ret = -ETIMEDOUT;

	for (i = 0; i < ARMV8_POLL; i++) {
		ret = dbg_rd(c, ED_EDSCR, v);
		if (!ret && (*v & mask) == want)
			return 0;
	}
	return ret ? ret : -ETIMEDOUT;
}

static int wait_halt_state(const struct armv8_core *c, bool halted, u32 *v)
{
	int i, ret = -ETIMEDOUT;

	for (i = 0; i < ARMV8_POLL; i++) {
		ret = dbg_rd(c, ED_EDSCR, v);
		if (!ret && edscr_halted(*v) == halted)
			return 0;
	}
	return ret ? ret : -ETIMEDOUT;
}

static int core_open(const struct armv8_core *c)
{
	return mem_ap_open(c->ap);
}

/*
 * Unlock debug + CTI, clear the OS lock, enable halting debug and route
 * CTI channel 0 -> halt, channel 1 -> restart.
 */
static int core_setup(const struct armv8_core *c)
{
	u32 v;
	int ret;

	ret = core_open(c);
	if (!ret)
		ret = dbg_wr(c, ED_LAR, CS_UNLOCK);
	if (!ret)
		ret = cti_wr(c, ED_LAR, CS_UNLOCK);
	if (!ret)
		ret = dbg_wr(c, ED_OSLAR, 0U);
	if (!ret)
		ret = dbg_rd(c, ED_EDPRSR, &v);	/* also clears EDPRSR.SPD */
	if (ret)
		goto fail;
	if (!(v & EDPRSR_PU)) {
		printf("%s: core powered down (EDPRSR 0x%08x)\n", c->name, v);
		return -ENODEV;
	}

	ret = dbg_rd(c, ED_EDSCR, &v);
	if (!ret && !(v & EDSCR_HDE))
		ret = dbg_wr(c, ED_EDSCR, v | EDSCR_HDE);
	if (!ret)
		ret = cti_wr(c, CTI_CONTROL, 1U);
	if (!ret)
		ret = cti_wr(c, CTI_OUTEN(0), CTI_CH_HALT);
	if (!ret)
		ret = cti_wr(c, CTI_OUTEN(1), CTI_CH_RESTART);
	if (!ret)
		ret = cti_wr(c, CTI_GATE, CTI_GATE_VAL);
	if (!ret)
		return 0;
fail:
	printf("%s: debug setup failed (ack %u)\n", c->name, dap_last_ack());
	return ret;
}

int armv8_require_halt(const struct armv8_core *c)
{
	u32 v;
	int ret;

	ret = core_open(c);
	if (!ret)
		ret = dbg_rd(c, ED_EDSCR, &v);
	if (ret)
		return ret;
	if (!edscr_halted(v)) {
		printf("%s not halted (EDSCR 0x%08x) - halt it first\n", c->name, v);
		return -EBUSY;
	}
	return 0;
}

/* True when the halted core is in AArch64 at its current EL */
static int core_is_a64(const struct armv8_core *c, bool *a64)
{
	u32 v;
	int ret;

	*a64 = false;
	if (!c->aarch64)
		return 0;
	ret = dbg_rd(c, ED_EDSCR, &v);
	if (ret)
		return ret;
	*a64 = (EDSCR_RW(v) >> EDSCR_EL(v)) & 1U;
	return 0;
}

/* ------------------------------------------------------------------ */
/* Instruction execution in Debug state                               */
/* ------------------------------------------------------------------ */

/* Write one (already EDITR-formatted) instruction, wait ITE, check ERR */
static int itr_exec(const struct armv8_core *c, u32 itr, u32 *edscr)
{
	int ret;

	ret = edscr_wait(c, EDSCR_ITE, EDSCR_ITE, edscr);
	if (!ret)
		ret = dbg_wr(c, ED_EDITR, itr);
	if (!ret)
		ret = edscr_wait(c, EDSCR_ITE, EDSCR_ITE, edscr);
	if (ret) {
		printf("%s: ITR 0x%08x timeout (EDSCR 0x%08x)\n", c->name, itr, *edscr);
		return ret;
	}
	if (*edscr & EDSCR_ERR) {
		printf("%s: ITR 0x%08x error (EDSCR 0x%08x)\n", c->name, itr, *edscr);
		dbg_wr(c, ED_EDRCR, EDRCR_CSE);
		return -EIO;
	}
	return 0;
}

static int t32_exec(const struct armv8_core *c, u32 op, u32 *edscr)
{
	return itr_exec(c, T32_ITR(op), edscr);
}

/* AArch32: Rt -> host via MCR DBGDTRTXint */
static int t32_read_r(const struct armv8_core *c, u32 t, u32 *v)
{
	u32 d;
	int ret;

	ret = t32_exec(c, T32_MCR_DTRTX(t), &d);
	if (ret)
		return ret;
	if (!(d & EDSCR_TXFULL)) {
		printf("%s: DTRTX empty after reading R%u (EDSCR 0x%08x)\n", c->name, t, d);
		return -EIO;
	}
	return dbg_rd(c, ED_DTRTX, v);
}

/* AArch32: host -> Rt via MRC DBGDTRRXint */
static int t32_write_r(const struct armv8_core *c, u32 t, u32 v)
{
	u32 d;
	int ret;

	ret = dbg_wr(c, ED_DTRRX, v);
	if (!ret)
		ret = t32_exec(c, T32_MRC_DTRRX(t), &d);
	return ret;
}

/* AArch32: run op with R0 as scratch (R0 is saved/restored) */
static int t32_read_sys(const struct armv8_core *c, u32 op, u32 *v)
{
	u32 r0, d;
	int ret;

	ret = t32_read_r(c, 0, &r0);
	if (!ret)
		ret = t32_exec(c, op, &d);
	if (!ret)
		ret = t32_read_r(c, 0, v);
	if (!ret)
		ret = t32_write_r(c, 0, r0);
	return ret;
}

static int t32_write_sys(const struct armv8_core *c, u32 op, u32 v)
{
	u32 r0, d;
	int ret;

	ret = t32_read_r(c, 0, &r0);
	if (!ret)
		ret = t32_write_r(c, 0, v);
	if (!ret)
		ret = t32_exec(c, op, &d);
	if (!ret)
		ret = t32_write_r(c, 0, r0);
	return ret;
}

/* AArch64: Xt -> host via MSR DBGDTR_EL0 (DTRRX = [63:32], DTRTX = [31:0]) */
static int a64_read_x(const struct armv8_core *c, u32 t, u64 *v)
{
	u32 d, hi, lo;
	int ret;

	ret = itr_exec(c, A64_MSR_DBGDTR(t), &d);
	if (ret)
		return ret;
	if (!(d & EDSCR_TXFULL)) {
		printf("%s: DTRTX empty after reading X%u (EDSCR 0x%08x)\n", c->name, t, d);
		return -EIO;
	}
	ret = dbg_rd(c, ED_DTRRX, &hi);
	if (!ret)
		ret = dbg_rd(c, ED_DTRTX, &lo);
	if (!ret)
		*v = ((u64)hi << 32) | lo;
	return ret;
}

/* AArch64: host -> Xt via MRS DBGDTR_EL0 (DTRTX = [63:32], DTRRX = [31:0]) */
static int a64_write_x(const struct armv8_core *c, u32 t, u64 v)
{
	u32 d;
	int ret;

	ret = dbg_wr(c, ED_DTRTX, (u32)(v >> 32));
	if (!ret)
		ret = dbg_wr(c, ED_DTRRX, (u32)v);
	if (!ret)
		ret = itr_exec(c, A64_MRS_DBGDTR(t), &d);
	return ret;
}

static int a64_read_sys(const struct armv8_core *c, u32 op, u64 *v)
{
	u64 x0;
	u32 d;
	int ret;

	ret = a64_read_x(c, 0, &x0);
	if (!ret)
		ret = itr_exec(c, op, &d);
	if (!ret)
		ret = a64_read_x(c, 0, v);
	if (!ret)
		ret = a64_write_x(c, 0, x0);
	return ret;
}

static int a64_write_sys(const struct armv8_core *c, u32 op, u64 v)
{
	u64 x0;
	u32 d;
	int ret;

	ret = a64_read_x(c, 0, &x0);
	if (!ret)
		ret = a64_write_x(c, 0, v);
	if (!ret)
		ret = itr_exec(c, op, &d);
	if (!ret)
		ret = a64_write_x(c, 0, x0);
	return ret;
}

/* ------------------------------------------------------------------ */
/* Target ops                                                         */
/* ------------------------------------------------------------------ */

int armv8_probe(const struct armv8_core *c)
{
	u32 midr, arch, edscr, prsr;
	int ret;

	ret = core_setup(c);
	if (ret)
		return ret;

	ret = dbg_rd(c, ED_MIDR, &midr);
	if (!ret)
		ret = dbg_rd(c, ED_DEVARCH, &arch);
	if (!ret)
		ret = dbg_rd(c, ED_EDSCR, &edscr);
	if (!ret)
		ret = dbg_rd(c, ED_EDPRSR, &prsr);
	if (ret) {
		printf("%s debug read failed (ack %u)\n", c->name, dap_last_ack());
		return ret;
	}

	printf("%s MIDR     0x%08x (impl 0x%02x part 0x%03x r%up%u)%s\n", c->name,
	       midr, midr >> 24, (midr >> 4) & 0xFFF, (midr >> 20) & 0xF, midr & 0xF,
	       ((midr >> 4) & 0xFFF) == 0xD03 ? " Cortex-A53" :
	       ((midr >> 4) & 0xFFF) == 0xD13 ? " Cortex-R52" : "");
	printf("%s DEVARCH  0x%08x\n", c->name, arch);
	printf("%s EDSCR    0x%08x (%s)\n", c->name, edscr, edscr_status_name(edscr));
	printf("%s EDPRSR   0x%08x\n", c->name, prsr);
	return 0;
}

int armv8_halt(const struct armv8_core *c)
{
	u32 v = 0;
	int ret;

	ret = core_setup(c);
	if (!ret)
		ret = cti_wr(c, CTI_INTACK, CTI_TRIG_HALT);
	if (!ret)
		ret = cti_wr(c, CTI_APPPULSE, CTI_CH_HALT);
	if (!ret)
		ret = wait_halt_state(c, true, &v);
	/* Ack the halt trigger now so a later restart is not re-halted */
	if (!ret)
		ret = cti_wr(c, CTI_INTACK, CTI_TRIG_HALT);

	printf("%s halt %s (EDSCR 0x%08x %s)\n", c->name, ret ? "FAILED" : "OK", v,
	       edscr_status_name(v));
	if (ret && (v & EDSCR_SDD))
		printf("  EDSCR.SDD set: secure debug disabled\n");
	return ret;
}

/* Restart via CTI channel 1; caller decides what EDSCR state to expect */
static int core_restart(const struct armv8_core *c)
{
	u32 v;
	int i, ret;

	ret = dbg_wr(c, ED_EDRCR, EDRCR_CSE | EDRCR_CSPA);
	if (!ret)
		ret = cti_wr(c, CTI_INTACK, CTI_TRIG_HALT);
	for (i = 0; !ret && i < ARMV8_POLL; i++) {
		ret = cti_rd(c, CTI_TRIGOUTSTATUS, &v);
		if (!ret && !(v & CTI_TRIG_HALT))
			break;
	}
	if (!ret && i == ARMV8_POLL)
		ret = -ETIMEDOUT;
	if (!ret)
		ret = cti_wr(c, CTI_APPPULSE, CTI_CH_RESTART);
	return ret;
}

int armv8_run(const struct armv8_core *c)
{
	u32 v = 0;
	int ret;

	ret = core_setup(c);
	if (!ret)
		ret = core_restart(c);
	if (!ret)
		ret = wait_halt_state(c, false, &v);

	printf("%s run %s (EDSCR 0x%08x %s)\n", c->name, ret ? "FAILED" : "OK", v,
	       edscr_status_name(v));
	return ret;
}

int armv8_step(const struct armv8_core *c)
{
	u32 ecr, edscr, v = 0;
	u64 pc = 0;
	bool a64;
	int ret, ret2;

	ret = armv8_require_halt(c);
	if (!ret)
		ret = core_setup(c);
	if (!ret)
		ret = dbg_rd(c, ED_EDECR, &ecr);
	if (!ret)
		ret = dbg_rd(c, ED_EDSCR, &edscr);
	if (ret)
		return ret;

	/* Halting step with interrupts masked (EDSCR.INTdis) */
	ret = dbg_wr(c, ED_EDSCR, edscr | EDSCR_INTDIS_MASK);
	if (!ret)
		ret = dbg_wr(c, ED_EDECR, ecr | EDECR_SS);
	if (!ret)
		ret = core_restart(c);
	if (!ret)
		ret = wait_halt_state(c, true, &v);

	ret2 = dbg_wr(c, ED_EDECR, ecr & ~EDECR_SS);
	if (!ret2) {
		u32 now;

		ret2 = dbg_rd(c, ED_EDSCR, &now);
		if (!ret2)
			ret2 = dbg_wr(c, ED_EDSCR, (now & ~EDSCR_INTDIS_MASK) |
				      (edscr & EDSCR_INTDIS_MASK));
	}
	if (!ret)
		ret = ret2;
	if (!ret)
		ret = core_is_a64(c, &a64);
	if (!ret)
		ret = armv8_reg_read(c, a64 ? ARMV8_A64_PC : ARMV8_A32_PC, &pc);

	printf("%s step %s (EDSCR 0x%08x %s, PC 0x%llx)\n", c->name,
	       ret ? "FAILED" : "OK", v, edscr_status_name(v), pc);
	return ret;
}

int armv8_state(const struct armv8_core *c)
{
	u32 edscr, prsr, ecr, trig;
	int ret;

	ret = core_open(c);
	if (!ret)
		ret = dbg_rd(c, ED_EDSCR, &edscr);
	if (!ret)
		ret = dbg_rd(c, ED_EDPRSR, &prsr);
	if (!ret)
		ret = dbg_rd(c, ED_EDECR, &ecr);
	if (!ret)
		ret = cti_rd(c, CTI_TRIGOUTSTATUS, &trig);
	if (ret) {
		printf("%s state read failed (ack %u)\n", c->name, dap_last_ack());
		return ret;
	}

	printf("EDSCR  0x%08x: %s, EL%u %s%s%s%s%s\n", edscr, edscr_status_name(edscr),
	       EDSCR_EL(edscr),
	       !edscr_halted(edscr) ? "" :
	       (c->aarch64 && ((EDSCR_RW(edscr) >> EDSCR_EL(edscr)) & 1U)) ?
	       "AArch64" : "AArch32",
	       edscr & EDSCR_HDE ? " HDE" : "",
	       edscr & EDSCR_SDD ? " SDD" : "",
	       edscr & EDSCR_ERR ? " ERR" : "",
	       edscr & EDSCR_TXFULL ? " TXfull" : "");
	printf("EDPRSR 0x%08x:%s%s%s%s%s%s%s\n", prsr,
	       prsr & EDPRSR_PU ? " PU" : " POWERED-DOWN",
	       prsr & EDPRSR_SPD ? " SPD" : "",
	       prsr & EDPRSR_R ? " RESET" : "",
	       prsr & EDPRSR_HALTED ? " HALTED" : "",
	       prsr & EDPRSR_OSLK ? " OSLK" : "",
	       prsr & EDPRSR_DLK ? " DLK" : "",
	       prsr & EDPRSR_SDR ? " SDR" : "");
	printf("EDECR  0x%08x\n", ecr);
	printf("CTI TRIGOUTSTATUS 0x%08x\n", trig);
	return 0;
}

int armv8_reg_read(const struct armv8_core *c, u32 n, u64 *v)
{
	bool a64;
	u32 r;
	int ret;

	ret = armv8_require_halt(c);
	if (!ret)
		ret = core_is_a64(c, &a64);
	if (ret)
		return ret;

	if (a64) {
		if (n > ARMV8_A64_REG_MAX)
			return -EINVAL;
		if (n < ARMV8_A64_SP)
			return a64_read_x(c, n, v);
		if (n == ARMV8_A64_SP)
			return a64_read_sys(c, A64_MOV_X0_SP, v);
		ret = a64_read_sys(c, n == ARMV8_A64_PC ? A64_MRS_DLR_X0 :
				   A64_MRS_DSPSR_X0, v);
		return ret;
	}

	if (n > ARMV8_A32_REG_MAX)
		return -EINVAL;
	if (n < ARMV8_A32_PC)
		ret = t32_read_r(c, n, &r);
	else
		ret = t32_read_sys(c, n == ARMV8_A32_PC ? T32_MRC_DLR(0) :
				   T32_MRC_DSPSR(0), &r);
	if (!ret)
		*v = r;
	return ret;
}

int armv8_reg_write(const struct armv8_core *c, u32 n, u64 v)
{
	bool a64;
	int ret;

	ret = armv8_require_halt(c);
	if (!ret)
		ret = core_is_a64(c, &a64);
	if (ret)
		return ret;

	if (a64) {
		if (n > ARMV8_A64_REG_MAX)
			return -EINVAL;
		if (n < ARMV8_A64_SP)
			return a64_write_x(c, n, v);
		if (n == ARMV8_A64_SP)
			return a64_write_sys(c, A64_MOV_SP_X0, v);
		return a64_write_sys(c, n == ARMV8_A64_PC ? A64_MSR_DLR_X0 :
				     A64_MSR_DSPSR_X0, v);
	}

	if (n > ARMV8_A32_REG_MAX)
		return -EINVAL;
	if (n < ARMV8_A32_PC)
		return t32_write_r(c, n, (u32)v);
	return t32_write_sys(c, n == ARMV8_A32_PC ? T32_MCR_DLR(0) :
			     T32_MCR_DSPSR(0), (u32)v);
}

int armv8_regs(const struct armv8_core *c)
{
	static const char *const a32_name[] = {
		"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
		"r10", "r11", "r12", "sp", "lr", "pc", "cpsr",
	};
	u32 edscr, n, max;
	bool a64;
	u64 v;
	int ret;

	ret = armv8_require_halt(c);
	if (!ret)
		ret = core_is_a64(c, &a64);
	if (!ret)
		ret = dbg_rd(c, ED_EDSCR, &edscr);
	if (ret)
		return ret;

	printf("%s halted in %s EL%u\n", c->name, a64 ? "AArch64" : "AArch32",
	       EDSCR_EL(edscr));
	max = a64 ? ARMV8_A64_REG_MAX : ARMV8_A32_REG_MAX;
	for (n = 0; n <= max; n++) {
		char name[8];

		ret = armv8_reg_read(c, n, &v);
		if (a64)
			snprintf(name, sizeof(name), "%s",
				 n == ARMV8_A64_SP ? "sp" : n == ARMV8_A64_PC ? "pc" :
				 n == ARMV8_A64_PSTATE ? "pstate" : "");
		if (a64 && n < ARMV8_A64_SP)
			snprintf(name, sizeof(name), "x%u", n);
		if (ret) {
			printf("%-7s read failed (%d)\n", a64 ? name : a32_name[n], ret);
			return ret;
		}
		if (a64)
			printf("%-7s 0x%016llx\n", name, v);
		else
			printf("%-7s 0x%08x\n", a32_name[n], (u32)v);
	}
	return 0;
}
