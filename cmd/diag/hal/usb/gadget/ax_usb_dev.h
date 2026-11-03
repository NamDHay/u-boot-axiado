/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Bare-metal USB device (UDC) ops interface for Axiado AX3000.
 *
 * No Linux gadget framework, no list_head, no spinlock, no IRQ: everything
 * is polled from ops->poll().  The only Linux-style dependency is
 * <linux/usb/ch9.h> for the USB wire-format types.
 */

#ifndef __AX_USB_DEV_H__
#define __AX_USB_DEV_H__

#include <linux/types.h>
#include <linux/usb/ch9.h>

struct ax_usb_dev;
struct ax_usb_req;

/* Max bytes per request: one TRB (HW limit is 64KB per TRB) */
#define AX_USB_REQ_MAX_LEN	65536

/**
 * struct ax_usb_req - one transfer on a non-control endpoint
 *
 * @buf:      data buffer. Must be ARCH_DMA_MINALIGN aligned and its size
 *            rounded up to ARCH_DMA_MINALIGN (cache maintenance is done by
 *            the backend on this range). RAM must be 1:1 mapped for the DMA.
 * @length:   number of bytes to transfer (<= AX_USB_REQ_MAX_LEN)
 * @zero:     IN only: append a zero length packet when length is a
 *            multiple of wMaxPacketSize
 * @complete: called from ops->poll() context when the request finishes
 *            (status 0 = ok, <0 = error / flushed)
 * @actual, @status: filled in by the backend
 * @next, @trb: backend private, do not touch
 */
struct ax_usb_req {
	void *buf;
	u32 length;
	u32 actual;
	int status;
	bool zero;
	void (*complete)(struct ax_usb_dev *udev, u8 ep_addr,
			 struct ax_usb_req *req);
	void *context;

	struct ax_usb_req *next;
	void *trb;
	dma_addr_t dma;
};

struct ax_usb_status {
	bool initialized;
	bool started;
	bool connected;
	int device_state;		/* enum usb_device_state */
	enum usb_device_speed speed;
	enum usb_device_speed max_speed;
	u8 dev_addr;
	u32 num_enabled_eps;
	u32 ep_enable;			/* HW EP_ENABLE reg */
	u32 ep_running;			/* HW EP_RUNNING reg */
	u32 control, hw_status, portsc;
	u32 cnt_setup, cnt_xfer_evt, cnt_port_evt;
	u32 cnt_bus_reset;
};

/**
 * struct ax_usb_func_ops - function layer, provided by the caller of start()
 *
 * @setup:      Non-standard (or not handled by backend) control request.
 *              Return the number of bytes to send in the DATA IN stage
 *              (data pointed by *buf, which must stay valid and be
 *              ARCH_DMA_MINALIGN aligned), 0 for a no-data request (status
 *              only), or <0 to STALL. Host->device data stages with
 *              wLength != 0 are not supported (STALLed by the backend).
 * @set_config: SET_CONFIGURATION (cfg == 0 means deconfigure). <0 = STALL.
 * @reset:      bus reset / reconnect: all non-control EPs are disabled.
 * @connect:    link is up and speed is known.
 * @disconnect: cable removed.
 */
struct ax_usb_func_ops {
	int  (*setup)(struct ax_usb_dev *udev,
		      const struct usb_ctrlrequest *ctrl, void **buf);
	int  (*set_config)(struct ax_usb_dev *udev, u8 cfg);
	void (*reset)(struct ax_usb_dev *udev);
	void (*connect)(struct ax_usb_dev *udev, enum usb_device_speed speed);
	void (*disconnect)(struct ax_usb_dev *udev);
};

/**
 * struct ax_usb_dev_ops - controller backend ops
 *
 * @init:       reset the controller, allocate rings/contexts, init EP0.
 * @exit:       stop and release everything.
 * @start:      attach @func, enable VBUS detect / pull-up. Needs poll().
 * @stop:       detach, reset the controller back to the post-init state.
 * @poll:       run the state machine: VBUS detect, port events, setup
 *              packets, transfer events. Call it in a tight loop.
 * @status:     snapshot of the driver/controller state.
 * @ep_enable:  enable a bulk/interrupt endpoint (isoc not supported).
 *              @comp may be NULL below SuperSpeed.
 * @ep_disable: disable an endpoint; pending requests complete with
 *              -ESHUTDOWN.
 * @ep_queue:   queue one request (one TRB). -EBUSY when the ring is full.
 * @ep_halt:    set / clear halt on an endpoint.
 */
struct ax_usb_dev_ops {
	int  (*init)(struct ax_usb_dev *udev);
	void (*exit)(struct ax_usb_dev *udev);
	int  (*start)(struct ax_usb_dev *udev,
		      const struct ax_usb_func_ops *func);
	int  (*stop)(struct ax_usb_dev *udev);
	int  (*poll)(struct ax_usb_dev *udev);
	void (*status)(struct ax_usb_dev *udev, struct ax_usb_status *st);
	int  (*ep_enable)(struct ax_usb_dev *udev,
			  const struct usb_endpoint_descriptor *desc,
			  const struct usb_ss_ep_comp_descriptor *comp);
	int  (*ep_disable)(struct ax_usb_dev *udev, u8 ep_addr);
	int  (*ep_queue)(struct ax_usb_dev *udev, u8 ep_addr,
			 struct ax_usb_req *req);
	int  (*ep_halt)(struct ax_usb_dev *udev, u8 ep_addr, int halt);
};

struct ax_usb_dev {
	const struct ax_usb_dev_ops *ops;
	void *priv;		/* backend private data */
};

/* Corigine (CRG) backend, crg_udc_bm.c */
struct ax_usb_dev *crg_udc_bm_create(void __iomem *base,
				     enum usb_device_speed max_speed);
void crg_udc_bm_destroy(struct ax_usb_dev *udev);

#endif /* __AX_USB_DEV_H__ */
