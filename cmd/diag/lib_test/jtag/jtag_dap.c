// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * Host JTAG test library: TAP chain + DAP client on top of ax_jtag_ops.
 * Ported from the bare-metal jtag_cmd.c (__dap_*, __ap_*, __mem_*).
 */

#include <stdio.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>

#include "ax_jtag.h"
#include "jtag_dap.h"

#define DAP_RETRY	100

static u8 s_tdo_lag = 1U;
static u8 s_ack;
static bool s_dap_open;
/* MEM-APs already opened (CSW set up); debug AP and AXI-AP alternate */
#define MEM_AP_CACHE	4
static u32 s_mem_ap_open[MEM_AP_CACHE];
static u32 s_mem_ap_cnt;

/* ------------------------------------------------------------------ */
/* TAP level                                                          */
/* ------------------------------------------------------------------ */

int jtag_ir_scan(u32 ir, u32 bits)
{
	struct jtag_xfer x = {
		.type = JTAG_SIR_XFER,
		.direction = JTAG_WRITE_XFER,
		.length = bits,
	};
	u8 buf[4];

	if (!bits || bits > 32)
		return -EINVAL;

	memcpy(buf, &ir, sizeof(buf));
	return ax_jtag_xfer(AX_JTAG_PORT, &x, buf);
}

int jtag_dr_scan(u64 tdi, u32 bits, u64 *tdo)
{
	struct jtag_xfer x = {
		.type = JTAG_SDR_XFER,
		.direction = JTAG_READ_WRITE_XFER,
		.length = bits,
	};
	u8 buf[8];
	int ret;

	if (!bits || bits > 64)
		return -EINVAL;

	memcpy(buf, &tdi, sizeof(buf));
	ret = ax_jtag_xfer(AX_JTAG_PORT, &x, buf);
	if (ret)
		return ret;

	if (tdo) {
		*tdo = 0;
		memcpy(tdo, buf, sizeof(buf));
		if (bits < 64)
			*tdo &= (1ULL << bits) - 1ULL;
	}
	return 0;
}

void jtag_print_idcode(const char *name, u32 id)
{
	printf("%-12s IDCODE 0x%08x  version 0x%x part 0x%04x mfg 0x%03x\n",
	       name, id, (id >> 28) & 0xF, (id >> 12) & 0xFFFF,
	       (id >> 1) & 0x7FF);
}

void dap_invalidate(void)
{
	s_dap_open = false;
	s_mem_ap_cnt = 0;
}

int jtag_start(u32 freq_hz)
{
	int ret;

	dap_invalidate();

	ret = ax_jtag_enable(AX_JTAG_PORT);
	if (ret)
		return ret;

	if (freq_hz) {
		ret = ax_jtag_freq_set(AX_JTAG_PORT, freq_hz);
		if (ret && ret != -ENOSYS)
			return ret;
	}
	return 0;
}

int jtag_stop(void)
{
	dap_invalidate();
	return ax_jtag_disable(AX_JTAG_PORT);
}

/* After TLR the master TAP selects IDCODE, so a plain 32-bit DR scan reads it. */
int jtag_read_master_idcode(u32 *id)
{
	u64 r = 0;
	int ret;

	ret = jtag_dr_scan(0, 32, &r);
	if (ret)
		return ret;
	*id = (u32)r;

	if (*id == 0xFFFFFFFFU) {
		printf("IDCODE 0xFFFFFFFF: TDO stuck high - target TAP not responding\n");
		printf("  (TDO not connected, voltage mismatch, target unpowered/in reset, SWD-only)\n");
		return -ENODEV;
	}
	if (*id == 0) {
		printf("IDCODE 0x00000000: TDO stuck low\n");
		return -ENODEV;
	}
	if (!(*id & 1U)) {
		printf("IDCODE LSB = 0 (0x%08x): TAP is in BYPASS, no IDCODE register\n", *id);
		return -ENODEV;
	}
	return 0;
}

/* Master TAP: IR = T_USER0, DR = tdr_secondary_tap_sel -> DAP TAP joins the chain. */
int jtag_enable_dap_tap(void)
{
	int ret;

	ret = jtag_ir_scan(JTAG_MASTER_IR_USER0, JTAG_MASTER_IR_LEN);
	if (ret)
		return ret;
	return jtag_dr_scan(JTAG_MASTER_TDR_SEC_TAP, 32, NULL);
}

int jtag_read_chain_idcode(u32 *master, u32 *dap)
{
	u32 m0, m1, d;
	u64 r = 0;
	int ret;

	ret = jtag_read_master_idcode(&m0);
	if (ret)
		return ret;

	ret = jtag_enable_dap_tap();
	if (ret)
		return ret;

	/* 9-bit IR = IDCODE in both TAPs, then 2 x 32-bit DR */
	ret = jtag_ir_scan(JTAG_CHAIN_IR_IDCODE, JTAG_CHAIN_IR_LEN);
	if (!ret)
		ret = jtag_dr_scan(0, 64, &r);
	if (ret)
		return ret;

	d = (u32)r;		/* DAP TAP is closest to TDO */
	m1 = (u32)(r >> 32);

	*master = m0;
	*dap = d;

	if (m1 != m0) {
		printf("Secondary TAP enable did not take effect (master IDCODE 0x%08x -> 0x%08x)\n",
		       m0, m1);
		return -EIO;
	}
	if (d == 0xFFFFFFFFU || !(d & 1U)) {
		printf("DAP IDCODE invalid (0x%08x)\n", d);
		return -EIO;
	}

	/* The chain is now in place: DPACC/APACC work from here without re-enabling */
	return 0;
}

/* ------------------------------------------------------------------ */
/* DAP level                                                          */
/* ------------------------------------------------------------------ */

/*
 * One DPACC/APACC scan: 35 DAP bits (RnW | A[3:2] | DATA[31:0]) plus one
 * master BYPASS bit. TDO returns the ACK and data of the *previous* scan.
 */
static int dap_scan(bool ap, u8 a, bool rd, u32 din, u32 *dout, u8 *ack)
{
	u64 tdi, tdo = 0;
	int ret;

	tdi = (((u64)din << 3) | ((u64)(a & 3U) << 1) | (rd ? 1U : 0U)) << s_tdo_lag;

	ret = jtag_ir_scan(ap ? JTAG_IR_APACC : JTAG_IR_DPACC, JTAG_CHAIN_IR_LEN);
	if (!ret)
		ret = jtag_dr_scan(tdi, 36U + s_tdo_lag, &tdo);
	if (ret)
		return ret;

	tdo >>= s_tdo_lag;
	*ack = (u8)(tdo & 7U);
	if (dout)
		*dout = (u32)(tdo >> 3);
	return 0;
}

/* Issue a request, then flush it with RDBUFF to get its ACK and data. */
static int dap_op(bool ap, u8 a, bool rd, u32 din, u32 *dout)
{
	u32 d = 0;
	u8 ack = 0;
	int i, ret;

	for (i = 0; i < DAP_RETRY; i++) {
		ret = dap_scan(ap, a, rd, din, NULL, &ack);
		if (!ret)
			ret = dap_scan(false, DP_RDBUFF, true, 0, &d, &ack);
		if (ret)
			return ret;
		if (ack != DAP_ACK_WAIT)
			break;
	}
	s_ack = ack;
	if (dout)
		*dout = d;

	if (ack == DAP_ACK_OK || ack == DAP_ACK_OK_V5)
		return 0;
	return ack == DAP_ACK_WAIT ? -ETIMEDOUT : -EIO;
}

u8 dap_last_ack(void)
{
	return s_ack;
}

bool dap_is_open(void)
{
	return s_dap_open;
}

int dap_dp_read(u8 a, u32 *v)
{
	return dap_op(false, a, true, 0, v);
}

int dap_dp_write(u8 a, u32 v)
{
	return dap_op(false, a, false, v, NULL);
}

int dap_clear_sticky(void)
{
	return dap_dp_write(DP_CTRL_STAT, DP_PWRUP_REQ | DP_STICKY_CLR);
}

int dap_open(void)
{
	u32 master, dap, v = 0;
	int i, ret;

	ret = jtag_start(0);
	if (ret) {
		printf("JTAG enable failed (%d)\n", ret);
		return ret;
	}

	ret = jtag_read_chain_idcode(&master, &dap);
	if (ret)
		return ret;
	jtag_print_idcode("Master TAP", master);
	jtag_print_idcode("DAP TAP", dap);

	/* DPIDR bit 0 is always 1: use it to find the TDO alignment (lag 1, then 0) */
	for (s_tdo_lag = 1U;; s_tdo_lag--) {
		if (!dap_dp_read(DP_DPIDR, &v) && (v & 1U))
			break;
		if (s_tdo_lag == 0U) {
			printf("DPIDR read failed (ack %u, data 0x%08x) - DAP TAP not in chain\n",
			       s_ack, v);
			s_tdo_lag = 1U;
			return -EIO;
		}
	}
	printf("DPIDR      0x%08x (exp 0x%08x) tdo_lag %u ack %u\n",
	       v, DP_DPIDR_EXPECTED, s_tdo_lag, s_ack);

	/* Debug + system power-up */
	ret = dap_dp_write(DP_CTRL_STAT, DP_PWRUP_REQ);
	if (ret) {
		printf("CTRL/STAT write failed (ack %u)\n", s_ack);
		return ret;
	}
	for (i = 0; i < DAP_RETRY; i++) {
		if (!dap_dp_read(DP_CTRL_STAT, &v) &&
		    (v & DP_PWRUP_ACK) == DP_PWRUP_ACK)
			break;
	}
	printf("CTRL/STAT  0x%08x (exp 0xF4000000)%s\n", v,
	       i == DAP_RETRY ? "  power-up ACK missing" : "");
	if (i == DAP_RETRY)
		return -ETIMEDOUT;

	s_dap_open = true;
	return 0;
}

/* ADIv6: SELECT holds AP address [31:4], A[3:2] picks the word in the bank */
int ap_read(u32 ap, u32 off, u32 *v)
{
	int ret;

	ret = dap_op(false, DP_SELECT, false, (ap + off) & ~0xFU, NULL);
	if (ret)
		return ret;
	return dap_op(true, (off >> 2) & 3U, true, 0, v);
}

int ap_write(u32 ap, u32 off, u32 v)
{
	int ret;

	ret = dap_op(false, DP_SELECT, false, (ap + off) & ~0xFU, NULL);
	if (ret)
		return ret;
	return dap_op(true, (off >> 2) & 3U, false, v, NULL);
}

int mem_ap_open(u32 ap)
{
	u32 i, v;
	int ret;

	for (i = 0; s_dap_open && i < s_mem_ap_cnt; i++)
		if (s_mem_ap_open[i] == ap)
			return 0;

	if (!s_dap_open) {
		ret = dap_open();
		if (ret)
			return ret;
	}

	ret = ap_read(ap, AP_IDR, &v);
	if (ret || !v) {
		printf("AP 0x%x IDR read failed or 0 - no AP there (ack %u)\n", ap, s_ack);
		return ret ? ret : -ENODEV;
	}
	printf("AP 0x%x IDR 0x%08x\n", ap, v);

	/* 32-bit accesses, no auto-increment; keep the other CSW bits */
	ret = ap_read(ap, AP_CSW, &v);
	if (!ret)
		ret = ap_write(ap, AP_CSW, (v & ~AP_CSW_SIZE_INC_MASK) | AP_CSW_SIZE_32);
	if (ret) {
		printf("AP 0x%x CSW setup failed (ack %u)\n", ap, s_ack);
		return ret;
	}

	if (s_mem_ap_cnt < MEM_AP_CACHE)
		s_mem_ap_open[s_mem_ap_cnt++] = ap;
	return 0;
}

int mem_read32(u32 ap, u32 addr, u32 *v)
{
	int ret;

	ret = ap_write(ap, AP_TAR, addr);
	if (ret)
		return ret;
	return ap_read(ap, AP_DRW, v);
}

int mem_write32(u32 ap, u32 addr, u32 v)
{
	int ret;

	ret = ap_write(ap, AP_TAR, addr);
	if (ret)
		return ret;
	return ap_write(ap, AP_DRW, v);
}

/*
 * Shared body of mem_write_block()/mem_read_block(). CSW, TAR and DRW all
 * live in the 0xD00 bank, so SELECT is written once and the APACC scans go
 * straight to A[3:2]. TAR auto-increment is only guaranteed within a 1 KB
 * block, so TAR is reloaded at every boundary.
 */
static int mem_block(u32 ap, u32 addr, u32 *buf, u32 count, bool rd)
{
	u32 csw, i;
	int ret, ret2;

	if (addr & 3U)
		return -EINVAL;
	if (!count)
		return 0;

	ret = ap_read(ap, AP_CSW, &csw);	/* also sets SELECT to the 0xD00 bank */
	if (ret)
		return ret;
	ret = dap_op(true, (AP_CSW >> 2) & 3U, false,
		     (csw & ~AP_CSW_SIZE_INC_MASK) | AP_CSW_SIZE_32 |
		     AP_CSW_ADDRINC_SINGLE, NULL);
	if (ret)
		return ret;

	for (i = 0; i < count; i++, addr += 4) {
		if (!i || !(addr & (AP_TAR_INC_WRAP - 1U))) {
			ret = dap_op(true, (AP_TAR >> 2) & 3U, false, addr, NULL);
			if (ret)
				break;
		}
		if (rd)
			ret = dap_op(true, (AP_DRW >> 2) & 3U, true, 0, &buf[i]);
		else
			ret = dap_op(true, (AP_DRW >> 2) & 3U, false, buf[i], NULL);
		if (ret)
			break;
	}
	if (ret)
		printf("%s 0x%08x failed (ack %u)\n", rd ? "read" : "write", addr, s_ack);

	/* Back to 32-bit / no auto-increment, as mem_ap_open() left it */
	ret2 = ap_write(ap, AP_CSW, (csw & ~AP_CSW_SIZE_INC_MASK) | AP_CSW_SIZE_32);
	return ret ? ret : ret2;
}

int mem_write_block(u32 ap, u32 addr, const u32 *buf, u32 count)
{
	return mem_block(ap, addr, (u32 *)buf, count, false);
}

int mem_read_block(u32 ap, u32 addr, u32 *buf, u32 count)
{
	return mem_block(ap, addr, buf, count, true);
}
