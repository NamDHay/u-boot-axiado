// SPDX-License-Identifier: (GPL-1.0+ OR MIT)
/*
 * "ax_usb_dev" U-Boot command: bring-up test for the bare-metal CRG UDC
 * backend (crg_udc_bm.c) through struct ax_usb_dev_ops.
 *
 *   ax_usb_dev init [base [hs|ss|fs]]
 *   ax_usb_dev start <addr> <size> [seconds]
 *   ax_usb_dev stop
 *   ax_usb_dev status
 *
 * The function is a USB Mass Storage device (Bulk-Only Transport, SCSI
 * transparent command set) whose single LUN is a RAM region [addr, addr +
 * size): on a Linux host it shows up as /dev/sdX, "dd if=file of=/dev/sdX"
 * puts the file at <addr>, "dd if=/dev/sdX" reads it back.
 *
 * Copyright (c) 2023-25 Axiado Corporation.
 */

#include <dm.h>
#include <command.h>
#include <console.h>
#include <cpu_func.h>
#include <malloc.h>
#include <mapmem.h>
#include <stdio.h>
#include <time.h>
#include <vsprintf.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/usb/ch9.h>
#include "ax_usb_dev.h"

#ifndef CONFIG_AX_USB_DEV_BASE
#define CONFIG_AX_USB_DEV_BASE	0x0
#endif

#define AX_USB_REG_SIZE		0x4000

#define MSC_VID			0x0525
#define MSC_PID			0xa4a5		/* same as Linux g_mass_storage */
#define MSC_EP_IN		(USB_DIR_IN | 1)
#define MSC_EP_OUT		1
#define MSC_SECTOR		512
#define MSC_CHUNK		65536		/* one TRB per chunk */
#define MSC_DESC_LEN		512

#define CBW_SIG			0x43425355	/* "USBC" */
#define CSW_SIG			0x53425355	/* "USBS" */
#define CSW_PASSED		0
#define CSW_FAILED		1

#define BOT_REQ_RESET		0xff
#define BOT_REQ_GET_MAX_LUN	0xfe

struct msc_cbw {
	__le32 sig;
	__le32 tag;
	__le32 data_len;
	u8 flags;
	u8 lun;
	u8 cb_len;
	u8 cb[16];
} __attribute__((packed));

struct msc_csw {
	__le32 sig;
	__le32 tag;
	__le32 residue;
	u8 status;
} __attribute__((packed));

enum msc_state { MSC_CBW, MSC_DATA_IN, MSC_DATA_OUT, MSC_CSW, MSC_WEDGED };

struct ax_test {
	struct ax_usb_dev *udev;
	enum usb_device_speed speed;
	u8 config;
	bool ep_en;

	u8 *desc_buf;			/* control IN data, DMA aligned */
	struct ax_usb_req in_req, out_req;
};

struct ax_msc {
	/* backing store */
	ulong addr;
	u64 sectors;
	u8 *ram;

	/* DMA buffers: all inside 4KB / 64KB aligned blocks (no 64KB crossing) */
	u8 *ctl;			/* cbw at +0, csw at +64 */
	u8 *bounce;			/* MSC_CHUNK, 64KB aligned */

	/* current command */
	enum msc_state state;
	u32 tag, data_len;
	bool dir_in;
	u8 cb[16];
	u32 bytes_left, sent;
	u8 *ram_ptr;
	bool from_ram, drain, zlp_done, fail;
	u8 sk, asc, ascq;

	/* statistics */
	u32 n_cmd, n_read, n_write, n_bad_cbw, err;
	u64 bytes_rd, bytes_wr;
	u8 last_op;
};

static struct ax_test t;
static struct ax_msc m;

#define m_cbw()	((struct msc_cbw *)m.ctl)
#define m_csw()	((struct msc_csw *)(m.ctl + 64))

/* ------------------------------ descriptors ---------------------------- */
static u16 bulk_mps(enum usb_device_speed speed)
{
	if (speed >= USB_SPEED_SUPER)
		return 1024;
	if (speed == USB_SPEED_HIGH)
		return 512;
	return 64;
}

static int build_device_desc(u8 *p)
{
	struct usb_device_descriptor d = {
		.bLength		= USB_DT_DEVICE_SIZE,
		.bDescriptorType	= USB_DT_DEVICE,
		.bcdUSB			= cpu_to_le16(t.speed >= USB_SPEED_SUPER ?
						      0x0300 : 0x0210),
		.bDeviceClass		= 0,	/* defined by the interface */
		.bMaxPacketSize0	= t.speed >= USB_SPEED_SUPER ? 9 : 64,
		.idVendor		= cpu_to_le16(MSC_VID),
		.idProduct		= cpu_to_le16(MSC_PID),
		.bcdDevice		= cpu_to_le16(0x0100),
		.iManufacturer		= 1,
		.iProduct		= 2,
		.iSerialNumber		= 3,
		.bNumConfigurations	= 1,
	};

	memcpy(p, &d, sizeof(d));
	return sizeof(d);
}

static void fill_ep_desc(struct usb_endpoint_descriptor *e, u8 addr)
{
	memset(e, 0, sizeof(*e));
	e->bLength = USB_DT_ENDPOINT_SIZE;
	e->bDescriptorType = USB_DT_ENDPOINT;
	e->bEndpointAddress = addr;
	e->bmAttributes = USB_ENDPOINT_XFER_BULK;
	e->wMaxPacketSize = cpu_to_le16(bulk_mps(t.speed));
	e->bInterval = 0;
}

static void fill_comp_desc(struct usb_ss_ep_comp_descriptor *c)
{
	memset(c, 0, sizeof(*c));
	c->bLength = 6;
	c->bDescriptorType = 0x30;	/* SS endpoint companion */
	c->bMaxBurst = 0;
}

static int build_config_desc(u8 *p)
{
	struct usb_config_descriptor cfg = {
		.bLength		= USB_DT_CONFIG_SIZE,
		.bDescriptorType	= USB_DT_CONFIG,
		.bNumInterfaces		= 1,
		.bConfigurationValue	= 1,
		.bmAttributes		= 0xc0,		/* self powered */
		.bMaxPower		= 1,
	};
	struct usb_interface_descriptor ifc = {
		.bLength		= USB_DT_INTERFACE_SIZE,
		.bDescriptorType	= USB_DT_INTERFACE,
		.bNumEndpoints		= 2,
		.bInterfaceClass	= USB_CLASS_MASS_STORAGE,
		.bInterfaceSubClass	= 0x06,		/* SCSI transparent */
		.bInterfaceProtocol	= 0x50,		/* bulk-only transport */
	};
	struct usb_endpoint_descriptor ep;
	struct usb_ss_ep_comp_descriptor comp;
	u8 *q = p + sizeof(cfg);

	memcpy(q, &ifc, sizeof(ifc));
	q += sizeof(ifc);

	fill_ep_desc(&ep, MSC_EP_IN);
	memcpy(q, &ep, USB_DT_ENDPOINT_SIZE);	/* sizeof(ep) is 9: audio ext. */
	q += USB_DT_ENDPOINT_SIZE;
	if (t.speed >= USB_SPEED_SUPER) {
		fill_comp_desc(&comp);
		memcpy(q, &comp, sizeof(comp));
		q += sizeof(comp);
	}

	fill_ep_desc(&ep, MSC_EP_OUT);
	memcpy(q, &ep, USB_DT_ENDPOINT_SIZE);
	q += USB_DT_ENDPOINT_SIZE;
	if (t.speed >= USB_SPEED_SUPER) {
		memcpy(q, &comp, sizeof(comp));
		q += sizeof(comp);
	}

	cfg.wTotalLength = cpu_to_le16(q - p);
	memcpy(p, &cfg, sizeof(cfg));
	return q - p;
}

static int build_qualifier_desc(u8 *p)
{
	/* only meaningful for USB2 speeds */
	static const u8 q[10] = { 10, USB_DT_DEVICE_QUALIFIER, 0x10, 0x02,
				  0x00, 0x00, 0x00, 64, 1, 0 };

	memcpy(p, q, sizeof(q));
	return sizeof(q);
}

static int build_bos_desc(u8 *p)
{
	static const u8 bos[] = {
		/* BOS header: total 22, 2 capabilities */
		5, USB_DT_BOS, 22, 0, 2,
		/* USB 2.0 extension: LPM */
		7, 0x10, 0x02, 0x02, 0x00, 0x00, 0x00,
		/* SuperSpeed USB: speeds HS|SS, full func at SS, U1 1us, U2 500us */
		10, 0x10, 0x03, 0x00, 0x0e, 0x00, 0x01, 0x01, 0xf4, 0x01,
	};

	memcpy(p, bos, sizeof(bos));
	return sizeof(bos);
}

static int build_string_desc(u8 *p, u8 idx)
{
	static const char * const strs[] = { NULL, "Axiado", "AX3000 USB RAM disk",
					     "0001" };
	const char *s;
	int i, n;

	if (idx == 0) {
		p[0] = 4;
		p[1] = USB_DT_STRING;
		p[2] = 0x09;
		p[3] = 0x04;		/* en-US */
		return 4;
	}
	if (idx >= ARRAY_SIZE(strs))
		return -EINVAL;

	s = strs[idx];
	n = strlen(s);
	p[0] = 2 + 2 * n;
	p[1] = USB_DT_STRING;
	for (i = 0; i < n; i++) {
		p[2 + 2 * i] = s[i];
		p[3 + 2 * i] = 0;
	}
	return p[0];
}

/* ------------------------ mass storage (BOT + SCSI) ---------------------- */
static void msc_cbw_done(struct ax_usb_dev *udev, u8 ep, struct ax_usb_req *req);
static void msc_in_done(struct ax_usb_dev *udev, u8 ep, struct ax_usb_req *req);
static void msc_out_done(struct ax_usb_dev *udev, u8 ep, struct ax_usb_req *req);
static void msc_csw_done(struct ax_usb_dev *udev, u8 ep, struct ax_usb_req *req);

static inline u32 get_be32(const u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static inline void put_be32(u8 *p, u32 v)
{
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >> 8;
	p[3] = v;
}

static bool msc_req_aborted(const struct ax_usb_req *req)
{
	if (!req->status)
		return false;
	if (req->status != -ESHUTDOWN && req->status != -ECONNRESET)
		m.err++;
	return true;
}

static void msc_queue_cbw(void)
{
	struct ax_usb_req *r = &t.out_req;

	m.state = MSC_CBW;
	r->buf = m.ctl;
	r->length = sizeof(struct msc_cbw);
	r->zero = false;
	r->complete = msc_cbw_done;
	if (t.udev->ops->ep_queue(t.udev, MSC_EP_OUT, r))
		m.err++;
}

static void msc_send_csw(u8 status)
{
	struct ax_usb_req *r = &t.in_req;
	struct msc_csw *csw = m_csw();

	csw->sig = cpu_to_le32(CSW_SIG);
	csw->tag = cpu_to_le32(m.tag);
	csw->residue = cpu_to_le32(m.data_len > m.sent ? m.data_len - m.sent : 0);
	csw->status = status;

	m.state = MSC_CSW;
	r->buf = csw;
	r->length = sizeof(*csw);
	r->zero = false;
	r->complete = msc_csw_done;
	if (t.udev->ops->ep_queue(t.udev, MSC_EP_IN, r))
		m.err++;
}

static void msc_csw_done(struct ax_usb_dev *udev, u8 ep, struct ax_usb_req *req)
{
	if (msc_req_aborted(req))
		return;
	msc_queue_cbw();
}

/* device -> host data stage */
static void msc_in_next(void)
{
	struct ax_usb_req *r = &t.in_req;
	u32 n;

	if (!m.bytes_left) {
		/* host expects data but we have none: send a ZLP first */
		if (!m.sent && m.data_len && !m.zlp_done) {
			m.zlp_done = true;
			r->buf = NULL;
			r->length = 0;
			r->zero = false;
			r->complete = msc_in_done;
			if (t.udev->ops->ep_queue(t.udev, MSC_EP_IN, r))
				m.err++;
			return;
		}
		msc_send_csw(m.fail ? CSW_FAILED : CSW_PASSED);
		return;
	}

	n = min_t(u32, m.bytes_left, MSC_CHUNK);
	if (m.from_ram)
		memcpy(m.bounce, m.ram_ptr, n);

	r->buf = m.bounce;
	r->length = n;
	/* short response: a ZLP terminates the data stage if needed */
	r->zero = (n == m.bytes_left) && (m.sent + n < m.data_len);
	r->complete = msc_in_done;
	if (t.udev->ops->ep_queue(t.udev, MSC_EP_IN, r))
		m.err++;
}

static void msc_in_done(struct ax_usb_dev *udev, u8 ep, struct ax_usb_req *req)
{
	if (msc_req_aborted(req))
		return;

	m.sent += req->actual;
	m.bytes_left -= min_t(u32, m.bytes_left, req->actual);
	if (m.from_ram) {
		m.ram_ptr += req->actual;
		m.bytes_rd += req->actual;
	}
	msc_in_next();
}

/* host -> device data stage */
static void msc_out_next(void)
{
	struct ax_usb_req *r = &t.out_req;
	u32 n;

	if (!m.bytes_left) {
		msc_send_csw(m.fail ? CSW_FAILED : CSW_PASSED);
		return;
	}

	n = min_t(u32, m.bytes_left, MSC_CHUNK);
	r->buf = m.bounce;
	r->length = n;
	r->zero = false;
	r->complete = msc_out_done;
	if (t.udev->ops->ep_queue(t.udev, MSC_EP_OUT, r))
		m.err++;
}

static void msc_out_done(struct ax_usb_dev *udev, u8 ep, struct ax_usb_req *req)
{
	if (msc_req_aborted(req))
		return;

	if (!m.drain) {
		memcpy(m.ram_ptr, m.bounce, req->actual);
		m.ram_ptr += req->actual;
		m.bytes_wr += req->actual;
	}
	m.sent += req->actual;

	if (req->actual < req->length) {
		/* host sent less than announced */
		m.fail = true;
		m.bytes_left = 0;
	} else {
		m.bytes_left -= req->actual;
	}
	msc_out_next();
}

static void msc_set_sense(u8 sk, u8 asc, u8 ascq)
{
	m.sk = sk;
	m.asc = asc;
	m.ascq = ascq;
	m.fail = true;
}

static void msc_start_data(void)
{
	if (m.dir_in) {
		m.state = MSC_DATA_IN;
		msc_in_next();
	} else if (m.bytes_left) {
		m.state = MSC_DATA_OUT;
		msc_out_next();
	} else {
		msc_send_csw(m.fail ? CSW_FAILED : CSW_PASSED);
	}
}

static void msc_do_scsi(void)
{
	const u8 *cb = m.cb;
	u8 *b = m.bounce;
	enum { K_NONE, K_SMALL, K_READ, K_WRITE } kind = K_NONE;
	u32 resp = 0, lba = 0, n = 0;

	m.sent = 0;
	m.bytes_left = 0;
	m.fail = false;
	m.zlp_done = false;
	m.from_ram = false;
	m.drain = false;
	m.n_cmd++;
	m.last_op = cb[0];

	/* sense data survives until the next command other than these two */
	if (cb[0] != 0x03 && cb[0] != 0x12)
		m.sk = m.asc = m.ascq = 0;

	switch (cb[0]) {
	case 0x00:		/* TEST UNIT READY */
	case 0x1b:		/* START STOP UNIT */
	case 0x1e:		/* PREVENT ALLOW MEDIUM REMOVAL */
	case 0x35:		/* SYNCHRONIZE CACHE(10) */
		break;

	case 0x03:		/* REQUEST SENSE */
		memset(b, 0, 18);
		b[0] = 0x70;
		b[2] = m.sk;
		b[7] = 10;
		b[12] = m.asc;
		b[13] = m.ascq;
		m.sk = m.asc = m.ascq = 0;
		resp = 18;
		kind = K_SMALL;
		break;

	case 0x12:		/* INQUIRY */
		if ((cb[1] & 1) || cb[2]) {
			msc_set_sense(5, 0x24, 0);	/* invalid field in CDB */
			break;
		}
		memset(b, 0, 36);
		b[0] = 0x00;			/* direct access block device */
		b[2] = 0x04;			/* SPC-2 */
		b[3] = 0x02;
		b[4] = 31;
		memcpy(b + 8, "Axiado  ", 8);
		memcpy(b + 16, "AX3000 RAM disk ", 16);
		memcpy(b + 32, "0001", 4);
		resp = 36;
		kind = K_SMALL;
		break;

	case 0x25:		/* READ CAPACITY(10) */
		put_be32(b, m.sectors > 0xffffffffULL ? 0xffffffff : m.sectors - 1);
		put_be32(b + 4, MSC_SECTOR);
		resp = 8;
		kind = K_SMALL;
		break;

	case 0x1a:		/* MODE SENSE(6): header only, write enabled */
		memset(b, 0, 4);
		b[0] = 3;
		resp = 4;
		kind = K_SMALL;
		break;

	case 0x5a:		/* MODE SENSE(10) */
		memset(b, 0, 8);
		b[1] = 6;
		resp = 8;
		kind = K_SMALL;
		break;

	case 0x28:		/* READ(10) */
	case 0x2a:		/* WRITE(10) */
		lba = get_be32(cb + 2);
		n = (cb[7] << 8) | cb[8];
		kind = cb[0] == 0x28 ? K_READ : K_WRITE;
		break;

	default:
		msc_set_sense(5, 0x20, 0);		/* invalid command opcode */
		break;
	}

	switch (kind) {
	case K_SMALL:
		if (m.dir_in)
			m.bytes_left = min_t(u32, resp, m.data_len);
		break;
	case K_READ:
	case K_WRITE:
		if ((kind == K_READ) != m.dir_in || m.data_len != n * MSC_SECTOR) {
			msc_set_sense(5, 0x24, 0);
		} else if ((u64)lba + n > m.sectors) {
			msc_set_sense(5, 0x21, 0);	/* LBA out of range */
		} else if (n) {
			m.ram_ptr = m.ram + (u64)lba * MSC_SECTOR;
			m.bytes_left = m.data_len;
			m.from_ram = kind == K_READ;
			if (kind == K_READ)
				m.n_read++;
			else
				m.n_write++;
		}
		break;
	default:
		break;
	}

	/* host->device data we do not want: swallow it, then fail/pass */
	if (!m.dir_in && m.data_len && !m.bytes_left) {
		m.drain = true;
		m.bytes_left = m.data_len;
	}

	msc_start_data();
}

static void msc_cbw_done(struct ax_usb_dev *udev, u8 ep, struct ax_usb_req *req)
{
	struct msc_cbw *cbw = m_cbw();

	if (msc_req_aborted(req))
		return;

	if (req->actual != sizeof(*cbw) || le32_to_cpu(cbw->sig) != CBW_SIG ||
	    cbw->lun != 0 || !cbw->cb_len || cbw->cb_len > 16 ||
	    (cbw->flags & 0x7f)) {
		/* invalid CBW: stall both pipes until reset recovery */
		m.n_bad_cbw++;
		m.state = MSC_WEDGED;
		t.udev->ops->ep_halt(t.udev, MSC_EP_IN, 1);
		t.udev->ops->ep_halt(t.udev, MSC_EP_OUT, 1);
		return;
	}

	m.tag = le32_to_cpu(cbw->tag);
	m.data_len = le32_to_cpu(cbw->data_len);
	m.dir_in = !!(cbw->flags & 0x80);
	memcpy(m.cb, cbw->cb, 16);
	msc_do_scsi();
}

static void msc_disable_eps(void)
{
	if (!t.ep_en)
		return;
	t.udev->ops->ep_disable(t.udev, MSC_EP_IN);
	t.udev->ops->ep_disable(t.udev, MSC_EP_OUT);
	t.ep_en = false;
}

static int msc_enable_eps(void)
{
	struct usb_endpoint_descriptor ep;
	struct usb_ss_ep_comp_descriptor comp;
	const struct ax_usb_dev_ops *ops = t.udev->ops;
	int ret;

	fill_comp_desc(&comp);

	fill_ep_desc(&ep, MSC_EP_IN);
	ret = ops->ep_enable(t.udev, &ep, &comp);
	if (ret)
		return ret;
	fill_ep_desc(&ep, MSC_EP_OUT);
	ret = ops->ep_enable(t.udev, &ep, &comp);
	if (ret) {
		ops->ep_disable(t.udev, MSC_EP_IN);
		return ret;
	}
	t.ep_en = true;

	m.sk = m.asc = m.ascq = 0;
	msc_queue_cbw();
	return 0;
}

/* BOT "reset recovery": cheapest correct way is to restart both endpoints */
static void msc_reset(void)
{
	msc_disable_eps();
	if (t.config)
		msc_enable_eps();
}

/* ------------------------------ func ops -------------------------------- */
static int test_setup(struct ax_usb_dev *udev,
		      const struct usb_ctrlrequest *c, void **buf)
{
	u16 value = le16_to_cpu(c->wValue);
	u8 type = value >> 8, idx = value & 0xff;
	int len;

	/* ---- mass storage class requests ---- */
	if ((c->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
		if (c->bRequest == BOT_REQ_GET_MAX_LUN &&
		    c->bRequestType == (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE)) {
			t.desc_buf[0] = 0;	/* one LUN */
			*buf = t.desc_buf;
			return 1;
		}
		if (c->bRequest == BOT_REQ_RESET &&
		    c->bRequestType == (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE)) {
			msc_reset();
			return 0;
		}
		return -EOPNOTSUPP;
	}

	if ((c->bRequestType & USB_TYPE_MASK) != USB_TYPE_STANDARD)
		return -EOPNOTSUPP;

	if (!(c->bRequestType & USB_DIR_IN)) {
		/* only alt setting 0 exists: nothing to do */
		if (c->bRequest == USB_REQ_SET_INTERFACE)
			return (value == 0 && !c->wIndex) ? 0 : -EINVAL;
		return -EOPNOTSUPP;
	}

	*buf = t.desc_buf;
	memset(t.desc_buf, 0, MSC_DESC_LEN);

	switch (c->bRequest) {
	case USB_REQ_GET_DESCRIPTOR:
		switch (type) {
		case USB_DT_DEVICE:
			len = build_device_desc(t.desc_buf);
			break;
		case USB_DT_CONFIG:
			len = build_config_desc(t.desc_buf);
			break;
		case USB_DT_STRING:
			len = build_string_desc(t.desc_buf, idx);
			break;
		case USB_DT_DEVICE_QUALIFIER:
			if (t.speed >= USB_SPEED_SUPER)
				return -EINVAL;
			len = build_qualifier_desc(t.desc_buf);
			break;
		case USB_DT_BOS:
			len = build_bos_desc(t.desc_buf);
			break;
		default:
			return -EINVAL;	/* other-speed etc: STALL */
		}
		return len;
	case USB_REQ_GET_CONFIGURATION:
		t.desc_buf[0] = t.config;
		return 1;
	case USB_REQ_GET_INTERFACE:
		t.desc_buf[0] = 0;
		return 1;
	default:
		return -EOPNOTSUPP;
	}
}

static int test_set_config(struct ax_usb_dev *udev, u8 cfg)
{
	msc_disable_eps();
	t.config = 0;

	if (cfg == 0)
		return 0;
	if (cfg != 1)
		return -EINVAL;

	t.config = cfg;
	if (msc_enable_eps()) {
		t.config = 0;
		return -EIO;
	}
	return 0;
}

static void test_connect(struct ax_usb_dev *udev, enum usb_device_speed speed)
{
	t.speed = speed;
	printf("ax_usb_dev: link up, speed %d\n", speed);
}

static void test_disconnect(struct ax_usb_dev *udev)
{
	/* the backend already disabled the endpoints and flushed requests */
	t.ep_en = false;
	t.config = 0;
	m.state = MSC_CBW;
	printf("ax_usb_dev: disconnect/reset\n");
}

static const struct ax_usb_func_ops test_func = {
	.setup		= test_setup,
	.set_config	= test_set_config,
	.connect	= test_connect,
	.disconnect	= test_disconnect,
};

/* ------------------------------- command -------------------------------- */
static const char * const state_names[] = {
	"not attached", "attached", "powered", "reconnecting",
	"unauthenticated", "default", "address", "configured", "suspended",
};

static const char *speed_name(enum usb_device_speed s)
{
	switch (s) {
	case USB_SPEED_LOW:		return "low";
	case USB_SPEED_FULL:		return "full";
	case USB_SPEED_HIGH:		return "high";
	case USB_SPEED_SUPER:		return "super";
	case USB_SPEED_SUPER_PLUS:	return "super-plus";
	default:			return "unknown";
	}
}

static int test_alloc(void)
{
	if (t.desc_buf)
		return 0;

	/* 4KB / 64KB alignment: a buffer never crosses a 64KB boundary */
	t.desc_buf = memalign(4096, 4096);
	m.ctl = memalign(4096, 4096);
	m.bounce = memalign(MSC_CHUNK, MSC_CHUNK);
	if (!t.desc_buf || !m.ctl || !m.bounce) {
		free(t.desc_buf);
		free(m.ctl);
		free(m.bounce);
		t.desc_buf = m.ctl = m.bounce = NULL;
		return -ENOMEM;
	}
	memset(t.desc_buf, 0, 4096);
	memset(m.ctl, 0, 4096);
	memset(m.bounce, 0, MSC_CHUNK);
	flush_dcache_range((ulong)t.desc_buf, (ulong)t.desc_buf + 4096);
	flush_dcache_range((ulong)m.ctl, (ulong)m.ctl + 4096);
	flush_dcache_range((ulong)m.bounce, (ulong)m.bounce + MSC_CHUNK);
	return 0;
}

static int do_init(int argc, char *const argv[])
{
	enum usb_device_speed speed = USB_SPEED_SUPER;
	ulong base = CONFIG_AX_USB_DEV_BASE;
	void __iomem *regs;
	int ret;

	if (argc > 2)
		base = hextoul(argv[2], NULL);
	if (argc > 3) {
		if (!strcmp(argv[3], "hs"))
			speed = USB_SPEED_HIGH;
		else if (!strcmp(argv[3], "fs"))
			speed = USB_SPEED_FULL;
		else if (strcmp(argv[3], "ss"))
			return CMD_RET_USAGE;
	}
	if (!base) {
		printf("no controller base address: give it as argument or set CONFIG_AX_USB_DEV_BASE\n");
		return CMD_RET_FAILURE;
	}

	if (t.udev) {
		crg_udc_bm_destroy(t.udev);
		t.udev = NULL;
	}

	ret = test_alloc();
	if (ret)
		return CMD_RET_FAILURE;

	regs = map_physmem(base, AX_USB_REG_SIZE, MAP_NOCACHE);
	t.udev = crg_udc_bm_create(regs, speed);
	if (!t.udev) {
		printf("ax_usb_dev: create failed\n");
		return CMD_RET_FAILURE;
	}

	ret = t.udev->ops->init(t.udev);
	if (ret) {
		printf("ax_usb_dev: init failed (%d)\n", ret);
		crg_udc_bm_destroy(t.udev);
		t.udev = NULL;
		return CMD_RET_FAILURE;
	}

	t.speed = speed;
	t.config = 0;
	t.ep_en = false;
	printf("ax_usb_dev: initialised, base 0x%lx, max speed %s\n", base,
	       speed_name(speed));
	return CMD_RET_SUCCESS;
}

static int do_start(int argc, char *const argv[])
{
	struct ax_usb_status st;
	ulong addr, size, timeout_s = 0, t0;
	int ret;

	if (!t.udev) {
		printf("ax_usb_dev: not initialised, run 'ax_usb_dev init' first\n");
		return CMD_RET_FAILURE;
	}
	if (argc < 4)
		return CMD_RET_USAGE;

	addr = hextoul(argv[2], NULL);
	size = hextoul(argv[3], NULL);
	if (argc > 4)
		timeout_s = dectoul(argv[4], NULL);

	if (!size || size % MSC_SECTOR) {
		printf("ax_usb_dev: size must be a non-zero multiple of %d bytes\n",
		       MSC_SECTOR);
		return CMD_RET_FAILURE;
	}

	m.addr = addr;
	m.sectors = size / MSC_SECTOR;
	m.ram = map_sysmem(addr, size);
	m.n_cmd = m.n_read = m.n_write = m.n_bad_cbw = m.err = 0;
	m.bytes_rd = m.bytes_wr = 0;
	m.sk = m.asc = m.ascq = 0;
	m.state = MSC_CBW;

	ret = t.udev->ops->start(t.udev, &test_func);
	if (ret) {
		printf("ax_usb_dev: start failed (%d)\n", ret);
		return CMD_RET_FAILURE;
	}

	printf("ax_usb_dev: exporting RAM 0x%lx..0x%lx (%llu sectors) as a USB disk\n",
	       addr, addr + size - 1, (unsigned long long)m.sectors);
	printf("ax_usb_dev: connect the cable to a host. Ctrl-C to quit%s\n",
	       timeout_s ? " (or wait for timeout)" : "");

	t0 = get_timer(0);
	for (;;) {
		if (ctrlc())
			break;
		if (timeout_s && get_timer(t0) > timeout_s * 1000)
			break;
		t.udev->ops->poll(t.udev);
	}

	t.udev->ops->status(t.udev, &st);
	msc_disable_eps();
	t.udev->ops->stop(t.udev);
	printf("ax_usb_dev: stopped (setup %u, xfer evt %u, scsi cmds %u, read %llu B, written %llu B, bad CBW %u, err %u)\n",
	       st.cnt_setup, st.cnt_xfer_evt, m.n_cmd,
	       (unsigned long long)m.bytes_rd, (unsigned long long)m.bytes_wr,
	       m.n_bad_cbw, m.err);
	return CMD_RET_SUCCESS;
}

static int do_stop(void)
{
	struct ax_usb_status st;

	if (!t.udev) {
		printf("ax_usb_dev: not initialised\n");
		return CMD_RET_FAILURE;
	}

	t.udev->ops->status(t.udev, &st);
	if (!st.initialized)
		return CMD_RET_SUCCESS;

	msc_disable_eps();
	t.udev->ops->stop(t.udev);
	printf("ax_usb_dev: stopped\n");
	return CMD_RET_SUCCESS;
}

static int do_status(void)
{
	struct ax_usb_status st;

	if (!t.udev) {
		printf("ax_usb_dev: not initialised\n");
		return CMD_RET_SUCCESS;
	}

	t.udev->ops->status(t.udev, &st);
	printf("initialised : %s\n", st.initialized ? "yes" : "no");
	printf("started     : %s\n", st.started ? "yes" : "no");
	printf("max speed   : %s\n", speed_name(st.max_speed));
	if (!st.initialized)
		return CMD_RET_SUCCESS;

	printf("connected   : %s\n", st.connected ? "yes" : "no");
	printf("link speed  : %s\n", speed_name(st.speed));
	printf("dev state   : %s\n",
	       (st.device_state >= 0 && st.device_state < ARRAY_SIZE(state_names)) ?
	       state_names[st.device_state] : "?");
	printf("dev address : %u\n", st.dev_addr);
	printf("enabled eps : %u (ep_enable 0x%08x, ep_running 0x%08x)\n",
	       st.num_enabled_eps, st.ep_enable, st.ep_running);
	printf("regs        : control 0x%08x status 0x%08x portsc 0x%08x\n",
	       st.control, st.hw_status, st.portsc);
	printf("events      : setup %u, xfer %u, port %u, bus reset %u\n",
	       st.cnt_setup, st.cnt_xfer_evt, st.cnt_port_evt, st.cnt_bus_reset);
	printf("RAM disk    : 0x%lx, %llu sectors of %d B\n", m.addr,
	       (unsigned long long)m.sectors, MSC_SECTOR);
	printf("scsi        : %u cmds (last op 0x%02x), %u reads, %u writes, bad CBW %u, err %u\n",
	       m.n_cmd, m.last_op, m.n_read, m.n_write, m.n_bad_cbw, m.err);
	printf("traffic     : read %llu B, written %llu B\n",
	       (unsigned long long)m.bytes_rd, (unsigned long long)m.bytes_wr);
	return CMD_RET_SUCCESS;
}

static int do_ax_usb_dev(struct cmd_tbl *cmdtp, int flag, int argc,
			 char *const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "init"))
		return do_init(argc, argv);
	if (!strcmp(argv[1], "start"))
		return do_start(argc, argv);
	if (!strcmp(argv[1], "stop"))
		return do_stop();
	if (!strcmp(argv[1], "status"))
		return do_status();

	return CMD_RET_USAGE;
}

U_BOOT_CMD(ax_usb_dev, 5, 0, do_ax_usb_dev,
	   "AX3000 bare-metal USB device controller test",
	   "init [base [hs|ss|fs]]  - reset controller, allocate rings, init EP0\n"
	   "ax_usb_dev start <addr> <size> [seconds]\n"
	   "                           - export RAM <addr>,<size> (hex) as a USB mass\n"
	   "                             storage disk and poll until Ctrl-C (or\n"
	   "                             timeout), then stop\n"
	   "ax_usb_dev stop            - detach and reset the controller\n"
	   "ax_usb_dev status          - controller / enumeration / disk traffic status"
);
