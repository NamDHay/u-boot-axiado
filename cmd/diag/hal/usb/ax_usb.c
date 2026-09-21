// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */
#include <dm.h>
#include <stdio.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <time.h>
#include <irq_func.h>

#include "ax_diag.h"
#include "ax_gpio.h"
#include "ax_usb.h"
#include "xhci/xhci.h"

#define AX_USB_NUM_PORTS 6

/* Reset-control register block inside the USBPHY8 CSR space */
#ifndef USB_EXT_REGS_START_ADDR
#define USB_EXT_REGS_START_ADDR			0x0000A000
#endif
#ifndef REG_USB3_0_RESET_CTRL_ADRS_OFFSET
#define REG_USB3_0_RESET_CTRL_ADRS_OFFSET	0x000	/* +4 per USB3 controller */
#endif
#ifndef REG_USB2_0_RESET_CTRL_ADRS_OFFSET
#define REG_USB2_0_RESET_CTRL_ADRS_OFFSET	0x008	/* +4 per USB2 controller */
#endif

static struct axh_ctrl g_ctrl;
static int g_active;
static int g_port = -1;

static u32 *ax_usb_rst_reg(int port)
{
	uintptr_t base = (uintptr_t)AX3000_CSR_BASE_ADRS_USBPHY8 + USB_EXT_REGS_START_ADDR;

	if (port < 4)
		return (u32 *)(base + REG_USB2_0_RESET_CTRL_ADRS_OFFSET + port * 4);
	return (u32 *)(base + REG_USB3_0_RESET_CTRL_ADRS_OFFSET + (port - 4) * 4);
}

static int ax_usb_port_valid(int port, int protocol)
{
	if (port < 0 || port >= AX_USB_NUM_PORTS)
		return 0;
	if (protocol == AX_USB_PROTO_2)
		return 1;
	return protocol == AX_USB_PROTO_3 && port >= 4;
}

static void ax_usb_board_prepare(int port)
{
	if (port == 3) {
		ax_gpio_set_direction(110, 0);
        ax_gpio_set_value(110, 1);
    }
}

/* ------------------------------------------------------------------ */
/* Reset control                                                       */
/* ------------------------------------------------------------------ */
int ax_usb_port_reset(int port)
{
	u32 *r;

	if (port < 0 || port >= AX_USB_NUM_PORTS)
		return -EINVAL;
	r = ax_usb_rst_reg(port);
	printf("USB reset reg addr: 0x%p\n", r);

	if (port < 4) {
        setbits_le32(r, BIT(0));
		mdelay(10);
        setbits_le32(r, BIT(1));
	} else {
        setbits_le32(r, BIT(0));
		mdelay(10);
        setbits_le32(r, BIT(1));
        clrbits_le32(r, BIT(2));
	}
	return 0;
}

void ax_usb_port_power_down(int port)
{
	u32 *r;

	if (port < 0 || port >= AX_USB_NUM_PORTS)
		return;
	r = ax_usb_rst_reg(port);

	if (port < 4) {
        clrbits_le32(r, BIT(1));    /* PHY */
        clrbits_le32(r, BIT(0));    /* controller */
	} else {
        clrbits_le32(r, BIT(1));
		mdelay(300);
        setbits_le32(r, BIT(2));
		mdelay(100);
        clrbits_le32(r, BIT(0));
	}
	mdelay(300);
}

void ax_usb_stop_port(int port)
{
	ax_usb_port_power_down(port);
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */
void ax_usb_stop(void)
{
	if (!g_active)
		return;
	axh_ctrl_stop(&g_ctrl);
	ax_usb_port_power_down(g_port);
	g_active = 0;
	g_port = -1;
}

int ax_usb_active(void)
{
	return g_active;
}

int ax_usb_init(int port, int protocol)
{
	int ret, attempt;

	if (!ax_usb_port_valid(port, protocol)) {
		printf("Invalid Port: %d, Protocol = %d\n", port, protocol);
		return -EINVAL;
	}
	ax_usb_stop();			/* one session at a time */
	ax_usb_board_prepare(port);
	printf("Scan USB port_%d (USB%d.0)\n", port, protocol);

	for (attempt = 0; attempt < 2; attempt++) {
		ax_usb_port_reset(port);
		ret = axh_ctrl_init(&g_ctrl, port, protocol);
		if (ret) {
			printf("Failed to initialize USB host controller (%d)\n", ret);
			axh_ctrl_stop(&g_ctrl);
			ax_usb_port_power_down(port);
			return ret;
		}
		ret = axh_wait_device(&g_ctrl, AXH_CONNECT_TIMEOUT_MS);
		if (!ret)
			break;
		axh_ctrl_stop(&g_ctrl);
	}
	if (ret) {
		printf("USB device not detected\n");
		ax_usb_port_power_down(port);
		return ret;
	}

	g_port = port;
	g_active = 1;
	ret = axh_enumerate(&g_ctrl);
	if (ret)
		printf("Enumeration failed (%d)\n", ret);
	return ret;
}

int ax_usb_get_info(struct ax_usb_info *info)
{
	struct axh_device *dev = &g_ctrl.dev;
	const struct axh_msc *m;

	if (!g_active || !dev->slot_id)
		return -ENODEV;

	memset(info, 0, sizeof(*info));
	info->port = g_port;
	info->protocol = g_ctrl.protocol;
	info->speed = dev->speed;
	info->speed_str = axh_speed_str(dev->speed);
	info->vid = le16_to_cpu(dev->ddesc.idVendor);
	info->pid = le16_to_cpu(dev->ddesc.idProduct);
	info->if_class = dev->idesc.bInterfaceClass;
	info->if_subclass = dev->idesc.bInterfaceSubClass;
	info->if_protocol = dev->idesc.bInterfaceProtocol;
	memcpy(info->manufacturer, dev->manufacturer, sizeof(info->manufacturer));
	memcpy(info->product, dev->product, sizeof(info->product));
	memcpy(info->serial, dev->serial, sizeof(info->serial));

	m = axh_msc_get(dev);
	if (m) {
		info->is_msc = 1;
		info->blk_size = m->blk_size;
		info->blk_count = m->blk_count;
		memcpy(info->scsi_vendor, m->vendor, sizeof(info->scsi_vendor));
		memcpy(info->scsi_product, m->product, sizeof(info->scsi_product));
		memcpy(info->scsi_rev, m->rev, sizeof(info->scsi_rev));
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* Mass Storage                                                        */
/* ------------------------------------------------------------------ */
static struct axh_device *ax_usb_msc_dev(void)
{
	if (!g_active || !axh_msc_get(&g_ctrl.dev)) {
		printf("no USB mass-storage device initialised (use: usbval init ...)\n");
		return NULL;
	}
	return &g_ctrl.dev;
}

int ax_usb_msc_read(u64 lba, u32 blkcnt, void *buf)
{
	struct axh_device *d = ax_usb_msc_dev();

	return d ? axh_msc_read(d, lba, blkcnt, buf) : -ENODEV;
}

int ax_usb_msc_write(u64 lba, u32 blkcnt, const void *buf)
{
	struct axh_device *d = ax_usb_msc_dev();

	return d ? axh_msc_write(d, lba, blkcnt, buf) : -ENODEV;
}

int ax_usb_msc_erase(u64 lba, u32 blkcnt)
{
	struct axh_device *d = ax_usb_msc_dev();

	return d ? axh_msc_erase(d, lba, blkcnt) : -ENODEV;
}

int ax_usb_msc_test(u64 lba, u32 blkcnt, u32 pattern)
{
	struct axh_device *d = ax_usb_msc_dev();

	return d ? axh_msc_selftest(d, lba, blkcnt, pattern) : -ENODEV;
}

/* ------------------------------------------------------------------ */
/* Board scan (legacy "test_all")                                      */
/* ------------------------------------------------------------------ */
int ax_usb_test_all(void)
{
	static const struct {
		u8 port;
		u8 proto;
	} list[] = {
		{ 5, AX_USB_PROTO_2 }, { 4, AX_USB_PROTO_2 }, { 4, AX_USB_PROTO_3 },
		{ 3, AX_USB_PROTO_2 }, { 2, AX_USB_PROTO_2 }, { 1, AX_USB_PROTO_2 },
		{ 0, AX_USB_PROTO_2 },
	};
	int i, fails = 0;

	for (i = 0; i < (int)ARRAY_SIZE(list); i++) {
		int ret = ax_usb_init(list[i].port, list[i].proto);

		printf("port %d USB%d.0: %s\n", list[i].port, list[i].proto,
		       ret ? "FAIL" : "PASS");
		if (ret)
			fails++;
		ax_usb_stop();	/* no-op if init already powered the port down */
	}
	return fails;
}
