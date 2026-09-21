/*
 * xhci_ring.c - DMA pool, command / event / transfer rings, control and
 *               bulk transfer descriptors.
 *
 * Everything is polled: no interrupt handler is installed.  Only one
 * command or transfer is ever outstanding, so a stale/unexpected event is
 * simply logged and dropped.
 */

#include <dm.h>
#include <stdio.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <time.h>

#include "ax_diag.h"
#include "ax_usb.h"
#include "xhci.h"

/* ------------------------------------------------------------------ */
/* DMA pool (bump allocator over the fixed carve-out)                  */
/* ------------------------------------------------------------------ */
void *axh_dma_alloc(struct axh_ctrl *x, size_t size, size_t align)
{
	uintptr_t cur = ((uintptr_t)x->pool_cur + align - 1) &
			~(uintptr_t)(align - 1);

	if (cur + size > (uintptr_t)x->pool_end) {
		printf("xHCI DMA pool exhausted (need 0x%x bytes)\n", (u32)size);
		return NULL;
	}
	x->pool_cur = (u8 *)(cur + size);
	memset((void *)cur, 0, size);
	axh_cache_flush((void *)cur, size);
	return (void *)cur;
}

const char *axh_cc_str(u32 code)
{
	switch (code) {
	case AXH_CC_SUCCESS:		return "success";
	case AXH_CC_DATA_BUF_ERR:	return "data buffer error";
	case AXH_CC_BABBLE:		return "babble";
	case AXH_CC_XACT_ERR:		return "USB transaction error";
	case AXH_CC_TRB_ERR:		return "TRB error";
	case AXH_CC_STALL:		return "stall";
	case 7:				return "resource error";
	case 8:				return "bandwidth error";
	case AXH_CC_NO_SLOTS:		return "no slots available";
	case 11:			return "slot not enabled";
	case 12:			return "endpoint not enabled";
	case AXH_CC_SHORT_PKT:		return "short packet";
	case AXH_CC_PARAM_ERR:		return "parameter error";
	case 18:			return "bandwidth overrun";
	case AXH_CC_CTX_STATE_ERR:	return "context state error";
	case 20:			return "no ping response";
	case 21:			return "event ring full";
	case 22:			return "incompatible device";
	case 23:			return "missed service";
	case 36:			return "split transaction error";
	default:			return "unknown";
	}
}

/* ------------------------------------------------------------------ */
/* Rings                                                               */
/* ------------------------------------------------------------------ */
static inline void axh_trb_write(struct axh_trb *t, u32 d0, u32 d1, u32 d2, u32 d3)
{
	t->dw[0] = cpu_to_le32(d0);
	t->dw[1] = cpu_to_le32(d1);
	t->dw[2] = cpu_to_le32(d2);
	axh_wmb();
	t->dw[3] = cpu_to_le32(d3);	/* cycle bit last: hands the TRB to the xHC */
	axh_cache_flush(t, sizeof(*t));
}

int axh_ring_init(struct axh_ctrl *x, struct axh_ring *r, u32 num_trbs)
{
	size_t bytes = num_trbs * sizeof(struct axh_trb);
	struct axh_trb *link;
	u64 dma;

	/* alignment == size (power of two) => ring never crosses a 64 KiB boundary */
	r->first = axh_dma_alloc(x, bytes, bytes);
	if (!r->first)
		return -ENOMEM;

	r->num_trbs = num_trbs;
	r->enq = r->first;
	r->cycle = 1;

	dma = (u64)(uintptr_t)r->first;
	link = &r->first[num_trbs - 1];
	axh_trb_write(link, AXH_LO32(dma), AXH_HI32(dma), 0,
		      AXH_TRB_TYPE(AXH_TRB_LINK) | AXH_TRB_TC);
	return 0;
}

/* Queue one TRB; d3 must not contain the cycle bit. */
struct axh_trb *axh_ring_queue(struct axh_ring *r, u32 d0, u32 d1, u32 d2, u32 d3)
{
	struct axh_trb *t = r->enq;
	struct axh_trb *link;

	axh_trb_write(t, d0, d1, d2, d3 | r->cycle);
	r->enq++;

	link = &r->first[r->num_trbs - 1];
	if (r->enq == link) {
		u32 l3 = le32_to_cpu(link->dw[3]);

		l3 = (l3 & ~AXH_TRB_CYCLE) | r->cycle;
		link->dw[3] = cpu_to_le32(l3);
		axh_cache_flush(link, sizeof(*link));
		r->enq = r->first;
		r->cycle ^= 1;
	}
	return t;
}

static inline void axh_ring_db(struct axh_ctrl *x, u32 slot, u32 target)
{
	axh_wmb();
	writel(target, &x->db[slot]);
}

/* ------------------------------------------------------------------ */
/* Event ring (single interrupter, single segment)                     */
/* ------------------------------------------------------------------ */
int axh_evt_init(struct axh_ctrl *x)
{
	struct axh_event_ring *e = &x->evt;
	struct axh_intr_regs *ir = &x->run->ir[0];
	size_t bytes = AXH_EVT_RING_TRBS * sizeof(struct axh_trb);
	u64 ring_dma, erst_dma;

	e->first = axh_dma_alloc(x, bytes, bytes);
	e->erst = axh_dma_alloc(x, sizeof(struct axh_erst_entry), 64);
	if (!e->first || !e->erst)
		return -ENOMEM;

	e->num_trbs = AXH_EVT_RING_TRBS;
	e->deq = e->first;
	e->cycle = 1;

	ring_dma = (u64)(uintptr_t)e->first;
	erst_dma = (u64)(uintptr_t)e->erst;
	e->erst->addr_lo = cpu_to_le32(AXH_LO32(ring_dma));
	e->erst->addr_hi = cpu_to_le32(AXH_HI32(ring_dma));
	e->erst->size = cpu_to_le32(AXH_EVT_RING_TRBS);
	axh_cache_flush(e->erst, sizeof(*e->erst));

	/* order per spec: ERSTSZ, ERDP, ERSTBA (writing ERSTBA starts the ring) */
	writel(1, &ir->erstsz);
	writel(AXH_HI32(ring_dma), &ir->erdp_hi);
	writel(AXH_LO32(ring_dma), &ir->erdp_lo);
	writel(AXH_HI32(erst_dma), &ir->erstba_hi);
	writel(AXH_LO32(erst_dma), &ir->erstba_lo);
	writel(0, &ir->imod);
	writel(AXH_IMAN_IE | AXH_IMAN_IP, &ir->iman);
	return 0;
}

/* Consume one event if present. Returns 1 if @ev was filled, 0 if ring empty. */
static int axh_evt_next(struct axh_ctrl *x, struct axh_event *ev)
{
	struct axh_event_ring *e = &x->evt;
	struct axh_intr_regs *ir = &x->run->ir[0];
	struct axh_trb *t = e->deq;
	u32 d0, d1, d2, d3;
	u64 erdp;

	axh_cache_inval(t, sizeof(*t));
	d3 = le32_to_cpu(t->dw[3]);
	if ((d3 & AXH_TRB_CYCLE) != e->cycle)
		return 0;

	d0 = le32_to_cpu(t->dw[0]);
	d1 = le32_to_cpu(t->dw[1]);
	d2 = le32_to_cpu(t->dw[2]);

	ev->type = AXH_TRB_TYPE_GET(d3);
	ev->code = d2 >> 24;
	ev->residue = d2 & 0xffffff;
	ev->slot = d3 >> 24;
	ev->ep = (d3 >> 16) & 0x1f;
	ev->trb_ptr = ((u64)d1 << 32) | d0;
	ev->dw0 = d0;

	e->deq++;
	if (e->deq == e->first + e->num_trbs) {
		e->deq = e->first;
		e->cycle ^= 1;
	}

	erdp = (u64)(uintptr_t)e->deq;
	writel(AXH_HI32(erdp), &ir->erdp_hi);
	writel(AXH_LO32(erdp) | AXH_ERDP_EHB, &ir->erdp_lo);
	/* acknowledge (both are write-1-to-clear) */
	writel(AXH_STS_EINT, &x->op->usbsts);
	writel(readl(&ir->iman) | AXH_IMAN_IP, &ir->iman);
	return 1;
}

void axh_evt_drain(struct axh_ctrl *x)
{
	struct axh_event ev;

	while (axh_evt_next(x, &ev))
		printf("drop stale event type %d code %d\n", ev.type, ev.code);
}

int axh_evt_wait(struct axh_ctrl *x, u32 type, u32 slot, u32 ep,
		 struct axh_event *ev, u32 timeout_ms)
{
	unsigned long start = get_timer(0);

	do {
		while (axh_evt_next(x, ev)) {
			if (ev->type == type &&
			    (type != AXH_TRB_EV_TRANSFER ||
			     (ev->slot == slot && ev->ep == ep)))
				return 0;
			if (ev->type == AXH_TRB_EV_PORT_STATUS)
				printf("port status event, port %d\n", ev->dw0 >> 24);
			else
				printf("unexpected event type %d slot %d ep %d code %d\n",
					   ev->type, ev->slot, ev->ep, ev->code);
		}
		udelay(20);
	} while (get_timer(start) < timeout_ms);

	return -ETIMEDOUT;
}

/* ------------------------------------------------------------------ */
/* Commands                                                            */
/* ------------------------------------------------------------------ */
int axh_cmd(struct axh_ctrl *x, u32 d0, u32 d1, u32 d2, u32 d3,
	    struct axh_event *ev)
{
	struct axh_event local;
	int ret;

	if (!ev)
		ev = &local;

	axh_evt_drain(x);
	axh_ring_queue(&x->cmd_ring, d0, d1, d2, d3);
	axh_ring_db(x, 0, 0);

	ret = axh_evt_wait(x, AXH_TRB_EV_CMD_COMPLETION, 0, 0, ev, AXH_CMD_TIMEOUT_MS);
	if (ret) {
		printf("command (type %d) timed out\n", AXH_TRB_TYPE_GET(d3));
		return ret;
	}
	if (ev->code != AXH_CC_SUCCESS) {
		printf("command (type %d) failed: %s (%d)\n",
			   AXH_TRB_TYPE_GET(d3), axh_cc_str(ev->code), ev->code);
		return -EIO;
	}
	return 0;
}

/*
 * Bring a halted endpoint back: Reset Endpoint, then move the dequeue
 * pointer past everything already queued (the failed TD is dropped).
 */
int axh_ep_recover(struct axh_device *dev, u8 dci)
{
	struct axh_ctrl *x = dev->ctrl;
	struct axh_ring *r = &dev->ep[dci].ring;
	u64 deq = (u64)(uintptr_t)r->enq | r->cycle;
	int ret;

	ret = axh_cmd(x, 0, 0, 0,
		      AXH_TRB_TYPE(AXH_TRB_CMD_RESET_EP) | AXH_TRB_EP_ID(dci) |
		      AXH_TRB_SLOT(dev->slot_id), NULL);
	if (ret)
		return ret;

	return axh_cmd(x, AXH_LO32(deq), AXH_HI32(deq), 0,
		       AXH_TRB_TYPE(AXH_TRB_CMD_SET_DEQ) | AXH_TRB_EP_ID(dci) |
		       AXH_TRB_SLOT(dev->slot_id), NULL);
}

/* ------------------------------------------------------------------ */
/* Transfers                                                           */
/* ------------------------------------------------------------------ */
static int axh_xfer_wait(struct axh_device *dev, u8 dci, u32 timeout_ms)
{
	struct axh_event ev;
	int ret;

	ret = axh_evt_wait(dev->ctrl, AXH_TRB_EV_TRANSFER, dev->slot_id, dci,
			   &ev, timeout_ms);
	if (ret) {
		printf("transfer timeout on slot %d ep %d\n", dev->slot_id, dci);
		return ret;
	}

	switch (ev.code) {
	case AXH_CC_SUCCESS:
	case AXH_CC_SHORT_PKT:
		return 0;
	case AXH_CC_STALL:
		printf("ep %d stalled\n", dci);
		axh_ep_recover(dev, dci);
		return -EPIPE;
	default:
		printf("transfer error on ep %d: %s (%d)\n", dci,
			   axh_cc_str(ev.code), ev.code);
		axh_ep_recover(dev, dci);
		return -EIO;
	}
}

/*
 * Control transfer on EP0 (SETUP / [DATA] / STATUS).  @data may be any
 * memory; it is bounced through the DMA control buffer.
 */
int axh_control_msg(struct axh_device *dev, u8 reqtype, u8 req, u16 value,
		    u16 index, void *data, u16 len)
{
	struct axh_ctrl *x = dev->ctrl;
	struct axh_ring *ring = &dev->ep[1].ring;
	u64 dma = (u64)(uintptr_t)x->ctrl_buf;
	int in = !!(reqtype & AXU_DIR_IN);
	u32 trt = len ? (in ? 3 : 2) : 0;
	int ret;

	if (len > AXH_CTRL_BUF_SIZE)
		return -EINVAL;
	if (len) {
		if (in)
			memset(x->ctrl_buf, 0, len);
		else
			memcpy(x->ctrl_buf, data, len);
		axh_cache_flush(x->ctrl_buf, len);
	}

	axh_evt_drain(x);

	/* setup stage: 8 bytes of immediate data */
	axh_ring_queue(ring,
		       (u32)reqtype | ((u32)req << 8) | ((u32)value << 16),
		       (u32)index | ((u32)len << 16), 8,
		       AXH_TRB_TYPE(AXH_TRB_SETUP) | AXH_TRB_IDT | AXH_TRB_TRT(trt));
	/* data stage */
	if (len)
		axh_ring_queue(ring, AXH_LO32(dma), AXH_HI32(dma), AXH_TRB_LEN(len),
			       AXH_TRB_TYPE(AXH_TRB_DATA) | (in ? AXH_TRB_DIR_IN : 0));
	/* status stage: opposite direction of data, IN when there is no data */
	axh_ring_queue(ring, 0, 0, 0,
		       AXH_TRB_TYPE(AXH_TRB_STATUS) | AXH_TRB_IOC |
		       ((len && in) ? 0 : AXH_TRB_DIR_IN));

	axh_ring_db(x, dev->slot_id, 1);
	ret = axh_xfer_wait(dev, 1, AXH_CTRL_TIMEOUT_MS);
	if (ret)
		return ret;

	if (len && in) {
		axh_cache_inval(x->ctrl_buf, len);
		memcpy(data, x->ctrl_buf, len);
	}
	return 0;
}

/*
 * Bulk transfer of @len bytes on endpoint @dci (odd = IN).  @buf must be
 * DMA-able (identity mapped).  The TD is split so no TRB crosses a 64 KiB
 * boundary and TRBs are chained (CH) with a TD-size remainder as required
 * by xHCI 1.0+.  Short packets are treated as success.
 */
int axh_bulk_xfer(struct axh_device *dev, u8 dci, void *buf, u32 len)
{
	struct axh_ctrl *x = dev->ctrl;
	struct axh_ep *ep;
	u64 addr = (u64)(uintptr_t)buf;
	u32 mps, remaining = len, sent = 0, chunk, total_pkts, rem_pkts, flags;
	int in = dci & 1;
	int ret;

	if (dci < 2 || dci >= AXH_MAX_EPS)
		return -EINVAL;
	ep = &dev->ep[dci];
	mps = le16_to_cpu(ep->desc.wMaxPacketSize) & 0x7ff;
	if (!ep->valid || !mps)
		return -EINVAL;

	axh_cache_flush(buf, len);
	axh_evt_drain(x);

	total_pkts = (len + mps - 1) / mps;
	do {
		chunk = AXH_TRB_MAX_LEN - (u32)(addr & (AXH_TRB_MAX_LEN - 1));
		if (chunk > remaining)
			chunk = remaining;
		remaining -= chunk;

		if (remaining) {
			rem_pkts = total_pkts - (sent + chunk) / mps;
			if (rem_pkts > 31)
				rem_pkts = 31;
			flags = AXH_TRB_CH;
		} else {
			rem_pkts = 0;
			flags = AXH_TRB_IOC;
		}

		axh_ring_queue(&ep->ring, AXH_LO32(addr), AXH_HI32(addr),
			       AXH_TRB_LEN(chunk) | AXH_TRB_TD_SIZE(rem_pkts),
			       AXH_TRB_TYPE(AXH_TRB_NORMAL) | flags);
		addr += chunk;
		sent += chunk;
	} while (remaining);

	axh_ring_db(x, dev->slot_id, dci);
	ret = axh_xfer_wait(dev, dci, AXH_BULK_TIMEOUT_MS);
	if (!ret && in)
		axh_cache_inval(buf, len);
	return ret;
}

