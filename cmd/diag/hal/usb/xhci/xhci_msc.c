/*
 * xhci_msc.c - USB Mass Storage class driver (Bulk-Only Transport + SCSI)
 *
 *   SCSI layer : INQUIRY, TEST UNIT READY, REQUEST SENSE, READ CAPACITY(10/16),
 *                READ/WRITE(10/16), SYNCHRONIZE CACHE
 *   BOT layer  : CBW -> [data] -> CSW with stall/reset recovery
 *   Block API  : read / write / erase (zero-fill) / write-read-verify selftest
 *
 * Replaces baremetal disk.c.  Fixes over the original: LBA is 32/64-bit
 * (was uint8_t), the CDB is built byte-wise big-endian, the transfer is
 * chunked through a cache-aligned bounce buffer, and CSW tag/signature/
 * status are checked.
 */
#include <dm.h>
#include <stdio.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <time.h>
#include <malloc.h>

#include "ax_diag.h"
#include "xhci.h"

#define AXH_CBW_SIG		0x43425355U
#define AXH_CSW_SIG		0x53425355U
#define AXH_MSC_BOUNCE_SIZE	(128 * 1024)

struct axh_msc_cbw {
	u32 sig;
	u32 tag;
	u32 data_len;
	u8 flags;
	u8 lun;
	u8 cb_len;
	u8 cb[16];
} __attribute__((packed));

struct axh_msc_csw {
	u32 sig;
	u32 tag;
	u32 residue;
	u8 status;
} __attribute__((packed));

static struct axh_msc g_msc;

static inline void be32_put(u8 *p, u32 v)
{
	p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

static inline void be64_put(u8 *p, u64 v)
{
	be32_put(p, (u32)(v >> 32));
	be32_put(p + 4, (u32)v);
}

static inline u32 be32_get(const u8 *p)
{
	return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3];
}

static inline u64 be64_get(const u8 *p)
{
	return ((u64)be32_get(p) << 32) | be32_get(p + 4);
}

/* ------------------------------------------------------------------ */
/* Bulk-Only Transport                                                 */
/* ------------------------------------------------------------------ */
static int msc_clear_halt(struct axh_msc *m, u8 dci)
{
	u16 ep_addr = (dci >> 1) | ((dci & 1) ? 0x80 : 0);

	return axh_control_msg(m->dev, 0x02, AXU_REQ_CLEAR_FEATURE, 0, ep_addr, NULL, 0);
}

/* Bulk-Only Mass Storage Reset + clear both bulk halts (BOT 5.3.4) */
static void msc_reset_recovery(struct axh_msc *m)
{
	printf("MSC: reset recovery\n");
	axh_control_msg(m->dev, 0x21, 0xff, 0, m->iface, NULL, 0);
	msc_clear_halt(m, m->in_dci);
	msc_clear_halt(m, m->out_dci);
}

/*
 * Returns 0 on command passed, -EIO on command failed (CSW status 1, check
 * sense), other negative errno on transport failure.  @data must be DMA-able.
 */
static int msc_bot(struct axh_msc *m, const u8 *cdb, u8 cdb_len,
		   void *data, u32 dlen, int in, u32 *residue)
{
	struct axh_device *dev = m->dev;
	struct axh_msc_cbw *cbw = m->cbw;
	struct axh_msc_csw *csw = m->csw;
	u8 data_dci = in ? m->in_dci : m->out_dci;
	u32 tag = ++m->tag;
	int ret, i;

	memset(cbw, 0, sizeof(*cbw));
	cbw->sig = cpu_to_le32(AXH_CBW_SIG);
	cbw->tag = cpu_to_le32(tag);
	cbw->data_len = cpu_to_le32(dlen);
	cbw->flags = in ? 0x80 : 0;
	cbw->lun = m->lun;
	cbw->cb_len = cdb_len;
	memcpy(cbw->cb, cdb, cdb_len);

	printf("CBW: op 0x%x len %d %s\n", cdb[0], dlen, in ? "IN" : "OUT");
	ret = axh_bulk_xfer(dev, m->out_dci, cbw, sizeof(*cbw));
	if (ret) {
		msc_reset_recovery(m);
		return ret;
	}

	if (dlen) {
		ret = axh_bulk_xfer(dev, data_dci, data, dlen);
		if (ret == -EPIPE) {
			msc_clear_halt(m, data_dci);	/* data stall: still fetch CSW */
		} else if (ret) {
			msc_reset_recovery(m);
			return ret;
		}
	}

	for (i = 0; i < 2; i++) {
		memset(csw, 0, sizeof(*csw));
		ret = axh_bulk_xfer(dev, m->in_dci, csw, sizeof(*csw));
		if (ret == -EPIPE) {
			msc_clear_halt(m, m->in_dci);
			continue;
		}
		break;
	}
	if (ret) {
		msc_reset_recovery(m);
		return ret;
	}

	if (le32_to_cpu(csw->sig) != AXH_CSW_SIG || le32_to_cpu(csw->tag) != tag) {
		printf("MSC: invalid CSW (sig 0x%x tag 0x%x, expected 0x%x)\n",
			   le32_to_cpu(csw->sig), le32_to_cpu(csw->tag), tag);
		msc_reset_recovery(m);
		return -EPROTO;
	}
	if (residue)
		*residue = le32_to_cpu(csw->residue);

	if (csw->status == 0)
		return 0;
	if (csw->status == 1)
		return -EIO;

	printf("MSC: phase error\n");
	msc_reset_recovery(m);
	return -EPROTO;
}

/* ------------------------------------------------------------------ */
/* SCSI commands                                                       */
/* ------------------------------------------------------------------ */
static int scsi_tur(struct axh_msc *m)
{
	u8 cdb[6] = { 0x00 };

	return msc_bot(m, cdb, sizeof(cdb), NULL, 0, 1, NULL);
}

static int scsi_request_sense(struct axh_msc *m, u8 *key, u8 *asc, u8 *ascq)
{
	u8 cdb[6] = { 0x03, 0, 0, 0, 18, 0 };
	int ret;

	memset(m->bounce, 0, 18);
	ret = msc_bot(m, cdb, sizeof(cdb), m->bounce, 18, 1, NULL);
	if (ret)
		return ret;
	*key = m->bounce[2] & 0xf;
	*asc = m->bounce[12];
	*ascq = m->bounce[13];
	return 0;
}

static int scsi_inquiry(struct axh_msc *m)
{
	u8 cdb[6] = { 0x12, 0, 0, 0, 36, 0 };
	int ret;

	memset(m->bounce, 0, 36);
	ret = msc_bot(m, cdb, sizeof(cdb), m->bounce, 36, 1, NULL);
	if (ret)
		return ret;
	memcpy(m->vendor, m->bounce + 8, 8);
	memcpy(m->product, m->bounce + 16, 16);
	memcpy(m->rev, m->bounce + 32, 4);
	m->vendor[8] = m->product[16] = m->rev[4] = '\0';
	return 0;
}

static int scsi_read_capacity(struct axh_msc *m)
{
	u8 cdb10[10] = { 0x25 };
	u8 cdb16[16] = { 0x9e, 0x10 };
	int ret;

	memset(m->bounce, 0, 32);
	ret = msc_bot(m, cdb10, sizeof(cdb10), m->bounce, 8, 1, NULL);
	if (ret)
		return ret;
	m->blk_count = (u64)be32_get(m->bounce) + 1;
	m->blk_size = be32_get(m->bounce + 4);

	if (be32_get(m->bounce) == 0xffffffffU) {	/* > 2 TiB: READ CAPACITY(16) */
		be32_put(cdb16 + 10, 32);
		memset(m->bounce, 0, 32);
		ret = msc_bot(m, cdb16, sizeof(cdb16), m->bounce, 32, 1, NULL);
		if (ret)
			return ret;
		m->blk_count = be64_get(m->bounce) + 1;
		m->blk_size = be32_get(m->bounce + 8);
	}
	return m->blk_size ? 0 : -EPROTO;
}

static int scsi_sync_cache(struct axh_msc *m)
{
	u8 cdb[10] = { 0x35 };

	return msc_bot(m, cdb, sizeof(cdb), NULL, 0, 0, NULL);
}

static int scsi_rw(struct axh_msc *m, int write, u64 lba, u32 cnt, void *buf)
{
	u8 cdb[16] = { 0 };
	u8 len;
	u32 bytes = cnt * m->blk_size;
	u8 key, asc, ascq;
	int ret, try;

	if (lba + cnt > 0xffffffffULL || cnt > 0xffff) {
		cdb[0] = write ? 0x8a : 0x88;		/* READ/WRITE(16) */
		be64_put(cdb + 2, lba);
		be32_put(cdb + 10, cnt);
		len = 16;
	} else {
		cdb[0] = write ? 0x2a : 0x28;		/* READ/WRITE(10) */
		be32_put(cdb + 2, (u32)lba);
		cdb[7] = cnt >> 8;
		cdb[8] = cnt;
		len = 10;
	}

	for (try = 0; try < 2; try++) {
		ret = msc_bot(m, cdb, len, buf, bytes, !write, NULL);
		if (ret != -EIO)
			break;
		if (!scsi_request_sense(m, &key, &asc, &ascq))
			printf("MSC: sense key 0x%x asc 0x%02x ascq 0x%02x\n", key, asc, ascq);
	}
	return ret;
}

/* ------------------------------------------------------------------ */
/* Probe                                                               */
/* ------------------------------------------------------------------ */
int axh_msc_probe(struct axh_device *dev)
{
	struct axh_ctrl *x = dev->ctrl;
	struct axh_msc *m = &g_msc;
	u8 dci, key, asc, ascq, lun_buf[1] = { 0 };
	int i, ret;

	memset(m, 0, sizeof(*m));
	dev->priv = NULL;
	m->dev = dev;
	m->iface = dev->idesc.bInterfaceNumber;

	for (dci = 2; dci < AXH_MAX_EPS; dci++) {
		struct axh_ep *ep = &dev->ep[dci];

		if (!ep->valid || (ep->desc.bmAttributes & 3) != AXU_EP_BULK)
			continue;
		if (dci & 1) {
			if (!m->in_dci)
				m->in_dci = dci;
		} else if (!m->out_dci) {
			m->out_dci = dci;
		}
	}
	if (!m->in_dci || !m->out_dci) {
		printf("MSC: bulk endpoints not found\n");
		return -ENODEV;
	}
	if (dev->idesc.bInterfaceSubClass != AXU_SUBCLASS_SCSI ||
	    dev->idesc.bInterfaceProtocol != AXU_PROTO_BOT) {
		printf("MSC: only SCSI / Bulk-Only devices are supported (sub 0x%x proto 0x%x)\n",
			   dev->idesc.bInterfaceSubClass, dev->idesc.bInterfaceProtocol);
		return -ENOSYS;
	}

	m->cbw = axh_dma_alloc(x, 64, 64);
	m->csw = axh_dma_alloc(x, 64, 64);
	m->bounce = axh_dma_alloc(x, AXH_MSC_BOUNCE_SIZE, 4096);
	if (!m->cbw || !m->csw || !m->bounce)
		return -ENOMEM;

	/* GET MAX LUN (a stall means single LUN) */
	if (!axh_control_msg(dev, 0xa1, 0xfe, 0, m->iface, lun_buf, 1))
		m->max_lun = lun_buf[0];
	m->lun = 0;
	printf("MSC: max LUN %d\n", m->max_lun);

	ret = scsi_inquiry(m);
	if (ret)
		return ret;

	/* the device may need time (and a few sense reads) before it is ready */
	for (i = 0; i < 20; i++) {
		ret = scsi_tur(m);
		if (!ret)
			break;
		if (ret != -EIO)
			return ret;
		scsi_request_sense(m, &key, &asc, &ascq);
		printf("MSC: not ready (key 0x%x asc 0x%02x ascq 0x%02x)\n", key, asc, ascq);
		mdelay(200);
	}
	if (ret) {
		printf("MSC: unit not ready\n");
		return ret;
	}
	printf("MSC: ready request done\n");

	ret = scsi_read_capacity(m);
	if (ret)
		return ret;
	m->chunk_blks = AXH_MSC_BOUNCE_SIZE / m->blk_size;
	if (!m->chunk_blks) {
		printf("MSC: block size %d too large\n", m->blk_size);
		return -EPROTO;
	}

	printf("MSC: %s %s %s, %d blocks x %d bytes\n", m->vendor, m->product, m->rev,
		    (u32)m->blk_count, m->blk_size);
	dev->priv = m;
	return 0;
}

const struct axh_msc *axh_msc_get(struct axh_device *dev)
{
	return dev ? (const struct axh_msc *)dev->priv : NULL;
}

/* ------------------------------------------------------------------ */
/* Block API                                                           */
/* ------------------------------------------------------------------ */
static int msc_check_range(struct axh_msc *m, u64 lba, u32 cnt)
{
	if (!m)
		return -ENODEV;
	if (lba >= m->blk_count || lba + cnt > m->blk_count) {
		printf("MSC: range %d+%d beyond end of device (%d blocks)\n",
			   (u32)lba, cnt, (u32)m->blk_count);
		return -EINVAL;
	}
	return 0;
}

int axh_msc_read(struct axh_device *dev, u64 lba, u32 cnt, void *buf)
{
	struct axh_msc *m = dev->priv;
	u8 *dst = buf;
	int ret = msc_check_range(m, lba, cnt);

	while (!ret && cnt) {
		u32 n = AXH_MIN(cnt, m->chunk_blks);

		ret = scsi_rw(m, 0, lba, n, m->bounce);
		if (ret)
			break;
		memcpy(dst, m->bounce, n * m->blk_size);
		dst += n * m->blk_size;
		lba += n;
		cnt -= n;
	}
	return ret;
}

int axh_msc_write(struct axh_device *dev, u64 lba, u32 cnt, const void *buf)
{
	struct axh_msc *m = dev->priv;
	const u8 *src = buf;
	int ret = msc_check_range(m, lba, cnt);

	while (!ret && cnt) {
		u32 n = AXH_MIN(cnt, m->chunk_blks);

		memcpy(m->bounce, src, n * m->blk_size);
		ret = scsi_rw(m, 1, lba, n, m->bounce);
		if (ret)
			break;
		src += n * m->blk_size;
		lba += n;
		cnt -= n;
	}
	if (!ret && scsi_sync_cache(m))
		printf("MSC: SYNCHRONIZE CACHE not supported\n");
	return ret;
}

/* Erase = zero-fill (plain SCSI disks have no ERASE; UNMAP is optional) */
int axh_msc_erase(struct axh_device *dev, u64 lba, u32 cnt)
{
	struct axh_msc *m = dev->priv;
	u32 total = cnt, done = 0;
	int ret = msc_check_range(m, lba, cnt);

	if (ret)
		return ret;
	memset(m->bounce, 0, m->chunk_blks * m->blk_size);

	while (cnt) {
		u32 n = AXH_MIN(cnt, m->chunk_blks);

		ret = scsi_rw(m, 1, lba, n, m->bounce);
		if (ret)
			return ret;
		lba += n;
		cnt -= n;
		done += n;
		printf("\rerase: %u/%u blocks", done, total);
	}
	printf("\n");
	if (scsi_sync_cache(m))
		printf("MSC: SYNCHRONIZE CACHE not supported\n");
	return 0;
}

/* Write an incrementing 32-bit pattern, read it back, compare. */
int axh_msc_selftest(struct axh_device *dev, u64 lba, u32 cnt, u32 pattern)
{
	struct axh_msc *m = dev->priv;
	u32 chunk_blks, off_words = 0;
	u8 *wbuf = NULL, *rbuf = NULL;
	int ret = msc_check_range(m, lba, cnt);

	if (ret)
		return ret;
	chunk_blks = AXH_MIN(m->chunk_blks, 64U * 1024 / m->blk_size);
	if (!chunk_blks)
		chunk_blks = 1;
	wbuf = malloc(chunk_blks * m->blk_size);
	rbuf = malloc(chunk_blks * m->blk_size);
	if (!wbuf || !rbuf) {
		ret = -ENOMEM;
		goto out;
	}

	while (cnt) {
		u32 n = AXH_MIN(cnt, chunk_blks);
		u32 words = n * m->blk_size / 4, i;
		u32 *w = (u32 *)wbuf;

		for (i = 0; i < words; i++)
			w[i] = pattern + off_words + i;

		ret = axh_msc_write(dev, lba, n, wbuf);
		if (ret)
			break;
		memset(rbuf, 0, n * m->blk_size);
		ret = axh_msc_read(dev, lba, n, rbuf);
		if (ret)
			break;
		if (memcmp(wbuf, rbuf, n * m->blk_size)) {
			u32 *r = (u32 *)rbuf;

			for (i = 0; i < words && w[i] == r[i]; i++)
				;
			printf("MSC: mismatch at LBA %d word %d: wrote 0x%08x read 0x%08x\n",
				   (u32)lba, i, w[i], r[i]);
			ret = -EIO;
			break;
		}
		off_words += words;
		lba += n;
		cnt -= n;
	}
out:
	free(wbuf);
	free(rbuf);
	return ret;
}
