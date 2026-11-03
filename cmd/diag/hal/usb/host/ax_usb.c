// SPDX-License-Identifier: GPL-2.0+
/* Thin API over the xHCI driver (PCIe card or local MMIO) + USB mass storage */
#include <mapmem.h>
#include "ax_usb.h"
#include "xhci.h"

#define XHCI_MMIO_MAP_SIZE	0x10000

static struct xhci g_x;
static bool g_up;
static bool g_pcie;	/* backend in use */

static struct xhci_udev *get_msc(int idx)
{
	int i, n = 0;

	if (!g_up)
		return NULL;
	for (i = 0; i < XHCI_MAX_DEV; i++) {
		if (g_x.udev[i].present && g_x.udev[i].is_msc) {
			if (n++ == idx)
				return &g_x.udev[i];
		}
	}
	return NULL;
}

/* Common start: g_x.hw must already be attached to a backend. */
static int usb_start(void)
{
	int ret, i, n = 0;

	printf("usb: backend %s, dma offset 0x%llx\n", g_x.hw.ops->name, xhci_dma_offset);

	ret = xhci_init(&g_x);
	if (ret)
		return ret;
	g_up = true;

	ret = xhci_scan_ports(&g_x);
	if (ret < 0)
		return ret;

	for (i = 0; i < XHCI_MAX_DEV; i++) {
		struct xhci_udev *d = &g_x.udev[i];

		if (!d->present || !d->is_msc)
			continue;
		ret = xhci_msc_probe(&g_x, d);
		if (ret) {
			printf("usb: port %u: storage init failed (%d)\n", d->port, ret);
			d->is_msc = false;
			continue;
		}
		n++;
	}
	return n;
}

int ax_usb_init(u8 pcie_port)
{
	int ret;

	if (g_up) {
		printf("usb: already up, run stop first\n");
		return -EBUSY;
	}
	memset(&g_x, 0, sizeof(g_x));

	/* buses 0..1: root port bridge on 0, card behind it on 1 */
	ret = ax_xhci_pci_probe(&g_x.pci, pcie_port, 0, 1);
	if (ret) {
		printf("usb: no xHCI card found (RP init / link / bus numbers?)\n");
		return ret;
	}

	ret = ax_xhci_pci_enable(&g_x.pci, false);	/* polling; true = MSI ack only */
	if (ret)
		return ret;
	xhci_hw_attach_pcie(&g_x.hw, &g_x.pci);
	g_pcie = true;

	ret = usb_start();
	if (ret < 0 && !g_up)
		ax_xhci_pci_disable(&g_x.pci);
	return ret;
}

int ax_usb_init_mmio(ulong base, u64 dma_off)
{
	int ret;

	if (g_up) {
		printf("usb: already up, run stop first\n");
		return -EBUSY;
	}
	memset(&g_x, 0, sizeof(g_x));

	ret = xhci_hw_attach_mmio(&g_x.hw, map_sysmem(base, XHCI_MMIO_MAP_SIZE), dma_off);
	if (ret)
		return ret;
	g_pcie = false;

	return usb_start();
}

void ax_usb_stop(void)
{
	if (!g_up)
		return;
	xhci_stop(&g_x);
	if (g_pcie)
		ax_xhci_pci_disable(&g_x.pci);
	g_up = false;
}

int ax_usb_dev_count(void)
{
	int i, n = 0;

	if (!g_up)
		return 0;
	for (i = 0; i < XHCI_MAX_DEV; i++)
		if (g_x.udev[i].present && g_x.udev[i].is_msc)
			n++;
	return n;
}

int ax_usb_dev_info(int idx, struct ax_usb_info *info)
{
	struct xhci_udev *d = get_msc(idx);

	if (!d)
		return -ENODEV;
	strlcpy(info->vendor, d->vendor, sizeof(info->vendor));
	strlcpy(info->product, d->product, sizeof(info->product));
	info->blocks = d->blocks;
	info->blksz = d->blksz;
	info->port = d->port;
	info->speed = d->speed;
	return 0;
}

int ax_usb_read(int idx, u64 lba, u32 count, void *buf)
{
	struct xhci_udev *d = get_msc(idx);

	return d ? xhci_msc_rw(&g_x, d, false, lba, count, buf) : -ENODEV;
}

int ax_usb_write(int idx, u64 lba, u32 count, const void *buf)
{
	struct xhci_udev *d = get_msc(idx);

	return d ? xhci_msc_rw(&g_x, d, true, lba, count, (void *)buf) : -ENODEV;
}

int ax_usb_erase(int idx, u64 lba, u32 count)
{
	struct xhci_udev *d = get_msc(idx);

	return d ? xhci_msc_erase(&g_x, d, lba, count) : -ENODEV;
}