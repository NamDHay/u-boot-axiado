// SPDX-License-Identifier: GPL-2.0+
/*
 * Minimal polling xHCI host driver: controller init, root-port enumeration,
 * control + bulk transfers. Enough for USB mass storage (bulk-only).
 */
#include "xhci.h"

#ifdef AX_XHCI_DMA_POOL_BASE
#include <mapmem.h>
#endif
#ifndef AX_XHCI_DMA_POOL_SIZE
#define AX_XHCI_DMA_POOL_SIZE	0x80000
#endif
#define XHCI_POOL_MIN		0x20000		/* 128KB: one device + 64KB bounce */
static u8 *pool_base;
static size_t pool_size;
static size_t pool_off;

void xhci_pool_reset(void)
{
	pool_off = 0;
}

void *xhci_pool_alloc(size_t size, size_t align)
{
	size_t o;

	if (!pool_base) {
#ifdef AX_XHCI_DMA_POOL_BASE
		pool_size = AX_XHCI_DMA_POOL_SIZE;
		pool_base = map_sysmem(AX_XHCI_DMA_POOL_BASE, pool_size);
#else
		size_t sz;

		/* 64KB aligned so the 64KB bounce buffer never crosses a 64KB line;
		 * halve the request until the (small) heap can satisfy it */
		for (sz = AX_XHCI_DMA_POOL_SIZE; sz >= XHCI_POOL_MIN; sz >>= 1) {
			pool_base = memalign(XHCI_BOUNCE_SZ, sz);
			if (pool_base) {
				pool_size = sz;
				break;
			}
		}
#endif
		if (!pool_base) {
			printf("xhci: cannot allocate DMA pool (heap too small, need >= %u KB aligned 64KB)\n",
			       XHCI_POOL_MIN / 1024);
			return NULL;
		}
		printf("xhci: DMA pool %lu KB at %p\n", (ulong)(pool_size / 1024), pool_base);
	}
	o = ALIGN(pool_off, align);
	if (o + size > pool_size) {
		printf("xhci: DMA pool exhausted (need %lu more bytes)\n",
		       (ulong)(o + size - pool_size));
		return NULL;
	}
	pool_off = o + size;
	return pool_base + o;
}

static int wait_reg(struct xhci *x, u32 off, u32 mask, u32 val, u32 ms)
{
	while (ms--) {
		if ((xr(x, off) & mask) == val)
			return 0;
		mdelay(1);
	}
	return -ETIMEDOUT;
}

static u32 *ctx_at(struct xhci *x, void *base, u32 idx)
{
	return (u32 *)((u8 *)base + idx * x->csz);
}

/* ------------------------------------------------------------------ */
/* Controller init / stop                                              */
/* ------------------------------------------------------------------ */
int xhci_init(struct xhci *x)
{
	u32 hcs1, hcs2, hcc, sp, i, psz, shift, page;
	u64 *spa;
	u32 *erst;
	int ret;

	xhci_pool_reset();

	/* 64KB bounce first: it needs 64KB alignment, so keep it at pool offset 0 */
	x->bounce = xhci_zalloc(XHCI_BOUNCE_SZ, XHCI_BOUNCE_SZ);
	if (!x->bounce)
		return -ENOMEM;

	hcs1 = xr(x, XHCI_HCSPARAMS1);
	hcs2 = xr(x, XHCI_HCSPARAMS2);
	hcc  = xr(x, XHCI_HCCPARAMS1);
	x->max_slots = hcs1 & 0xff;
	x->max_ports = hcs1 >> 24;
	x->csz = (hcc & BIT(2)) ? 64 : 32;
	if (x->max_slots > XHCI_MAX_SLOTS)
		x->max_slots = XHCI_MAX_SLOTS;

	/* halt, then reset */
	if (!(xr(x, XOP(x, OP_USBSTS)) & STS_HCH)) {
		xw(x, XOP(x, OP_USBCMD), xr(x, XOP(x, OP_USBCMD)) & ~CMD_RS);
		ret = wait_reg(x, XOP(x, OP_USBSTS), STS_HCH, STS_HCH, 100);
		if (ret)
			return ret;
	}
	xw(x, XOP(x, OP_USBCMD), CMD_HCRST);
	ret = wait_reg(x, XOP(x, OP_USBCMD), CMD_HCRST, 0, 1000);
	if (!ret)
		ret = wait_reg(x, XOP(x, OP_USBSTS), STS_CNR, 0, 1000);
	if (ret) {
		printf("xhci: reset timeout\n");
		return ret;
	}

	xw(x, XOP(x, OP_CONFIG), x->max_slots);

	/* DCBAA + scratchpad */
	x->dcbaa = xhci_zalloc((x->max_slots + 1) * 8, 64);
	if (!x->dcbaa)
		return -ENOMEM;

	sp = (((hcs2 >> 21) & 0x1f) << 5) | ((hcs2 >> 27) & 0x1f);
	if (sp) {
		psz = xr(x, XOP(x, OP_PAGESIZE)) & 0xffff;
		shift = 0;
		while (shift < 15 && !(psz & (1u << shift)))
			shift++;
		page = 4096u << shift;

		spa = xhci_zalloc(sp * 8, 64);
		if (!spa)
			return -ENOMEM;
		for (i = 0; i < sp; i++) {
			void *pg = xhci_zalloc(page, page);

			if (!pg)
				return -ENOMEM;
			spa[i] = xhci_cpu_to_bus(pg);
		}
		xhci_clean(spa, sp * 8);
		x->dcbaa[0] = xhci_cpu_to_bus(spa);
	}
	xhci_clean(x->dcbaa, (x->max_slots + 1) * 8);
	xhci_hw_wr64(&x->hw, XOP(x, OP_DCBAAP), xhci_cpu_to_bus(x->dcbaa));

	/* command ring */
	x->cmd = xhci_ring_alloc(XHCI_RING_TRBS);
	if (!x->cmd)
		return -ENOMEM;
	xhci_hw_wr64(&x->hw, XOP(x, OP_CRCR),
		     xhci_cpu_to_bus(x->cmd->trbs) | CRCR_RCS);

	/* event ring: one segment */
	x->evt_n = XHCI_RING_TRBS;
	x->evt = xhci_zalloc(x->evt_n * sizeof(struct xhci_trb), 4096);
	x->erst = xhci_zalloc(16, 64);
	if (!x->evt || !x->erst)
		return -ENOMEM;
	x->evt_deq = 0;
	x->evt_ccs = 1;

	erst = x->erst;
	erst[0] = lower_32_bits(xhci_cpu_to_bus(x->evt));
	erst[1] = upper_32_bits(xhci_cpu_to_bus(x->evt));
	erst[2] = x->evt_n;
	erst[3] = 0;
	xhci_clean(x->erst, 16);

	xw(x, XRT(x, IR_ERSTSZ), 1);
	xhci_hw_wr64(&x->hw, XRT(x, IR_ERDP), xhci_cpu_to_bus(x->evt));
	xhci_hw_wr64(&x->hw, XRT(x, IR_ERSTBA), xhci_cpu_to_bus(x->erst));

	/* DMA buffers */
	x->ctrl_buf = xhci_zalloc(512, 64);
	if (!x->ctrl_buf)
		return -ENOMEM;

	/* optional MSI: events are still polled, the handler only acks */
	if (x->hw.msi)
		xw(x, XRT(x, IR_IMAN), IMAN_IE | IMAN_IP);

	xw(x, XOP(x, OP_USBCMD), CMD_RS | (x->hw.msi ? CMD_INTE : 0));
	ret = wait_reg(x, XOP(x, OP_USBSTS), STS_HCH, 0, 100);
	if (ret) {
		printf("xhci: failed to start\n");
		return ret;
	}

	printf("xhci: v%x.%02x slots %u ports %u ctx %uB scratchpad %u\n",
	       x->hw.hci_version >> 8, x->hw.hci_version & 0xff,
	       x->max_slots, x->max_ports, x->csz, sp);
	return 0;
}

void xhci_stop(struct xhci *x)
{
	xw(x, XOP(x, OP_USBCMD), xr(x, XOP(x, OP_USBCMD)) & ~CMD_RS);
	wait_reg(x, XOP(x, OP_USBSTS), STS_HCH, STS_HCH, 100);
	xw(x, XOP(x, OP_USBCMD), CMD_HCRST);
	wait_reg(x, XOP(x, OP_USBCMD), CMD_HCRST, 0, 1000);
}

/* ------------------------------------------------------------------ */
/* Endpoint recovery                                                   */
/* ------------------------------------------------------------------ */
static int ep_recover(struct xhci *x, struct xhci_udev *d, u32 dci,
		      struct xhci_ring *r)
{
	u64 deq;
	int cc;

	/* a non-halted EP returns "context state error": harmless here */
	xhci_cmd(x, 0, 0, TRB_T(TRB_RESET_EP) | (d->slot << 24) | (dci << 16), NULL);

	deq = xhci_cpu_to_bus(&r->trbs[r->enq]) | r->pcs;
	cc = xhci_cmd(x, deq, 0, TRB_T(TRB_SET_DEQ) | (d->slot << 24) | (dci << 16), NULL);
	return cc == CC_SUCCESS ? 0 : -EIO;
}

/* ------------------------------------------------------------------ */
/* Control transfer (EP0)                                              */
/* ------------------------------------------------------------------ */
int xhci_ctrl_msg(struct xhci *x, struct xhci_udev *d, u8 rt, u8 req,
		  u16 val, u16 idx, void *buf, u16 len)
{
	bool in = rt & 0x80;
	u32 trt = len ? (in ? 3 : 2) : 0;
	struct xhci_trb ev;
	u64 setup;
	int cc;

	if (len > 512)
		return -EINVAL;

	if (len && !in) {
		memcpy(x->ctrl_buf, buf, len);
		xhci_clean(x->ctrl_buf, len);
	} else if (len) {
		xhci_inval(x->ctrl_buf, len);
	}

	setup = (u64)(rt | (req << 8) | ((u32)val << 16)) |
		((u64)(idx | ((u32)len << 16)) << 32);
	xhci_ring_enq(d->ep0, setup, 8, TRB_T(TRB_SETUP) | TRB_IDT | (trt << 16));
	if (len)
		xhci_ring_enq(d->ep0, xhci_cpu_to_bus(x->ctrl_buf), len,
			      TRB_T(TRB_DATA) | (in ? TRB_DIR_IN : 0));
	xhci_ring_enq(d->ep0, 0, 0,
		      TRB_T(TRB_STATUS) | TRB_IOC | ((len && in) ? 0 : TRB_DIR_IN));

	xhci_ring_db(x, d->slot, 1);
	cc = xhci_xfer_wait(x, d->slot, 1, XHCI_CTRL_TIMEOUT_MS, &ev);
	if (cc < 0)
		return cc;
	if (cc != CC_SUCCESS && cc != CC_SHORT) {
		ep_recover(x, d, 1, d->ep0);
		return cc == CC_STALL ? -EPIPE : -EIO;
	}

	if (len && in) {
		xhci_inval(x->ctrl_buf, len);
		memcpy(buf, x->ctrl_buf, len);
	}
	return 0;
}

int xhci_clear_halt(struct xhci *x, struct xhci_udev *d, u8 ep_addr)
{
	/* CLEAR_FEATURE(ENDPOINT_HALT) to the endpoint */
	return xhci_ctrl_msg(x, d, 0x02, 1, 0, ep_addr, NULL, 0);
}

/* ------------------------------------------------------------------ */
/* Bulk transfer (through the 64KB bounce buffer)                      */
/* ------------------------------------------------------------------ */
int xhci_bulk(struct xhci *x, struct xhci_udev *d, bool in, void *buf,
	      u32 len, u32 *actual)
{
	struct xhci_ring *r = in ? d->ring_in : d->ring_out;
	u32 dci = in ? d->dci_in : d->dci_out;
	u32 done = 0, n, got, resid;
	struct xhci_trb ev;
	int cc;

	while (done < len) {
		n = min_t(u32, len - done, XHCI_BOUNCE_SZ);

		if (in) {
			xhci_inval(x->bounce, n);
		} else {
			memcpy(x->bounce, (u8 *)buf + done, n);
			xhci_clean(x->bounce, n);
		}

		xhci_ring_enq(r, xhci_cpu_to_bus(x->bounce), n,
			      TRB_T(TRB_NORMAL) | TRB_IOC | TRB_ISP);
		xhci_ring_db(x, d->slot, dci);

		cc = xhci_xfer_wait(x, d->slot, dci, XHCI_XFER_TIMEOUT_MS, &ev);
		if (cc < 0)
			return cc;			/* timeout */
		if (cc != CC_SUCCESS && cc != CC_SHORT) {
			ep_recover(x, d, dci, r);
			return cc == CC_STALL ? -EPIPE : -EIO;
		}

		resid = ev.status & 0xffffff;
		got = n - resid;
		if (in) {
			xhci_inval(x->bounce, got);
			memcpy((u8 *)buf + done, x->bounce, got);
		}
		done += got;
		if (got < n)
			break;				/* short packet ends the transfer */
	}

	if (actual)
		*actual = done;
	return 0;
}

/* ------------------------------------------------------------------ */
/* Port handling                                                       */
/* ------------------------------------------------------------------ */
static int port_reset(struct xhci *x, u32 p)
{
	u32 off = XOP(x, OP_PORTSC(p)), v;
	int i;

	v = xr(x, off);
	/* acknowledge stale change bits */
	xw(x, off, PORTSC_NEUTRAL(v) | PORTSC_CHG_ALL);

	/* USB3 ports normally train to U0 on their own */
	if (PORTSC_SPEED(v) >= USB_SPEED_SS && (v & PORTSC_PED) && PORTSC_PLS(v) == 0)
		return 0;

	xw(x, off, PORTSC_NEUTRAL(xr(x, off)) | PORTSC_PR);
	for (i = 0; i < 500; i++) {
		v = xr(x, off);
		if (v & PORTSC_PRC)
			break;
		mdelay(1);
	}
	if (!(v & PORTSC_PRC))
		return -ETIMEDOUT;

	xw(x, off, PORTSC_NEUTRAL(v) | PORTSC_PRC | PORTSC_WRC);
	mdelay(50);			/* reset recovery */

	v = xr(x, off);
	if (!(v & PORTSC_PED))
		return -ENODEV;
	return 0;
}

/* ------------------------------------------------------------------ */
/* Device enumeration                                                  */
/* ------------------------------------------------------------------ */
static int parse_msc(struct xhci_udev *d, const u8 *cfg, u16 len)
{
	const u8 *p = cfg, *end = cfg + len;
	bool in_if = false, found = false;
	int last = -1;		/* 1 = last EP was IN, 0 = OUT */

	while (p + 2 <= end && p[0]) {
		if (p[1] == 4 && p + 9 <= end) {		/* interface */
			in_if = !found && p[3] == 0 && p[5] == 0x08 &&
				p[6] == 0x06 && p[7] == 0x50;
			if (in_if) {
				d->iface = p[2];
				found = true;
			}
			last = -1;
		} else if (p[1] == 5 && in_if && p + 7 <= end) {	/* endpoint */
			u16 mps = (p[4] | (p[5] << 8)) & 0x7ff;

			if ((p[3] & 3) == 2) {		/* bulk */
				if (p[2] & 0x80) {
					d->in_ep = p[2] & 0xf;
					d->in_mps = mps;
					last = 1;
				} else {
					d->out_ep = p[2] & 0xf;
					d->out_mps = mps;
					last = 0;
				}
			}
		} else if (p[1] == 0x30 && in_if && p + 3 <= end) {	/* SS companion */
			if (last == 1)
				d->in_burst = p[2];
			else if (last == 0)
				d->out_burst = p[2];
		}
		p += p[0];
	}
	return (found && d->in_ep && d->out_ep) ? 0 : -ENODEV;
}

static int configure_eps(struct xhci *x, struct xhci_udev *d)
{
	u32 *c, maxdci, i;
	u64 deq;
	int cc;

	d->ring_in = xhci_ring_alloc(XHCI_RING_TRBS);
	d->ring_out = xhci_ring_alloc(XHCI_RING_TRBS);
	if (!d->ring_in || !d->ring_out)
		return -ENOMEM;

	d->dci_in = d->in_ep * 2 + 1;
	d->dci_out = d->out_ep * 2;
	maxdci = max(d->dci_in, d->dci_out);

	/* start from the controller's view of the slot context */
	xhci_inval(d->out_ctx, 32 * x->csz);
	memset(d->in_ctx, 0, 33 * x->csz);
	memcpy(ctx_at(x, d->in_ctx, 1), ctx_at(x, d->out_ctx, 0), x->csz);

	c = ctx_at(x, d->in_ctx, 1);
	c[0] = (c[0] & ~(0x1fu << 27)) | (maxdci << 27);

	c = ctx_at(x, d->in_ctx, 0);
	c[1] = BIT(0) | BIT(d->dci_in) | BIT(d->dci_out);

	for (i = 0; i < 2; i++) {
		bool in = i == 0;
		u32 dci = in ? d->dci_in : d->dci_out;
		u16 mps = in ? d->in_mps : d->out_mps;
		u8 burst = in ? d->in_burst : d->out_burst;
		struct xhci_ring *r = in ? d->ring_in : d->ring_out;

		c = ctx_at(x, d->in_ctx, dci + 1);
		deq = xhci_cpu_to_bus(r->trbs);
		c[0] = 0;
		c[1] = (3 << 1) | ((in ? EP_TYPE_BULK_IN : EP_TYPE_BULK_OUT) << 3) |
		       ((u32)burst << 8) | ((u32)mps << 16);
		c[2] = lower_32_bits(deq) | 1;
		c[3] = upper_32_bits(deq);
		c[4] = mps;
	}
	xhci_clean(d->in_ctx, 33 * x->csz);

	cc = xhci_cmd(x, xhci_cpu_to_bus(d->in_ctx), 0,
		      TRB_T(TRB_CONFIG_EP) | (d->slot << 24), NULL);
	return cc == CC_SUCCESS ? 0 : -EIO;
}

static int enum_device(struct xhci *x, u32 port)
{
	struct xhci_udev *d = NULL;
	struct xhci_trb ev;
	u8 desc[512];
	u32 speed, mps, init_mps, *c;
	u16 total;
	u64 deq;
	int i, cc, ret;

	for (i = 0; i < XHCI_MAX_DEV; i++) {
		if (!x->udev[i].present) {
			d = &x->udev[i];
			break;
		}
	}
	if (!d)
		return -ENOMEM;
	memset(d, 0, sizeof(*d));

	speed = PORTSC_SPEED(xr(x, XOP(x, OP_PORTSC(port))));
	if (!speed)
		return -ENODEV;

	cc = xhci_cmd(x, 0, 0, TRB_T(TRB_ENABLE_SLOT), &ev);
	if (cc != CC_SUCCESS) {
		printf("xhci: enable slot failed (%d)\n", cc);
		return -EIO;
	}
	d->slot = EVT_SLOT(ev.ctrl);
	d->port = port;
	d->speed = speed;

	d->out_ctx = xhci_zalloc(32 * x->csz, 64);
	d->in_ctx = xhci_zalloc(33 * x->csz, 64);
	d->ep0 = xhci_ring_alloc(64);
	if (!d->out_ctx || !d->in_ctx || !d->ep0)
		return -ENOMEM;

	init_mps = (speed >= USB_SPEED_SS) ? 512 : (speed == USB_SPEED_HS ? 64 : 8);

	c = ctx_at(x, d->in_ctx, 0);
	c[1] = BIT(0) | BIT(1);			/* add slot + EP0 */
	c = ctx_at(x, d->in_ctx, 1);
	c[0] = (1u << 27) | (speed << 20);	/* 1 context entry */
	c[1] = port << 16;			/* root hub port number */
	c = ctx_at(x, d->in_ctx, 2);		/* EP0 */
	deq = xhci_cpu_to_bus(d->ep0->trbs);
	c[1] = (3 << 1) | (EP_TYPE_CONTROL << 3) | (init_mps << 16);
	c[2] = lower_32_bits(deq) | 1;
	c[3] = upper_32_bits(deq);
	c[4] = 8;
	xhci_clean(d->in_ctx, 33 * x->csz);

	x->dcbaa[d->slot] = xhci_cpu_to_bus(d->out_ctx);
	xhci_clean(&x->dcbaa[d->slot], 8);

	cc = xhci_cmd(x, xhci_cpu_to_bus(d->in_ctx), 0,
		      TRB_T(TRB_ADDRESS_DEV) | (d->slot << 24), NULL);
	if (cc != CC_SUCCESS) {
		printf("xhci: address device failed (%d)\n", cc);
		return -EIO;
	}
	mdelay(10);

	/* first 8 bytes of the device descriptor give bMaxPacketSize0 */
	ret = xhci_ctrl_msg(x, d, 0x80, 6, 0x0100, 0, desc, 8);
	if (ret)
		return ret;
	mps = (speed >= USB_SPEED_SS) ? (1u << desc[7]) : desc[7];
	if (mps && mps != init_mps) {
		c = ctx_at(x, d->in_ctx, 0);
		c[1] = BIT(1);
		c = ctx_at(x, d->in_ctx, 2);
		c[1] = (c[1] & 0xffff) | (mps << 16);
		xhci_clean(d->in_ctx, 33 * x->csz);
		cc = xhci_cmd(x, xhci_cpu_to_bus(d->in_ctx), 0,
			      TRB_T(TRB_EVAL_CTX) | (d->slot << 24), NULL);
		if (cc != CC_SUCCESS)
			return -EIO;
	}

	/* configuration descriptor: 9 bytes, then the full thing */
	ret = xhci_ctrl_msg(x, d, 0x80, 6, 0x0200, 0, desc, 9);
	if (ret)
		return ret;
	total = min_t(u16, desc[2] | (desc[3] << 8), sizeof(desc));
	ret = xhci_ctrl_msg(x, d, 0x80, 6, 0x0200, 0, desc, total);
	if (ret)
		return ret;

	d->present = true;
	if (parse_msc(d, desc, total)) {
		printf("xhci: port %u: not a bulk-only mass-storage device\n", port);
		return 0;
	}

	ret = xhci_ctrl_msg(x, d, 0x00, 9, desc[5], 0, NULL, 0);	/* SET_CONFIGURATION */
	if (ret)
		return ret;
	ret = configure_eps(x, d);
	if (ret)
		return ret;

	d->is_msc = true;
	return 0;
}

int xhci_scan_ports(struct xhci *x)
{
	static const char *const spd[] = { "?", "FS", "LS", "HS", "SS", "SS+" };
	u32 p, v, found = 0;
	int ret;

	for (p = 1; p <= x->max_ports; p++) {
		v = xr(x, XOP(x, OP_PORTSC(p)));
		if (!(v & PORTSC_PP)) {
			xw(x, XOP(x, OP_PORTSC(p)), PORTSC_NEUTRAL(v) | PORTSC_PP);
			mdelay(20);
		}
	}
	mdelay(100);		/* let devices connect / USB3 link train */

	for (p = 1; p <= x->max_ports; p++) {
		v = xr(x, XOP(x, OP_PORTSC(p)));
		if (!(v & PORTSC_CCS))
			continue;

		ret = port_reset(x, p);
		if (ret) {
			printf("xhci: port %u reset failed (%d)\n", p, ret);
			continue;
		}
		v = xr(x, XOP(x, OP_PORTSC(p)));
		printf("xhci: port %u: device, speed %s\n", p,
		       spd[min_t(u32, PORTSC_SPEED(v), 5)]);

		ret = enum_device(x, p);
		if (ret)
			printf("xhci: port %u: enumeration failed (%d)\n", p, ret);
		else
			found++;
	}
	return found;
}