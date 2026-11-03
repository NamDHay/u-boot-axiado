// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Corigine USB3 device controller (Axiado AX3000), bare-metal backend.
 *
 * Ported from the Linux/U-Boot gadget version of crg_udc.c. Differences:
 *  - no usb_gadget framework, exposes struct ax_usb_dev_ops instead
 *  - polling only (no IRQ, no kthread): everything runs in ops->poll()
 *  - no spinlock, no list_head, no dma_map_*: explicit D-cache maintenance,
 *    RAM is expected to be 1:1 mapped
 *  - one request == one TRB (<= 64KB); no SG, no isoc, no streams, no
 *    dequeue
 *
 * Copyright (c) 2019 Corigine Inc.
 * Copyright (c) 2023-25 Axiado Corporation.
 */

#include <dm.h>
#include <cpu_func.h>
#include <log.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/usb/ch9.h>
#include "crg_udc.h"
#include "ax_usb_dev.h"

#define CRG_ERST_SIZE			1
#define CRG_EVENT_RING_SIZE		256
#define CRG_NUM_EP_CX			32
#define CRG_RING_NUM			1

#define TRB_MAX_BUFFER_SIZE		65536
#define CRGUDC_CONTROL_EP_TD_RING_SIZE	16
#define CRGUDC_BULK_EP_TD_RING_SIZE	256
#define CRGUDC_INT_EP_TD_RING_SIZE	8
#define CRGUDC_ROLE_DEVICE		0x1

#define STATE_USB_LINK_STABLE		4
#define CRG_VBUS_POLL_MS		1000

/* ------------ feature switches, same defaults as the Linux driver ------- */
#define U12_FORBIDDEN			1
#define REINIT_EP0_ON_BUS_RESET

enum ep_state_e {
	EP_STATE_DISABLED = 0,
	EP_STATE_RUNNING = 1,
	EP_STATE_HALTED = 2,
	EP_STATE_STOPPED = 3
};

enum ep_type_e {
	EP_TYPE_INVALID = 0,
	EP_TYPE_ISOCH_OUTBOUND,
	EP_TYPE_BULK_OUTBOUND,
	EP_TYPE_INTR_OUTBOUND,
	EP_TYPE_INVALID2,
	EP_TYPE_ISOCH_INBOUND,
	EP_TYPE_BULK_INBOUND,
	EP_TYPE_INTR_INBOUND
};

enum trb_type_e {
	TRB_TYPE_RSVD = 0,
	TRB_TYPE_XFER_NORMAL,
	TRB_TYPE_RSVD2,
	TRB_TYPE_XFER_DATA_STAGE,
	TRB_TYPE_XFER_STATUS_STAGE,
	TRB_TYPE_XFER_DATA_ISOCH,
	TRB_TYPE_LINK,
	TRB_TYPE_RSVD7,
	TRB_TYPE_NO_OP,

	TRB_TYPE_EVT_TRANSFER = 32,
	TRB_TYPE_EVT_CMD_COMPLETION = 33,
	TRB_TYPE_EVT_PORT_STATUS_CHANGE = 34,
	TRB_TYPE_EVT_MFINDEX_WRAP = 39,
	TRB_TYPE_EVT_SETUP_PKT = 40,
};

enum trb_cmpl_codes_e {
	CMPL_CODE_INVALID = 0,
	CMPL_CODE_SUCCESS,
	CMPL_CODE_DATA_BUFFER_ERR,
	CMPL_CODE_BABBLE_DETECTED_ERR,
	CMPL_CODE_USB_TRANS_ERR,
	CMPL_CODE_TRB_ERR,
	CMPL_CODE_TRB_STALL,
	CMPL_CODE_INVALID_STREAM_TYPE_ERR = 10,
	CMPL_CODE_SHORT_PKT = 13,
	CMPL_CODE_RING_UNDERRUN,
	CMPL_CODE_RING_OVERRUN,
	CMPL_CODE_EVENT_RING_FULL_ERR = 21,
	CMPL_CODE_STOPPED = 26,
	CMPL_CODE_STOPPED_LENGTH_INVALID = 27,
	CMPL_CODE_ISOCH_BUFFER_OVERRUN = 31,
	CMPL_CODE_PROTOCOL_STALL = 192,
	CMPL_CODE_SETUP_TAG_MISMATCH = 193,
	CMPL_CODE_HALTED = 194,
	CMPL_CODE_HALTED_LENGTH_INVALID = 195,
	CMPL_CODE_DISABLED = 196,
	CMPL_CODE_DISABLED_LENGTH_INVALID = 197,
};

/* ------------------------- HW data structures --------------------------- */
struct crg_trb {
	__le32 dw0;
	__le32 dw1;
#define TRB_TRANSFER_LEN_MASK		0x0001FFFF
#define TRB_TRANSFER_LEN_SHIFT		0
#define TRB_TD_SIZE_MASK		0x003E0000
#define TRB_TD_SIZE_SHIFT		17
#define TRB_INTR_TARGET_MASK		0xFFC00000
#define TRB_INTR_TARGET_SHIFT		22
	__le32 dw2;
#define TRB_CYCLE_BIT_MASK		0x00000001
#define TRB_CYCLE_BIT_SHIFT		0
#define TRB_LINK_TOGGLE_CYCLE_MASK	0x00000002
#define TRB_LINK_TOGGLE_CYCLE_SHIFT	1
#define TRB_INTR_ON_SHORT_PKT_MASK	0x00000004
#define TRB_INTR_ON_SHORT_PKT_SHIFT	2
#define TRB_CHAIN_BIT_MASK		0x00000010
#define TRB_CHAIN_BIT_SHIFT		4
#define TRB_INTR_ON_COMPLETION_MASK	0x00000020
#define TRB_INTR_ON_COMPLETION_SHIFT	5
#define TRB_APPEND_ZLP_MASK		0x00000080
#define TRB_APPEND_ZLP_SHIFT		7
#define TRB_TYPE_MASK			0x0000FC00
#define TRB_TYPE_SHIFT			10
#define DATA_STAGE_TRB_DIR_MASK		0x00010000
#define DATA_STAGE_TRB_DIR_SHIFT	16
#define TRB_SETUP_TAG_MASK		0x00060000
#define TRB_SETUP_TAG_SHIFT		17
#define STATUS_STAGE_TRB_STALL_MASK	0x00080000
#define STATUS_STAGE_TRB_STALL_SHIFT	19
#define STATUS_STAGE_TRB_SET_ADDR_MASK	0x00100000
#define STATUS_STAGE_TRB_SET_ADDR_SHIFT	20
	__le32 dw3;
};

struct crg_evt {
	__le32 dw0;
	__le32 dw1;
#define EVE_TRB_TRAN_LEN_MASK		0x0001FFFF
#define EVE_TRB_TRAN_LEN_SHIFT		0
#define EVE_TRB_COMPL_CODE_MASK		0xFF000000
#define EVE_TRB_COMPL_CODE_SHIFT	24
	__le32 dw2;
#define EVE_TRB_CYCLE_BIT_MASK		0x00000001
#define EVE_TRB_CYCLE_BIT_SHIFT		0
#define EVE_TRB_TYPE_MASK		0x0000FC00
#define EVE_TRB_TYPE_SHIFT		10
#define EVE_TRB_ENDPOINT_ID_MASK	0x001F0000
#define EVE_TRB_ENDPOINT_ID_SHIFT	16
#define EVE_TRB_SETUP_TAG_MASK		0x00600000
#define EVE_TRB_SETUP_TAG_SHIFT		21
	__le32 dw3;
};

struct crg_epcx {
#define EP_CX_LOGICAL_EP_NUM_MASK	0x00000078
#define EP_CX_LOGICAL_EP_NUM_SHIFT	3
#define EP_CX_INTERVAL_MASK		0x00FF0000
#define EP_CX_INTERVAL_SHIFT		16
	__le32 dw0;
#define EP_CX_EP_TYPE_MASK		0x00000038
#define EP_CX_EP_TYPE_SHIFT		3
#define EP_CX_MAX_BURST_SIZE_MASK	0x0000FF00
#define EP_CX_MAX_BURST_SIZE_SHIFT	8
#define EP_CX_MAX_PACKET_SIZE_MASK	0xFFFF0000
#define EP_CX_MAX_PACKET_SIZE_SHIFT	16
	__le32 dw1;
#define EP_CX_DEQ_CYC_STATE_MASK	0x00000001
#define EP_CX_DEQ_CYC_STATE_SHIFT	0
#define EP_CX_TR_DQPT_LO_MASK		0xFFFFFFF0
#define EP_CX_TR_DQPT_LO_SHIFT		4
	__le32 dw2;
	__le32 dw3;
};

struct crg_erst {
	__le32 seg_addr_lo;
	__le32 seg_addr_hi;
	__le32 seg_size;
	__le32 rsvd;
};

/* ---------------------------- driver state ------------------------------ */
#define WAIT_FOR_SETUP			0
#define SETUP_PKT_PROCESS_IN_PROGRESS	1
#define DATA_STAGE_XFER			2
#define DATA_STAGE_RECV			3
#define STATUS_STAGE_XFER		4
#define STATUS_STAGE_RECV		5

#define CTRL_REQ_QUEUE_DEPTH		5

struct crg_ep {
	u8 dci;
	u8 addr;
	bool has_desc;
	bool is_in;
	u8 type;			/* USB_ENDPOINT_XFER_* */
	u8 interval;
	u8 maxburst;
	u16 maxp;
	int state;
	bool wedge;

	void *ring_va;
	dma_addr_t ring_dma;
	u32 ring_len;
	struct crg_trb *first, *last, *enq, *deq;
	u8 pcs;

	struct ax_usb_req *head, *tail;
};

struct crg_setup_packet {
	struct usb_ctrlrequest ctrl;
	u16 setup_tag;
};

struct crg_udc {
	struct ax_usb_dev udev;
	void __iomem *base;
	struct crg_uccr *uccr;
	struct crg_uicr *uicr;

	enum usb_device_speed max_speed, speed;
	const struct ax_usb_func_ops *func;
	bool inited, started;

	struct crg_ep ep[CRG_NUM_EP_CX];
	void *epcx_va;
	dma_addr_t epcx_dma;
	struct crg_epcx *p_epcx;

	void *erst_va;
	dma_addr_t erst_dma;
	void *evt_va;
	dma_addr_t evt_dma;
	struct crg_evt *evt_dq, *evt_last;
	u8 ccs;

	u8 *ep0_buf;			/* aligned scratch for GET_STATUS/SET_SEL */
	struct { u16 u2_pel, u2_sel; u8 u1_pel, u1_sel; } sel;

	u8 setup_status;
	struct crg_setup_packet ctrl_req_queue[CTRL_REQ_QUEUE_DEPTH];
	u8 ctrl_req_enq_idx;
	void (*setup_fn_cb)(struct crg_udc *u);
	void (*ep0_data_cb)(struct crg_udc *u);

	int device_state;		/* enum usb_device_state */
	u16 dev_addr;
	u8 setup_tag;
	u8 set_tm;
	u32 num_enabled_eps;
	int connected;
	unsigned u2_rwe:1;
	unsigned feature_u1_enable:1;
	unsigned feature_u2_enable:1;
	int setup_tag_mismatch_found;
	int portsc_on_reconnecting;

	int cable_cnt;
	ulong vbus_base;

	u32 cnt_setup, cnt_xfer_evt, cnt_port_evt, cnt_bus_reset;
};

#define to_crg(ud)	container_of(ud, struct crg_udc, udev)

#define crg_err(fmt, ...)	printf("crg_udc: " fmt, ##__VA_ARGS__)
#define crg_dbg(fmt, ...)	debug("crg_udc: " fmt, ##__VA_ARGS__)

/* ------------------------- DMA / cache helpers --------------------------- */
static void crg_cache_flush(const void *p, size_t len)
{
	ulong s = ALIGN_DOWN((ulong)p, ARCH_DMA_MINALIGN);
	ulong e = ALIGN((ulong)p + len, ARCH_DMA_MINALIGN);

	flush_dcache_range(s, e);
}

static void crg_cache_inv(const void *p, size_t len)
{
	ulong s = ALIGN_DOWN((ulong)p, ARCH_DMA_MINALIGN);
	ulong e = ALIGN((ulong)p + len, ARCH_DMA_MINALIGN);

	invalidate_dcache_range(s, e);
}

/* zeroed, cache-line aligned, flushed; RAM is 1:1 so dma == phys */
static void *crg_dma_alloc(size_t len, dma_addr_t *dma)
{
	size_t sz = ALIGN(len, ARCH_DMA_MINALIGN);
	void *p = memalign(ARCH_DMA_MINALIGN, sz);

	if (!p)
		return NULL;
	memset(p, 0, sz);
	flush_dcache_range((ulong)p, (ulong)p + sz);
	*dma = virt_to_phys(p);
	return p;
}

static void crg_trb_flush(const struct crg_trb *trb)
{
	crg_cache_flush(trb, sizeof(*trb));
}

/* ---------------------------- small helpers ----------------------------- */
static inline u32 index2dci(u16 index)
{
	if (index == 0)
		return 0;
	return (index & USB_ENDPOINT_NUMBER_MASK) * 2 +
	       ((index & USB_DIR_IN) ? 0 : 1);
}

static inline u8 dci2addr(u32 dci)
{
	return (dci / 2) | ((dci & 1) ? 0 : USB_DIR_IN);
}

static struct crg_ep *crg_find_ep(struct crg_udc *u, u8 addr)
{
	u32 dci = index2dci(addr);

	if (dci == 1 || dci >= CRG_NUM_EP_CX)
		return NULL;
	return &u->ep[dci];
}

static int get_ep_state(struct crg_udc *u, int dci)
{
	if (dci < 0 || dci == 1)
		return -EINVAL;
	return u->ep[dci].state;
}

/* ------------------------------ commands -------------------------------- */
static int crg_issue_command(struct crg_udc *u, enum crg_cmd_type type,
			     u32 param0, u32 param1)
{
	struct crg_uccr *uccr = u->uccr;
	bool check_complete = false;
	u32 tmp;
	int timeout = 100000;

	if (crg_reg_read(&uccr->control) & CRG_U3DC_CTRL_RUN)
		check_complete = true;

	if (check_complete &&
	    (crg_reg_read(&uccr->cmd_control) & CRG_U3DC_CMD_CTRL_ACTIVE)) {
		crg_err("%s prev command is not complete!\n", __func__);
		return -EBUSY;
	}

	wmb();
	crg_reg_write(&uccr->cmd_param0, param0);
	crg_reg_write(&uccr->cmd_param1, param1);

	/* IOC ignored: polling */
	tmp = CRG_U3DC_CMD_CTRL_ACTIVE | CRG_U3DC_CMD_CTRL_TYPE(type);
	crg_reg_write(&uccr->cmd_control, tmp);
	crg_dbg("cmd type=%d par0=0x%x par1=0x%x\n", type, param0, param1);

	if (check_complete) {
		do {
			tmp = crg_reg_read(&uccr->cmd_control);
			if (!(tmp & CRG_U3DC_CMD_CTRL_ACTIVE))
				break;
			udelay(1);
		} while (--timeout);

		if (!timeout) {
			crg_err("cmd %d timeout\n", type);
			return -ETIMEDOUT;
		}
		if (CRG_U3DC_CMD_CTRL_STATUS_GET(tmp)) {
			crg_dbg("cmd %d failed\n", type);
			return -EIO;
		}
	}

	return 0;
}

static void knock_doorbell(struct crg_udc *u, int dci)
{
	wmb();
	crg_reg_write(&u->uccr->doorbell, CRG_U3DC_DB_TARGET(dci));
}

/* ------------------------------- rings ---------------------------------- */
static void setup_link_trb(struct crg_trb *link, bool toggle, dma_addr_t next)
{
	u32 dw = 0;

	link->dw0 = cpu_to_le32(lower_32_bits(next));
	link->dw1 = cpu_to_le32(upper_32_bits(next));
	link->dw2 = 0;

	dw = SETF_VAR(TRB_TYPE, dw, TRB_TYPE_LINK);
	dw = SETF_VAR(TRB_LINK_TOGGLE_CYCLE, dw, toggle ? 1 : 0);
	link->dw3 = cpu_to_le32(dw);
	crg_trb_flush(link);
}

static struct crg_trb *ring_dma_to_virt(struct crg_ep *ep, u64 addr)
{
	u64 off;

	if (addr & 0xf)
		return NULL;
	off = addr - ep->ring_dma;
	if (addr < ep->ring_dma || off >= ep->ring_len)
		return NULL;
	return ep->first + off / sizeof(struct crg_trb);
}

static bool ring_is_link(const struct crg_trb *trb)
{
	return GETF(TRB_TYPE, le32_to_cpu(trb->dw3)) == TRB_TYPE_LINK;
}

/* room for exactly one more TRB? (one slot is always kept free) */
static bool ring_has_room(struct crg_ep *ep)
{
	struct crg_trb *nxt = ep->enq + 1;

	if (ring_is_link(nxt))
		nxt = ep->first;
	return nxt != ep->deq;
}

/* enqueue pointer ++, handles the link TRB / cycle toggle */
static void ring_advance_enq(struct crg_ep *ep)
{
	ep->enq++;
	if (ring_is_link(ep->enq)) {
		u32 dw = le32_to_cpu(ep->enq->dw3);

		if (GETF(TRB_LINK_TOGGLE_CYCLE, dw)) {
			dw = SETF_VAR(TRB_CYCLE_BIT, dw, ep->pcs);
			ep->enq->dw3 = cpu_to_le32(dw);
			ep->pcs ^= 1;
			crg_trb_flush(ep->enq);
		}
		ep->enq = ep->first;
	}
}

static int ring_alloc(struct crg_ep *ep, u32 ring_size)
{
	if (!ep->ring_va) {
		dma_addr_t dma;
		size_t len = ring_size * sizeof(struct crg_trb);
		void *va = crg_dma_alloc(len, &dma);

		if (!va)
			return -ENOMEM;
		ep->ring_va = va;
		ep->ring_dma = dma;
		ep->ring_len = len;
		ep->first = va;
		ep->last = ep->first + ring_size - 1;
	}
	return 0;
}

static void ring_reset(struct crg_ep *ep)
{
	memset(ep->first, 0, ep->ring_len);
	setup_link_trb(ep->last, true, ep->ring_dma);
	crg_cache_flush(ep->first, ep->ring_len);
	ep->enq = ep->first;
	ep->deq = ep->first;
	ep->pcs = 1;
}

/* ------------------------------- requests ------------------------------- */
static void req_done(struct crg_ep *ep, struct crg_udc *u,
		     struct ax_usb_req *req, int status)
{
	/* pop head (the only request type that ever completes is the head) */
	if (ep->head == req) {
		ep->head = req->next;
		if (!ep->head)
			ep->tail = NULL;
	}
	req->next = NULL;

	if (req->status == -EINPROGRESS)
		req->status = status;

	if (!ep->is_in && req->length)
		crg_cache_inv(req->buf, req->length);

	if (req->complete)
		req->complete(&u->udev, ep->addr, req);
}

static void nuke(struct crg_udc *u, struct crg_ep *ep, int status)
{
	while (ep->head)
		req_done(ep, u, ep->head, status);
}

static void advance_dequeue_pt(struct crg_ep *ep)
{
	if (ep->head && ep->head->trb)
		ep->deq = ep->head->trb;
	else
		ep->deq = ep->enq;
}

static void update_dequeue_pt(const struct crg_evt *ev, struct crg_ep *ep)
{
	u64 addr = (u64)le32_to_cpu(ev->dw0) | ((u64)le32_to_cpu(ev->dw1) << 32);
	struct crg_trb *deq = ring_dma_to_virt(ep, addr);

	if (!deq)
		return;
	deq++;
	if (ring_is_link(deq))
		deq = ep->first;
	ep->deq = deq;
}

/* ------------------------------ ep contexts ----------------------------- */
static void crg_epcx_setup(struct crg_udc *u, struct crg_ep *ep)
{
	struct crg_epcx *epcx = u->p_epcx + ep->dci - 2;
	enum ep_type_e ep_type;
	u32 dw;

	/* controller direction is opposite to the host one */
	if (!ep->is_in)
		ep_type = ep->type + EP_TYPE_INVALID2;
	else
		ep_type = ep->type;

	dw = 0;
	dw = SETF_VAR(EP_CX_LOGICAL_EP_NUM, dw, ep->dci / 2);
	dw = SETF_VAR(EP_CX_INTERVAL, dw, ep->interval);
	epcx->dw0 = cpu_to_le32(dw);

	dw = 0;
	dw = SETF_VAR(EP_CX_EP_TYPE, dw, ep_type);
	dw = SETF_VAR(EP_CX_MAX_PACKET_SIZE, dw, ep->maxp);
	dw = SETF_VAR(EP_CX_MAX_BURST_SIZE, dw, ep->maxburst);
	epcx->dw1 = cpu_to_le32(dw);

	dw = lower_32_bits(ep->ring_dma) & EP_CX_TR_DQPT_LO_MASK;
	dw = SETF_VAR(EP_CX_DEQ_CYC_STATE, dw, ep->pcs);
	epcx->dw2 = cpu_to_le32(dw);
	epcx->dw3 = cpu_to_le32(upper_32_bits(ep->ring_dma));

	crg_cache_flush(epcx, sizeof(*epcx));
	wmb();
}

static void crg_epcx_update_dqptr(struct crg_udc *u, struct crg_ep *ep)
{
	struct crg_epcx *epcx = u->p_epcx + ep->dci - 2;
	dma_addr_t dq;
	u32 dw;

	if (ep->dci == 0) {
		crg_err("cannot update dqptr for ep0\n");
		return;
	}

	dq = ep->ring_dma + (ep->deq - ep->first) * sizeof(struct crg_trb);

	dw = lower_32_bits(dq) & EP_CX_TR_DQPT_LO_MASK;
	dw = SETF_VAR(EP_CX_DEQ_CYC_STATE, dw, ep->pcs);
	epcx->dw2 = cpu_to_le32(dw);
	epcx->dw3 = cpu_to_le32(upper_32_bits(dq));
	crg_cache_flush(epcx, sizeof(*epcx));
	wmb();

	crg_issue_command(u, CRG_CMD_SET_TR_DQPTR, 1U << ep->dci, 0);
}

/* ------------------------------ TRB builders ---------------------------- */
static void setup_status_trb(struct crg_udc *u, struct crg_trb *trb,
			     u8 pcs, u8 set_addr, u8 stall)
{
	u32 tmp = 0;
	u32 dir = (u->setup_status == STATUS_STAGE_XFER) ? 0 : 1;

	trb->dw0 = 0;
	trb->dw1 = 0;
	trb->dw2 = cpu_to_le32(SETF_VAR(TRB_INTR_TARGET, 0, 0));

	tmp = SETF_VAR(TRB_CYCLE_BIT, tmp, pcs);
	tmp = SETF_VAR(TRB_INTR_ON_COMPLETION, tmp, 1);
	tmp = SETF_VAR(TRB_TYPE, tmp, TRB_TYPE_XFER_STATUS_STAGE);
	tmp = SETF_VAR(DATA_STAGE_TRB_DIR, tmp, dir);
	tmp = SETF_VAR(TRB_SETUP_TAG, tmp, u->setup_tag);
	tmp = SETF_VAR(STATUS_STAGE_TRB_STALL, tmp, stall);
	tmp = SETF_VAR(STATUS_STAGE_TRB_SET_ADDR, tmp, set_addr);
	trb->dw3 = cpu_to_le32(tmp);
}

static void setup_datastage_trb(struct crg_trb *trb, dma_addr_t dma, u8 pcs,
				u32 len, u8 azp, u8 dir, u8 setup_tag)
{
	u32 tmp;

	trb->dw0 = cpu_to_le32(lower_32_bits(dma));
	trb->dw1 = cpu_to_le32(upper_32_bits(dma));

	tmp = 0;
	tmp = SETF_VAR(TRB_TRANSFER_LEN, tmp, len);
	tmp = SETF_VAR(TRB_TD_SIZE, tmp, 0);
	tmp = SETF_VAR(TRB_INTR_TARGET, tmp, 0);
	trb->dw2 = cpu_to_le32(tmp);

	tmp = 0;
	tmp = SETF_VAR(TRB_CYCLE_BIT, tmp, pcs);
	tmp = SETF_VAR(TRB_INTR_ON_SHORT_PKT, tmp, 1);
	tmp = SETF_VAR(TRB_INTR_ON_COMPLETION, tmp, 1);
	tmp = SETF_VAR(TRB_TYPE, tmp, TRB_TYPE_XFER_DATA_STAGE);
	tmp = SETF_VAR(TRB_APPEND_ZLP, tmp, azp);
	tmp = SETF_VAR(DATA_STAGE_TRB_DIR, tmp, dir);
	tmp = SETF_VAR(TRB_SETUP_TAG, tmp, setup_tag);
	trb->dw3 = cpu_to_le32(tmp);
}

static void setup_normal_trb(struct crg_trb *trb, dma_addr_t dma, u8 pcs,
			     u32 len, u8 short_pkt, u8 azp)
{
	u32 tmp;

	trb->dw0 = cpu_to_le32(lower_32_bits(dma));
	trb->dw1 = cpu_to_le32(upper_32_bits(dma));

	tmp = 0;
	tmp = SETF_VAR(TRB_TRANSFER_LEN, tmp, len);
	tmp = SETF_VAR(TRB_TD_SIZE, tmp, 0);
	tmp = SETF_VAR(TRB_INTR_TARGET, tmp, 0);
	trb->dw2 = cpu_to_le32(tmp);

	tmp = 0;
	tmp = SETF_VAR(TRB_CYCLE_BIT, tmp, pcs);
	tmp = SETF_VAR(TRB_INTR_ON_SHORT_PKT, tmp, short_pkt);
	tmp = SETF_VAR(TRB_CHAIN_BIT, tmp, 0);
	tmp = SETF_VAR(TRB_INTR_ON_COMPLETION, tmp, 1);
	tmp = SETF_VAR(TRB_APPEND_ZLP, tmp, azp);
	tmp = SETF_VAR(TRB_TYPE, tmp, TRB_TYPE_XFER_NORMAL);
	trb->dw3 = cpu_to_le32(tmp);
}

/* ------------------------------- EP0 path ------------------------------- */
/* status stage TRB (zero length), optionally STALL / SET_ADDRESS flavour */
static void ep0_build_status(struct crg_udc *u, u8 set_addr, u8 stall)
{
	struct crg_ep *ep = &u->ep[0];

	setup_status_trb(u, ep->enq, ep->pcs, set_addr, stall);
	crg_trb_flush(ep->enq);
	ring_advance_enq(ep);
	knock_doorbell(u, 0);
}

static int ep0_queue_data(struct crg_udc *u, void *buf, u32 len, bool dir_in,
			  bool zlp)
{
	struct crg_ep *ep = &u->ep[0];

	if (ep->state != EP_STATE_RUNNING)
		return -EINVAL;
	if (ep->enq != ep->deq) {
		crg_err("ep0 ring busy (enq %p != deq %p)\n", ep->enq, ep->deq);
		return -EBUSY;
	}

	if (dir_in)
		crg_cache_flush(buf, len);
	else
		crg_cache_inv(buf, len);

	/* dir bit: 0 = device->host (DATA_STAGE_XFER), 1 = host->device */
	setup_datastage_trb(ep->enq, virt_to_phys(buf), ep->pcs, len,
			    zlp ? 1 : 0, dir_in ? 0 : 1, u->setup_tag);
	crg_trb_flush(ep->enq);
	ring_advance_enq(ep);
	knock_doorbell(u, 0);

	return 0;
}

static void set_ep0_halt(struct crg_udc *u)
{
	struct crg_ep *ep = &u->ep[0];
	int st = get_ep_state(u, 0);

	if (st == EP_STATE_HALTED || st == EP_STATE_DISABLED)
		return;

	ep0_build_status(u, 0, 1);
	ep->state = EP_STATE_HALTED;
}

/* a control transfer stage finished */
static void ep0_req_complete(struct crg_udc *u)
{
	switch (u->setup_status) {
	case DATA_STAGE_XFER:
		u->setup_status = STATUS_STAGE_RECV;
		ep0_build_status(u, 0, 0);
		break;
	case DATA_STAGE_RECV:
		if (u->ep0_data_cb) {
			u->ep0_data_cb(u);
			u->ep0_data_cb = NULL;
		}
		u->setup_status = STATUS_STAGE_XFER;
		ep0_build_status(u, 0, 0);
		break;
	default:
		if (u->setup_fn_cb)
			u->setup_fn_cb(u);
		u->setup_fn_cb = NULL;
		u->setup_status = WAIT_FOR_SETUP;
		break;
	}
}

/* ------------------------------- halt ----------------------------------- */
static int set_ep_halt(struct crg_udc *u, int dci)
{
	struct crg_ep *ep = &u->ep[dci];
	u32 param0 = 1U << dci;
	int timeout = 100000;

	if (dci == 0)
		return 0;
	if (ep->state == EP_STATE_DISABLED || ep->state == EP_STATE_HALTED)
		return 0;

	crg_issue_command(u, CRG_CMD_SET_HALT, param0, 0);
	while ((crg_reg_read(&u->uccr->ep_running) & param0) && --timeout)
		udelay(1);

	nuke(u, ep, -ECONNRESET);
	ep->deq = ep->enq;
	ep->state = EP_STATE_HALTED;
	return 0;
}

static int ep_halt(struct crg_udc *u, struct crg_ep *ep, int halt,
		   int ignore_wedge)
{
	u32 param0;
	int do_halt, timeout = 100000;
	bool reset_seq_only = false;

	if (!ep->has_desc)
		return -EINVAL;
	if (ep->type == USB_ENDPOINT_XFER_ISOC)
		return -EOPNOTSUPP;
	if (ep->dci == 0)
		return 0;
	if (ep->state == EP_STATE_DISABLED)
		return 0;

	if (ep->state == EP_STATE_HALTED) {
		if (halt)
			return 0;
		if (ep->wedge && !ignore_wedge) {
			do_halt = -1;
			reset_seq_only = true;
		} else {
			do_halt = 0;
		}
	} else if (halt) {
		do_halt = 1;
	} else {
		do_halt = 0;
		reset_seq_only = true;
	}

	param0 = 1U << ep->dci;
	if (do_halt == 1) {
		crg_issue_command(u, CRG_CMD_SET_HALT, param0, 0);
		while ((crg_reg_read(&u->uccr->ep_running) & param0) &&
		       --timeout)
			udelay(1);

		if (u->speed >= USB_SPEED_SUPER) {
			nuke(u, ep, -ECONNRESET);
			ep->deq = ep->enq;
		}
		ep->state = EP_STATE_HALTED;
	} else if (do_halt == 0) {
		crg_issue_command(u, CRG_CMD_RESET_SEQNUM, param0, 0);

		if (!reset_seq_only) {
			/* CLEAR_HALT first, then SET_TR_DQPTR */
			crg_issue_command(u, CRG_CMD_CLEAR_HALT, param0, 0);
			crg_epcx_update_dqptr(u, ep);
		}

		ep->wedge = false;
		ep->state = EP_STATE_RUNNING;

		if (ep->head) {
			struct crg_trb *nxt = ep->deq + 1;

			if (ring_is_link(nxt)) {
				ep->pcs ^= 1;
				crg_epcx_update_dqptr(u, ep);
				ep->pcs ^= 1;
			}
			knock_doorbell(u, ep->dci);
		}
	} else {
		/* wedged EP denies CLEAR_HALT */
		if (reset_seq_only)
			crg_issue_command(u, CRG_CMD_RESET_SEQNUM, param0, 0);
	}

	return 0;
}

/* ------------------------- controller state helpers --------------------- */
static void crg_run(struct crg_udc *u)
{
	struct crg_uccr *uccr = u->uccr;
	u32 val;

	val = crg_reg_read(&uccr->config1);
	val |= (CRG_U3DC_CFG1_CSC_EVENT_EN | CRG_U3DC_CFG1_PEC_EVENT_EN |
		CRG_U3DC_CFG1_PPC_EVENT_EN | CRG_U3DC_CFG1_PRC_EVENT_EN |
		CRG_U3DC_CFG1_PLC_EVENT_EN | CRG_U3DC_CFG1_CEC_EVENT_EN);
	crg_reg_write(&uccr->config1, val);

	val = crg_reg_read(&uccr->control);
	val |= (CRG_U3DC_CTRL_SYSERR_EN | CRG_U3DC_CTRL_INT_EN);
	crg_reg_write(&uccr->control, val);

	val = crg_reg_read(&uccr->control);
	val |= CRG_U3DC_CTRL_RUN;
	crg_reg_write(&uccr->control, val);
}

static void crg_clear_portpm(struct crg_udc *u)
{
	struct crg_uccr *uccr = u->uccr;
	u32 tmp = crg_reg_read(&uccr->u3portpmsc);

	tmp &= ~CRG_U3DC_U3PORTPM_U1IEN;
	tmp = SETF_VAR(CRG_U3DC_U3PORTPM_U1TMOUT, tmp, 0);
	tmp &= ~CRG_U3DC_U3PORTPM_U2IEN;
	tmp = SETF_VAR(CRG_U3DC_U3PORTPM_U2TMOUT, tmp, 0);
	crg_reg_write(&uccr->u3portpmsc, tmp);

	u->feature_u1_enable = 0;
	u->feature_u2_enable = 0;
}

static void enable_setup(struct crg_udc *u)
{
	struct crg_uccr *uccr = u->uccr;
	u32 val = crg_reg_read(&uccr->config1);

	val |= CRG_U3DC_CFG1_SETUP_EVENT_EN;
	crg_reg_write(&uccr->config1, val);

	u->device_state = USB_STATE_DEFAULT;
	u->setup_status = WAIT_FOR_SETUP;
}

static int is_event_ring_empty(struct crg_udc *u)
{
	struct crg_evt *ev = u->evt_dq;

	if (!ev)
		return 1;
	crg_cache_inv(ev, sizeof(*ev));
	return GETF(EVE_TRB_CYCLE_BIT, le32_to_cpu(ev->dw3)) != u->ccs;
}

static void update_ep0_maxpacketsize(struct crg_udc *u)
{
	u16 mps = (u->speed >= USB_SPEED_SUPER) ? 512 : 64;

	crg_issue_command(u, CRG_CMD_UPDATE_EP0_CFG, CRG_CMD1_0_MPS(mps), 0);
	u->ep[0].maxp = mps;
}

/* ------------------------ allocation / init of HW structs --------------- */
static int init_event_ring(struct crg_udc *u)
{
	struct crg_uicr *uicr = u->uicr;
	struct crg_erst *erst;

	if (!u->erst_va) {
		u->erst_va = crg_dma_alloc(CRG_ERST_SIZE * sizeof(*erst),
					   &u->erst_dma);
		if (!u->erst_va)
			return -ENOMEM;
	}
	if (!u->evt_va) {
		u->evt_va = crg_dma_alloc(CRG_EVENT_RING_SIZE *
					  sizeof(struct crg_evt), &u->evt_dma);
		if (!u->evt_va)
			return -ENOMEM;
	}

	u->evt_dq = u->evt_va;
	u->evt_last = (struct crg_evt *)u->evt_va + (CRG_EVENT_RING_SIZE - 1);
	u->ccs = 1;

	/* clear event ring: avoid HW seeing dirty data */
	memset(u->evt_va, 0, CRG_EVENT_RING_SIZE * sizeof(struct crg_evt));
	crg_cache_flush(u->evt_va, CRG_EVENT_RING_SIZE * sizeof(struct crg_evt));

	erst = u->erst_va;
	erst->seg_addr_lo = cpu_to_le32(lower_32_bits(u->evt_dma));
	erst->seg_addr_hi = cpu_to_le32(upper_32_bits(u->evt_dma));
	erst->seg_size = cpu_to_le32(CRG_EVENT_RING_SIZE);
	erst->rsvd = 0;
	crg_cache_flush(erst, sizeof(*erst));
	wmb();

	crg_reg_write(&uicr->erstsz, CRG_ERST_SIZE);
	crg_reg_write(&uicr->erstbalo, lower_32_bits(u->erst_dma));
	crg_reg_write(&uicr->erstbahi, upper_32_bits(u->erst_dma));
	crg_reg_write(&uicr->erdplo,
		      lower_32_bits(u->evt_dma) | CRG_U3DC_ERDPLO_EHB);
	crg_reg_write(&uicr->erdphi, upper_32_bits(u->evt_dma));

	crg_reg_write(&uicr->iman,
		      CRG_U3DC_IMAN_INT_EN | CRG_U3DC_IMAN_INT_PEND);
	crg_reg_write(&uicr->imod, 4000);

	return 0;
}

static int init_device_context(struct crg_udc *u)
{
	struct crg_uccr *uccr = u->uccr;

	/* ep0 is not part of the ep contexts */
	if (!u->epcx_va) {
		u->epcx_va = crg_dma_alloc((CRG_NUM_EP_CX - 2) *
					   sizeof(struct crg_epcx),
					   &u->epcx_dma);
		if (!u->epcx_va)
			return -ENOMEM;
	}
	u->p_epcx = u->epcx_va;

	crg_reg_write(&uccr->dcbaplo, lower_32_bits(u->epcx_dma));
	crg_reg_write(&uccr->dcbaphi, upper_32_bits(u->epcx_dma));
	return 0;
}

static int reset_data_struct(struct crg_udc *u)
{
	struct crg_uccr *uccr = u->uccr;
	u32 val, tmp;
	int ret;

	val = crg_reg_read(&uccr->control);
	val &= ~(CRG_U3DC_CTRL_INT_EN | CRG_U3DC_CTRL_RUN);
	crg_reg_write(&uccr->control, val);

	switch (u->max_speed) {
	case USB_SPEED_FULL:
		tmp = CRG_U3DC_CFG0_MAXSPEED_FS;
		break;
	case USB_SPEED_HIGH:
		tmp = CRG_U3DC_CFG0_MAXSPEED_HS;
		break;
	case USB_SPEED_SUPER:
		tmp = CRG_U3DC_CFG0_MAXSPEED_SS;
		break;
	case USB_SPEED_SUPER_PLUS:
		tmp = CRG_U3DC_CFG0_MAXSPEED_SSP;
		break;
	default:
		return -EINVAL;
	}
	crg_reg_write(&uccr->config0, tmp);

	ret = init_event_ring(u);
	if (ret)
		return ret;
	return init_device_context(u);
}

static int init_ep0(struct crg_udc *u)
{
	struct crg_ep *ep = &u->ep[0];
	u32 p0, p1;
	int ret;

	ret = ring_alloc(ep, CRGUDC_CONTROL_EP_TD_RING_SIZE);
	if (ret)
		return ret;
	ring_reset(ep);
	ep->tail = ep->head = NULL;

	p0 = (lower_32_bits(ep->ring_dma) & CRG_CMD0_0_DQPTRLO_MASK) |
	     CRG_CMD0_0_DCS(ep->pcs);
	p1 = upper_32_bits(ep->ring_dma);
	crg_issue_command(u, CRG_CMD_INIT_EP0, p0, p1);

	ep->state = EP_STATE_RUNNING;
	return 0;
}

static void init_ep_info(struct crg_udc *u)
{
	int i;

	for (i = 0; i < CRG_NUM_EP_CX; i++) {
		u->ep[i].dci = i;
		u->ep[i].addr = dci2addr(i);
		u->ep[i].state = EP_STATE_DISABLED;
	}
	u->ep[0].addr = 0;
	u->ep[0].has_desc = true;
	u->ep[0].type = USB_ENDPOINT_XFER_CONTROL;
	u->ep[0].maxp = 64;
}

/* ------------------------------- reset / reinit ------------------------- */
static int crg_reset(struct crg_udc *u)
{
	struct crg_uccr *uccr = u->uccr;
	u32 i, tmp, val, count = 0;

	val = crg_reg_read(&uccr->control);
	val |= CRG_U3DC_CTRL_SWRST;
	crg_reg_write(&uccr->control, val);
	do {
		mdelay(10);
		tmp = crg_reg_read(&uccr->control);
		if (++count == 50) {
			crg_err("reset error\n");
			return -ETIMEDOUT;
		}
	} while (tmp & CRG_U3DC_CTRL_SWRST);

	crg_clear_portpm(u);

	u->setup_status = WAIT_FOR_SETUP;
	u->device_state = USB_STATE_ATTACHED;
	u->dev_addr = 0;

	for (i = 2; i < CRG_NUM_EP_CX; i++) {
		if (u->ep[i].has_desc)
			nuke(u, &u->ep[i], -ESHUTDOWN);
		u->ep[i].state = EP_STATE_DISABLED;
	}
	u->num_enabled_eps = 0;

	nuke(u, &u->ep[0], -ESHUTDOWN);

	u->ctrl_req_enq_idx = 0;
	memset(u->ctrl_req_queue, 0, sizeof(u->ctrl_req_queue));
	return 0;
}

static void crg_reinit(struct crg_udc *u)
{
	struct crg_uccr *uccr = u->uccr;
	u32 i, tmp;

	u->setup_status = WAIT_FOR_SETUP;
	u->device_state = USB_STATE_RECONNECTING;

	/* disable all the endpoints */
	tmp = crg_reg_read(&uccr->ep_enable);
	crg_reg_write(&uccr->ep_enable, tmp);
	for (i = 0; i < 50; i++) {
		if (crg_reg_read(&uccr->ep_enable) == 0)
			break;
	}

	for (i = 2; i < CRG_NUM_EP_CX; i++) {
		if (u->ep[i].has_desc)
			nuke(u, &u->ep[i], -ESHUTDOWN);
		u->ep[i].state = EP_STATE_DISABLED;
	}
	u->num_enabled_eps = 0;

	/* ep0 is re-initialised in prepare_for_setup() once the event ring
	 * is empty
	 */
	if (u->dev_addr != 0) {
		crg_issue_command(u, CRG_CMD_SET_ADDR, CRG_CMD2_0_DEV_ADDR(0), 0);
		u->dev_addr = 0;
	}

	crg_clear_portpm(u);

	u->cnt_bus_reset++;
	if (u->func && u->func->disconnect)
		u->func->disconnect(&u->udev);
}

static int prepare_for_setup(struct crg_udc *u)
{
	struct crg_ep *ep0 = &u->ep[0];
	u32 p0, p1;
	dma_addr_t dq;

	if (!is_event_ring_empty(u) || u->portsc_on_reconnecting == 1)
		return -EBUSY;

#ifdef REINIT_EP0_ON_BUS_RESET
	/* make ep0 dequeue pointer follow the enqueue pointer: remaining
	 * xfer TRBs become dumb ones that no longer produce xfer events
	 */
	nuke(u, ep0, -ESHUTDOWN);
	u->ctrl_req_enq_idx = 0;
	memset(u->ctrl_req_queue, 0, sizeof(u->ctrl_req_queue));

	dq = ep0->ring_dma + (ep0->enq - ep0->first) * sizeof(struct crg_trb);
	p0 = (lower_32_bits(dq) & CRG_CMD0_0_DQPTRLO_MASK) |
	     CRG_CMD0_0_DCS(ep0->pcs);
	p1 = upper_32_bits(dq);
	crg_issue_command(u, CRG_CMD_INIT_EP0, p0, p1);

	ep0->deq = ep0->enq;
#endif
	enable_setup(u);
	return 0;
}

/* ------------------------- control: standard requests ------------------- */
static void set_address_cmpl(struct crg_udc *u)
{
	if (u->device_state == USB_STATE_DEFAULT && u->dev_addr != 0) {
		u->device_state = USB_STATE_ADDRESS;
	} else if (u->device_state == USB_STATE_ADDRESS) {
		if (u->dev_addr == 0)
			u->device_state = USB_STATE_DEFAULT;
	}
}

static void set_sel_cmpl(struct crg_udc *u)
{
	const u8 *b = u->ep0_buf;

	crg_cache_inv(u->ep0_buf, 64);
	u->sel.u1_sel = b[0];
	u->sel.u1_pel = b[1];
	u->sel.u2_sel = b[2] | (b[3] << 8);
	u->sel.u2_pel = b[4] | (b[5] << 8);
}

static void set_test_mode_cmpl(struct crg_udc *u)
{
	if (u->set_tm) {
		struct crg_uccr *uccr = u->uccr;
		u32 tmp = crg_reg_read(&uccr->u2portpmsc);

		tmp = SETF_VAR(CRG_U3DC_U2PORTPM_TM, tmp, u->set_tm);
		crg_reg_write(&uccr->u2portpmsc, tmp);
		u->set_tm = 0;
	}
}

static void do_get_status(struct crg_udc *u, const struct usb_ctrlrequest *c)
{
	u16 value = le16_to_cpu(c->wValue), index = le16_to_cpu(c->wIndex);
	u16 length = le16_to_cpu(c->wLength);
	u32 status_val = 0;

	if (value || length > 2 || !length)
		goto stall;

	switch (c->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		if (index)
			goto stall;
		if (u->speed <= USB_SPEED_HIGH && u->u2_rwe)
			status_val |= BIT(USB_DEVICE_REMOTE_WAKEUP);
		if (u->speed >= USB_SPEED_SUPER) {
			if (u->feature_u1_enable)
				status_val |= BIT(USB_DEV_STAT_U1_ENABLED);
			if (u->feature_u2_enable)
				status_val |= BIT(USB_DEV_STAT_U2_ENABLED);
		}
		status_val |= USB_DEVICE_SELF_POWERED;
		break;
	case USB_RECIP_INTERFACE:
		status_val = 0;
		break;
	case USB_RECIP_ENDPOINT: {
		u32 dci = index2dci(index);

		if (dci == 1 || dci >= CRG_NUM_EP_CX)
			goto stall;
		if (u->device_state == USB_STATE_ADDRESS && dci != 0)
			goto stall;
		if (u->device_state == USB_STATE_CONFIGURED) {
			if (get_ep_state(u, dci) == EP_STATE_DISABLED)
				goto stall;
			if (get_ep_state(u, dci) == EP_STATE_HALTED)
				status_val = BIT(USB_ENDPOINT_HALT);
		}
		break;
	}
	default:
		goto stall;
	}

	*(__le16 *)u->ep0_buf = cpu_to_le16(status_val);
	u->setup_status = DATA_STAGE_XFER;
	if (ep0_queue_data(u, u->ep0_buf, 2, true, false))
		goto stall;
	return;
stall:
	set_ep0_halt(u);
}

static void do_set_address(struct crg_udc *u, u16 value, u16 index, u16 length)
{
	u8 set_addr = 0;

	if (value > 127 || index != 0 || length != 0)
		goto stall;

	if ((u->device_state == USB_STATE_DEFAULT && value != 0) ||
	    u->device_state == USB_STATE_ADDRESS) {
		u->dev_addr = value;
		crg_issue_command(u, CRG_CMD_SET_ADDR, CRG_CMD2_0_DEV_ADDR(value), 0);
		set_addr = 1;
	}

	u->setup_status = STATUS_STAGE_XFER;
	u->setup_fn_cb = set_address_cmpl;
	ep0_build_status(u, set_addr, 0);
	return;
stall:
	set_ep0_halt(u);
}

static void do_set_sel(struct crg_udc *u, u16 value, u16 index, u16 length)
{
	if (u->device_state == USB_STATE_DEFAULT || index || value || length != 6) {
		set_ep0_halt(u);
		return;
	}

	u->setup_status = DATA_STAGE_RECV;
	u->ep0_data_cb = set_sel_cmpl;
	if (ep0_queue_data(u, u->ep0_buf, 6, false, false))
		set_ep0_halt(u);
}

/* returns true when handled here (status stage queued or STALLed) */
static bool do_set_feature(struct crg_udc *u, const struct usb_ctrlrequest *c)
{
	u16 value = le16_to_cpu(c->wValue), index = le16_to_cpu(c->wIndex);
	u16 length = le16_to_cpu(c->wLength);
	bool set_feat = (c->bRequest == USB_REQ_SET_FEATURE);
	u8 type = c->bRequestType & (USB_RECIP_MASK | USB_TYPE_MASK);

	if (length != 0 || u->device_state == USB_STATE_DEFAULT)
		goto stall;

	if (type == (USB_RECIP_ENDPOINT | USB_TYPE_STANDARD)) {
		u32 dci;

		if (u->device_state == USB_STATE_ADDRESS && index != 0)
			goto stall;
		dci = index2dci(index);
		if (dci == 1 || dci >= CRG_NUM_EP_CX)
			goto stall;
		if (ep_halt(u, &u->ep[dci], set_feat ? 1 : 0, 0) < 0)
			goto stall;
	} else if (type == (USB_RECIP_DEVICE | USB_TYPE_STANDARD)) {
		switch (value) {
		case USB_DEVICE_REMOTE_WAKEUP:
			if (u->device_state < USB_STATE_DEFAULT ||
			    u->speed >= USB_SPEED_SUPER)
				goto stall;
			u->u2_rwe = set_feat;
			break;
		case USB_DEVICE_U1_ENABLE:
		case USB_DEVICE_U2_ENABLE: {
			struct crg_uccr *uccr = u->uccr;
			u32 tmp;

			if (u->device_state != USB_STATE_CONFIGURED ||
			    (index & 0xff))
				goto stall;
			/* U1/U2 stay forbidden (same as the Linux driver):
			 * the request is acked but the timeouts remain 0
			 */
			tmp = crg_reg_read(&uccr->u3portpmsc);
			if (value == USB_DEVICE_U1_ENABLE) {
				tmp = SETF_VAR(CRG_U3DC_U3PORTPM_U1TMOUT, tmp, 0);
				u->feature_u1_enable = set_feat;
			} else {
				tmp = SETF_VAR(CRG_U3DC_U3PORTPM_U2TMOUT, tmp, 0);
				u->feature_u2_enable = set_feat;
			}
			crg_reg_write(&uccr->u3portpmsc, tmp);
			break;
		}
		case USB_DEVICE_TEST_MODE:
			if (u->speed != USB_SPEED_HIGH ||
			    u->device_state < USB_STATE_DEFAULT)
				goto stall;
			u->set_tm = index >> 8;
			u->setup_fn_cb = set_test_mode_cmpl;
			break;
		default:
			goto stall;
		}
	} else if (type == (USB_RECIP_INTERFACE | USB_TYPE_STANDARD)) {
		if (u->device_state != USB_STATE_CONFIGURED)
			goto stall;
		/* FUNC_SUSPEND: just ack */
	} else {
		goto stall;
	}

	u->setup_status = STATUS_STAGE_XFER;
	ep0_build_status(u, 0, 0);
	return true;
stall:
	set_ep0_halt(u);
	return true;
}

static void crg_handle_setup_pkt(struct crg_udc *u,
				 const struct usb_ctrlrequest *c, u8 tag)
{
	u16 value = le16_to_cpu(c->wValue);
	u16 index = le16_to_cpu(c->wIndex);
	u16 length = le16_to_cpu(c->wLength);
	void *buf = NULL;
	int ret;

	crg_dbg("setup %02x %02x v=%04x i=%04x l=%d\n", c->bRequestType,
		c->bRequest, value, index, length);
	u->cnt_setup++;

	/* EP0 comes back to running when a new setup packet arrives */
	u->ep[0].state = EP_STATE_RUNNING;
	u->setup_tag = tag;
	u->setup_status = SETUP_PKT_PROCESS_IN_PROGRESS;
	u->setup_fn_cb = NULL;
	u->ep0_data_cb = NULL;

	if ((c->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		switch (c->bRequest) {
		case USB_REQ_GET_STATUS:
			if ((c->bRequestType & (USB_DIR_IN | USB_TYPE_MASK)) !=
			    (USB_DIR_IN | USB_TYPE_STANDARD)) {
				u->setup_status = WAIT_FOR_SETUP;
				return;
			}
			do_get_status(u, c);
			return;
		case USB_REQ_SET_ADDRESS:
			if (c->bRequestType != (USB_DIR_OUT | USB_RECIP_DEVICE |
						USB_TYPE_STANDARD)) {
				u->setup_status = WAIT_FOR_SETUP;
				return;
			}
			do_set_address(u, value, index, length);
			return;
		case USB_REQ_SET_SEL:
			if (c->bRequestType != (USB_DIR_OUT | USB_RECIP_DEVICE |
						USB_TYPE_STANDARD)) {
				u->setup_status = WAIT_FOR_SETUP;
				return;
			}
			do_set_sel(u, value, index, length);
			return;
		case USB_REQ_SET_ISOCH_DELAY:
			if (c->bRequestType != (USB_DIR_OUT | USB_RECIP_DEVICE |
						USB_TYPE_STANDARD))
				break;
			u->setup_status = STATUS_STAGE_XFER;
			ep0_build_status(u, 0, 0);
			return;
		case USB_REQ_CLEAR_FEATURE:
		case USB_REQ_SET_FEATURE:
			if (do_set_feature(u, c))
				return;
			break;
		case USB_REQ_SET_CONFIGURATION:
			if (u->device_state <= USB_STATE_DEFAULT) {
				set_ep0_halt(u);
				return;
			}
			if (u->device_state == USB_STATE_ADDRESS)
				u->device_state = USB_STATE_CONFIGURED;
			if (u->func && u->func->set_config &&
			    u->func->set_config(&u->udev, value & 0xff) < 0) {
				set_ep0_halt(u);
				return;
			}
			if (!(value & 0xff) && u->num_enabled_eps == 0)
				u->device_state = USB_STATE_ADDRESS;
			u->setup_status = STATUS_STAGE_XFER;
			ep0_build_status(u, 0, 0);
			return;
		default:
			break;
		}
	}

	/* everything else goes to the function layer */
	if (!u->func || !u->func->setup) {
		set_ep0_halt(u);
		return;
	}

	if (length && !(c->bRequestType & USB_DIR_IN)) {
		/* host->device data stage is not supported */
		set_ep0_halt(u);
		return;
	}

	ret = u->func->setup(&u->udev, c, &buf);
	if (ret < 0) {
		set_ep0_halt(u);
		return;
	}

	if (length && ret > 0 && buf) {
		u32 len = min_t(u32, ret, length);
		bool zlp = len < length && (len % u->ep[0].maxp) == 0;

		u->setup_status = DATA_STAGE_XFER;
		if (ep0_queue_data(u, buf, len, true, zlp))
			set_ep0_halt(u);
	} else {
		u->setup_status = STATUS_STAGE_XFER;
		ep0_build_status(u, 0, 0);
	}
}

static void queue_setup_pkt(struct crg_udc *u, const struct usb_ctrlrequest *c,
			    u16 tag)
{
	if (u->ctrl_req_enq_idx == CTRL_REQ_QUEUE_DEPTH) {
		crg_err("ctrl request queue is full\n");
		return;
	}
	u->ctrl_req_queue[u->ctrl_req_enq_idx].ctrl = *c;
	u->ctrl_req_queue[u->ctrl_req_enq_idx].setup_tag = tag;
	u->ctrl_req_enq_idx++;
}

/* ------------------------------ xfer events ----------------------------- */
static void crg_handle_xfer_event(struct crg_udc *u, struct crg_evt *ev)
{
	u32 dw2 = le32_to_cpu(ev->dw2), dw3 = le32_to_cpu(ev->dw3);
	u32 dci = GETF(EVE_TRB_ENDPOINT_ID, dw3);
	u16 cc = GETF(EVE_TRB_COMPL_CODE, dw2);
	u32 resid = GETF(EVE_TRB_TRAN_LEN, dw2);
	struct crg_ep *ep;
	struct ax_usb_req *req;

	if (dci >= CRG_NUM_EP_CX || dci == 1)
		return;
	ep = &u->ep[dci];
	if (!ep->first || ep->state == EP_STATE_DISABLED)
		return;

	u->cnt_xfer_evt++;

	if (cc != CMPL_CODE_STOPPED && cc != CMPL_CODE_STOPPED_LENGTH_INVALID &&
	    cc != CMPL_CODE_DISABLED && cc != CMPL_CODE_DISABLED_LENGTH_INVALID &&
	    cc != CMPL_CODE_HALTED && cc != CMPL_CODE_HALTED_LENGTH_INVALID)
		update_dequeue_pt(ev, ep);

	switch (cc) {
	case CMPL_CODE_SUCCESS:
		if (dci == 0) {
			ep0_req_complete(u);
			break;
		}
		req = ep->head;
		if (!req)
			break;
		req->actual = req->length - resid;
		req_done(ep, u, req, 0);
		break;

	case CMPL_CODE_SHORT_PKT:
		if (dci == 0) {
			ep0_req_complete(u);
		} else if (!ep->is_in) {
			req = ep->head;
			if (req) {
				req->actual = req->length - resid;
				req_done(ep, u, req, 0);
			}
		}
		advance_dequeue_pt(ep);
		break;

	case CMPL_CODE_PROTOCOL_STALL:
		if (dci == 0) {
			u->setup_status = WAIT_FOR_SETUP;
		} else if (ep->head) {
			req_done(ep, u, ep->head, -EINVAL);
		}
		advance_dequeue_pt(ep);
		break;

	case CMPL_CODE_SETUP_TAG_MISMATCH: {
		u32 enq_idx = u->ctrl_req_enq_idx;

		crg_dbg("SETUP TAG MISMATCH\n");
		/* skip seqnum err event until the last one arrives */
		if (ep->deq == ep->enq) {
			/* drop all queued setup packets, only handle the
			 * latest one
			 */
			u->setup_status = WAIT_FOR_SETUP;
			if (enq_idx) {
				struct crg_setup_packet sp =
					u->ctrl_req_queue[enq_idx - 1];

				crg_handle_setup_pkt(u, &sp.ctrl, sp.setup_tag);
				memset(u->ctrl_req_queue, 0,
				       sizeof(u->ctrl_req_queue));
				u->ctrl_req_enq_idx = 0;
			}
		}
		u->setup_tag_mismatch_found = 1;
		break;
	}

	case CMPL_CODE_BABBLE_DETECTED_ERR:
	case CMPL_CODE_INVALID_STREAM_TYPE_ERR:
	case CMPL_CODE_RING_UNDERRUN:
	case CMPL_CODE_RING_OVERRUN:
	case CMPL_CODE_ISOCH_BUFFER_OVERRUN:
	case CMPL_CODE_USB_TRANS_ERR:
	case CMPL_CODE_TRB_ERR:
		crg_err("xfer event error cc=0x%x dci=%u\n", cc, dci);
		set_ep_halt(u, dci);
		break;

	case CMPL_CODE_STOPPED:
	case CMPL_CODE_STOPPED_LENGTH_INVALID:
		/* whoever stops an EP deals with the stopped TRBs itself */
		break;

	default:
		crg_dbg("unknown cc=0x%x dci=%u\n", cc, dci);
		break;
	}
}

/* ------------------------- VBUS / cable detect -------------------------- */
static void crg_vbus_enable(struct crg_udc *u, bool on)
{
	u32 v = crg_reg_read(u->base + CRG_UDC_VENDOR_REG);

	if (on)
		v |= CRG_UDC_VBUS_DETECT;
	else
		v &= ~CRG_UDC_VBUS_DETECT;
	crg_reg_write(u->base + CRG_UDC_VENDOR_REG, v);
}

/*
 * Port of g_dnl_board_usb_cable_connected(): the first
 * STATE_USB_LINK_STABLE+1 calls debounce the VBUS bit with the controller
 * toggled RUN, after that PP (bit 26 of the vendor reg) decides.
 */
static int crg_cable_connected(struct crg_udc *u)
{
	struct crg_uccr *uccr = u->uccr;
	u32 tmp, cfg0;
	bool pp;

	crg_run(u);

	if (u->cable_cnt < STATE_USB_LINK_STABLE) {
		tmp = crg_reg_read(u->base + CRG_UDC_VENDOR_REG);
		crg_reg_write(u->base + CRG_UDC_VENDOR_REG, tmp | CRG_UDC_VBUS_DETECT);

		tmp = crg_reg_read(&uccr->control);
		tmp |= CRG_U3DC_CTRL_RUN;
		crg_reg_write(&uccr->control, tmp);
		mdelay(10);
		pp = (crg_reg_read(u->base + CRG_UDC_VENDOR_REG) >> 26) & 1;
		tmp = crg_reg_read(&uccr->control);
		tmp &= ~CRG_U3DC_CTRL_RUN;
		crg_reg_write(&uccr->control, tmp);
		u->cable_cnt++;
	} else {
		pp = (crg_reg_read(u->base + CRG_UDC_VENDOR_REG) >> 26) & 1;
		if (!pp) {
			u->cable_cnt = 0;
			return 0;
		}
		if (u->device_state < USB_STATE_POWERED) {
			cfg0 = crg_reg_read(&uccr->config0);
			cfg0 |= 0xf0;
			crg_reg_write(&uccr->config0, cfg0);
			mdelay(3);
			crg_run(u);
			u->device_state = USB_STATE_POWERED;
		}
		return 1;
	}

	tmp = crg_reg_read(&uccr->portsc);
	if (tmp & CRG_U3DC_PORTSC_PP) {
		mdelay(100);
		tmp = crg_reg_read(&uccr->portsc);
		if ((tmp & CRG_U3DC_PORTSC_PP) &&
		    u->device_state < USB_STATE_POWERED) {
			/* set usb3 disable count to 15 */
			cfg0 = crg_reg_read(&uccr->config0);
			cfg0 |= 0xf0;
			crg_reg_write(&uccr->config0, cfg0);
			mdelay(3);
			crg_run(u);
			u->device_state = USB_STATE_POWERED;
		}
		if (u->cable_cnt == STATE_USB_LINK_STABLE)
			return 1;
	}

	return 0;
}

/* ----------------------------- port status ------------------------------ */
static int crg_speed_from_portsc(u32 portsc, enum usb_device_speed *speed,
				 bool allow_ls)
{
	switch (CRG_U3DC_PORTSC_SPEED_GET(portsc)) {
	case CRG_U3DC_PORTSC_SPEED_SSP:
		*speed = USB_SPEED_SUPER_PLUS;
		return 0;
	case CRG_U3DC_PORTSC_SPEED_SS:
		*speed = USB_SPEED_SUPER;
		return 0;
	case CRG_U3DC_PORTSC_SPEED_HS:
		*speed = USB_SPEED_HIGH;
		return 0;
	case CRG_U3DC_PORTSC_SPEED_FS:
		*speed = USB_SPEED_FULL;
		return 0;
	case CRG_U3DC_PORTSC_SPEED_LS:
		if (allow_ls) {
			*speed = USB_SPEED_LOW;
			return 0;
		}
		/* fallthrough */
	default:
		*speed = USB_SPEED_UNKNOWN;
		return -EINVAL;
	}
}

static int crg_handle_port_status(struct crg_udc *u)
{
	struct crg_uccr *uccr = u->uccr;
	enum usb_device_speed speed;
	u32 portsc_val, tmp;

	u->cnt_port_evt++;

	/* ack the W1C status bits */
	portsc_val = crg_reg_read(&uccr->portsc);
	crg_reg_write(&uccr->portsc, portsc_val);

	/* ---- port reset ---- */
	if (portsc_val & CRG_U3DC_PORTSC_PRC) {
		mdelay(3);
		tmp = crg_reg_read(&uccr->portsc);
		if (tmp & CRG_U3DC_PORTSC_PRC) {
			crg_dbg("PRC still set\n");
		} else if (tmp & CRG_U3DC_PORTSC_PR) {
			crg_dbg("PRC clear but PR set\n");
		} else {
			if (CRG_U3DC_PORTSC_PLS_GET(tmp) != 0 ||
			    !(tmp & CRG_U3DC_PORTSC_PED))
				return 0;
			if (crg_speed_from_portsc(tmp, &speed, true))
				return 0;

			if (u->device_state >= USB_STATE_DEFAULT)
				crg_reinit(u);

			u->speed = speed;
			update_ep0_maxpacketsize(u);
			u->connected = 1;

			if (u->device_state < USB_STATE_RECONNECTING)
				enable_setup(u);
			if (u->func && u->func->connect)
				u->func->connect(&u->udev, speed);
		}
	}

	/* ---- port connection change ---- */
	tmp = crg_reg_read(&uccr->portsc);
	if ((tmp & (CRG_U3DC_PORTSC_CCS | CRG_U3DC_PORTSC_PP)) ==
	    (CRG_U3DC_PORTSC_CCS | CRG_U3DC_PORTSC_PP)) {
		if (CRG_U3DC_PORTSC_PLS_GET(tmp) != 0 ||
		    !(tmp & CRG_U3DC_PORTSC_PED))
			return 0;
		if (crg_speed_from_portsc(tmp, &speed, false))
			return 0;

		u->speed = speed;
		update_ep0_maxpacketsize(u);
		u->connected = 1;

		if (u->device_state < USB_STATE_RECONNECTING)
			enable_setup(u);
	} else if (!(tmp & CRG_U3DC_PORTSC_CCS)) {
		int cable_connected;
		int ccs_drop_ignore = 0;

		if (CRG_U3DC_PORTSC_PLS_GET(tmp) == 0 &&
		    CRG_U3DC_PORTSC_SPEED_GET(tmp) < CRG_U3DC_PORTSC_SPEED_SS) {
			ccs_drop_ignore = 1;
			crg_dbg("ccs glitch detected on HS/FS\n");
		}

		if (!ccs_drop_ignore)
			u->speed = USB_SPEED_UNKNOWN;

		mdelay(150);
		cable_connected = crg_cable_connected(u);
		if (cable_connected && !ccs_drop_ignore) {
			u->device_state = USB_STATE_POWERED;
			crg_reinit(u);
			u->connected = 0;
		} else if (!cable_connected) {
			crg_dbg("cable disconnected, resetting controller\n");

			crg_reset(u);
			if (u->func && u->func->disconnect)
				u->func->disconnect(&u->udev);

			reset_data_struct(u);
			u->connected = 0;
			init_ep0(u);
			u->device_state = USB_STATE_ATTACHED;
			return -ECONNRESET;
		}
	}

	/* ---- port link change ---- */
	if (portsc_val & CRG_U3DC_PORTSC_PLC) {
		tmp = crg_reg_read(&uccr->portsc);
		if (CRG_U3DC_PORTSC_PLS_GET(tmp) == 0xf) {
			tmp = SETF_VAR(CRG_U3DC_PORTSC_PLS, tmp, 0x0);
			crg_reg_write(&uccr->portsc, tmp);
		} else if (CRG_U3DC_PORTSC_PLS_GET(tmp) == 0x3) {
			if (u->func && u->func->disconnect)
				u->func->disconnect(&u->udev);
		}
	}

	return 0;
}

static int crg_handle_event(struct crg_udc *u, struct crg_evt *ev)
{
	u32 dw3 = le32_to_cpu(ev->dw3);

	switch (GETF(EVE_TRB_TYPE, dw3)) {
	case TRB_TYPE_EVT_PORT_STATUS_CHANGE:
		if (u->device_state == USB_STATE_RECONNECTING) {
			u->portsc_on_reconnecting = 1;
			break;
		}
		return crg_handle_port_status(u);

	case TRB_TYPE_EVT_TRANSFER:
		if (u->device_state < USB_STATE_RECONNECTING) {
			crg_dbg("xfer event in state %d\n", u->device_state);
			break;
		}
		crg_handle_xfer_event(u, ev);
		break;

	case TRB_TYPE_EVT_SETUP_PKT: {
		struct usb_ctrlrequest setup;
		u8 tag = GETF(EVE_TRB_SETUP_TAG, dw3);

		/* the 8 setup bytes live in dw0/dw1 of the event */
		memcpy(&setup, &ev->dw0, sizeof(setup));

		if (u->setup_status != WAIT_FOR_SETUP) {
			/* previous setup hasn't completed: queue the new one */
			queue_setup_pkt(u, &setup, tag);
			break;
		}
		crg_handle_setup_pkt(u, &setup, tag);
		break;
	}
	default:
		crg_dbg("unexpected event type 0x%x\n", GETF(EVE_TRB_TYPE, dw3));
		break;
	}

	return 0;
}

static int crg_process_event_ring(struct crg_udc *u)
{
	struct crg_uicr *uicr = u->uicr;
	struct crg_evt *ev;
	dma_addr_t erdp;
	bool any = false;
	int ret;

	while (u->evt_dq) {
		crg_cache_inv(u->evt_dq, sizeof(*ev));
		rmb();
		ev = u->evt_dq;

		if (GETF(EVE_TRB_CYCLE_BIT, le32_to_cpu(ev->dw3)) != u->ccs)
			break;

		if (!any) {
			/* ack the interrupt-pending flag (W1C) */
			crg_reg_write(&uicr->iman,
				      crg_reg_read(&uicr->iman) |
				      CRG_U3DC_IMAN_INT_PEND);
			any = true;
		}

		ret = crg_handle_event(u, ev);
		if (ret == -ECONNRESET)
			return ret;	/* event ring was re-initialised */

		if (ev == u->evt_last) {
			u->ccs ^= 1;
			u->evt_dq = u->evt_va;
		} else {
			u->evt_dq++;
		}
	}

	if (any) {
		erdp = u->evt_dma +
		       ((u8 *)u->evt_dq - (u8 *)u->evt_va);
		crg_reg_write(&uicr->erdphi, upper_32_bits(erdp));
		crg_reg_write(&uicr->erdplo,
			      lower_32_bits(erdp) | CRG_U3DC_ERDPLO_EHB);
	}

	return 0;
}

/* =========================== ax_usb_dev_ops ============================== */
static int crg_ops_poll(struct ax_usb_dev *ud)
{
	struct crg_udc *u = to_crg(ud);
	struct crg_uccr *uccr = u->uccr;
	u32 st;

	if (!u->inited || !u->started)
		return 0;

	/* former vbus kthread: while not powered, retry once per second */
	if (u->device_state < USB_STATE_POWERED &&
	    get_timer(u->vbus_base) >= CRG_VBUS_POLL_MS) {
		u->vbus_base = get_timer(0);
		crg_cable_connected(u);
	}

	st = crg_reg_read(&uccr->status);
	if (st & CRG_U3DC_STATUS_SYS_ERR) {
		crg_err("system error!\n");
		crg_reg_write(&uccr->status, CRG_U3DC_STATUS_SYS_ERR);
	}
	if (st & CRG_U3DC_STATUS_EINT)
		crg_reg_write(&uccr->status, CRG_U3DC_STATUS_EINT);

	/* polling: look at the event ring regardless of EINT */
	crg_process_event_ring(u);

	if (u->device_state == USB_STATE_RECONNECTING &&
	    u->portsc_on_reconnecting == 1 && is_event_ring_empty(u)) {
		u->portsc_on_reconnecting = 0;
		crg_handle_port_status(u);
	}

	if (u->device_state == USB_STATE_RECONNECTING && u->connected == 1)
		prepare_for_setup(u);

	return 0;
}

static int crg_ops_init(struct ax_usb_dev *ud)
{
	struct crg_udc *u = to_crg(ud);
	int ret;

	if (u->inited)
		return 0;

	/* controller role = device */
	crg_reg_write(u->base + CRG_UDC_MODE_REG,
		      crg_reg_read(u->base + CRG_UDC_MODE_REG) |
		      CRGUDC_ROLE_DEVICE);

	u->uicr = u->base + CRG_UICR_OFFSET;
	u->uccr = u->base + CRG_UCCR_OFFSET;

	u->ep0_buf = memalign(ARCH_DMA_MINALIGN, ALIGN(64, ARCH_DMA_MINALIGN));
	if (!u->ep0_buf)
		return -ENOMEM;
	memset(u->ep0_buf, 0, ALIGN(64, ARCH_DMA_MINALIGN));

	ret = crg_reset(u);
	if (ret)
		goto err;

	crg_clear_portpm(u);

	ret = reset_data_struct(u);
	if (ret) {
		crg_err("reset_data_struct failed (%d)\n", ret);
		goto err;
	}

	init_ep_info(u);
	ret = init_ep0(u);
	if (ret)
		goto err;

	u->speed = USB_SPEED_UNKNOWN;
	u->connected = 0;
	u->cable_cnt = -1;
	u->inited = true;
	return 0;
err:
	free(u->ep0_buf);
	u->ep0_buf = NULL;
	return ret;
}

static int crg_ops_stop(struct ax_usb_dev *ud)
{
	struct crg_udc *u = to_crg(ud);

	if (!u->inited)
		return -ENODEV;

	u->device_state = USB_STATE_ATTACHED;
	if (u->started)
		crg_vbus_enable(u, false);

	crg_reset(u);
	reset_data_struct(u);
	u->connected = 0;
	u->func = NULL;
	u->speed = USB_SPEED_UNKNOWN;
	init_ep0(u);

	u->started = false;
	u->cable_cnt = -1;
	return 0;
}

static int crg_ops_start(struct ax_usb_dev *ud,
			 const struct ax_usb_func_ops *func)
{
	struct crg_udc *u = to_crg(ud);

	if (!u->inited)
		return -ENODEV;
	if (u->started)
		return -EBUSY;
	if (!func)
		return -EINVAL;

	u->func = func;
	u->cable_cnt = -1;
	u->cnt_setup = u->cnt_xfer_evt = u->cnt_port_evt = 0;
	u->cnt_bus_reset = 0;

	crg_vbus_enable(u, true);
	/* make the first poll run the cable detection right away */
	u->vbus_base = get_timer(0) - CRG_VBUS_POLL_MS;
	u->started = true;
	return 0;
}

static void crg_ops_exit(struct ax_usb_dev *ud)
{
	struct crg_udc *u = to_crg(ud);
	u32 tmp;
	int i;

	if (!u->inited)
		return;

	if (u->started)
		crg_ops_stop(ud);

	/* back to host role */
	tmp = crg_reg_read(u->base + CRG_UDC_MODE_REG) & ~CRGUDC_ROLE_DEVICE;
	crg_reg_write(u->base + CRG_UDC_MODE_REG, tmp);

	for (i = 0; i < CRG_NUM_EP_CX; i++) {
		free(u->ep[i].ring_va);
		u->ep[i].ring_va = NULL;
		u->ep[i].first = NULL;
		u->ep[i].has_desc = false;
	}
	free(u->epcx_va);
	free(u->erst_va);
	free(u->evt_va);
	free(u->ep0_buf);
	u->epcx_va = u->erst_va = u->evt_va = NULL;
	u->ep0_buf = NULL;
	u->evt_dq = NULL;
	u->inited = false;
}

static void crg_ops_status(struct ax_usb_dev *ud, struct ax_usb_status *st)
{
	struct crg_udc *u = to_crg(ud);

	memset(st, 0, sizeof(*st));
	st->initialized = u->inited;
	st->started = u->started;
	st->max_speed = u->max_speed;
	if (!u->inited)
		return;

	st->connected = u->connected;
	st->device_state = u->device_state;
	st->speed = u->speed;
	st->dev_addr = u->dev_addr;
	st->num_enabled_eps = u->num_enabled_eps;
	st->ep_enable = crg_reg_read(&u->uccr->ep_enable);
	st->ep_running = crg_reg_read(&u->uccr->ep_running);
	st->control = crg_reg_read(&u->uccr->control);
	st->hw_status = crg_reg_read(&u->uccr->status);
	st->portsc = crg_reg_read(&u->uccr->portsc);
	st->cnt_setup = u->cnt_setup;
	st->cnt_xfer_evt = u->cnt_xfer_evt;
	st->cnt_port_evt = u->cnt_port_evt;
	st->cnt_bus_reset = u->cnt_bus_reset;
}

static int crg_ops_ep_disable(struct ax_usb_dev *ud, u8 addr)
{
	struct crg_udc *u = to_crg(ud);
	struct crg_ep *ep = crg_find_ep(u, addr);

	if (!ep)
		return -EINVAL;
	if (ep->dci == 0)
		return 0;
	if (ep->state == EP_STATE_DISABLED)
		return -EINVAL;

	/* w1c: disable the endpoint */
	crg_reg_write(&u->uccr->ep_enable, 1U << ep->dci);

	nuke(u, ep, -ESHUTDOWN);
	u->num_enabled_eps--;
	ep->has_desc = false;

	memset(u->p_epcx + ep->dci - 2, 0, sizeof(struct crg_epcx));
	crg_cache_flush(u->p_epcx + ep->dci - 2, sizeof(struct crg_epcx));

	/* don't override a state already changed to default by a port reset */
	if (u->num_enabled_eps == 0 && u->device_state == USB_STATE_CONFIGURED)
		u->device_state = USB_STATE_ADDRESS;

	ep->state = EP_STATE_DISABLED;
	return 0;
}

static int crg_ops_ep_enable(struct ax_usb_dev *ud,
			     const struct usb_endpoint_descriptor *desc,
			     const struct usb_ss_ep_comp_descriptor *comp)
{
	struct crg_udc *u = to_crg(ud);
	struct crg_ep *ep;
	u32 ring_size;
	u16 maxp;
	int ret;

	if (!desc || desc->bDescriptorType != USB_DT_ENDPOINT)
		return -EINVAL;
	if (!u->inited || !u->func)
		return -ESHUTDOWN;

	ep = crg_find_ep(u, desc->bEndpointAddress);
	if (!ep || ep->dci < 2)
		return -EINVAL;

	switch (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
	case USB_ENDPOINT_XFER_BULK:
		ring_size = CRGUDC_BULK_EP_TD_RING_SIZE;
		break;
	case USB_ENDPOINT_XFER_INT:
		ring_size = CRGUDC_INT_EP_TD_RING_SIZE;
		break;
	default:
		return -EOPNOTSUPP;	/* isoc / control */
	}

	if (ep->state != EP_STATE_DISABLED && ep->has_desc)
		crg_ops_ep_disable(ud, desc->bEndpointAddress);

	maxp = le16_to_cpu(desc->wMaxPacketSize);
	ep->type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	ep->is_in = !!(desc->bEndpointAddress & USB_DIR_IN);
	ep->maxp = maxp & 0x07ff;
	ep->interval = desc->bInterval;
	ep->maxburst = 0;
	if (u->speed >= USB_SPEED_SUPER && comp)
		ep->maxburst = comp->bMaxBurst;
	ep->has_desc = true;

	ret = ring_alloc(ep, ring_size);
	if (ret) {
		ep->has_desc = false;
		return ret;
	}
	ring_reset(ep);
	ep->head = ep->tail = NULL;
	u->num_enabled_eps++;
	crg_epcx_setup(u, ep);

	crg_issue_command(u, CRG_CMD_CONFIG_EP, 1U << ep->dci, 0);

	if (u->device_state == USB_STATE_ADDRESS)
		u->device_state = USB_STATE_CONFIGURED;

	ep->wedge = false;
	ep->state = EP_STATE_RUNNING;
	return 0;
}

static int crg_ops_ep_queue(struct ax_usb_dev *ud, u8 addr,
			    struct ax_usb_req *req)
{
	struct crg_udc *u = to_crg(ud);
	struct crg_ep *ep = crg_find_ep(u, addr);
	struct crg_trb *trb;
	bool azp;

	if (!ep || ep->dci < 2 || !req)
		return -EINVAL;
	if (!ep->has_desc || ep->state == EP_STATE_DISABLED || !ep->first)
		return -EINVAL;
	if (req->length > AX_USB_REQ_MAX_LEN || (req->length && !req->buf))
		return -EINVAL;
	if (!ring_has_room(ep))
		return -EBUSY;

	req->status = -EINPROGRESS;
	req->actual = 0;
	req->next = NULL;
	req->dma = req->length ? virt_to_phys(req->buf) : 0;

	if (req->length) {
		if (ep->is_in)
			crg_cache_flush(req->buf, req->length);
		else
			crg_cache_inv(req->buf, req->length);
	}

	/* zero length packet only for IN transfers that end on a full packet */
	azp = ep->is_in && req->zero && req->length &&
	      (req->length % ep->maxp) == 0;

	trb = ep->enq;
	setup_normal_trb(trb, req->dma, ep->pcs, req->length,
			 ep->is_in ? 0 : 1, azp ? 1 : 0);
	crg_trb_flush(trb);
	req->trb = trb;

	/* link into the software queue */
	if (ep->tail)
		ep->tail->next = req;
	else
		ep->head = req;
	ep->tail = req;

	ring_advance_enq(ep);
	knock_doorbell(u, ep->dci);
	return 0;
}

static int crg_ops_ep_halt(struct ax_usb_dev *ud, u8 addr, int halt)
{
	struct crg_udc *u = to_crg(ud);
	struct crg_ep *ep = crg_find_ep(u, addr);

	if (!ep)
		return -EINVAL;
	return ep_halt(u, ep, halt, 1);
}

static const struct ax_usb_dev_ops crg_udc_bm_ops = {
	.init		= crg_ops_init,
	.exit		= crg_ops_exit,
	.start		= crg_ops_start,
	.stop		= crg_ops_stop,
	.poll		= crg_ops_poll,
	.status		= crg_ops_status,
	.ep_enable	= crg_ops_ep_enable,
	.ep_disable	= crg_ops_ep_disable,
	.ep_queue	= crg_ops_ep_queue,
	.ep_halt	= crg_ops_ep_halt,
};

struct ax_usb_dev *crg_udc_bm_create(void __iomem *base,
				     enum usb_device_speed max_speed)
{
	struct crg_udc *u;

	if (max_speed != USB_SPEED_FULL && max_speed != USB_SPEED_HIGH &&
	    max_speed != USB_SPEED_SUPER && max_speed != USB_SPEED_SUPER_PLUS)
		return NULL;

	u = calloc(1, sizeof(*u));
	if (!u)
		return NULL;

	u->base = base;
	u->max_speed = max_speed;
	u->speed = max_speed;
	u->cable_cnt = -1;
	u->udev.ops = &crg_udc_bm_ops;
	u->udev.priv = u;
	return &u->udev;
}

void crg_udc_bm_destroy(struct ax_usb_dev *udev)
{
	struct crg_udc *u;

	if (!udev)
		return;
	u = to_crg(udev);
	crg_ops_exit(udev);
	free(u);
}
