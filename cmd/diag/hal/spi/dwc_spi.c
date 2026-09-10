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

#include "ax_diag.h"

#include "ax_spi.h"
#include "ax_pinmux.h"

#include "dwc_spi.h"
#include "dwc_spi_regs.h"

#define TX_FIFO_SZ 0x4 
#define RX_FIFO_SZ 0x4 

#define SPI_NDF_SIZE(len, dfs) ((len) ? (((len * 8) / (dfs + 1)) - 1) : 0)
#define CONFIG_SPI_CLK 400000000

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif /* #ifndef MIN */

#define READ_FLASH_STATUS_CMD 0x70

#define SPI_MAX_DEV 7

#define DUMMY_VAL   0x0

/**
  @brief spi controller instance to operate one spi device
  */
struct spi_ctrl_t {
    void __iomem *base; 
    uint32_t chipsel; 
    uint32_t speed; 
    uint8_t is_ctrl; 
    uint8_t is_dws; 
    uint8_t mode;
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
        fifo_count = readb(spi->base + TXFLR);
    } while (fifo_count != 0);

    do {
        fifo_count = readb(spi->base + RXFLR);
    } while (fifo_count != 0);

    return 0;
}

static int spi_dw_set_rxftlr(struct spi_ctrl_t *spi, size_t len)
{
    if (!spi) 
        return -EINVAL;

    /* Disable SSI */
    writel(0x0, spi->base + SSIENR);

    uint32_t rxfltr = (len >= SPI_RX_BLOCK) ? ((SPI_RX_BLOCK / 4)) : ((len + 3) / 4);
    if (rxfltr != 0) {
        rxfltr = ((rxfltr - 1));
    }

    /* Set Rx Threshold */
    writel(rxfltr, spi->base + RXFTLR);

    /* Enable SSI */
    writel(0x1, spi->base + SSIENR);

    return 0;
}

static int spi_dw_set_txftlr(struct spi_ctrl_t *spi, size_t len)
{
    if (!spi) 
        return -EINVAL;

    /* Disable SSI */
    writel(0x0, spi->base + SSIENR);

    uint32_t txfltr = (len >= SPI_TX_BLOCK) ? ((SPI_TX_BLOCK / 4)) : ((len + 3) / 4);
    if (txfltr != 0) {
        txfltr = ((txfltr - 1) << TXFTLR_TXFTHR_BitAddressOffset);
    }

    /* Set Tx Threshold */
    writel(txfltr, spi->base + TXFTLR);

    /* Enable SSI */
    writel(0x1, spi->base + SSIENR);

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

    /* mdelay(5); */
    /* populate 4 bytes to tx fifo */
    if (1 == count) {
        writeb((data >> 24) & 0xff,
                spi->base + DR0);
    } else if (2 == count) {
        writew((data >> 16) & 0xffff,
                spi->base + DR0);
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
    /* printf("recv count = 0x%08x\r\n", rx_cnt); */

    if (rx_cnt >= SPI_MIN_RXFTLR) {
        *(uint32_t *)dst = readl(spi->base + DR0);
    }

    return 0;
}

static int spi_controller_init(struct spi_ctrl_t *spi)
{
    u32 ctrlr0;
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

    /* Set Tx/Rx FIFO Threshold Level */
    writel((SPI_MIN_TXFTLR - 1), spi->base + TXFTLR);
    writel((SPI_MIN_RXFTLR - 1), spi->base + RXFTLR);

    /* Write to Mask Reg to Enable Done Interrupt */
    writel(0x0, spi->base + IMR);

    /* Set the Chip Select */
    writel(1 << spi->chipsel, spi->base + SER);

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

    spi_dw_set_txftlr(spi, cfg->tx_len);
    spi_dw_set_rxftlr(spi, cfg->rx_len);

    /* enable ssi */
    writel(0x1, spi->base + SSIENR);

    return 0;
}

static int spi_controller_set_speed(struct spi_ctrl_t *spi)
{
    if (!spi)
        return -EINVAL;

    /* Disable SSI */
    writel(0x0, spi->base + SSIENR);

    /* Set clock divider */
    u16 clk_div;
    clk_div = CONFIG_SPI_CLK / spi->speed;
    clk_div /= 2;
    clk_div = (clk_div + 1) & 0xfffe;
    writel(clk_div, spi->base + BAUDR);
    printf("%s: speed=%d clk_div=0x%x\n", __func__, CONFIG_SPI_CLK, clk_div);

    /* Enable SSI */
    writel(0x1, spi->base + SSIENR);

    return 0;
}

static int spi_controller_set_mode(struct spi_ctrl_t *spi)
{
    if (!spi)
        return -EINVAL;

    /* Disable SSI */
    writel(0x0, spi->base + SSIENR);

    switch (spi->mode) {
        case SPI_MODE_0:
            /* SPOL = 0, SCPH = 0  */
            clrbits_le32(spi->base + CTRLR0, SPI_CTRL0_SCPH);
            clrbits_le32(spi->base + CTRLR0, SPI_CTRL0_SPOL);
            break;

        case SPI_MODE_1:
            /* SPOL = 0, SCPH = 1  */
            setbits_le32(spi->base + CTRLR0, SPI_CTRL0_SCPH);
            clrbits_le32(spi->base + CTRLR0, SPI_CTRL0_SPOL);
            break;

        case SPI_MODE_2:
            /* SPOL = 1, SCPH = 0 */
            clrbits_le32(spi->base + CTRLR0, SPI_CTRL0_SCPH);
            setbits_le32(spi->base + CTRLR0, SPI_CTRL0_SPOL);
            break;

        case SPI_MODE_3:
            /* SPOL = 1, SCPH = 1 */
            setbits_le32(spi->base + CTRLR0, SPI_CTRL0_SCPH);
            setbits_le32(spi->base + CTRLR0, SPI_CTRL0_SPOL);
            break;

        case SPI_INVALID_MODE:
        default:
            printf("Invalid SPI mode: %d", spi->mode);
            return -EINVAL;
            break;
    }

    printf("%s: mode %d\n", __func__, spi->mode);
    /* Enable SSI */
    writel(0x1, spi->base + SSIENR);

    return 0;
}

static int spi_controller_set_cs(struct spi_ctrl_t *spi)
{
    if (!spi)
        return -EINVAL;

    /* Disable SSI */
    writel(0x0, spi->base + SSIENR);

    /* Set the Chip Select */
    writel(1 << spi->chipsel, spi->base + SER);
    printf("%s: slave %d select\n", __func__, spi->chipsel);

    /* Enable SSI */
    writel(0x1, spi->base + SSIENR);

    return 0;
}

static int spi_controller_dw_xfer_complete(struct spi_ctrl_t *spi)
{
    u32 sr;
    ulong start = get_timer(0);

    if (!spi)
        return -EINVAL;

    while (get_timer(start) < 5000) {
        sr = readl(spi->base + SR);

        if ((sr & SPI_SR_TFE) &&
                !(sr & SPI_SR_BUSY))
            return 0;
    }

    return -ETIMEDOUT;
}

static int __hal_internal_spi_controller_tx(struct spi_ctrl_t *spi, const uint8_t *src, size_t src_len)
{
    u32 src_idx = 0;
    u8 tx_cnt = 0;
    int ret;

    if (!spi || (!src && src_len))
        return -EINVAL;

    spi_dw_get_tx_counter(spi, &tx_cnt);

    if ((src_len + tx_cnt) > SPI_TX_BLOCK) 
        return -ERANGE;

    while (src_idx < src_len) {
        u32 data = 0;
        u8 count = 0;

        /*
         * Pack up to 4 bytes into a u32.
         * Unused bytes remain 0x00.
         */
        while (count < TX_FIFO_SZ && src_idx < src_len) {
            data |= (uint32_t)src[src_idx++] << (count * 8);
            count++;
        }

        ret = spi_write_tx_fifo(spi, data, count);
        if (ret) {
            printf("SPI controller was not able to write to tx fifo.\n");
            return ret;
        }
    }

    return 0;
}

static int __hal_internal_spi_controller_rx(struct spi_ctrl_t *spi,
        uint8_t *dst, size_t dst_len)
{
    size_t dst_idx = 0;
    int ret;
    uint8_t rx_cnt = 0;
    uint8_t rxftlr = 0;

    if (!spi || (!dst && dst_len))
        return -EINVAL;

    if (!dst_len)
        return 0;

    spi_dw_get_rx_threshold(spi, &rxftlr);

    spi_dw_get_rx_counter(spi, &rx_cnt);
    while (rx_cnt < rxftlr) {
        spi_dw_get_rx_counter(spi, &rx_cnt);
    }

    while (dst_idx < dst_len) {
        size_t remaining = dst_len - dst_idx;
        size_t read_len = MIN(RX_FIFO_SZ, remaining);

        ret = spi_read_rx_fifo(spi, spi->rxbuf, RX_FIFO_SZ);
        if (ret) {
            printf("SPI controller was not able to read bytes from rx fifo.\n");
            return ret;
        }

        memcpy(dst + dst_idx, spi->rxbuf, read_len);
        dst_idx += read_len;
    }

    return 0;
}

static int spi_controller_xfer(struct spi_ctrl_t *spi, const uint8_t *src, size_t src_len,
        uint8_t *dst, size_t dst_len) 
{
    int ret;

    if ((NULL == spi) || ((NULL == src) && (NULL == dst)))
        return -EINVAL;

    if (src_len == 0) 
        return -ERANGE;

    if (src_len > SPI_TX_BLOCK) 
        return -ERANGE;

    struct spi_dw_config_t config = {
        .rx_len = (dst_len),
        .tx_len = (src_len),
        .adrs_len = SPI_CTRLR0_ADDR_L_32,
        .inst_len = SPI_CTRLR0_INST_L_8,
        .wait_cycle = SPI_CTRLR0_WAIT_CYCLES_8,
    };

    if ((src_len != 0) && (dst_len != 0))
        config.tmod = SPI_TMOD_TXRX;

    if ((src_len != 0) && (dst_len == 0)) {
        config.tmod = SPI_TMOD_TX;
        config.rx_len = src_len;
        config.tx_len = src_len;
    }

    if (dst_len > src_len)
        config.tmod = SPI_TMOD_RX;

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

    /* First populate n amount of bytes to the tx fifo */
    ret = __hal_internal_spi_controller_tx(spi, src, src_len);
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

    /* Receive all the bytes you can in return */
    if (config.tmod != SPI_TMOD_TX) {
        ret = __hal_internal_spi_controller_rx(spi, dst, dst_len);
        if (ret) {
            printf("%s - SPI controller receive failed\n", __func__);
            return ret;
        }
    }

    if (config.tmod != SPI_TMOD_RX) {
        /* Preventative measure flush fifo's */
        ret = spi_flush_fifo(spi);
        if (ret) {
            printf("%s - FIFO flush failed\n", __func__);
            return ret;
        }

        spi_dw_set_txftlr(spi, 0);
        spi_dw_set_rxftlr(spi, 0);
    }

    return 0;
}

static int dw_spi_init(unsigned int bus) 
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

    ret = spi_controller_init(&spi_dev);
    if (ret) {
        printf("spi controller init failed\n");
    }

    return ret;
}

static int dw_spi_xfer(unsigned int bus, unsigned int tx_len, const void *dout, 
        unsigned int rx_len, void *din) 
{
    int ret;

    spi_dev.base = spi_get_base(bus);
    if (!spi_dev.base)
        return -EINVAL;

    ret = spi_controller_xfer(&spi_dev, dout, tx_len, din, rx_len);
    if (ret) {
        printf("spi_controller_xfer failed\n");
    }

    return ret;
}

static int dw_spi_set_speed(unsigned int bus, uint hz)
{
    int ret;

    spi_dev.base = spi_get_base(bus);
    if (!spi_dev.base)
        return -EINVAL;

    spi_dev.speed = hz;
    ret = spi_controller_set_speed(&spi_dev);
    if (ret) {
        printf("spi controller set speed failed\n");
    }

    return ret;
}

static int dw_spi_set_cs(unsigned int bus, uint cs)
{
    int ret;

    spi_dev.base = spi_get_base(bus);
    if (!spi_dev.base)
        return -EINVAL;

    spi_dev.chipsel = cs;
    ret = spi_controller_set_cs(&spi_dev);
    if (ret) {
        printf("spi controller set slave failed\n");
    }

    return ret;
}

static int dw_spi_set_mode(unsigned int bus, uint mode)
{
    int ret;

    spi_dev.base = spi_get_base(bus);
    if (!spi_dev.base)
        return -EINVAL;

    spi_dev.mode = mode;
    ret = spi_controller_set_mode(&spi_dev);
    if (ret) {
        printf("spi controller set mode failed\n");
    }

    return ret;
}

struct ax_spi_ops spi = {
    .init = dw_spi_init,
    .set_speed = dw_spi_set_speed,
    .set_cs = dw_spi_set_cs,
    .set_mode = dw_spi_set_mode,
    .xfer = dw_spi_xfer,
};
