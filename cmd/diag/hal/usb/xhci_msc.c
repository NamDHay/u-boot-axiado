// SPDX-License-Identifier: GPL-2.0+
/* USB Mass Storage, Bulk-Only Transport (BOT) + SCSI transparent command set */
#include <asm/unaligned.h>
#include "xhci.h"

#define CBW_SIG		0x43425355	/* 'USBC' */
#define CSW_SIG		0x53425355	/* 'USBS' */

struct msc_cbw {
	u32 sig;
	u32 tag;
	u32 len;
	u8  flags;
	u8  lun;
	u8  cb_len;
	u8  cb[16];
} __packed;

struct msc_csw {
	u32 sig;
	u32 tag;
	u32 residue;
	u8  status;
} __packed;

static u8 ep_addr_in(struct xhci_udev *d)  { return d->in_ep | 0x80; }
static u8 ep_addr_out(struct xhci_udev *d) { return d->out_ep; }

/* BOT reset recovery (spec 5.3.4) */
static void bot_reset(struct xhci *x, struct xhci_udev *d)
{
	xhci_ctrl_msg(x, d, 0x21, 0xff, 0, d->iface, NULL, 0);
	xhci_clear_halt(x, d, ep_addr_in(d));
	xhci_clear_halt(x, d, ep_addr_out(d));
}

/*
 * Run one SCSI command. Returns 0 on pass, -EIO on command failure
 * (CSW status 1), other negative errno on transport errors.
 */
static int bot_cmd(struct xhci *x, struct xhci_udev *d, const u8 *cb, u8 cb_len,
		   void *data, u32 dlen, bool in)
{
	struct msc_cbw cbw;
	struct msc_csw csw;
	int ret, tries = 0;

	memset(&cbw, 0, sizeof(cbw));
	cbw.sig = CBW_SIG;
	cbw.tag = ++d->tag;
	cbw.len = dlen;
	cbw.flags = in ? 0x80 : 0;
	cbw.cb_len = cb_len;
	memcpy(cbw.cb, cb, cb_len);

	ret = xhci_bulk(x, d, false, &cbw, 31, NULL);
	if (ret)
		goto reset;

	if (dlen) {
		ret = xhci_bulk(x, d, in, data, dlen, NULL);
		if (ret == -EPIPE)
			xhci_clear_halt(x, d, in ? ep_addr_in(d) : ep_addr_out(d));
		else if (ret)
			goto reset;
	}

retry_csw:
	ret = xhci_bulk(x, d, true, &csw, 13, NULL);
	if (ret == -EPIPE && !tries++) {
		xhci_clear_halt(x, d, ep_addr_in(d));
		goto retry_csw;
	}
	if (ret)
		goto reset;

	if (csw.sig != CSW_SIG || csw.tag != cbw.tag || csw.status > 1)
		goto reset_phase;
	return csw.status ? -EIO : 0;

reset_phase:
	ret = -EPROTO;
reset:
	bot_reset(x, d);
	return ret;
}

/* ---- SCSI commands ---- */
static int scsi_inquiry(struct xhci *x, struct xhci_udev *d, u8 *buf36)
{
	u8 cb[6] = { 0x12, 0, 0, 0, 36, 0 };

	return bot_cmd(x, d, cb, 6, buf36, 36, true);
}

static int scsi_tur(struct xhci *x, struct xhci_udev *d)
{
	u8 cb[6] = { 0 };

	return bot_cmd(x, d, cb, 6, NULL, 0, false);
}

static int scsi_sense(struct xhci *x, struct xhci_udev *d)
{
	u8 cb[6] = { 0x03, 0, 0, 0, 18, 0 };
	u8 buf[18];

	return bot_cmd(x, d, cb, 6, buf, 18, true);
}

static int scsi_read_cap10(struct xhci *x, struct xhci_udev *d)
{
	u8 cb[10] = { 0x25 };
	u8 buf[8];
	u32 last;
	int ret;

	ret = bot_cmd(x, d, cb, 10, buf, 8, true);
	if (ret)
		return ret;
	last = get_unaligned_be32(buf);
	d->blksz = get_unaligned_be32(buf + 4);
	if (last == 0xffffffff) {
		printf("xhci: device > 2TB, READ CAPACITY(16) not implemented\n");
		return -EOPNOTSUPP;
	}
	d->blocks = (u64)last + 1;
	return 0;
}

int xhci_msc_probe(struct xhci *x, struct xhci_udev *d)
{
	u8 inq[36];
	int i, ret;

	ret = scsi_inquiry(x, d, inq);
	if (ret)
		return ret;
	memcpy(d->vendor, inq + 8, 8);
	memcpy(d->product, inq + 16, 16);
	for (i = 7; i >= 0 && d->vendor[i] == ' '; i--)
		d->vendor[i] = 0;
	for (i = 15; i >= 0 && d->product[i] == ' '; i--)
		d->product[i] = 0;

	/* wait for the unit: clear unit-attention with REQUEST SENSE */
	for (i = 0; i < 10; i++) {
		if (!scsi_tur(x, d))
			break;
		scsi_sense(x, d);
		mdelay(300);
	}
	if (i == 10)
		return -ENODEV;

	ret = scsi_read_cap10(x, d);
	if (ret)
		return ret;
	if (!d->blksz || d->blksz > XHCI_BOUNCE_SZ)
		return -EINVAL;
	return 0;
}

/* READ(10) / WRITE(10), split so that each command is <= 64KB */
int xhci_msc_rw(struct xhci *x, struct xhci_udev *d, bool write, u64 lba,
		u32 count, void *buf)
{
	u32 max_blk = XHCI_BOUNCE_SZ / d->blksz;
	u8 *p = buf;
	int ret;

	if (lba + count > d->blocks)
		return -ERANGE;

	while (count) {
		u32 n = min(count, max_blk);
		u8 cb[10] = { write ? 0x2a : 0x28 };

		put_unaligned_be32((u32)lba, &cb[2]);
		put_unaligned_be16((u16)n, &cb[7]);

		ret = bot_cmd(x, d, cb, 10, p, n * d->blksz, !write);
		if (ret) {
			scsi_sense(x, d);
			return ret;
		}
		lba += n;
		count -= n;
		p += n * d->blksz;
	}
	return 0;
}

/* "Erase" = write zeroes (mass storage has no portable erase command) */
int xhci_msc_erase(struct xhci *x, struct xhci_udev *d, u64 lba, u32 count)
{
	u32 max_blk = XHCI_BOUNCE_SZ / d->blksz;
	u8 *zero;
	int ret = 0;

	zero = calloc(1, XHCI_BOUNCE_SZ);
	if (!zero)
		return -ENOMEM;

	while (count && !ret) {
		u32 n = min(count, max_blk);

		ret = xhci_msc_rw(x, d, true, lba, n, zero);
		lba += n;
		count -= n;
	}
	free(zero);
	return ret;
}