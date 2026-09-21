// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __XHCI_H
#define __XHCI_H

#include <linux/types.h>
#include <linux/errno.h>
#include <asm/io.h>
#if defined(__has_include)
#if __has_include(<cpu_func.h>)
#include <cpu_func.h>
#endif
#endif
#include "xhci_regs.h"

/* ------------------------------------------------------------------ */
/* Build-time configuration                                            */
/* ------------------------------------------------------------------ */
/* Non-cacheable-friendly DMA carve-out (same region the baremetal code used) */
#ifndef AX_XHCI_MEM_BASE
#define AX_XHCI_MEM_BASE	0x90000000UL
#endif
#ifndef AX_XHCI_MEM_SIZE
#define AX_XHCI_MEM_SIZE	0x00200000UL
#endif

/* controller n (0..5) MMIO base: 0..3 = USB2 only, 4..5 = USB3 (+USB2 side) */
#ifndef AXH_MMIO_BASE0
#define AXH_MMIO_BASE0		0x41000000UL
#endif
#define AXH_MMIO_STRIDE		0x100000UL
#define AXH_MMIO_BASE(i)	(AXH_MMIO_BASE0 + (i) * AXH_MMIO_STRIDE)

/* 0: SET_CONFIGURATION then Configure Endpoint (baremetal order, proven on HW)
 * 1: Configure Endpoint then SET_CONFIGURATION (order used by Linux/U-Boot) */
#ifndef AXH_CFGEP_BEFORE_SETCONFIG
#define AXH_CFGEP_BEFORE_SETCONFIG 0
#endif

#define AXH_MAX_SLOTS		8
#define AXH_MAX_EPS		32	/* indexed by DCI, 1 = EP0 */
#define AXH_CMD_RING_TRBS	64
#define AXH_EVT_RING_TRBS	256
#define AXH_XFER_RING_TRBS	256
#define AXH_CTRL_BUF_SIZE	1024
#define AXH_TRB_MAX_LEN		0x10000U	/* a TRB must not cross 64 KiB */

#define AXH_CMD_TIMEOUT_MS	1000
#define AXH_CTRL_TIMEOUT_MS	2000
#define AXH_BULK_TIMEOUT_MS	5000
#define AXH_CONNECT_TIMEOUT_MS	2000
#define AXH_HALT_TIMEOUT_MS	1000

#define AXH_MIN(a, b)		((a) < (b) ? (a) : (b))

/* ------------------------------------------------------------------ */
/* Cache / barrier helpers                                             */
/* ------------------------------------------------------------------ */
#ifndef AXH_CACHELINE
#define AXH_CACHELINE		64
#endif

#ifdef CONFIG_SYS_DCACHE_OFF
static inline void axh_cache_flush(const void *p, size_t n) { (void)p; (void)n; }
static inline void axh_cache_inval(const void *p, size_t n) { (void)p; (void)n; }
#else
static inline void axh_cache_flush(const void *p, size_t n)
{
	uintptr_t s = (uintptr_t)p & ~(uintptr_t)(AXH_CACHELINE - 1);
	uintptr_t e = ((uintptr_t)p + n + AXH_CACHELINE - 1) &
		      ~(uintptr_t)(AXH_CACHELINE - 1);
	flush_dcache_range(s, e);
}

static inline void axh_cache_inval(const void *p, size_t n)
{
	uintptr_t s = (uintptr_t)p & ~(uintptr_t)(AXH_CACHELINE - 1);
	uintptr_t e = ((uintptr_t)p + n + AXH_CACHELINE - 1) &
		      ~(uintptr_t)(AXH_CACHELINE - 1);
	invalidate_dcache_range(s, e);
}
#endif

#if defined(__aarch64__)
#define axh_wmb()	__asm__ __volatile__("dsb sy" ::: "memory")
#elif defined(__arm__)
#define axh_wmb()	__asm__ __volatile__("dsb" ::: "memory")
#else
#define axh_wmb()	__asm__ __volatile__("" ::: "memory")
#endif

#define AXH_W(dst, v)		((dst) = cpu_to_le32(v))
#define AXH_R(src)		le32_to_cpu(src)

/* ------------------------------------------------------------------ */
/* USB descriptors / requests (prefixed to avoid clashing with usb.h)  */
/* ------------------------------------------------------------------ */
#define AXU_DIR_IN		0x80
#define AXU_REQ_GET_STATUS	0
#define AXU_REQ_CLEAR_FEATURE	1
#define AXU_REQ_GET_DESCRIPTOR	6
#define AXU_REQ_SET_CONFIG	9

#define AXU_DT_DEVICE		1
#define AXU_DT_CONFIG		2
#define AXU_DT_STRING		3
#define AXU_DT_INTERFACE	4
#define AXU_DT_ENDPOINT		5
#define AXU_DT_SS_EP_COMP	0x30
#define AXU_DT_SSP_ISOC_COMP	0x31

#define AXU_EP_CTRL		0
#define AXU_EP_ISOC		1
#define AXU_EP_BULK		2
#define AXU_EP_INT		3

#define AXU_CLASS_MSC		0x08
#define AXU_SUBCLASS_SCSI	0x06
#define AXU_PROTO_BOT		0x50

struct axu_device_desc {
	u8  bLength;
	u8  bDescriptorType;
	u16 bcdUSB;
	u8  bDeviceClass;
	u8  bDeviceSubClass;
	u8  bDeviceProtocol;
	u8  bMaxPacketSize0;
	u16 idVendor;
	u16 idProduct;
	u16 bcdDevice;
	u8  iManufacturer;
	u8  iProduct;
	u8  iSerialNumber;
	u8  bNumConfigurations;
} __attribute__((packed));

struct axu_config_desc {
	u8  bLength;
	u8  bDescriptorType;
	u16 wTotalLength;
	u8  bNumInterfaces;
	u8  bConfigurationValue;
	u8  iConfiguration;
	u8  bmAttributes;
	u8  bMaxPower;
} __attribute__((packed));

struct axu_interface_desc {
	u8 bLength;
	u8 bDescriptorType;
	u8 bInterfaceNumber;
	u8 bAlternateSetting;
	u8 bNumEndpoints;
	u8 bInterfaceClass;
	u8 bInterfaceSubClass;
	u8 bInterfaceProtocol;
	u8 iInterface;
} __attribute__((packed));

struct axu_endpoint_desc {
	u8  bLength;
	u8  bDescriptorType;
	u8  bEndpointAddress;
	u8  bmAttributes;
	u16 wMaxPacketSize;
	u8  bInterval;
} __attribute__((packed));

struct axu_ss_comp_desc {
	u8  bLength;
	u8  bDescriptorType;
	u8  bMaxBurst;
	u8  bmAttributes;
	u16 wBytesPerInterval;
} __attribute__((packed));

struct axu_ssp_isoc_comp_desc {
	u8  bLength;
	u8  bDescriptorType;
	u16 wReserved;
	u32 dwBytesPerInterval;
} __attribute__((packed));

/* ------------------------------------------------------------------ */
/* Driver objects                                                      */
/* ------------------------------------------------------------------ */
struct axh_ctrl;

struct axh_ring {
	struct axh_trb *first;
	struct axh_trb *enq;
	u32 num_trbs;		/* including the trailing link TRB */
	u8 cycle;		/* producer cycle state */
};

struct axh_erst_entry {
	u32 addr_lo;
	u32 addr_hi;
	u32 size;
	u32 rsvd;
};

struct axh_event_ring {
	struct axh_trb *first;
	struct axh_trb *deq;
	struct axh_erst_entry *erst;
	u32 num_trbs;
	u8 cycle;		/* consumer cycle state */
};

struct axh_event {
	u32 type;
	u32 code;
	u32 slot;
	u32 ep;			/* endpoint id (DCI) */
	u32 residue;		/* bytes not transferred */
	u64 trb_ptr;
	u32 dw0;
};

struct axh_ep {
	struct axu_endpoint_desc desc;
	struct axu_ss_comp_desc ss;
	struct axu_ssp_isoc_comp_desc ssp;
	u8 valid;
	u8 has_ss;
	u8 has_ssp;
	struct axh_ring ring;
};

struct axh_device {
	struct axh_ctrl *ctrl;
	u8 slot_id;
	u8 speed;
	u8 root_port;		/* 0-based root hub port index */
	u8 addr;
	u32 route;
	u16 ep0_mps;
	u16 langid;
	void *out_ctx;		/* device context (xHC owned) */
	void *in_ctx;		/* input context (SW owned) */
	struct axu_device_desc ddesc;
	struct axu_config_desc cdesc;
	struct axu_interface_desc idesc;
	struct axh_ep ep[AXH_MAX_EPS];	/* [1] = EP0, [n] = DCI n */
	char manufacturer[64];
	char product[64];
	char serial[64];
	u8 cfg_raw[AXH_CTRL_BUF_SIZE];
	void *priv;		/* class driver state (struct axh_msc) */
};

struct axh_ctrl {
	int index;		/* controller 0..5 */
	int protocol;		/* 2 or 3: which root port is used */
	struct axh_cap_regs *cap;
	struct axh_op_regs *op;
	struct axh_run_regs *run;
	u32 *db;
	u16 hci_version;
	u8 max_slots;
	u8 max_ports;
	u8 ctx_size;		/* 32 or 64 */
	u8 root_port;		/* 0-based */
	u32 page_size;
	struct {
		u8 major;
		u8 port_offset;	/* 1-based */
		u8 port_count;
	} proto[2];		/* [0] = USB2, [1] = USB3 */

	u8 *pool_base;
	u8 *pool_cur;
	u8 *pool_end;

	u64 *dcbaa;
	struct axh_ring cmd_ring;
	struct axh_event_ring evt;
	u8 *ctrl_buf;

	struct axh_device dev;
	int running;
};

/* Context accessors (input ctx: ctrl, slot, ep0(dci1)...; output ctx: slot, ep0...) */
static inline struct axh_input_ctrl_ctx *axh_in_ctrl(void *in)
{
	return (struct axh_input_ctrl_ctx *)in;
}
static inline struct axh_slot_ctx *axh_in_slot(struct axh_ctrl *x, void *in)
{
	return (struct axh_slot_ctx *)((u8 *)in + x->ctx_size);
}
static inline struct axh_ep_ctx *axh_in_ep(struct axh_ctrl *x, void *in, u8 dci)
{
	return (struct axh_ep_ctx *)((u8 *)in + x->ctx_size * (dci + 1));
}
static inline struct axh_slot_ctx *axh_out_slot(void *out)
{
	return (struct axh_slot_ctx *)out;
}
static inline struct axh_ep_ctx *axh_out_ep(struct axh_ctrl *x, void *out, u8 dci)
{
	return (struct axh_ep_ctx *)((u8 *)out + x->ctx_size * dci);
}

#define AXH_LO32(a)		((u32)((u64)(a) & 0xffffffffU))
#define AXH_HI32(a)		((u32)(((u64)(a) >> 16) >> 16))

/* ------------------------------------------------------------------ */
/* xhci_ring.c                                                         */
/* ------------------------------------------------------------------ */
void *axh_dma_alloc(struct axh_ctrl *x, size_t size, size_t align);
int axh_ring_init(struct axh_ctrl *x, struct axh_ring *r, u32 num_trbs);
struct axh_trb *axh_ring_queue(struct axh_ring *r, u32 d0, u32 d1, u32 d2, u32 d3);
int axh_evt_init(struct axh_ctrl *x);
void axh_evt_drain(struct axh_ctrl *x);
int axh_evt_wait(struct axh_ctrl *x, u32 type, u32 slot, u32 ep,
		 struct axh_event *ev, u32 timeout_ms);
int axh_cmd(struct axh_ctrl *x, u32 d0, u32 d1, u32 d2, u32 d3,
	    struct axh_event *ev);
int axh_ep_recover(struct axh_device *dev, u8 dci);
int axh_control_msg(struct axh_device *dev, u8 reqtype, u8 req, u16 value,
		    u16 index, void *data, u16 len);
int axh_bulk_xfer(struct axh_device *dev, u8 dci, void *buf, u32 len);
const char *axh_cc_str(u32 code);

/* ------------------------------------------------------------------ */
/* xhci.c                                                              */
/* ------------------------------------------------------------------ */
int axh_ctrl_init(struct axh_ctrl *x, int index, int protocol);
void axh_ctrl_stop(struct axh_ctrl *x);
int axh_wait_device(struct axh_ctrl *x, u32 timeout_ms);
int axh_enumerate(struct axh_ctrl *x);
const char *axh_speed_str(u8 speed);

/* ------------------------------------------------------------------ */
/* xhci_msc.c                                                          */
/* ------------------------------------------------------------------ */
struct axh_msc_cbw;
struct axh_msc_csw;

struct axh_msc {
	struct axh_device *dev;
	u8 iface;
	u8 in_dci;
	u8 out_dci;
	u8 max_lun;
	u8 lun;
	u32 tag;
	u32 blk_size;
	u64 blk_count;
	u32 chunk_blks;
	char vendor[9];
	char product[17];
	char rev[5];
	struct axh_msc_cbw *cbw;
	struct axh_msc_csw *csw;
	u8 *bounce;
};

int axh_msc_probe(struct axh_device *dev);
const struct axh_msc *axh_msc_get(struct axh_device *dev);
int axh_msc_read(struct axh_device *dev, u64 lba, u32 cnt, void *buf);
int axh_msc_write(struct axh_device *dev, u64 lba, u32 cnt, const void *buf);
int axh_msc_erase(struct axh_device *dev, u64 lba, u32 cnt);
int axh_msc_selftest(struct axh_device *dev, u64 lba, u32 cnt, u32 pattern);

#endif /* __XHCI_H */
