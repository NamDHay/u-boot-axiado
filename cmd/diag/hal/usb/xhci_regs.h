/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __XHCI_REGS_H
#define __XHCI_REGS_H

#include <linux/bitops.h>

/* ---- Operational registers (offset from op base = CAPLENGTH) ---- */
#define OP_USBCMD	0x00
#define OP_USBSTS	0x04
#define OP_PAGESIZE	0x08
#define OP_CRCR		0x18
#define OP_DCBAAP	0x30
#define OP_CONFIG	0x38
#define OP_PORTSC(n)	(0x400 + 0x10 * ((n) - 1))	/* n = 1..MaxPorts */

#define CMD_RS		BIT(0)
#define CMD_HCRST	BIT(1)
#define CMD_INTE	BIT(2)

#define STS_HCH		BIT(0)
#define STS_EINT	BIT(3)
#define STS_CNR		BIT(11)

#define CRCR_RCS	BIT(0)

/* ---- PORTSC ---- */
#define PORTSC_CCS		BIT(0)
#define PORTSC_PED		BIT(1)		/* RW1C */
#define PORTSC_PR		BIT(4)
#define PORTSC_PLS(v)		(((v) >> 5) & 0xf)
#define PORTSC_PP		BIT(9)
#define PORTSC_SPEED(v)		(((v) >> 10) & 0xf)
#define PORTSC_WRC		BIT(19)
#define PORTSC_PRC		BIT(21)
#define PORTSC_CHG_ALL		0x00fe0000u	/* RW1C change bits 17..23 */
/* value to write back without touching RW1C bits / link state strobe */
#define PORTSC_NEUTRAL(v)	((v) & ~(PORTSC_PED | 0x00ff0000u))

/* ---- Runtime: interrupter 0 (offset from runtime base) ---- */
#define RT_IR0		0x20
#define IR_IMAN		0x00
#define IR_ERSTSZ	0x08
#define IR_ERSTBA	0x10
#define IR_ERDP		0x18
#define IMAN_IP		BIT(0)
#define IMAN_IE		BIT(1)
#define ERDP_EHB	BIT(3)

/* ---- TRB ---- */
#define TRB_CYCLE	BIT(0)
#define TRB_TC		BIT(1)		/* Link: toggle cycle */
#define TRB_ISP		BIT(2)
#define TRB_IOC		BIT(5)
#define TRB_IDT		BIT(6)
#define TRB_DIR_IN	BIT(16)
#define TRB_T(t)	((u32)(t) << 10)
#define TRB_TYPE(c)	(((c) >> 10) & 0x3f)

#define TRB_NORMAL	1
#define TRB_SETUP	2
#define TRB_DATA	3
#define TRB_STATUS	4
#define TRB_LINK	6
#define TRB_ENABLE_SLOT	9
#define TRB_ADDRESS_DEV	11
#define TRB_CONFIG_EP	12
#define TRB_EVAL_CTX	13
#define TRB_RESET_EP	14
#define TRB_SET_DEQ	16
#define TRB_EVT_XFER	32
#define TRB_EVT_CMD	33
#define TRB_EVT_PORT	34

#define EVT_CC(s)	(((s) >> 24) & 0xff)
#define EVT_SLOT(c)	(((c) >> 24) & 0xff)
#define EVT_EP(c)	(((c) >> 16) & 0x1f)

#define CC_SUCCESS	1
#define CC_STALL	6
#define CC_SHORT	13

/* Endpoint types (EP context) */
#define EP_TYPE_BULK_OUT	2
#define EP_TYPE_CONTROL		4
#define EP_TYPE_BULK_IN		6

/* USB speeds (PORTSC / slot context) */
#define USB_SPEED_FS	1
#define USB_SPEED_LS	2
#define USB_SPEED_HS	3
#define USB_SPEED_SS	4

#endif