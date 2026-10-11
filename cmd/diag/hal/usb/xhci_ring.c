
// SPDX-License-Identifier: GPL-2.0+
/* xHCI producer rings (command / transfer), event ring polling, commands */
#include "xhci.h"

struct xhci_ring *xhci_ring_alloc(u32 n)
{
	struct xhci_ring *r;
	struct xhci_trb *link;
	u64 b;

	r = xhci_zalloc(sizeof(*r), 0);
	if (!r)
		return NULL;

	/* 4KB aligned: a 256-TRB segment never crosses a 64KB boundary */
	r->trbs = xhci_zalloc(n * sizeof(struct xhci_trb), 4096);
	if (!r->trbs)
		return NULL;
	r->n = n;
	r->enq = 0;
	r->pcs = 1;

	b = xhci_cpu_to_bus(r->trbs);
	link = &r->trbs[n - 1];
	link->p_lo = lower_32_bits(b);
	link->p_hi = upper_32_bits(b);
	link->ctrl = TRB_T(TRB_LINK) | TRB_TC;
	xhci_clean(r->trbs, n * sizeof(struct xhci_trb));
	return r;
}

/* Queue one TRB. Cycle bit is stamped with the ring's producer cycle state. */
struct xhci_trb *xhci_ring_enq(struct xhci_ring *r, u64 ptr, u32 status, u32 ctrl)
{
	struct xhci_trb *t = &r->trbs[r->enq];

	t->p_lo = lower_32_bits(ptr);
	t->p_hi = upper_32_bits(ptr);
	t->status = status;
	t->ctrl = (ctrl & ~TRB_CYCLE) | r->pcs;
	xhci_clean(t, sizeof(*t));

	if (++r->enq == r->n - 1) {
		struct xhci_trb *l = &r->trbs[r->enq];

		l->ctrl = TRB_T(TRB_LINK) | TRB_TC | r->pcs;
		xhci_clean(l, sizeof(*l));
		r->enq = 0;
		r->pcs ^= 1;
	}
	return t;
}

void xhci_ring_db(struct xhci *x, u32 slot, u32 target)
{
	xw(x, x->hw.db_off + slot * 4, target);
}

/* Pop one event from the event ring (polling). */
int xhci_event_poll(struct xhci *x, struct xhci_trb *ev, u32 timeout_ms)
{
	u32 loops = timeout_ms * 20;	/* 50us steps */

	while (loops--) {
		struct xhci_trb *t = &x->evt[x->evt_deq];

		xhci_inval(t, sizeof(*t));
		if ((t->ctrl & TRB_CYCLE) == x->evt_ccs) {
			*ev = *t;
			if (++x->evt_deq == x->evt_n) {
				x->evt_deq = 0;
				x->evt_ccs ^= 1;
			}
			xhci_hw_wr64(&x->hw, x->hw.rt_off + RT_IR0 + IR_ERDP,
				     xhci_cpu_to_bus(&x->evt[x->evt_deq]) | ERDP_EHB);
			return 0;
		}
		udelay(50);
	}
	return -ETIMEDOUT;
}

/* Debug: dump controller + ring state (called when a command times out) */
void xhci_dump_state(struct xhci *x, const char *why)
{
	struct xhci_trb *t;

	printf("xhci[%s]: USBCMD %08x USBSTS %08x\n", why,
	       xr(x, XOP(x, OP_USBCMD)), xr(x, XOP(x, OP_USBSTS)));
	printf("  CRCR %016llx DCBAAP %016llx\n",
	       xhci_hw_rd64(&x->hw, XOP(x, OP_CRCR)),
	       xhci_hw_rd64(&x->hw, XOP(x, OP_DCBAAP)));
	printf("  IMAN %08x ERSTSZ %08x ERSTBA %016llx ERDP %016llx\n",
	       xr(x, XRT(x, IR_IMAN)), xr(x, XRT(x, IR_ERSTSZ)),
	       xhci_hw_rd64(&x->hw, XRT(x, IR_ERSTBA)),
	       xhci_hw_rd64(&x->hw, XRT(x, IR_ERDP)));
	printf("  mine: cmd ring %016llx enq %u pcs %u | dcbaa %016llx | evt %016llx deq %u ccs %u\n",
	       xhci_cpu_to_bus(x->cmd->trbs), x->cmd->enq, x->cmd->pcs,
	       xhci_cpu_to_bus(x->dcbaa),
	       xhci_cpu_to_bus(x->evt), x->evt_deq, x->evt_ccs);

	t = &x->cmd->trbs[0];
	xhci_inval(t, sizeof(*t));
	printf("  cmd[0] %08x %08x %08x %08x\n", t->p_lo, t->p_hi, t->status, t->ctrl);
	t = &x->evt[0];
	xhci_inval(t, sizeof(*t));
	printf("  evt[0] %08x %08x %08x %08x\n", t->p_lo, t->p_hi, t->status, t->ctrl);
}

/*
 * Issue a command and wait for its completion event.
 * Returns the xHCI completion code (CC_SUCCESS == 1), or <0 on timeout.
 */
int xhci_cmd(struct xhci *x, u64 ptr, u32 status, u32 ctrl, struct xhci_trb *ev_out)
{
	struct xhci_trb *t = xhci_ring_enq(x->cmd, ptr, status, ctrl);
	u64 tb = xhci_cpu_to_bus(t);
	struct xhci_trb ev;
	int ret;

	xhci_ring_db(x, 0, 0);

	for (;;) {
		ret = xhci_event_poll(x, &ev, XHCI_CMD_TIMEOUT_MS);
		if (ret) {
			xhci_dump_state(x, "cmd timeout");
			return ret;
		}
		if (TRB_TYPE(ev.ctrl) != TRB_EVT_CMD)
			continue;	/* e.g. port status change: ignore */
		if ((((u64)ev.p_hi << 32) | ev.p_lo) != tb)
			continue;
		if (ev_out)
			*ev_out = ev;
		return EVT_CC(ev.status);
	}
}

/* Wait for a transfer event of (slot, dci). Returns completion code or <0. */
int xhci_xfer_wait(struct xhci *x, u32 slot, u32 dci, u32 timeout_ms,
		   struct xhci_trb *ev)
{
	int ret;

	for (;;) {
		ret = xhci_event_poll(x, ev, timeout_ms);
		if (ret)
			return ret;
		if (TRB_TYPE(ev->ctrl) == TRB_EVT_XFER &&
		    EVT_SLOT(ev->ctrl) == slot && EVT_EP(ev->ctrl) == dci)
			return EVT_CC(ev->status);
	}
}
