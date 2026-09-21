/*
 * xhci_regs.h - xHCI register, TRB and context definitions for the CRG
 *               (Corigine) UHC used on AX3000.
 *
 * Derived from the Validation team baremetal driver (crg_uhc.h,
 * Copyright (c) 2018 Corigine (Hong Kong) Limited).
 *
 * Naming: everything is prefixed AXH_/axh_ so it can live in the same U-Boot
 * image as the stock drivers/usb/host/xhci*.c without symbol/macro clashes.
 */
#ifndef __XHCI_REGS_H__
#define __XHCI_REGS_H__

#include <linux/types.h>

/* ------------------------------------------------------------------ */
/* Capability registers                                                */
/* ------------------------------------------------------------------ */
struct axh_cap_regs {
	u32 cap_hci;		/* [7:0] CAPLENGTH, [31:16] HCIVERSION */
	u32 hcsparams1;
	u32 hcsparams2;
	u32 hcsparams3;
	u32 hccparams1;
	u32 dboff;
	u32 rtsoff;
	u32 hccparams2;
};

#define AXH_CAPLENGTH(p)	((p) & 0xff)
#define AXH_HCIVERSION(p)	(((p) >> 16) & 0xffff)

#define AXH_HCS1_MAX_SLOTS(p)	((p) & 0xff)
#define AXH_HCS1_MAX_PORTS(p)	(((p) >> 24) & 0xff)

#define AXH_HCS2_ETE		BIT(8)
#define AXH_HCS2_MAX_SPBUF(p)	((((p) >> 16) & 0x3e0) | (((p) >> 27) & 0x1f))

#define AXH_HCC1_AC64		BIT(0)
#define AXH_HCC1_CSZ		BIT(2)	/* 64-byte contexts */
#define AXH_HCC1_PPC		BIT(3)	/* port power control */
#define AXH_HCC1_XECP(p)	(((p) >> 16) & 0xffff)

/* ------------------------------------------------------------------ */
/* Operational registers                                               */
/* ------------------------------------------------------------------ */
struct axh_op_regs {
	u32 usbcmd;
	u32 usbsts;
	u32 pagesize;
	u32 rsvd0[2];
	u32 dnctrl;
	u32 crcr_lo;
	u32 crcr_hi;
	u32 rsvd1[4];
	u32 dcbaap_lo;
	u32 dcbaap_hi;
	u32 config;
};

/* port register set lives at op + 0x400 + n * 0x10 (PORTSC is dword 0) */
#define AXH_OP_PORT_BASE	0x400
#define AXH_PORT_STRIDE		0x10

#define AXH_CMD_RUN		BIT(0)
#define AXH_CMD_HCRST		BIT(1)
#define AXH_CMD_INTE		BIT(2)
#define AXH_CMD_ETE		BIT(14)

#define AXH_STS_HCH		BIT(0)
#define AXH_STS_HSE		BIT(2)
#define AXH_STS_EINT		BIT(3)
#define AXH_STS_CNR		BIT(11)

#define AXH_CRCR_RCS		BIT(0)
#define AXH_CRCR_CA		BIT(2)
#define AXH_CRCR_CRR		BIT(3)

#define AXH_CONFIG_SLOTS_MASK	0xff

/* PORTSC */
#define AXH_PORTSC_CCS		BIT(0)
#define AXH_PORTSC_PED		BIT(1)
#define AXH_PORTSC_OCA		BIT(3)
#define AXH_PORTSC_PR		BIT(4)
#define AXH_PORTSC_PLS(p)	(((p) >> 5) & 0xf)
#define AXH_PORTSC_PP		BIT(9)
#define AXH_PORTSC_SPEED(p)	(((p) >> 10) & 0xf)
#define AXH_PORTSC_CSC		BIT(17)
#define AXH_PORTSC_PEC		BIT(18)
#define AXH_PORTSC_WRC		BIT(19)
#define AXH_PORTSC_OCC		BIT(20)
#define AXH_PORTSC_PRC		BIT(21)
#define AXH_PORTSC_PLC		BIT(22)
#define AXH_PORTSC_CEC		BIT(23)
#define AXH_PORTSC_DR		BIT(30)

#define AXH_PORTSC_CHANGE_MASK	(AXH_PORTSC_CSC | AXH_PORTSC_PEC | AXH_PORTSC_WRC | \
				 AXH_PORTSC_OCC | AXH_PORTSC_PRC | AXH_PORTSC_PLC | \
				 AXH_PORTSC_CEC)
#define AXH_PORTSC_RO_MASK	(AXH_PORTSC_CCS | AXH_PORTSC_OCA | (0xfU << 10) | AXH_PORTSC_DR)
#define AXH_PORTSC_RWS_MASK	((0xfU << 5) | AXH_PORTSC_PP | (0x3U << 14) | (0x7U << 25))
/* value that can be written back without side effects (RW1C bits read as 0) */
#define AXH_PORTSC_NEUTRAL(p)	((p) & (AXH_PORTSC_RO_MASK | AXH_PORTSC_RWS_MASK))

#define AXH_PLS_U0		0

/* USB speed ids as reported by PORTSC / slot context */
#define AXH_SPEED_FULL		1
#define AXH_SPEED_LOW		2
#define AXH_SPEED_HIGH		3
#define AXH_SPEED_SUPER		4
#define AXH_SPEED_SUPER_PLUS	5

/* ------------------------------------------------------------------ */
/* Runtime / doorbell registers                                        */
/* ------------------------------------------------------------------ */
struct axh_intr_regs {
	u32 iman;
	u32 imod;
	u32 erstsz;
	u32 rsvd;
	u32 erstba_lo;
	u32 erstba_hi;
	u32 erdp_lo;
	u32 erdp_hi;
};

struct axh_run_regs {
	u32 mfindex;
	u32 rsvd[7];
	struct axh_intr_regs ir[1];	/* only interrupter 0 is used */
};

#define AXH_IMAN_IP		BIT(0)
#define AXH_IMAN_IE		BIT(1)
#define AXH_ERDP_EHB		BIT(3)

/* ------------------------------------------------------------------ */
/* Extended capabilities                                               */
/* ------------------------------------------------------------------ */
#define AXH_EXT_CAP_ID(p)	((p) & 0xff)
#define AXH_EXT_CAP_NEXT(p)	(((p) >> 8) & 0xff)
#define AXH_EXT_CAP_PROTOCOL	2
#define AXH_EXT_PORT_MAJOR(p)	(((p) >> 24) & 0xff)
#define AXH_EXT_PORT_OFF(p)	((p) & 0xff)		/* 1-based first port */
#define AXH_EXT_PORT_COUNT(p)	(((p) >> 8) & 0xff)

/* ------------------------------------------------------------------ */
/* TRBs                                                                */
/* ------------------------------------------------------------------ */
struct axh_trb {
	u32 dw[4];
};

#define AXH_TRB_CYCLE		BIT(0)
#define AXH_TRB_TC		BIT(1)	/* link: toggle cycle */
#define AXH_TRB_ISP		BIT(2)
#define AXH_TRB_CH		BIT(4)
#define AXH_TRB_IOC		BIT(5)
#define AXH_TRB_IDT		BIT(6)
#define AXH_TRB_BSR		BIT(9)	/* Address Device */
#define AXH_TRB_TSP		BIT(9)	/* Reset Endpoint */
#define AXH_TRB_DIR_IN		BIT(16)

#define AXH_TRB_TRT(x)		(((u32)(x) & 3U) << 16)
#define AXH_TRB_TYPE(x)		((u32)(x) << 10)
#define AXH_TRB_TYPE_GET(d3)	(((d3) >> 10) & 0x3f)
#define AXH_TRB_EP_ID(x)	(((u32)(x) & 0x1fU) << 16)
#define AXH_TRB_SLOT(x)		((u32)(x) << 24)
#define AXH_TRB_LEN(x)		((u32)(x) & 0x1ffffU)
#define AXH_TRB_TD_SIZE(x)	(((u32)(x) & 0x1fU) << 17)

/* TRB types */
#define AXH_TRB_NORMAL		1
#define AXH_TRB_SETUP		2
#define AXH_TRB_DATA		3
#define AXH_TRB_STATUS		4
#define AXH_TRB_ISOC		5
#define AXH_TRB_LINK		6
#define AXH_TRB_CMD_ENABLE_SLOT	9
#define AXH_TRB_CMD_DISABLE_SLOT 10
#define AXH_TRB_CMD_ADDR_DEV	11
#define AXH_TRB_CMD_CONFIG_EP	12
#define AXH_TRB_CMD_EVAL_CTX	13
#define AXH_TRB_CMD_RESET_EP	14
#define AXH_TRB_CMD_STOP_EP	15
#define AXH_TRB_CMD_SET_DEQ	16
#define AXH_TRB_CMD_NOOP	23
#define AXH_TRB_EV_TRANSFER	32
#define AXH_TRB_EV_CMD_COMPLETION 33
#define AXH_TRB_EV_PORT_STATUS	34

/* completion codes */
#define AXH_CC_SUCCESS		1
#define AXH_CC_DATA_BUF_ERR	2
#define AXH_CC_BABBLE		3
#define AXH_CC_XACT_ERR		4
#define AXH_CC_TRB_ERR		5
#define AXH_CC_STALL		6
#define AXH_CC_NO_SLOTS		9
#define AXH_CC_PARAM_ERR	17
#define AXH_CC_CTX_STATE_ERR	19
#define AXH_CC_SHORT_PKT	13

/* ------------------------------------------------------------------ */
/* Contexts (32-byte layout; 64-byte layout handled via ctx_size)      */
/* ------------------------------------------------------------------ */
struct axh_slot_ctx {
	u32 info1;
	u32 info2;
	u32 info3;
	u32 info4;
	u32 rsvd[4];
};

struct axh_ep_ctx {
	u32 info1;
	u32 info2;
	u32 deq_lo;
	u32 deq_hi;
	u32 info3;
	u32 rsvd[3];
};

struct axh_input_ctrl_ctx {
	u32 drop;
	u32 add;
	u32 rsvd[6];
};

/* slot context */
#define AXH_SLOT_ROUTE(x)	((x) & 0xfffffU)
#define AXH_SLOT_SPEED(x)	(((u32)(x) & 0xfU) << 20)
#define AXH_SLOT_HUB		BIT(26)
#define AXH_SLOT_LAST_CTX_MASK	(0x1fU << 27)
#define AXH_SLOT_LAST_CTX(x)	(((u32)(x) & 0x1fU) << 27)
#define AXH_SLOT_ROOT_PORT(x)	(((u32)(x) & 0xffU) << 16)	/* 1-based */
#define AXH_SLOT_ADDR_MASK	0xffU

/* endpoint context */
#define AXH_EP_MULT(x)		(((u32)(x) & 3U) << 8)
#define AXH_EP_INTERVAL(x)	(((u32)(x) & 0xffU) << 16)
#define AXH_EP_ESIT_HI(x)	((((u32)(x) >> 16) & 0xffU) << 24)
#define AXH_EP_CERR(x)		(((u32)(x) & 3U) << 1)
#define AXH_EP_TYPE(x)		(((u32)(x) & 7U) << 3)
#define AXH_EP_MAX_BURST(x)	(((u32)(x) & 0xffU) << 8)
#define AXH_EP_MPS(x)		(((u32)(x) & 0xffffU) << 16)
#define AXH_EP_AVG_TRB_LEN(x)	((u32)(x) & 0xffffU)
#define AXH_EP_ESIT_LO(x)	(((u32)(x) & 0xffffU) << 16)

#define AXH_EP_TYPE_ISOC_OUT	1
#define AXH_EP_TYPE_BULK_OUT	2
#define AXH_EP_TYPE_INT_OUT	3
#define AXH_EP_TYPE_CTRL	4
#define AXH_EP_TYPE_ISOC_IN	5
#define AXH_EP_TYPE_BULK_IN	6
#define AXH_EP_TYPE_INT_IN	7

#endif /* __XHCI_REGS_H__ */
