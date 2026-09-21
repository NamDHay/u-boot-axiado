/*
 * xhci.c - xHCI controller bring-up, root port handling and enumeration.
 *
 * Flow (mirrors baremetal crg_host_init() + crg_enum_device()):
 *   axh_ctrl_init()    halt/reset, DCBAA, cmd ring, event ring, ETE, run
 *   axh_wait_device()  wait for connect on the selected root port
 *   axh_enumerate()    port reset -> Enable Slot -> Address Device ->
 *                      descriptors -> Set Config + Configure Endpoint
 *                      -> class probe (Mass Storage)
 */

#include <dm.h>
#include <stdio.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <time.h>

#include "ax_diag.h"
#include "ax_usb.h"
#include "xhci.h"
#include "xhci_regs.h"

static u32 *axh_portsc(struct axh_ctrl *x, u32 idx)
{
	return (u32 *)((u8 *)x->op + AXH_OP_PORT_BASE + idx * AXH_PORT_STRIDE);
}

static int axh_wait_bits(u32 *reg, u32 mask, u32 val, u32 timeout_ms)
{
	unsigned long start = get_timer(0);

	do {
		if ((readl(reg) & mask) == val)
			return 0;
		udelay(100);
	} while (get_timer(start) < timeout_ms);

	return ((readl(reg) & mask) == val) ? 0 : -ETIMEDOUT;
}

const char *axh_speed_str(u8 speed)
{
	switch (speed) {
	case AXH_SPEED_LOW:		return "Low-speed (1.5 Mb/s)";
	case AXH_SPEED_FULL:		return "Full-speed (12 Mb/s)";
	case AXH_SPEED_HIGH:		return "High-speed (480 Mb/s)";
	case AXH_SPEED_SUPER:		return "SuperSpeed (5 Gb/s)";
	case AXH_SPEED_SUPER_PLUS:	return "SuperSpeed+ (10 Gb/s)";
	default:			return "unknown";
	}
}

/* ------------------------------------------------------------------ */
/* Controller reset / start / stop                                     */
/* ------------------------------------------------------------------ */
static int axh_ctrl_reset(struct axh_ctrl *x)
{
	int ret;

	if (!(readl(&x->op->usbsts) & AXH_STS_HCH))
		writel(readl(&x->op->usbcmd) & ~AXH_CMD_RUN, &x->op->usbcmd);

	ret = axh_wait_bits(&x->op->usbsts, AXH_STS_HCH, AXH_STS_HCH, AXH_HALT_TIMEOUT_MS);
	if (ret) {
		printf("xHC did not halt\n");
		return ret;
	}
	printf("xHC halted\n");

	writel(readl(&x->op->usbcmd) | AXH_CMD_HCRST, &x->op->usbcmd);
	ret = axh_wait_bits(&x->op->usbcmd, AXH_CMD_HCRST, 0, AXH_HALT_TIMEOUT_MS);
	if (ret) {
		printf("xHC reset did not complete\n");
		return ret;
	}
	ret = axh_wait_bits(&x->op->usbsts, AXH_STS_CNR, 0, AXH_HALT_TIMEOUT_MS);
	if (ret) {
		printf("xHC stuck in Controller Not Ready\n");
		return ret;
	}
	printf("xHC reset done\n");
	return 0;
}

static int axh_ctrl_start(struct axh_ctrl *x)
{
	int ret;

	writel(readl(&x->op->usbcmd) | AXH_CMD_RUN, &x->op->usbcmd);
	ret = axh_wait_bits(&x->op->usbsts, AXH_STS_HCH, 0, AXH_HALT_TIMEOUT_MS);
	if (ret) {
		printf("xHC did not start\n");
		return ret;
	}
	x->running = 1;
	printf("XHCI started\n");
	return 0;
}

void axh_ctrl_stop(struct axh_ctrl *x)
{
	if (!x->op || !x->running)
		return;
	writel(readl(&x->op->usbcmd) & ~AXH_CMD_RUN, &x->op->usbcmd);
	axh_wait_bits(&x->op->usbsts, AXH_STS_HCH, AXH_STS_HCH, AXH_HALT_TIMEOUT_MS);
	x->running = 0;
}

/* ------------------------------------------------------------------ */
/* Extended capabilities / scratchpad / port power                     */
/* ------------------------------------------------------------------ */
static void axh_parse_ext_caps(struct axh_ctrl *x)
{
	u8 *base = (u8 *)x->cap;
	u32 off = AXH_HCC1_XECP(readl(&x->cap->hccparams1)) << 2;

	if (!off)
		return;

	for (;;) {
		u32 v = readl(base + off);
		u32 next;

		if (v == 0xffffffffU)
			break;
		if (AXH_EXT_CAP_ID(v) == AXH_EXT_CAP_PROTOCOL) {
			u32 d2 = readl(base + off + 8);
			u32 major = AXH_EXT_PORT_MAJOR(v);
			int idx = (major == 3) ? 1 : 0;

			x->proto[idx].major = major;
			x->proto[idx].port_offset = AXH_EXT_PORT_OFF(d2);
			x->proto[idx].port_count = AXH_EXT_PORT_COUNT(d2);
			printf("USB%d protocol cap: port offset %d count %d\n",
				   major, x->proto[idx].port_offset,
				   x->proto[idx].port_count);
		}
		next = AXH_EXT_CAP_NEXT(v);
		if (!next)
			break;
		off += next << 2;
	}
}

static int axh_select_root_port(struct axh_ctrl *x)
{
	int idx = (x->protocol == 3) ? 1 : 0;

	if (x->proto[idx].port_count) {
		x->root_port = x->proto[idx].port_offset - 1;
	} else {
		/* no protocol cap: fall back to the hard-wired layout [0]=USB2 [1]=USB3 */
		if (idx >= x->max_ports)
			return -ENODEV;
		x->root_port = idx;
	}
	if (x->root_port >= x->max_ports)
		return -ENODEV;
	return 0;
}

static int axh_setup_scratchpad(struct axh_ctrl *x)
{
	u32 n = AXH_HCS2_MAX_SPBUF(readl(&x->cap->hcsparams2));
	u64 *arr;
	u32 i;

	if (!n)
		return 0;
	arr = axh_dma_alloc(x, n * sizeof(u64), 64);
	if (!arr)
		return -ENOMEM;
	for (i = 0; i < n; i++) {
		void *pg = axh_dma_alloc(x, x->page_size, x->page_size);

		if (!pg)
			return -ENOMEM;
		arr[i] = cpu_to_le64((u64)(uintptr_t)pg);
	}
	axh_cache_flush(arr, n * sizeof(u64));
	x->dcbaa[0] = cpu_to_le64((u64)(uintptr_t)arr);
	printf("%d scratchpad buffers\n", n);
	return 0;
}

static void axh_ports_power(struct axh_ctrl *x)
{
	u32 i;

	if (!(readl(&x->cap->hccparams1) & AXH_HCC1_PPC))
		return;
	for (i = 0; i < x->max_ports; i++) {
		u32 *reg = axh_portsc(x, i);
		u32 v = readl(reg);

		if (!(v & AXH_PORTSC_PP))
			writel(AXH_PORTSC_NEUTRAL(v) | AXH_PORTSC_PP, reg);
	}
	mdelay(20);
}

/* ------------------------------------------------------------------ */
/* Controller init                                                     */
/* ------------------------------------------------------------------ */
int axh_ctrl_init(struct axh_ctrl *x, int index, int protocol)
{
	u32 caps, hcs1, hcc1, ps;
	u64 dma;
	int ret;

	memset(x, 0, sizeof(*x));
	x->index = index;
	x->protocol = protocol;
	x->cap = (struct axh_cap_regs *)(uintptr_t)AXH_MMIO_BASE(index);

	caps = readl(&x->cap->cap_hci);
	if (caps == 0 || caps == 0xffffffffU) {
		printf("controller %d not responding (CAPLENGTH/HCIVERSION=0x%x)\n",
			   index, caps);
		return -ENODEV;
	}
	x->hci_version = AXH_HCIVERSION(caps);
	x->op = (struct axh_op_regs *)((u8 *)x->cap + AXH_CAPLENGTH(caps));
	x->run = (struct axh_run_regs *)((u8 *)x->cap + (readl(&x->cap->rtsoff) & ~0x1fU));
	x->db = (u32 *)((u8 *)x->cap + (readl(&x->cap->dboff) & ~0x3U));

	hcs1 = readl(&x->cap->hcsparams1);
	hcc1 = readl(&x->cap->hccparams1);
	x->max_slots = AXH_MIN(AXH_HCS1_MAX_SLOTS(hcs1), AXH_MAX_SLOTS);
	x->max_ports = AXH_HCS1_MAX_PORTS(hcs1);
	x->ctx_size = (hcc1 & AXH_HCC1_CSZ) ? 64 : 32;

	x->pool_base = (u8 *)(uintptr_t)AX_XHCI_MEM_BASE;
	x->pool_cur = x->pool_base;
	x->pool_end = x->pool_base + AX_XHCI_MEM_SIZE;

	printf("xHCI %x.%02x: %d slots, %d ports, %d-byte contexts\n",
		    x->hci_version >> 8, x->hci_version & 0xff,
		    x->max_slots, x->max_ports, x->ctx_size);

	ret = axh_ctrl_reset(x);
	if (ret)
		return ret;

	ps = readl(&x->op->pagesize) & 0xffff;
	x->page_size = ps ? (1U << (__builtin_ctz(ps) + 12)) : 4096;

	/* enabled device slots */
	writel((readl(&x->op->config) & ~AXH_CONFIG_SLOTS_MASK) | x->max_slots,
	       &x->op->config);

	/* DCBAA */
	x->dcbaa = axh_dma_alloc(x, 512, 512);
	if (!x->dcbaa)
		return -ENOMEM;
	ret = axh_setup_scratchpad(x);
	if (ret)
		return ret;
	axh_cache_flush(x->dcbaa, 512);
	dma = (u64)(uintptr_t)x->dcbaa;
	writel(AXH_LO32(dma), &x->op->dcbaap_lo);
	writel(AXH_HI32(dma), &x->op->dcbaap_hi);

	/* command ring */
	ret = axh_ring_init(x, &x->cmd_ring, AXH_CMD_RING_TRBS);
	if (ret)
		return ret;
	dma = (u64)(uintptr_t)x->cmd_ring.first | AXH_CRCR_RCS;
	writel(AXH_LO32(dma), &x->op->crcr_lo);
	writel(AXH_HI32(dma), &x->op->crcr_hi);

	/* event ring */
	ret = axh_evt_init(x);
	if (ret)
		return ret;

	x->ctrl_buf = axh_dma_alloc(x, AXH_CTRL_BUF_SIZE, AXH_CACHELINE);
	if (!x->ctrl_buf)
		return -ENOMEM;

	writel(0, &x->op->dnctrl);		/* no device notification events */
	if (readl(&x->cap->hcsparams2) & AXH_HCS2_ETE)
		writel(readl(&x->op->usbcmd) | AXH_CMD_ETE, &x->op->usbcmd);

	axh_parse_ext_caps(x);
	ret = axh_select_root_port(x);
	if (ret) {
		printf("controller %d has no USB%d root port\n", index, protocol);
		return ret;
	}

	ret = axh_ctrl_start(x);
	if (ret)
		return ret;
	axh_ports_power(x);
	return 0;
}

/* ------------------------------------------------------------------ */
/* Root port                                                           */
/* ------------------------------------------------------------------ */
static void axh_port_clear_changes(struct axh_ctrl *x)
{
	u32 *reg = axh_portsc(x, x->root_port);
	u32 v = readl(reg);

	writel(AXH_PORTSC_NEUTRAL(v) | AXH_PORTSC_CHANGE_MASK, reg);
}

int axh_wait_device(struct axh_ctrl *x, u32 timeout_ms)
{
	u32 *reg = axh_portsc(x, x->root_port);
	unsigned long start = get_timer(0);

	while (!(readl(reg) & AXH_PORTSC_CCS)) {
		if (get_timer(start) >= timeout_ms)
			return -ENODEV;
		mdelay(10);
	}
	printf("USB%d port change (portsc 0x%x)\n", x->protocol, readl(reg));
	axh_port_clear_changes(x);

	mdelay(100);				/* connect debounce (TATTDB) */
	if (!(readl(reg) & AXH_PORTSC_CCS))
		return -ENODEV;
	return 0;
}

static int axh_port_reset(struct axh_ctrl *x, u8 *speed)
{
	u32 *reg = axh_portsc(x, x->root_port);
	u32 v;
	int ret;

	v = readl(reg);
	/* SuperSpeed ports train the link on their own: skip reset if already in U0 */
	if (x->protocol == 3) {
		unsigned long start = get_timer(0);

		while (get_timer(start) < 500) {
			v = readl(reg);
			if ((v & AXH_PORTSC_PED) && AXH_PORTSC_PLS(v) == AXH_PLS_U0)
				goto done;
			mdelay(10);
		}
	}

	printf("port reset (portsc 0x%x)\n", v);
	writel(AXH_PORTSC_NEUTRAL(v) | AXH_PORTSC_PR, reg);
	ret = axh_wait_bits(reg, AXH_PORTSC_PRC, AXH_PORTSC_PRC, 1000);
	if (ret) {
		printf("port reset timeout\n");
		return ret;
	}
	axh_port_clear_changes(x);
	mdelay(10);				/* reset recovery (TRSTRCY) */
done:
	v = readl(reg);
	if (!(v & AXH_PORTSC_CCS))
		return -ENODEV;
	if (!(v & AXH_PORTSC_PED) || !AXH_PORTSC_SPEED(v)) {
		printf("port not enabled after reset (portsc 0x%x)\n", v);
		return -EIO;
	}
	*speed = AXH_PORTSC_SPEED(v);
	return 0;
}

/* ------------------------------------------------------------------ */
/* Slot / address / configure                                          */
/* ------------------------------------------------------------------ */
static u16 axh_default_ep0_mps(u8 speed)
{
	switch (speed) {
	case AXH_SPEED_SUPER:
	case AXH_SPEED_SUPER_PLUS:	return 512;
	case AXH_SPEED_HIGH:		return 64;
	case AXH_SPEED_FULL:		return 64;	/* corrected after 1st descriptor read */
	default:			return 8;
	}
}

static int axh_alloc_slot(struct axh_ctrl *x, struct axh_device *dev)
{
	struct axh_event ev;
	u64 dma;
	int ret;

	ret = axh_cmd(x, 0, 0, 0, AXH_TRB_TYPE(AXH_TRB_CMD_ENABLE_SLOT), &ev);
	if (ret)
		return ret;
	dev->slot_id = ev.slot;
	if (!dev->slot_id || dev->slot_id > x->max_slots) {
		printf("invalid slot id %d\n", dev->slot_id);
		return -EINVAL;
	}
	printf("slot %d enabled\n", dev->slot_id);

	dev->out_ctx = axh_dma_alloc(x, 4096, 4096);
	dev->in_ctx = axh_dma_alloc(x, 4096, 4096);
	if (!dev->out_ctx || !dev->in_ctx)
		return -ENOMEM;

	dma = (u64)(uintptr_t)dev->out_ctx;
	x->dcbaa[dev->slot_id] = cpu_to_le64(dma);
	axh_cache_flush(&x->dcbaa[dev->slot_id], sizeof(u64));

	return axh_ring_init(x, &dev->ep[1].ring, AXH_XFER_RING_TRBS);
}

static int axh_address_device(struct axh_ctrl *x, struct axh_device *dev)
{
	struct axh_input_ctrl_ctx *ctrl = axh_in_ctrl(dev->in_ctx);
	struct axh_slot_ctx *slot = axh_in_slot(x, dev->in_ctx);
	struct axh_ep_ctx *ep0 = axh_in_ep(x, dev->in_ctx, 1);
	u64 deq = (u64)(uintptr_t)dev->ep[1].ring.first | dev->ep[1].ring.cycle;
	u64 dma = (u64)(uintptr_t)dev->in_ctx;
	int ret;

	memset(dev->in_ctx, 0, 4096);
	AXH_W(ctrl->add, BIT(0) | BIT(1));	/* slot + EP0 */

	AXH_W(slot->info1, AXH_SLOT_ROUTE(dev->route) | AXH_SLOT_SPEED(dev->speed) |
			   AXH_SLOT_LAST_CTX(1));
	AXH_W(slot->info2, AXH_SLOT_ROOT_PORT(dev->root_port + 1));

	AXH_W(ep0->info2, AXH_EP_CERR(3) | AXH_EP_TYPE(AXH_EP_TYPE_CTRL) |
			  AXH_EP_MPS(dev->ep0_mps));
	AXH_W(ep0->deq_lo, AXH_LO32(deq));
	AXH_W(ep0->deq_hi, AXH_HI32(deq));
	AXH_W(ep0->info3, AXH_EP_AVG_TRB_LEN(8));
	axh_cache_flush(dev->in_ctx, 4096);

	ret = axh_cmd(x, AXH_LO32(dma), AXH_HI32(dma), 0,
		      AXH_TRB_TYPE(AXH_TRB_CMD_ADDR_DEV) | AXH_TRB_SLOT(dev->slot_id), NULL);
	if (ret)
		return ret;

	axh_cache_inval(dev->out_ctx, 4096);
	dev->addr = AXH_R(axh_out_slot(dev->out_ctx)->info4) & AXH_SLOT_ADDR_MASK;
	printf("device_addr: %d, slot_id: %d\n", dev->addr, dev->slot_id);
	return 0;
}

static int axh_update_ep0_mps(struct axh_ctrl *x, struct axh_device *dev, u16 mps)
{
	struct axh_input_ctrl_ctx *ctrl = axh_in_ctrl(dev->in_ctx);
	struct axh_ep_ctx *in = axh_in_ep(x, dev->in_ctx, 1);
	struct axh_ep_ctx *out = axh_out_ep(x, dev->out_ctx, 1);
	u64 dma = (u64)(uintptr_t)dev->in_ctx;
	u32 info2;

	axh_cache_inval(dev->out_ctx, 4096);
	memset(dev->in_ctx, 0, 4096);
	memcpy(in, out, sizeof(*in));
	info2 = AXH_R(in->info2) & 0xffffU;
	AXH_W(in->info2, info2 | AXH_EP_MPS(mps));
	AXH_W(ctrl->add, BIT(1));
	axh_cache_flush(dev->in_ctx, 4096);

	return axh_cmd(x, AXH_LO32(dma), AXH_HI32(dma), 0,
		       AXH_TRB_TYPE(AXH_TRB_CMD_EVAL_CTX) | AXH_TRB_SLOT(dev->slot_id), NULL);
}

static u32 axh_log2(u32 v)
{
	u32 r = 0;

	while (v >>= 1)
		r++;
	return r;
}

/* xHCI endpoint context "Interval" (exponent, in 125 us units) */
static u32 axh_ep_interval(u8 speed, u8 type, u8 binterval)
{
	u32 i;

	if (type == AXU_EP_BULK || type == AXU_EP_CTRL)
		return 0;
	if (!binterval)
		binterval = 1;

	if (speed >= AXH_SPEED_HIGH) {
		i = binterval - 1;
		if (i > 15)
			i = 15;
	} else if (type == AXU_EP_ISOC) {
		i = binterval + 2;		/* frames -> 2^(n-1) * 8 uframes */
	} else {
		i = axh_log2(binterval) + 3;	/* FS/LS interrupt: ms -> uframes */
		if (i < 3)
			i = 3;
		if (i > 10)
			i = 10;
	}
	return i;
}

static void axh_fill_ep_ctx(struct axh_device *dev, struct axh_ep *ep,
			    struct axh_ep_ctx *c)
{
	u8 type = ep->desc.bmAttributes & 3;
	int in = !!(ep->desc.bEndpointAddress & 0x80);
	u16 wmps = le16_to_cpu(ep->desc.wMaxPacketSize);
	u32 mps = wmps & 0x7ff;
	u32 burst = 0, mult = 0, esit = 0, avg, ept;
	u64 deq = (u64)(uintptr_t)ep->ring.first | ep->ring.cycle;

	if (dev->speed >= AXH_SPEED_SUPER) {
		burst = ep->ss.bMaxBurst;
		if (type == AXU_EP_ISOC)
			mult = ep->ss.bmAttributes & 3;
		if (type == AXU_EP_ISOC || type == AXU_EP_INT) {
			esit = le16_to_cpu(ep->ss.wBytesPerInterval);
			if (ep->has_ssp && (ep->ss.bmAttributes & 0x80))
				esit = le32_to_cpu(ep->ssp.dwBytesPerInterval);
		}
	} else if (dev->speed == AXH_SPEED_HIGH) {
		burst = (wmps >> 11) & 3;	/* additional transactions per uframe */
		if (type == AXU_EP_ISOC || type == AXU_EP_INT)
			esit = mps * (burst + 1);
	} else if (type == AXU_EP_ISOC || type == AXU_EP_INT) {
		esit = mps;
	}

	if (type == AXU_EP_CTRL) {
		ept = AXH_EP_TYPE_CTRL;
		avg = 8;
	} else {
		ept = type + (in ? 4 : 0);
		avg = (type == AXU_EP_BULK) ? 3072 : esit;
	}

	memset(c, 0, sizeof(*c));
	AXH_W(c->info1, AXH_EP_MULT(mult) |
			AXH_EP_INTERVAL(axh_ep_interval(dev->speed, type, ep->desc.bInterval)) |
			AXH_EP_ESIT_HI(esit));
	AXH_W(c->info2, AXH_EP_CERR(type == AXU_EP_ISOC ? 0 : 3) | AXH_EP_TYPE(ept) |
			AXH_EP_MAX_BURST(burst) | AXH_EP_MPS(mps));
	AXH_W(c->deq_lo, AXH_LO32(deq));
	AXH_W(c->deq_hi, AXH_HI32(deq));
	AXH_W(c->info3, AXH_EP_AVG_TRB_LEN(avg) | AXH_EP_ESIT_LO(esit));
}

static int axh_configure_eps(struct axh_ctrl *x, struct axh_device *dev)
{
	struct axh_input_ctrl_ctx *ctrl = axh_in_ctrl(dev->in_ctx);
	struct axh_slot_ctx *islot = axh_in_slot(x, dev->in_ctx);
	struct axh_slot_ctx *oslot = axh_out_slot(dev->out_ctx);
	u64 dma = (u64)(uintptr_t)dev->in_ctx;
	u32 add = BIT(0);
	u8 dci, max_dci = 1;
	int ret;

	axh_cache_inval(dev->out_ctx, 4096);
	memset(dev->in_ctx, 0, 4096);
	memcpy(islot, oslot, sizeof(*islot));

	for (dci = 2; dci < AXH_MAX_EPS; dci++) {
		struct axh_ep *ep = &dev->ep[dci];

		if (!ep->valid)
			continue;
		ret = axh_ring_init(x, &ep->ring, AXH_XFER_RING_TRBS);
		if (ret)
			return ret;
		axh_fill_ep_ctx(dev, ep, axh_in_ep(x, dev->in_ctx, dci));
		add |= BIT(dci);
		max_dci = dci;
		printf("ep_idx=%d type=%d mps=%d\n", dci, ep->desc.bmAttributes & 3,
			    le16_to_cpu(ep->desc.wMaxPacketSize) & 0x7ff);
	}

	AXH_W(islot->info1, (AXH_R(islot->info1) & ~AXH_SLOT_LAST_CTX_MASK) |
			    AXH_SLOT_LAST_CTX(max_dci));
	AXH_W(ctrl->add, add);
	axh_cache_flush(dev->in_ctx, 4096);

	return axh_cmd(x, AXH_LO32(dma), AXH_HI32(dma), 0,
		       AXH_TRB_TYPE(AXH_TRB_CMD_CONFIG_EP) | AXH_TRB_SLOT(dev->slot_id), NULL);
}

/* ------------------------------------------------------------------ */
/* Descriptors                                                         */
/* ------------------------------------------------------------------ */
static int axh_get_desc(struct axh_device *dev, u8 type, u8 idx, u16 langid,
			void *buf, u16 len)
{
	return axh_control_msg(dev, AXU_DIR_IN, AXU_REQ_GET_DESCRIPTOR,
			       ((u16)type << 8) | idx, langid, buf, len);
}

static void axh_get_string(struct axh_device *dev, u8 idx, char *out, size_t max)
{
	u8 buf[255];
	u32 i, n;

	out[0] = '\0';
	if (!idx)
		return;
	if (axh_get_desc(dev, AXU_DT_STRING, idx, dev->langid, buf, sizeof(buf)))
		return;
	if (buf[1] != AXU_DT_STRING || buf[0] < 2)
		return;

	n = (buf[0] - 2) / 2;
	for (i = 0; i < n && i < max - 1; i++) {
		u16 c = buf[2 + 2 * i] | (buf[3 + 2 * i] << 8);

		out[i] = (c < 0x80 && c >= 0x20) ? (char)c : '?';
	}
	out[i] = '\0';
}

/* Walk the configuration descriptor, select one interface, collect its endpoints */
static int axh_parse_config(struct axh_device *dev, const u8 *buf, u16 total)
{
	const u8 *p, *end = buf + total;
	int sel = -1, in_sel = 0, last_dci = 0;

	/* pass 1: prefer a Mass Storage BOT interface, else the first interface */
	for (p = buf + buf[0]; p + 2 <= end && p[0] >= 2; p += p[0]) {
		const struct axu_interface_desc *i;

		if (p + p[0] > end)
			break;
		if (p[1] != AXU_DT_INTERFACE || p[0] < sizeof(*i))
			continue;
		i = (const struct axu_interface_desc *)p;
		if (i->bAlternateSetting)
			continue;
		if (sel < 0)
			sel = i->bInterfaceNumber;
		if (i->bInterfaceClass == AXU_CLASS_MSC &&
		    i->bInterfaceSubClass == AXU_SUBCLASS_SCSI &&
		    i->bInterfaceProtocol == AXU_PROTO_BOT) {
			sel = i->bInterfaceNumber;
			break;
		}
	}
	if (sel < 0)
		return -ENODEV;

	/* pass 2: copy the interface and its endpoints (+ SS companions) */
	for (p = buf + buf[0]; p + 2 <= end && p[0] >= 2; p += p[0]) {
		if (p + p[0] > end)
			break;
		switch (p[1]) {
		case AXU_DT_INTERFACE: {
			const struct axu_interface_desc *i = (const void *)p;

			in_sel = (i->bInterfaceNumber == sel && !i->bAlternateSetting);
			if (in_sel)
				memcpy(&dev->idesc, i, sizeof(dev->idesc));
			last_dci = 0;
			break;
		}
		case AXU_DT_ENDPOINT:
			if (in_sel && p[0] >= sizeof(struct axu_endpoint_desc)) {
				const struct axu_endpoint_desc *e = (const void *)p;
				int dci = (e->bEndpointAddress & 0xf) * 2 +
					  ((e->bEndpointAddress & 0x80) ? 1 : 0);

				if (dci < 2 || dci >= AXH_MAX_EPS)
					break;
				memcpy(&dev->ep[dci].desc, e, sizeof(*e));
				dev->ep[dci].valid = 1;
				last_dci = dci;
			}
			break;
		case AXU_DT_SS_EP_COMP:
			if (in_sel && last_dci && p[0] >= sizeof(struct axu_ss_comp_desc)) {
				memcpy(&dev->ep[last_dci].ss, p, sizeof(struct axu_ss_comp_desc));
				dev->ep[last_dci].has_ss = 1;
			}
			break;
		case AXU_DT_SSP_ISOC_COMP:
			if (in_sel && last_dci && p[0] >= sizeof(struct axu_ssp_isoc_comp_desc)) {
				memcpy(&dev->ep[last_dci].ssp, p,
				       sizeof(struct axu_ssp_isoc_comp_desc));
				dev->ep[last_dci].has_ssp = 1;
			}
			break;
		default:
			break;
		}
	}
	return 0;
}

static const char *axh_class_str(const struct axu_interface_desc *i)
{
	switch (i->bInterfaceClass) {
	case 0x01: return "Audio";
	case 0x02: return "Communication";
	case 0x03: return "HID";
	case 0x07: return "Printer";
	case 0x08: return "Mass Storage";
	case 0x09: return "Hub";
	case 0x0a: return "CDC data";
	case 0x0b: return "Smart card";
	case 0xff: return "Vendor specific";
	default:   return "Other";
	}
}

/* ------------------------------------------------------------------ */
/* Enumeration                                                         */
/* ------------------------------------------------------------------ */
int axh_enumerate(struct axh_ctrl *x)
{
	struct axh_device *dev = &x->dev;
	u8 speed = 0, bmps;
	u16 total;
	int ret;

	ret = axh_port_reset(x, &speed);
	if (ret)
		return ret;

	memset(dev, 0, sizeof(*dev));
	dev->ctrl = x;
	dev->speed = speed;
	dev->root_port = x->root_port;
	dev->ep0_mps = axh_default_ep0_mps(speed);
	dev->ep[1].valid = 1;
	printf("USB device speed: %s\n", axh_speed_str(speed));

	ret = axh_alloc_slot(x, dev);
	if (ret)
		return ret;
	ret = axh_address_device(x, dev);
	if (ret)
		return ret;
	mdelay(5);				/* SET_ADDRESS recovery */

	/* first 8 bytes of the device descriptor give bMaxPacketSize0 */
	ret = axh_get_desc(dev, AXU_DT_DEVICE, 0, 0, dev->cfg_raw, 8);
	if (ret)
		return ret;
	bmps = dev->cfg_raw[7];
	{
		u16 mps0 = (speed >= AXH_SPEED_SUPER) ? (1U << bmps) : bmps;

		if (mps0 && mps0 != dev->ep0_mps) {
			printf("EP0 max packet %d -> %d\n", dev->ep0_mps, mps0);
			dev->ep0_mps = mps0;
			ret = axh_update_ep0_mps(x, dev, mps0);
			if (ret)
				return ret;
		}
	}

	ret = axh_get_desc(dev, AXU_DT_DEVICE, 0, 0, dev->cfg_raw, sizeof(dev->ddesc));
	if (ret)
		return ret;
	memcpy(&dev->ddesc, dev->cfg_raw, sizeof(dev->ddesc));

	/* configuration descriptor: header first, then the full blob */
	ret = axh_get_desc(dev, AXU_DT_CONFIG, 0, 0, dev->cfg_raw, sizeof(dev->cdesc));
	if (ret)
		return ret;
	total = dev->cfg_raw[2] | (dev->cfg_raw[3] << 8);
	if (total < sizeof(dev->cdesc) || total > AXH_CTRL_BUF_SIZE) {
		printf("bad wTotalLength %d\n", total);
		return -EPROTO;
	}
	ret = axh_get_desc(dev, AXU_DT_CONFIG, 0, 0, dev->cfg_raw, total);
	if (ret)
		return ret;
	memcpy(&dev->cdesc, dev->cfg_raw, sizeof(dev->cdesc));
	ret = axh_parse_config(dev, dev->cfg_raw, total);
	if (ret) {
		printf("no usable interface in configuration\n");
		return ret;
	}

	/* strings (langid first, fall back to en-US) */
	dev->langid = 0x0409;
	{
		u8 lb[4];

		if (!axh_get_desc(dev, AXU_DT_STRING, 0, 0, lb, sizeof(lb)) &&
		    lb[0] >= 4 && lb[1] == AXU_DT_STRING)
			dev->langid = lb[2] | (lb[3] << 8);
	}
	axh_get_string(dev, dev->ddesc.iManufacturer, dev->manufacturer, sizeof(dev->manufacturer));
	axh_get_string(dev, dev->ddesc.iProduct, dev->product, sizeof(dev->product));
	axh_get_string(dev, dev->ddesc.iSerialNumber, dev->serial, sizeof(dev->serial));

	printf("VID:PID %04x:%04x  class %02x/%02x/%02x (%s)\n",
		    le16_to_cpu(dev->ddesc.idVendor), le16_to_cpu(dev->ddesc.idProduct),
		    dev->idesc.bInterfaceClass, dev->idesc.bInterfaceSubClass,
		    dev->idesc.bInterfaceProtocol, axh_class_str(&dev->idesc));
	printf("Vendor: %s  Product: %s  Serial: %s\n",
		    dev->manufacturer, dev->product, dev->serial);

	if (dev->ddesc.bDeviceClass == 0x09) {
		printf("USB hubs are not supported by this driver\n");
		return -ENOSYS;
	}

	if (AXH_CFGEP_BEFORE_SETCONFIG) {
		ret = axh_configure_eps(x, dev);
		if (ret)
			return ret;
	}
	ret = axh_control_msg(dev, 0x00, AXU_REQ_SET_CONFIG,
			      dev->cdesc.bConfigurationValue, 0, NULL, 0);
	if (ret)
		return ret;
	if (!AXH_CFGEP_BEFORE_SETCONFIG) {
		ret = axh_configure_eps(x, dev);
		if (ret)
			return ret;
	}

	if (dev->idesc.bInterfaceClass == AXU_CLASS_MSC)
		ret = axh_msc_probe(dev);

	return ret;
}
