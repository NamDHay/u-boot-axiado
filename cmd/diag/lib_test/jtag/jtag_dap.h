/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 *
 * Host JTAG test library: TAP chain + ADIv5/ADIv6 DAP client built on
 * top of the ax_jtag_ops HAL (ax_jtag_xfer()).  Target-core files
 * (m0.c, r52.c, a53.c ...) use only this API.
 */

#ifndef __AX_JTAG_DAP_H
#define __AX_JTAG_DAP_H

#include <linux/types.h>

#define AX_JTAG_PORT		0

/* Master TAP (5-bit IR) */
#define JTAG_MASTER_IR_LEN	5U
#define JTAG_MASTER_IR_IDCODE	0x09U
#define JTAG_MASTER_IR_BYPASS	0x1FU
#define JTAG_MASTER_IR_USER0	0x10U	/* T_USER0: TDR holding secondary_tap_sel */
#define JTAG_MASTER_TDR_SEC_TAP	0x00000800U	/* tdr_secondary_tap_sel = 1 */

/* Chain = DAP TAP (4-bit IR, TDO side) + master TAP (5-bit IR) = 9 bits */
#define JTAG_CHAIN_IR_LEN	9U
#define JTAG_CHAIN_IR_IDCODE	0x09EU	/* DAP IDCODE(0xE) | master IDCODE(0x09) */
#define JTAG_IR_DPACC		0x1FAU	/* DAP DPACC  | master BYPASS */
#define JTAG_IR_APACC		0x1FBU	/* DAP APACC  | master BYPASS */

/* This DAP answers 0b100 for OK (not the ADIv5 0b010); accept both */
#define DAP_ACK_OK		4U
#define DAP_ACK_OK_V5		2U
#define DAP_ACK_WAIT		1U

/* DP registers (A[3:2]) */
#define DP_DPIDR		0U
#define DP_CTRL_STAT		1U
#define DP_SELECT		2U
#define DP_RDBUFF		3U

#define DP_PWRUP_REQ		0x54000000U	/* CDBGPWRUPREQ|CSYSPWRUPREQ|... */
#define DP_PWRUP_ACK		0xA0000000U	/* CDBGPWRUPACK|CSYSPWRUPACK */
#define DP_STICKY_CLR		0x00000032U	/* STICKYERR|STICKYCMP|STICKYORUN */
#define DP_DPIDR_EXPECTED	0x4C013477U

/* ADIv6 MEM-AP register offsets (AP base + off) */
#define AP_CSW			0xD00U
#define AP_TAR			0xD04U
#define AP_DRW			0xD0CU
#define AP_IDR			0xDFCU

#define AP_CSW_SIZE_32		0x2U
#define AP_CSW_ADDRINC_SINGLE	0x10U
#define AP_CSW_SIZE_INC_MASK	0x37U
#define AP_TAR_INC_WRAP		0x400U	/* auto-inc only guaranteed inside 1 KB */

/* Raw TAP scans (bits <= 64). TAP starts and ends in Run-Test/Idle. */
int jtag_ir_scan(u32 ir, u32 bits);
int jtag_dr_scan(u64 tdi, u32 bits, u64 *tdo);

void jtag_print_idcode(const char *name, u32 id);

/* Enable the HAL controller (TRST + TLR -> RTI). Drops the DAP chain. */
int jtag_start(u32 freq_hz);
int jtag_stop(void);

/* IDCODE scans */
int jtag_read_master_idcode(u32 *id);
int jtag_enable_dap_tap(void);
int jtag_read_chain_idcode(u32 *master, u32 *dap);

/* DAP: open = start + chain + DPIDR (TDO lag autodetect) + power-up */
int dap_open(void);
bool dap_is_open(void);
void dap_invalidate(void);
u8 dap_last_ack(void);
int dap_dp_read(u8 a, u32 *v);
int dap_dp_write(u8 a, u32 v);
int dap_clear_sticky(void);

int ap_read(u32 ap, u32 off, u32 *v);
int ap_write(u32 ap, u32 off, u32 v);

/* MEM-AP: open checks AP IDR != 0 and sets CSW 32-bit / no auto-inc */
int mem_ap_open(u32 ap);
int mem_read32(u32 ap, u32 addr, u32 *v);
int mem_write32(u32 ap, u32 addr, u32 v);

/*
 * Bulk 32-bit transfers using CSW auto-increment (TAR re-written at every
 * 1 KB boundary). The AP must already be open; CSW is restored afterwards.
 */
int mem_write_block(u32 ap, u32 addr, const u32 *buf, u32 count);
int mem_read_block(u32 ap, u32 addr, u32 *buf, u32 count);

#endif /* __AX_JTAG_DAP_H */
