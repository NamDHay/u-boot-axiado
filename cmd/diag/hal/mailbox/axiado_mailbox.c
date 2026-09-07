// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <time.h>
#include <linux/delay.h>
#include <malloc.h>
#include <linux/log2.h>

#include "ax_diag.h"
#include "ax_mailbox.h"

#define MBX_MAX_CHAN    16
#define MBX_MSG_SIZE    0xFC

#define SP1_A53_INTID_MB_TO_A53_INTR_0				 274 /**< HW IPC Mailbox interrupt 0 */
#define SP1_A53_INTID_MB_TO_A53_INTR_1				 275 /**< HW IPC Mailbox interrupt 1 */
#define SP1_A53_INTID_MB_TO_A53_INTR_2				 276 /**< HW IPC Mailbox interrupt 2 */
#define SP1_A53_INTID_MB_TO_A53_INTR_3				 277 /**< HW IPC Mailbox interrupt 3 */
#define SP1_A53_INTID_MB_TO_A53_INTR_4				 278 /**< HW IPC Mailbox interrupt 4 */
#define SP1_A53_INTID_MB_TO_A53_INTR_5				 279 /**< HW IPC Mailbox interrupt 5 */
#define SP1_A53_INTID_MB_TO_A53_INTR_6				 280 /**< HW IPC Mailbox interrupt 6 */
#define SP1_A53_INTID_MB_TO_A53_INTR_7				 281 /**< HW IPC Mailbox interrupt 7 */
#define SP1_A53_INTID_MB_TO_A53_INTR_8				 282 /**< HW IPC Mailbox interrupt 8 */
#define SP1_A53_INTID_MB_TO_A53_INTR_9				 283 /**< HW IPC Mailbox interrupt 9 */
#define SP1_A53_INTID_MB_TO_A53_INTR_10				 284 /**< HW IPC Mailbox interrupt 10 */
#define SP1_A53_INTID_MB_TO_A53_INTR_11				 285 /**< HW IPC Mailbox interrupt 11 */

#define SP2_A53_INTID_MB_TO_A53_INTR_0				 286 /**< HW IPC Mailbox interrupt 0 */
#define SP2_A53_INTID_MB_TO_A53_INTR_1				 287 /**< HW IPC Mailbox interrupt 1 */
#define SP2_A53_INTID_MB_TO_A53_INTR_2				 288 /**< HW IPC Mailbox interrupt 2 */
#define SP2_A53_INTID_MB_TO_A53_INTR_3				 289 /**< HW IPC Mailbox interrupt 3 */
#define SP2_A53_INTID_MB_TO_A53_INTR_4				 290 /**< HW IPC Mailbox interrupt 4 */
#define SP2_A53_INTID_MB_TO_A53_INTR_5				 291 /**< HW IPC Mailbox interrupt 5 */
#define SP2_A53_INTID_MB_TO_A53_INTR_6				 292 /**< HW IPC Mailbox interrupt 6 */
#define SP2_A53_INTID_MB_TO_A53_INTR_7				 293 /**< HW IPC Mailbox interrupt 7 */
#define SP2_A53_INTID_MB_TO_A53_INTR_8				 294 /**< HW IPC Mailbox interrupt 8 */
#define SP2_A53_INTID_MB_TO_A53_INTR_9				 295 /**< HW IPC Mailbox interrupt 9 */
#define SP2_A53_INTID_MB_TO_A53_INTR_10				 296 /**< HW IPC Mailbox interrupt 10 */
#define SP2_A53_INTID_MB_TO_A53_INTR_11				 297 /**< HW IPC Mailbox interrupt 11 */

#define AX_TX_CHANS   8   /* 0–7 */
#define AX_RX_CHANS   8   /* 8–15 */
#define CHAN_STRIDE   0x4
#define TX_REG_STRIDE 0x40
#define RX_REG_STRIDE 0x30

struct axiado_mbox_data {
	u8 num_chans;
	u8 msg_size;
};

struct axiado_channel_data {
	void __iomem *mbox_reg;
	void __iomem *csr_reg;
	void __iomem *gic_reg;
	char name[16];
	void *rx_buffer;
	u8 channel_num;
	int irq;
	struct mbox_chan *chan;
	u8 chan_msg_size;
	u8 chan_state;
};

/* mailbox side */
struct axiado_mbox {
	struct mbox_controller mbox;
	const struct axiado_mbox_data *drv_data;
	void __iomem *tx_intr;
	void __iomem *rx_intr;
};

static int axiado_mailbox_send(uint chan, const void *data)
{
    return 0;
}

static int axiado_mailbox_recv(uint chan, void *data, ulong timeout_ms)
{
    return 0;
}

static int axiado_mailbox_request(uint chan)
{
    return 0;
}

static int axiado_mailbox_free(uint chan)
{
    return 0;
}

struct ax_mbox_ops mbox = {
    .request = axiado_mailbox_request,
    .free = axiado_mailbox_free,
    .send = axiado_mailbox_send,
    .recv = axiado_mailbox_recv,
};
