// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <irq_func.h>

#include "ax_diag.h"
#include "ax_uart.h"
#include "ax_pinmux.h"

/* Candence UART register offsets */
#define CDNS_UART_CNTR_REG		  0x0   // CONTROL REGISTER
#define CDNS_UART_MODE_REG		  0x4   // MODE REGISTER
#define CDNS_UART_IER_REG		  0x8   // INTERRUPT ENALE REGISTER
#define CDNS_UART_IDR_REG		  0xC   // INTERRUPT DISABLE REGISTER
#define CDNS_UART_IMR_REG		  0x10  // INTERRUPT MASK REGISTER
#define CDNS_UART_CISR_REG		  0x14  // CHANNEL INTERRUPT STATUS REGISTER
#define CDNS_UART_BRGR_REG		  0x18  // BAUD RATE GENERATOR REGISTER
#define CDNS_UART_RTR_REG		  0x1C  // RECEIVE TIMEOUT  REGISTER
#define CDNS_UART_RXFIFOTRL_REG	  0x20  // RECEIVE FIFO TRIGGER LEVEL REGISTER
#define CDNS_UART_MCR_REG		  0x24  // MODEM CONTROLREGISTER
#define CDNS_UART_MSR_REG		  0x28  // MODEM STATUS REGISTER
#define CDNS_UART_CSR_REG		  0x2C  // CHANNEL STATUS REGISTER
#define CDNS_UART_RXTXFIFO_REG	  0x30  // RECEIVE/TRANSMIT FIFO
#define CDNS_UART_BRDR_REG		  0x34  // BAUD RATE DIVIDER REGISTER
#define CDNS_UART_FCDR_REG		  0x38  // FLOW CONTROL DELAY REGISTER
#define CDNS_UART_IRMINRXPW_REG	  0x3C  // IR MINIMUM RECEIVED PULSE WIDTH
#define CDNS_UART_IRTXPW_REG	  0x40  // IR TRANSMITTED PULSE WIDTH
#define CDNS_UART_TXFIFOTRL_REG	  0x44  // TRANSMIT FIFO TRIGGER LEVEL
#define CDNS_UART_RXFIFOBYTES_REG 0x48  // RXFIFO BYTE STATUS

#define UART_CTRL_RX_RST	 BIT(0)
#define UART_CTRL_TX_RST	 BIT(1)
#define UART_CTRL_RX_ENABLE	 BIT(2)
#define UART_CTRL_RX_DISABLE BIT(3)
#define UART_CTRL_TX_ENABLE	 BIT(4)
#define UART_CTRL_TX_DISABLE BIT(5)

#define UART_MR_NO_PARITY 0x20
#define UART_MR_STOP_BITS 0X0
#define UART_MR_DATA_BITS 0X01000

#define UART_SR_INTR_RTRIG		  BIT(0)
#define UART_SR_INTR_REMPTY		  BIT(1)
#define UART_SR_INTR_TEMPTY		  BIT(3)
#define UART_SR_INTR_TFUL		  BIT(4)
#define UART_SR_INTR_RXACTIVE	  BIT(10)
#define UART_SR_INTR_TXACTIVE	  BIT(11)
#define UART_SR_INTR_TXALMOSTFULL BIT(14)

/* UART operational modes */
#define UART_MODE_NORMAL	 0x0
#define UART_MODE_ECHO		 0x1
#define UART_MODE_L_LOOPBACK 0x2
#define UART_MODE_R_LOOPBACK 0x3

#define UART_CHANNEL_MODE_BIT 0x8
#define UART_CHANNEL_MODE_0	  0x0 /**< Normal mode */
#define UART_CHANNEL_MODE_1	  BIT(8) /**< Automatic echo mode */
#define UART_CHANNEL_MODE_2	  BIT(9) /**< Local loopback mode */
#define UART_CHANNEL_MODE_3	  (BIT(9) | BIT(8)) /**< Remote loopback mode */

#define CONFIG_MAX_UART 9
#define CONFIG_UART_CLK 125000000

struct cdns_uart {
    void __iomem *base;
    int irq;
};

static struct cdns_uart uart_dev[CONFIG_MAX_UART] = {
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_UART_0, .irq = 144 },
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_UART_1, .irq = 145 },
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_UART_2, .irq = 146 },
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_UART,   .irq = 202 },
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_UART_4, .irq = 240 },
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_UART_5, .irq = 241 },
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_UART_6, .irq = 242 },
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_UART_7, .irq = 243 },
    { .base = (void __iomem *)AX3000_CSR_BASE_ADRS_UART_8, .irq = 244 },
};

void cdns_uart_interrupt_handler(void *arg)
{
    struct cdns_uart *dev = arg;

    if (!dev)
        return;

    readl(dev->base + CDNS_UART_CISR_REG);
}

static int cdns_uart_init(unsigned instance) 
{
    if (instance >= CONFIG_MAX_UART)
        return -EINVAL;

    struct cdns_uart *dev = &uart_dev[instance];
    u32 val;
    char name[8];

    val = readl(dev->base + CDNS_UART_CNTR_REG);
    if (val & UART_CTRL_TX_ENABLE)
        return 0;

    sprintf(name, "UART%d", instance);
    ax_pinmux_set_state(name);

    /* Reset FIFOs */
    writel(UART_CTRL_RX_RST |
            UART_CTRL_TX_RST,
            dev->base + CDNS_UART_CNTR_REG);

    irq_install_handler(dev->irq, cdns_uart_interrupt_handler, dev);
    writel(0xFFFFFFFF, dev->base + CDNS_UART_IER_REG);
    writel(0x3F, dev->base + CDNS_UART_RTR_REG);
    writel(0x2, dev->base + CDNS_UART_TXFIFOTRL_REG);

    /* Enable RX/TX */
    writel(UART_CTRL_TX_ENABLE |
            UART_CTRL_RX_ENABLE,
            dev->base + CDNS_UART_CNTR_REG);

    writel(UART_MR_NO_PARITY, dev->base + CDNS_UART_MODE_REG);

    return 0;
}

static void cdns_uart_exit(unsigned instance) 
{
    struct cdns_uart *dev = &uart_dev[instance];

    writel(UART_CTRL_TX_DISABLE | UART_CTRL_RX_DISABLE, 
            dev->base + CDNS_UART_CNTR_REG);
}

static int cdns_uart_setbaud(unsigned instance, int baud) 
{
    /* Calculation results. */
    struct cdns_uart *dev = &uart_dev[instance];
    unsigned long clock = CONFIG_UART_CLK;
    unsigned int calc_bauderror, bdiv, bgen;
    unsigned long calc_baud = 0;

    /* Covering case where input clock is so slow */
    if (clock < 1000000 && baud > 4800)
        baud = 4800;

    /*                master clock
     * Baud rate = ------------------
     *              bgen * (bdiv + 1)
     *
     * Find acceptable values for baud generation.
     */
    for (bdiv = 4; bdiv < 255; bdiv++) {
        bgen = DIV_ROUND_CLOSEST(clock, baud * (bdiv + 1));
        if (bgen < 2 || bgen > 65535)
            continue;

        calc_baud = clock / (bgen * (bdiv + 1));

        /*
         * Use first calculated baudrate with
         * an acceptable (<3%) error
         */
        if (baud > calc_baud)
            calc_bauderror = baud - calc_baud;
        else
            calc_bauderror = calc_baud - baud;
        if (((calc_bauderror * 100) / baud) < 3)
            break;
    }

    writel(bdiv, dev->base + CDNS_UART_BRDR_REG);
    writel(bgen, dev->base + CDNS_UART_BRGR_REG);

    printf("BDIV: %d; BGEN: %d\n", bdiv, bgen);

    return 0;
}

static int cdns_uart_getc(unsigned instance) 
{
    struct cdns_uart *dev = &uart_dev[instance];

    if (readl(dev->base + CDNS_UART_CSR_REG) & UART_SR_INTR_REMPTY)
        return -EAGAIN;

    return readl(dev->base + CDNS_UART_RXTXFIFO_REG) & 0xFF;
}

static int cdns_uart_putc(unsigned instance, const char c) 
{
    struct cdns_uart *dev = &uart_dev[instance];

    if (readl(dev->base + CDNS_UART_CSR_REG) & UART_SR_INTR_TFUL)
        return -EAGAIN;

    writel((u8)c, dev->base + CDNS_UART_RXTXFIFO_REG);

    return 0;
}

struct ax_uart_ops uart = {
    .init = cdns_uart_init,
    .exit = cdns_uart_exit,
    .setbaud = cdns_uart_setbaud,
    .putc = cdns_uart_putc,
    .getc = cdns_uart_getc,
};
