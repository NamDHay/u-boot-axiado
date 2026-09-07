// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>

#include "ax_diag.h"

#include "ax_spi.h"
#include "ax_pinmux.h"

#include "dwc_spi.h"
#include "dwc_spi_regs.h"

#define SPI_SPEED_25MHZ 0x14
#define SPI_SPEED_50MHZ 0xA
#define SPI_SPEED_60MHZ 0x8
#define SPI_SPEED_80MHZ 0x6
#define SPI_SPEED_125MHZ 0x4

#define TX_FIFO_SZ 0x4 
#define RX_FIFO_SZ 0x4 

#define SPI_NDF_SIZE(len, dfs) ((len) ? (((len * 8) / (dfs + 1)) - 1) : 0)

#define NUM_WORDS 12

#define CONFIG_SPI_CLK 400000000
#define READ_FLASH_STATUS_CMD 0x70

#define SPI_MAX_DEV 7

#define DUMMY_VAL   0x0

/**
  @brief spi controller instance to operate one spi device
  */
struct spi_ctrl_t {
    void __iomem *base; 
    uint32_t chipsel; 
    uint32_t scdr; 
    uint8_t is_ctrl; 
    uint8_t is_dws; 
    uint8_t ctrl_mode;
    uint8_t txbuf[TX_FIFO_SZ] __attribute__((aligned(4)));
    uint8_t rxbuf[RX_FIFO_SZ] __attribute__((aligned(4)));
    u32 fifo_len;
};
static struct spi_ctrl_t spi_dev;

static void __iomem *spi_get_base(unsigned int bus)
{
    if (bus > SPI_MAX_DEV || bus == 4)
        return NULL;
    
    switch (bus) {
        case 0:
            return (void __iomem *)AX3000_CSR_BASE_ADRS_SPI_0;
        case 1:
            return (void __iomem *)AX3000_CSR_BASE_ADRS_SPI_1;
        case 2:
            return (void __iomem *)AX3000_CSR_BASE_ADRS_SPI_2;
        case 3:
            return (void __iomem *)AX3000_CSR_BASE_ADRS_SPI_3;
        case 5:
            return (void __iomem *)AX3000_CSR_BASE_ADRS_SPI_5;
        case 6:
            return (void __iomem *)AX3000_CSR_BASE_ADRS_SPI_6;
        default:
            return NULL;
    }
}

static int spi_flush_fifo(struct spi_ctrl_t *spi)
{
    if (!spi)
        return -EINVAL;

    volatile u8 fifo_count = 0;

    do {
		fifo_count = readl(spi->base + TXFLR);
	} while (fifo_count != 0);

    do {
		fifo_count = readl(spi->base + RXFLR);
	} while (fifo_count != 0);

    return 0;
}

static int spi_dw_set_rxftlr(struct spi_ctrl_t *spi, size_t len)
{
    if (!spi) 
        return -EINVAL;

    uint32_t rxfltr = (len >= SPI_RX_BLOCK) ? ((SPI_RX_BLOCK / 4)) : (len / 4);
    if (rxfltr != 0) {
        rxfltr = ((rxfltr - 1));
    }

    /* Set Rx Threshold */
    writel(rxfltr, spi->base + RXFTLR);

    return 0;
}

static int spi_dw_set_txftlr(struct spi_ctrl_t *spi, size_t len)
{
    if (!spi) 
        return -EINVAL;

    uint32_t txfltr = (len >= SPI_TX_BLOCK) ? ((SPI_TX_BLOCK / 4)) : (len / 4);
    if (txfltr != 0) {
        txfltr = ((txfltr - 1) << TXFTLR_TXFTHR_BitAddressOffset);
    }

    /* Set Tx Threshold */
    writel(txfltr, spi->base + TXFTLR);

    return 0;
}

static inline void spi_dw_get_rx_counter(struct spi_ctrl_t *spi, uint8_t *cnt)
{
		*cnt = readb(spi->base + RXFLR);
}

static inline void spi_dw_get_rx_threshold(struct spi_ctrl_t *spi, uint8_t *rxftlr)
{
		*rxftlr = readl(spi->base + RXFTLR);
}

static inline void spi_dw_get_tx_counter(struct spi_ctrl_t *spi, uint8_t *cnt)
{
		*cnt = readb(spi->base + TXFLR);
}

static int spi_write_tx_fifo(struct spi_ctrl_t *spi, uint32_t data, uint8_t count)
{
    if (spi == NULL) 
        return -EINVAL;

    /* populate 4 bytes to tx fifo */
    if (1 == count) {
        writeb(data, spi->base + DR0);
    } else if (2 == count) {
        writew(data, spi->base + DR0);
    } else {
        writel(data, spi->base + DR0);
    }
    return 0;
}

static int spi_read_rx_fifo(struct spi_ctrl_t *spi, void *dst, size_t size)
{
    if ((spi == NULL) || (dst == NULL)) 
        return -EINVAL;

    if (size == 0) {
        printf("Spi read from rx fifo aborted. size %ld.\n", size);
        return -EINVAL;
    }

    /* Read 4 bytes from spi fifo if rx counter has bytes to read */
    uint8_t rx_cnt = 0;
    while(!rx_cnt){
        spi_dw_get_rx_counter(spi, &rx_cnt);
    }

    if (rx_cnt >= SPI_MIN_RXFTLR) {
        *(uint32_t *)dst = readl(spi->base + DR0);
    }

    return 0;
}

static int spi_controller_init(struct spi_ctrl_t *spi, unsigned int speed)
{
    u32 ctrlr0;
    u16 clk_div;
    int ret;

    if (!spi)
        return -EINVAL;

    /* Disable SSI */
    writel(0x0, spi->base + SSIENR);

    /* Zero out CTRL Register-0 */
    writel(0x0, spi->base + CTRLR0);

    ctrlr0 = (SPI_CTRL0_IS_MST_CONTROLLER | DFS_32_BIT);
    writel(ctrlr0, spi->base + CTRLR0);
    writel(0xFF, spi->base + CTRLR1);

    /* Set clock divider */
    clk_div = CONFIG_SPI_CLK / speed;
    clk_div /= 2;
    clk_div = (clk_div + 1) & 0xfffe;
    writel(clk_div, spi->base + BAUDR);
    spi->scdr = clk_div;
    printf("dw_spi:0x%8p speed=%d clk_div=%d\n", spi->base, CONFIG_SPI_CLK, clk_div);

    /* Set Tx/Rx FIFO Threshold Level */
    writel((SPI_MIN_TXFTLR - 1), spi->base + TXFTLR);
    writel((SPI_MIN_RXFTLR - 1), spi->base + RXFTLR);

    /* Write to Mask Reg to Enable Done Interrupt */
    writel(SPI_IMR_ALL, spi->base + IMR);

    /* Set the Chip Select */
    writel(BIT(spi->chipsel), spi->base + SER);

    /* Enable SSI */
    writel(0x1, spi->base + SSIENR);

    ret = spi_flush_fifo(spi);
    if (ret) {
        printf("%s - FIFO flush failed\n", __func__);
        return ret;
    }

    return 0;
}

static int spi_reconfig(struct spi_ctrl_t *spi, struct spi_dw_config_t *cfg)
{
    uint32_t l_ctrlr0 = SPI_CTRL0_IS_MST_CONTROLLER;
    uint32_t l_ctrlr1 = 0x0;
    if (!spi || !cfg)
        return -EINVAL;

    /* disable spi */
    writel(0x0, spi->base + SSIENR);

    uint32_t dfs = (SPI_DFS_SIZE(cfg->rx_len) & DFS_32_BIT);
    l_ctrlr0 |= dfs;

    /* set transfer mode */
    l_ctrlr0 |= (cfg->tmod << SPI_CTRL0_TMOD_0);
    /* if transmit mode is rx, set number of data frame */
    if (cfg->tmod == SPI_TMOD_RX) {
        /* compute ndf */
        l_ctrlr1 = SPI_NDF_SIZE(cfg->rx_len, dfs);
        writel((cfg->adrs_len | cfg->inst_len | cfg->wait_cycle),
                spi->base + SPI_CTRLR0);
    } else {
        /* keep the default */
        writel(SPI_CTRLR0_INST_L_8, spi->base + SPI_CTRLR0);
    }

    writel(l_ctrlr0, spi->base + CTRLR0);
    writel(l_ctrlr1, spi->base + CTRLR1);

    /* enable ssi */
    writel(0x1, spi->base + SSIENR);

    spi_dw_set_txftlr(spi, cfg->tx_len);
    spi_dw_set_rxftlr(spi, cfg->rx_len);

    return 0;
}

static int spi_controller_dw_xfer_complete(struct spi_ctrl_t *spi)
{
    u32 temp;

    if (!spi)
        return -EINVAL;

    while (1) {
        /* Check if the device is not busy or Transmit fifo is empty */
        temp = readl(spi->base + SR); // Read SR
        if (((temp & SPI_SR_BUSY) != SPI_SR_BUSY) || ((temp & SPI_SR_TFE) == SPI_SR_TFE)) {
            break;
        }

        /* Check if there is transmit fifo empty interrupt */
        temp = readl(spi->base + ISR); // Read ISR
        if ((temp & SPI_ISR_TXEIS) == SPI_ISR_TXEIS) {
            break;
        }
    }

    return 0;
}

static int __hal_internal_spi_controller_tx(struct spi_ctrl_t *spi, const uint8_t *src, size_t src_len,
												  uint32_t dummy_len)
{
    uint32_t src_idx = 0;
    uint32_t total_writes = src_len + dummy_len;
    uint8_t tx_cnt = 0;
    int ret;

    if (!spi)
        return -EINVAL;

    spi_dw_get_tx_counter(spi, &tx_cnt);

    if ((dummy_len + src_len + tx_cnt) > SPI_TX_BLOCK) 
        return -ERANGE;

    uint8_t cur_write = 0x0;
    /* This loop will write a max of TX_BLOCK to tx fifo from src array */
    for (size_t src_cnt = 0; src_cnt < total_writes; src_cnt += TX_FIFO_SZ) {
        for (tx_cnt = 0; tx_cnt < TX_FIFO_SZ; tx_cnt++) {
            if (src_idx < src_len) {
                spi->txbuf[tx_cnt] = src[src_idx++];
            } else if (dummy_len-- != 0) {
                spi->txbuf[tx_cnt] = DUMMY_VAL;
            }

            cur_write++;
        }
        ret = spi_write_tx_fifo(spi, *(uint32_t *)((void *)spi->txbuf), cur_write);
        if (ret) {
            printf("SPI controller was not able to write to tx fifo.\n");
            return -EACCES;
        }
        cur_write = 0;
    }

    return 0;
}

static int __hal_internal_spi_controller_rx(struct spi_ctrl_t *spi, uint8_t *dst, size_t dst_len,
        uint32_t dummy_len)
{
    int ret;
    uint32_t dst_idx = 0;
    uint32_t total_reads = dst_len + dummy_len;

    if (!spi)
        return -EINVAL;

    uint8_t dummy_word = (dummy_len / RX_FIFO_SZ);
    uint8_t dummy_byte = (dummy_len % RX_FIFO_SZ);

    bool first_frame = true;
    /* Read a max of RX_BLOCK from rx fifo and populate dst array */
    for (size_t dst_cnt = 0; dst_cnt < total_reads; dst_cnt += RX_FIFO_SZ) {
        ret = spi_read_rx_fifo(spi, spi->rxbuf, RX_FIFO_SZ);
        if (ret) {
            printf("SPI controller was not able to read bytes from rx fifo.\n");
            return -EACCES;
        }
        /* Skip word containing only dummy byte */
        if (dummy_word != 0) {
            dummy_word--;
        } else {
            /*
             * Leaving dummy bytes, start copying the required data 
             * The following 2 loop ensure that the data order is preserved. 
             * For e.g if there are 2 dummy bytes in the received 4 bytes, 
             * lower 2 bytes contains the 2 bytes of MSB of the data and 
             * the 2 byte LSB of data will be in next fifo read.
             * 
             */
            if (dst_idx < (dst_len + dummy_byte)) {
                if (dst_len > dst_idx) {
                    for (uint8_t lc = 0; lc < (RX_FIFO_SZ - dummy_byte); lc++) {
                        dst[dst_idx + dummy_byte + lc] = spi->rxbuf[lc];
                    }
                }

                if (false == first_frame) {
                    for (uint8_t lc = 0; lc < dummy_byte; lc++) {
                        dst[dst_idx - RX_FIFO_SZ + lc] = spi->rxbuf[RX_FIFO_SZ - dummy_byte + lc];
                    }
                } else {
                    first_frame = false;
                }
                dst_idx += 4;
            }
        }
    }

    return 0;
}

static int spi_controller_xfer(struct spi_ctrl_t *spi, const uint8_t *src, size_t src_len, uint32_t src_dummy,
        uint8_t *dst, size_t dst_len, uint32_t dst_dummy) 
{
    int ret;

    if (!spi || (!src && !dst))
        return -EINVAL;

    if ((0 == src_len) && (0 == dst_len))
        return -ERANGE;

    struct spi_dw_config_t config = {
        .rx_len = (dst_len + dst_dummy),
        .tx_len = (src_len + src_dummy),

        /* Set for RX mode only */
        .adrs_len = SPI_CTRLR0_ADDR_L_32,
        .inst_len = SPI_CTRLR0_INST_L_8,
        .wait_cycle = SPI_CTRLR0_WAIT_CYCLES_8,
    };

    if (src && dst)
        config.tmod = SPI_TMOD_TXRX;
    else if (src)
        config.tmod = SPI_TMOD_RX;
    else if (dst)
        config.tmod = SPI_TMOD_TX;

    printf("%s: 0x%8p tmod:%d rx_len %d tx_len %d\n", __func__, spi->base, config.tmod, config.rx_len, config.tx_len);

    ret = spi_reconfig(spi, &config);
    if (ret) {
        printf("%s - reconfigure failed\n", __func__);
        return ret;
    }

    /* as a defensive check, flush fifo's here, in case rx fifo has any outstanding data */
    ret = spi_flush_fifo(spi);
    if (ret) {
        printf("%s - FIFO flush failed\n", __func__);
        return ret;
    }
    printf("DUT 1\n");

    /* First populate n amount of bytes to the tx fifo */
    ret = __hal_internal_spi_controller_tx(spi, src, src_len, src_dummy);
    if (ret) {
        printf("%s - SPI controller transfer failed\n", __func__);
        return ret;
    }

    /* After populating tx fifo we should transfer our bytes */
    ret = spi_controller_dw_xfer_complete(spi);
    if (ret) {
        printf("%s - transfer failed\n", __func__);
        return ret;
    }
    printf("DUT 2\n");

    /* Receive all the bytes you can in return */
    ret = __hal_internal_spi_controller_rx(spi, dst, dst_len, dst_dummy);
    if (ret) {
        printf("%s - SPI controller receive failed\n", __func__);
        return ret;
    }
    printf("DUT 3\n");
    /* Preventative measure flush fifo's */

    ret = spi_flush_fifo(spi);
    if (ret) {
        printf("%s - FIFO flush failed\n", __func__);
        return ret;
    }
    printf("DUT 4\n");

    if (config.tmod != SPI_TMOD_RX) {
        spi_dw_set_txftlr(spi, 0);
        spi_dw_set_rxftlr(spi, 0);
    }

    return 0;
}

static int dw_spi_setup(unsigned int bus, unsigned int cs,
        unsigned int max_hz, unsigned int mode) 
{
    int ret;
    u32 version;
    char name[5];

    spi_dev.base = spi_get_base(bus);
    if (!spi_dev.base)
        return -EINVAL;

    sprintf(name, "SPI%d", bus);
    ax_pinmux_set_state(name);

    version = readl(spi_dev.base + SSIC_VERSION_ID);
    printf("SPI_%d:0x%8p ssi_version_id=%c.%c%c%c\n",
            bus, spi_dev.base,
            version >> 24, version >> 16, version >> 8, version);

    spi_dev.chipsel = cs;
    ret = spi_controller_init(&spi_dev, max_hz);
    if (ret) {
        printf("spi controller init failed\n");
    }

    return ret;
}

static int dw_spi_xfer(unsigned int bus, unsigned int bitlen, const void *dout,
        void *din) 
{
    int ret;

    spi_dev.base = spi_get_base(bus);
    if (!spi_dev.base)
        return -EINVAL;

    ret = spi_controller_xfer(&spi_dev, dout, bitlen, 0, din, bitlen, 0);
    if (ret) {
        printf("spi_controller_xfer failed\n");
    }

    return ret;
}

struct ax_spi_ops spi = {
    .setup = dw_spi_setup,
    .xfer = dw_spi_xfer,
};
