// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __USB_H
#define __USB_H

#include <linux/types.h>
#include <stdbool.h>

enum ax_usb_protocol {
	AX_USB_PROTO_2 = 2,
	AX_USB_PROTO_3 = 3,
};

enum ax_usb_speed {
	AX_USB_SPEED_UNKNOWN = 0,
	AX_USB_SPEED_LOW,
	AX_USB_SPEED_FULL,
	AX_USB_SPEED_HIGH,
	AX_USB_SPEED_SUPER,
};

enum ax_usb_direction {
	AX_USB_DIR_OUT = 0,
	AX_USB_DIR_IN = 1,
};

enum ax_usb_device_class {
	AX_USB_CLASS_UNKNOWN = 0,
	AX_USB_CLASS_HUB = 0x09,
	AX_USB_CLASS_MASS_STORAGE = 0x08,
	AX_USB_CLASS_HID = 0x03,
};

struct ax_usb_info {
	int port;
	int protocol;
	u8 speed;
	const char *speed_str;
	u16 vid;
	u16 pid;
	u8 if_class;
	u8 if_subclass;
	u8 if_protocol;
	char manufacturer[64];
	char product[64];
	char serial[64];
	int is_msc;
	u32 blk_size;
	u64 blk_count;
	char scsi_vendor[9];
	char scsi_product[17];
	char scsi_rev[5];
};

/* ---- controller / PHY power ---- */
int  ax_usb_port_reset(int port);	/* release controller + PHY resets */
void ax_usb_port_power_down(int port);	/* assert PHY + controller resets */
 
/* ---- lifecycle ---- */
/*
 * Power up @port, start the xHC, wait for a device on the USB@protocol root
 * port, enumerate it (and probe Mass Storage).  Returns 0 or -errno.
 */
int  ax_usb_init(int port, int protocol);
void ax_usb_stop(void);			/* stop xHC and power the active port down */
void ax_usb_stop_port(int port);	/* power a port down without an active session */
int  ax_usb_active(void);
int  ax_usb_get_info(struct ax_usb_info *info);
 
/* ---- Mass Storage ---- */
int  ax_usb_msc_read(u64 lba, u32 blkcnt, void *buf);
int  ax_usb_msc_write(u64 lba, u32 blkcnt, const void *buf);
int  ax_usb_msc_erase(u64 lba, u32 blkcnt);
int  ax_usb_msc_test(u64 lba, u32 blkcnt, u32 pattern);	/* write + read back + compare */
 
/* ---- whole-board scan (legacy "test_all"): returns number of failing ports ---- */
int  ax_usb_test_all(void);

#endif /* __USB_H */
