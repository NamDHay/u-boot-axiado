/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __XHCI_H
#define __XHCI_H

#include <dm.h>
#include <malloc.h>
#include <cpu_func.h>
#include <errno.h>
#include <asm/cache.h>
#include <linux/delay.h>
#include "ax_diag.h"
#include "ax_xhci_pci.h"
#include "xhci_hw.h"
#include "xhci_regs.h"

#define XHCI_MAX_SLOTS		32
#define XHCI_MAX_DEV		4
#define XHCI_RING_TRBS		256
#define XHCI_BOUNCE_SZ		65536	/* 64KB aligned, one TRB never crosses 64KB */
#define XHCI_CMD_TIMEOUT_MS	2000
#define XHCI_CTRL_TIMEOUT_MS	3000
#define XHCI_XFER_TIMEOUT_MS	10000

/*
 * Fixed DMA pool in DDR (xHCI rings, contexts, bounce buffer live here).
 * Must be 64KB aligned, AX_XHCI_DMA_POOL_SIZE bytes (default 512KB) long, and not
 * overlap any buffer used with "ax_usb read/write" nor U-Boot itself.
 */
#ifndef AX_XHCI_DMA_POOL_BASE
#define AX_XHCI_DMA_POOL_BASE	(AX3000_MEM_BASE_ADRS_DDR_LO + 0x10000000UL)
#endif

/* DMA alignment: at least 64 (xHCI) and at least the cache line */
#define XALIGN	(ARCH_DMA_MINALIGN > 64 ? ARCH_DMA_MINALIGN : 64)

/* NOTE: TRB / context words are little-endian; this code assumes an LE CPU. */
struct xhci_trb {
	u32 p_lo;
	u32 p_hi;
	u32 status;
	u32 ctrl;
};

struct xhci_ring {
	struct xhci_trb *trbs;
	u32 n;		/* total TRBs incl. link */
	u32 enq;
	u32 pcs;	/* producer cycle state */
};

struct xhci_udev {
	bool present;
	bool is_msc;
	u8 slot;
	u8 port;
	u8 speed;

	void *in_ctx;
	void *out_ctx;
	struct xhci_ring *ep0;

	/* mass-storage (bulk-only) */
	u8 iface;
	u8 in_ep, out_ep;		/* endpoint numbers (no direction bit) */
	u8 dci_in, dci_out;
	u16 in_mps, out_mps;
	u8 in_burst, out_burst;
	struct xhci_ring *ring_in, *ring_out;
	u32 tag;
	u64 blocks;
	u32 blksz;
	char vendor[9];
	char product[17];
};

struct xhci {
	struct xhci_hw hw;		/* register access backend */
	struct ax_xhci_pci pci;		/* used by the PCIe backend only */

	u32 max_slots, max_ports, csz;	/* csz = context size 32/64 */
	u64 *dcbaa;
	struct xhci_ring *cmd;

	struct xhci_trb *evt;
	void *erst;
	u32 evt_n, evt_deq, evt_ccs;

	u8 *ctrl_buf;			/* 512B DMA buffer for control transfers */
	u8 *bounce;			/* 64KB DMA bounce buffer for bulk */

	struct xhci_udev udev[XHCI_MAX_DEV];
};

/* ---- register helpers ---- */
static inline u32 xr(struct xhci *x, u32 off) { return xhci_hw_rd32(&x->hw, off); }
static inline void xw(struct xhci *x, u32 off, u32 v) { xhci_hw_wr32(&x->hw, off, v); }
#define XOP(x, r)	((x)->hw.op_off + (r))
#define XRT(x, r)	((x)->hw.rt_off + RT_IR0 + (r))

/* ---- cache / DMA helpers ---- */
static inline void xhci_clean(void *p, size_t len)
{
	unsigned long s = (unsigned long)p & ~((unsigned long)XALIGN - 1);
	unsigned long e = ALIGN((unsigned long)p + len, XALIGN);

	flush_dcache_range(s, e);
}

static inline void xhci_inval(void *p, size_t len)
{
	unsigned long s = (unsigned long)p & ~((unsigned long)XALIGN - 1);
	unsigned long e = ALIGN((unsigned long)p + len, XALIGN);

	invalidate_dcache_range(s, e);
}

/*
 * All DMA memory comes from one pool that is reset on every xhci_init(), so
 * stop/init cycles do not leak. By default the pool is one 64KB-aligned heap
 * block allocated once; define AX_XHCI_DMA_POOL_BASE (physical address of RAM the
 * controller can reach) to use a fixed region instead. AX_XHCI_DMA_POOL_SIZE
 * overrides the default 512KB.
 */
void *xhci_pool_alloc(size_t size, size_t align);
void  xhci_pool_reset(void);

static inline void *xhci_zalloc(size_t size, size_t align)
{
	void *p;

	if (align < XALIGN)
		align = XALIGN;
	size = ALIGN(size, XALIGN);
	p = xhci_pool_alloc(size, align);
	if (p) {
		memset(p, 0, size);
		xhci_clean(p, size);
	}
	return p;
}

/* ---- xhci_ring.c ---- */
struct xhci_ring *xhci_ring_alloc(u32 ntrbs);
struct xhci_trb *xhci_ring_enq(struct xhci_ring *r, u64 ptr, u32 status, u32 ctrl);
void xhci_ring_db(struct xhci *x, u32 slot, u32 target);
int  xhci_event_poll(struct xhci *x, struct xhci_trb *ev, u32 timeout_ms);
void xhci_dump_state(struct xhci *x, const char *why);
int  xhci_cmd(struct xhci *x, u64 ptr, u32 status, u32 ctrl, struct xhci_trb *ev_out);
int  xhci_xfer_wait(struct xhci *x, u32 slot, u32 dci, u32 timeout_ms,
		    struct xhci_trb *ev);

/* ---- xhci.c ---- */
int  xhci_init(struct xhci *x);
void xhci_stop(struct xhci *x);
int  xhci_scan_ports(struct xhci *x);
int  xhci_ctrl_msg(struct xhci *x, struct xhci_udev *d, u8 rt, u8 req,
		   u16 val, u16 idx, void *buf, u16 len);
int  xhci_bulk(struct xhci *x, struct xhci_udev *d, bool in, void *buf,
	       u32 len, u32 *actual);
int  xhci_clear_halt(struct xhci *x, struct xhci_udev *d, u8 ep_addr);

/* ---- xhci_msc.c ---- */
int  xhci_msc_probe(struct xhci *x, struct xhci_udev *d);
int  xhci_msc_rw(struct xhci *x, struct xhci_udev *d, bool write, u64 lba,
		 u32 count, void *buf);
int  xhci_msc_erase(struct xhci *x, struct xhci_udev *d, u64 lba, u32 count);

#endif
