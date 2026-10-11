// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <asm/io.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include <malloc.h>
#include <irq_func.h>
#include <display_options.h>

#include "ax_diag.h"
#include "ax_mailbox.h"

#define CHAN_STRIDE   0x4
#define TX_REG_STRIDE 0x40
#define RX_REG_STRIDE 0x30

#define AX_MBOX_READ_ADDRESS    (AX3000_MEM_BASE_ADRS_DDR_LO + 0x20000000)
#define AX_MBOX_WRITE_ADDRESS   (AX3000_MEM_BASE_ADRS_DDR_LO + 0x10000000)

struct ax_mbx_priv {
	void __iomem *mbox;
	void __iomem *csr;
};

struct axiado_channel_data {
    struct ax_mbx_priv tx;
    struct ax_mbx_priv rx;

	void *rx_buffer;

	int irq;
	uint chan_msg_size;
	uint channel_num;
	uint chan_state;
	char name[16];
};

/* mailbox side */
struct axiado_mbox {
	void __iomem *tx_base;
	void __iomem *rx_base;
	uint msg_size;
	u8 num_chans;
};

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

#define SP2_A53_INTID_MB_TO_A53_INTR_0				 285 /**< HW IPC Mailbox interrupt 0 */
#define SP2_A53_INTID_MB_TO_A53_INTR_1				 286 /**< HW IPC Mailbox interrupt 1 */
#define SP2_A53_INTID_MB_TO_A53_INTR_2				 287 /**< HW IPC Mailbox interrupt 2 */
#define SP2_A53_INTID_MB_TO_A53_INTR_3				 288 /**< HW IPC Mailbox interrupt 3 */
#define SP2_A53_INTID_MB_TO_A53_INTR_4				 289 /**< HW IPC Mailbox interrupt 4 */
#define SP2_A53_INTID_MB_TO_A53_INTR_5				 290 /**< HW IPC Mailbox interrupt 5 */
#define SP2_A53_INTID_MB_TO_A53_INTR_6				 291 /**< HW IPC Mailbox interrupt 6 */
#define SP2_A53_INTID_MB_TO_A53_INTR_7				 292 /**< HW IPC Mailbox interrupt 7 */
#define SP2_A53_INTID_MB_TO_A53_INTR_8				 293 /**< HW IPC Mailbox interrupt 8 */
#define SP2_A53_INTID_MB_TO_A53_INTR_9				 294 /**< HW IPC Mailbox interrupt 9 */
#define SP2_A53_INTID_MB_TO_A53_INTR_10				 295 /**< HW IPC Mailbox interrupt 10 */
#define SP2_A53_INTID_MB_TO_A53_INTR_11				 296 /**< HW IPC Mailbox interrupt 11 */

static struct axiado_mbox mbox_dev = {
    .tx_base = (void __iomem *)AX3000_CSR_BASE_ADRS_SP0_GP_MBX,
    .rx_base = (void __iomem *)AX3000_CSR_BASE_ADRS_SP1_GP_MBX,
    .msg_size = 0x100,
    .num_chans = 12,
};

static struct axiado_channel_data *ch_data;

static void axiado_rx_thread(void *arg)
{
    struct axiado_channel_data *priv = arg;
    void __iomem *data_reg;
    int num_words;
    u32 *word_data;

    if (readl(priv->rx.csr) & BIT(0))
        return;

    data_reg = priv->rx.mbox;
    word_data = (u32 *)priv->rx_buffer;
    num_words = priv->chan_msg_size / sizeof(u32);

    while (num_words--) {
        if (readl(priv->rx.csr) & BIT(0))
            break;

        *word_data = readl(data_reg);

        word_data++;
    }
}

static int axiado_mailbox_request(void)
{
    struct axiado_mbox *mb = &mbox_dev;
    unsigned int i;

    printf("%s\n", __func__);
    printf("\ttx_base=0x%8p,\n\trx_base=0x%8p\n",
            mb->tx_base,
            mb->rx_base);

    printf("\tnum_chans=%d,\n\tmsg_size=%d\n",
            mb->num_chans,
            mb->msg_size);

    ch_data = (struct axiado_channel_data *)
        malloc(mb->num_chans * sizeof(*ch_data));

    if (!ch_data)
        return -ENOMEM;

    for (i = 0; i < mb->num_chans; i++) {
        ch_data[i].tx.mbox = mb->tx_base + (i * CHAN_STRIDE);
        ch_data[i].tx.csr = mb->tx_base + TX_REG_STRIDE + (i * CHAN_STRIDE);

        ch_data[i].rx.mbox = mb->rx_base + (i * CHAN_STRIDE);
        ch_data[i].rx.csr = mb->rx_base + RX_REG_STRIDE + (i * CHAN_STRIDE);

        ch_data[i].irq = SP1_A53_INTID_MB_TO_A53_INTR_0 + i;

        irq_install_handler(IRQ_VEC(ch_data[i].irq, IRQ_TRIGGER_LEVEL), axiado_rx_thread, &ch_data[i]);

        printf("RX chan %d → irq %d\n", i, ch_data[i].irq);
        ch_data[i].channel_num = i;
        ch_data[i].chan_state = 0;
        ch_data[i].chan_msg_size = mb->msg_size;
        snprintf(ch_data[i].name, sizeof(ch_data[i].name), "chan-%d", i);

        ch_data[i].rx_buffer =
            (void *)((uintptr_t)AX_MBOX_WRITE_ADDRESS +
                    i * mb->msg_size);
    }

    return 0;
}

static int axiado_mailbox_free(void)
{
    return 0;
}

static int axiado_mailbox_startup(uint chan)
{
    struct axiado_channel_data *priv = ch_data;
    printf("Startup called for channel %s\n", priv[chan].name);
    priv[chan].chan_state = 0;
    writel(BIT(5), priv[chan].tx.csr);
    writel(BIT(5), priv[chan].rx.csr);
    return 0;
}

static int axiado_mailbox_shutdown(uint chan)
{
    struct axiado_channel_data *priv = ch_data;
    printf("Shutdown called for channel %s\n", priv[chan].name);
    priv[chan].chan_state = 0;
    writel(BIT(5), priv[chan].tx.csr);
    writel(BIT(5), priv[chan].rx.csr);
    return 0;
}

static int axiado_mailbox_send(uint chan, const void *data)
{ 
    struct axiado_mbox *mb = &mbox_dev;
    struct axiado_channel_data *priv = ch_data;
    void __iomem *data_reg;
    u32 *word_data;
    int num_words;
    u32 actual_len;
    u32 actual_words;
    u32 max_words = mb->msg_size / sizeof(u32);

    printf("Sending on %s\n", priv[chan].name);

    if (readl(priv->tx.csr) & BIT(2))
        writel(BIT(2), priv->tx.csr);

    if (!(readl(priv->tx.csr) & BIT(0))) {
        printf("%s: %s last data has not finished\n", __func__, priv[chan].name);
        return -EBUSY;
    }

    actual_len = le32_to_cpu(*((u32 *)data));

    if (actual_len == 0 || actual_len > mb->msg_size)
        actual_len = mb->msg_size;

    actual_words = min_t(u32,
            (actual_len + sizeof(u32) - 1) / sizeof(u32),
            max_words);

    for (data_reg = priv->tx.mbox,
            num_words = actual_words,
            word_data = (u32 *)data;
            num_words; num_words--, word_data++)
        writel(*word_data, data_reg);

    printf("%s: %s sent\n", __func__, priv[chan].name);

    return 0;
}

static int axiado_mailbox_recv(uint chan, void *data, ulong timeout_ms)
{
    struct axiado_channel_data *priv = ch_data;

    printf("Receiving on %s\n", priv[chan].name);

    print_buffer(0,
            priv[chan].rx_buffer,
            sizeof(u32),
            priv[chan].chan_msg_size / sizeof(u32),
            4);

    if (data)
        memcpy(data,
                priv[chan].rx_buffer,
                priv[chan].chan_msg_size);

    return 0;
}

struct ax_mbox_ops mbox = {
    .request = axiado_mailbox_request,
    .free = axiado_mailbox_free,
    .startup = axiado_mailbox_startup,
    .shutdown = axiado_mailbox_shutdown,
    .send = axiado_mailbox_send,
    .recv = axiado_mailbox_recv,
};
