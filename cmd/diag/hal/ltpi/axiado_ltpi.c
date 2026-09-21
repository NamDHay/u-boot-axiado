// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <linux/delay.h>

#include "ax_diag.h"

#include "ax_ltpi.h"
#include "ax_pinmux.h"
#include "axiado_ltpi_regs.h"

#define LINK_DETECT 	 0x00
#define LINK_SPEED 	 0x01
#define ADVERTISE  	 0x02
#define CONFIGURATION  	 0x03
#define OPERATIONAL  	 0x04
#define FREQ_25MHZ  	 0x0
#define FREQ_50MHZ  	 0x50
#define FREQ_75MHZ  	 0x75
#define FREQ_100MHZ  	 0x100
#define FREQ_150MHZ  	 0x150
#define FREQ_200MHZ  	 0x200
#define FREQ_250MHZ  	 0x250
#define FREQ_300MHZ  	 0x300
#define FREQ_400MHZ  	 0x400
#define FREQ_600MHZ  	 0x600
#define FREQ_800MHZ  	 0x800
#define FREQ_1GHZ  	 0x1000

#define REG_UART_LTPI_SELECT_REG_ADRS_OFFSET 0x118

#define LTPI_TIMEOUT 0x100
#define LTPI_CONFIG_USE_PLL 1

struct ax_ltpi_ctrl_t {
	void __iomem *csr;                /**< LTPI DSUB  base address */
	void __iomem *ctrl_base;          /**< LTPI controller base address */
    void __iomem *tx_phy_base;        /**< LTPI TX PHY base address */
    void __iomem *rx_phy_base;        /**< LTPI RX PHY base address */
    void __iomem *spi_fpga_base;      /**< SPIx fpga wrapper base address */
    void __iomem *uart0_base;         /**< LTPI_UART0 channel base address (Note: Not TCU_UART0) */
    void __iomem *uart1_base;         /**< LTPI_UART1 channel base address (Note: Not TCU_UART1) */
};

static struct ax_ltpi_ctrl_t ltpi_base = {
	.csr = (void __iomem *)AX3000_CSR_BASE_ADRS_DSUB_LTPI,
	.ctrl_base = (void __iomem *)AX3000_CSR_OFFSET_ADRS_LTPI_CNTRL,
	.tx_phy_base = (void __iomem *)AX3000_CSR_OFFSET_ADRS_LTPI_TX_PHY,
	.rx_phy_base = (void __iomem *)AX3000_CSR_OFFSET_ADRS_LTPI_RX_PHY,
};

static inline void axiado_ltpi_write32(void __iomem *base, u32 offset, u32 val)
{
    writel(val, (void __iomem *)(base + offset));
}

static inline u32 axiado_ltpi_read32(void __iomem *base, u32 offset)
{
    return readl((void __iomem *)(base + offset));
}

static int ltpi_clear_error_flag(struct ax_ltpi_ctrl_t *ltpi)
{
    uint32_t reg;

    /* Clear bit 1 -> 5 */
    axiado_ltpi_write32(ltpi->ctrl_base, REG_LTPI_LINK_STATUS_ADRS_OFFSET,
            LTPI_LINK_STATUS_CLEAR_MASK);

    mdelay(100);

    /* Wait until bits are cleared */
    uint32_t timeout = LTPI_TIMEOUT;
    do {
        reg = axiado_ltpi_read32(ltpi->ctrl_base,
                REG_LTPI_LINK_STATUS_ADRS_OFFSET);

        if ((reg & LTPI_LINK_STATUS_CLEAR_MASK) == 0)
            return 0;

    } while (--timeout);

    printf("Timeout: Error flags are not cleared\r\n");

    return -ETIMEDOUT;
}

static int ltpi_tx_phy_init(struct ax_ltpi_ctrl_t *ltpi)
{
    uint32_t offset, reg_val;
    printf("Check Tx base = 0x%8p\r\n", ltpi->tx_phy_base);

    /*
    Step1: Power up.
    Step2: Send 0xf8 to reg08[31:24] to enable internal PLL and lanes.
    Step3: Send 0x12 to reg00[15:8] to disable internal logic.
    Step4: Wait some intervals for PLL lock.
    Step5: Send 0x92 to reg00[15:8] to enable internal logic.
    Step6: Wait a period before normal transmission.
    */

    /* Send 0xf8 to reg08[31:24] to enable internal PLL and lanes. */
    offset = 0x08;
    reg_val = axiado_ltpi_read32(ltpi->tx_phy_base, offset);
    reg_val &= ~(0xFF << 24);
    reg_val |= (0xF8 << 24);
    axiado_ltpi_write32(ltpi->tx_phy_base, offset, reg_val);

    /* Send 0x12 to reg00[15:8] to disable internal logic. */
    offset = 0x00;
    reg_val = axiado_ltpi_read32(ltpi->tx_phy_base, offset);
    reg_val &= ~(0xFF << 8);
    reg_val |= (0x12 << 8);
    axiado_ltpi_write32(ltpi->tx_phy_base, offset, reg_val);

    /* Wait some intervals for PLL lock. */
    printf("Polling for TX PLL locked...\r\n");
    uint32_t timeout = LTPI_TIMEOUT;
    offset = 0x78;
    do {
        reg_val = axiado_ltpi_read32(ltpi->ctrl_base, offset);
        if (reg_val & (1U << 0x0)) {
            printf("TX PLL LOCKED\r\n");
            break;
        }
        mdelay(10);
    } while (--timeout);
    if (timeout <= 0) {
        printf("[Time out]: TX PLL lock timeout!\r\n");
        return -ETIMEDOUT;
    }

    /* Send 0x92 to reg00[15:8] to enable internal logic. */
    offset = 0x00;
    reg_val = axiado_ltpi_read32(ltpi->tx_phy_base, offset);
    reg_val &= ~(0xFF << 8);
    reg_val |= (0x92 << 8);
    axiado_ltpi_write32(ltpi->tx_phy_base, offset, reg_val);

    /* Wait a period before normal transmission. */
    mdelay(10);

    return 0;
}

static inline uint32_t ltpi_get_rx_phy_offset(uint32_t offset_first,
        uint32_t offset_second)
{
    offset_first &= 0x3; // keep only 2 bits
    offset_second &= 0x1F; // keep only 5 bits

    return (offset_first << 5) | offset_second;
}

static int ltpi_rx_phy_init(struct ax_ltpi_ctrl_t *ltpi)
{
    printf("Check Rx base = 0x%8p\r\n", ltpi->rx_phy_base);

#ifdef LTPI_CONFIG_USE_PLL
    /* Using PLL */
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x00, 0x027FE3FD); // pre_div = 2
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x04, 0x00207F02); // fb_div = 2
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x1C, 0x0000000A); // post_div = 10
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x10, 0x04F98000); // digtal clock from PLL
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x1C, 0x0000020A); // set reg_pll_outclken
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x00, 0x027FE2FD); // power PLL
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x40, 0xFC78100C); // external alignment, lsb
#else
    /* Bypass PLL */
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x00, 0x027FE3FD); // pre_div = 2
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x04, 0x00007F02); // fb_div = 2
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x1C, 0x0000000A); // post_div = 10
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x10,
            0x04F88000); // digtal clock from clk lane
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x00, 0x027FE2FD);
    axiado_ltpi_write32(ltpi->rx_phy_base, 0x40, 0xFC76100C); //external alignment, lsb
#endif

    printf("Polling for RX PLL locked...\r\n");
    uint32_t timeout = LTPI_TIMEOUT;
    uint32_t reg_val;
    do {
        reg_val = axiado_ltpi_read32(ltpi->ctrl_base, 0x78);
        if (reg_val & (1U << 14)) {
            printf("RX PLL LOCKED\r\n");
            return 0;
        }
        mdelay(10);
    } while (--timeout);
    if (timeout <= 0) {
        printf("[Time out]: RX PLL lock timeout!\r\n");
    }
    return -ETIMEDOUT;
}

static int ltpi_phy_init(struct ax_ltpi_ctrl_t *ltpi)
{
    int ret;
    ret = ltpi_tx_phy_init(ltpi);
    if (ret) {
        printf("LTPI TX PHY init failed\r\n");
        return ret;
    }
    printf("LTPI TX PHY init completed\r\n");

    ret = ltpi_rx_phy_init(ltpi);
    if (ret) {
        printf("LTPI RX PHY init failed\r\n");
        return ret;
    }
    printf("LTPI RX PHY init completed\r\n");

    return 0;
}

static int ltpi_controller_init(struct ax_ltpi_ctrl_t *ltpi)
{
    uint32_t reg_val;

    /* rx_serdes_parallel_data_reverse */
    reg_val = axiado_ltpi_read32(ltpi->ctrl_base, 0x78);
    reg_val |= (1 << 16) | (1 << 15);
    axiado_ltpi_write32(ltpi->ctrl_base, 0x78, reg_val);

    /* bypass PLL locked check logic */
    reg_val = axiado_ltpi_read32(ltpi->ctrl_base, 0x60);
    reg_val |= (1 << 0);
    axiado_ltpi_write32(ltpi->ctrl_base, 0x60, reg_val);

    /* debug: remove data channel feature to reach Operational */
    axiado_ltpi_write32(ltpi->ctrl_base, 0x14, 0x7F020007);

    printf("LTPI Controller init completed\r\n");
    return 0;
}

static int ltpi_init(void)
{
    struct ax_ltpi_ctrl_t *ltpi = &ltpi_base;
    int ret;
    
    ret = ltpi_phy_init(ltpi);
    if (ret) {
        printf("LTPI PHY init failed\r\n");
        return ret;
    }
    printf("LTPI TX RX PHY init completed\r\n");

    mdelay(1000);

    ret = ltpi_controller_init(ltpi);
    if (ret) {
        printf("LTPI Controller init failed\r\n");
        return ret;
    }

    mdelay(3000);

    return 0;
}

static void ltpi_link_speed(struct ax_ltpi_ctrl_t *ltpi, uint32_t clk_frq)
{
    uint32_t temp;

    temp = axiado_ltpi_read32(
            ltpi->ctrl_base,
            REG_LTPI_DETECT_CAPABILITIES_LOCAL_ADRS_OFFSET); // 0x04
    switch (clk_frq) {
        case FREQ_50MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 9;
            break;
        case FREQ_75MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 10;
            break;
        case FREQ_100MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 11;
            break;
        case FREQ_150MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 12;
            break;
        case FREQ_200MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 13;
            break;
        case FREQ_250MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 14;
            break;
        case FREQ_300MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 15;
            break;
        case FREQ_400MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 16;
            break;
        case FREQ_600MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 17;
            break;
        case FREQ_800MHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 18;
            break;
        case FREQ_1GHZ:
            temp = temp & 0xFFFFFEFF;
            temp = temp | 1 << 19;
            break;
        default:
            temp = temp | 1 << 8;
            break;
    }
    axiado_ltpi_write32(ltpi->ctrl_base,
            REG_LTPI_DETECT_CAPABILITIES_LOCAL_ADRS_OFFSET,
            temp); // Configure clk frequency
}

static void clk_freq_change(struct ax_ltpi_ctrl_t *ltpi, uint32_t clk_frq)
{
    uint32_t freq;
    uint32_t temp;
    switch (clk_frq) {
        case FREQ_50MHZ:
            freq = 0x14;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            break;
        case FREQ_75MHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0x214000;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0x10;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            break;
        case FREQ_100MHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0xF000;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0xA;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            break;
        case FREQ_150MHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0x206000;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0x8;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            temp = axiado_ltpi_read32(ltpi->ctrl_base,
                    0x80); // ltpi_link_retraining_request
            temp = temp | 0x2;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x80, temp);
            break;
        case FREQ_200MHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0x7800;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0x14;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            temp = axiado_ltpi_read32(ltpi->ctrl_base,
                    0x80); // ltpi_link_retraining_request
            temp = temp | 0x2;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x80, temp);
            break;
        case FREQ_250MHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0x6000;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0x4;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            temp = axiado_ltpi_read32(ltpi->ctrl_base,
                    0x80); // ltpi_link_retraining_request
            temp = temp | 0x2;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x80, temp);
            break;
        case FREQ_300MHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0x205000;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0x4;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            temp = axiado_ltpi_read32(ltpi->ctrl_base,
                    0x80); // ltpi_link_retraining_request
            temp = temp | 0x2;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x80, temp);
            break;
        case FREQ_400MHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0x1003C00;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0x4;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            temp = axiado_ltpi_read32(ltpi->ctrl_base,
                    0x80); // ltpi_link_retraining_request
            temp = temp | 0x2;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x80, temp);
            break;
        case FREQ_600MHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0x202800;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0x2;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            temp = axiado_ltpi_read32(ltpi->ctrl_base,
                    0x80); // ltpi_link_retraining_request
            temp = temp | 0x2;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x80, temp);
            break;
        case FREQ_800MHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0x1001E00;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0x2;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            temp = axiado_ltpi_read32(ltpi->ctrl_base,
                    0x80); // ltpi_link_retraining_request
            temp = temp | 0x2;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x80, temp);
            break;
        case FREQ_1GHZ:
            temp = axiado_ltpi_read32(ltpi->ctrl_base, 0xDC); //
            temp = temp | 0x101800;
            axiado_ltpi_write32(ltpi->ctrl_base, 0xDC,
                    temp); // Configure clk frequency
            freq = 0x2;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            temp = axiado_ltpi_read32(ltpi->ctrl_base,
                    0x80); // ltpi_link_retraining_request
            temp = temp | 0x2;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x80, temp);
            break;
        default:
            freq = 0x28;
            freq = (freq << 4) | 0xB;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x0bc,
                    freq); // Configure clk frequency
            temp = axiado_ltpi_read32(ltpi->ctrl_base,
                    0x80); // ltpi_link_retraining_request
            temp = temp | 0x2;
            axiado_ltpi_write32(ltpi->ctrl_base, 0x80, temp);
            break;
    }
}

static void ltpi_reset(void)
{
    struct ax_ltpi_ctrl_t *ltpi = &ltpi_base;
    uint32_t reg;

    /* Trigger LTPI reset software */
    reg = axiado_ltpi_read32(ltpi->ctrl_base, 0x5C); // ltpi_link_control reg
    reg |= (0x1 << 0x0);
    axiado_ltpi_write32(ltpi->ctrl_base, 0x5C, reg);
    mdelay(10);

    /* Release LTPI sw reset software */
    reg = axiado_ltpi_read32(ltpi->ctrl_base, 0x5C); // ltpi_link_control reg
    reg &= ~(0x1 << 0x0);
    axiado_ltpi_write32(ltpi->ctrl_base, 0x5C, reg);

    /* Trigger LTPI reset module */
    reg = axiado_ltpi_read32(ltpi->ctrl_base,
            0x64); // ltpi_control_status_reg1
    reg |= (0x1 << 0x0);
    axiado_ltpi_write32(ltpi->ctrl_base, 0x64, reg);
    mdelay(10);

    /* Release LTPI reset module */
    reg = axiado_ltpi_read32(ltpi->ctrl_base,
            0x64); // ltpi_control_status_reg1
    reg &= ~(0x1 << 0x0);
    axiado_ltpi_write32(ltpi->ctrl_base, 0x64, reg);
}

static int ltpi_wait_for_linkup(struct ax_ltpi_ctrl_t *ltpi, uint32_t timeout)
{
    uint32_t local_rd, remote_rd;
    uint32_t local_st, remote_st;
    uint32_t cnt = timeout;
    int ret;

    local_rd = axiado_ltpi_read32(
            ltpi->ctrl_base,
            REG_LTPI_LINK_STATUS_ADRS_OFFSET); /// local ltpi link state
    local_st = (local_rd >> 16) & 0xF;
    remote_rd = axiado_ltpi_read32(
            ltpi->ctrl_base,
            REG_LTPI_LINK_STATUS_ADRS_OFFSET); /// local ltpi link state
    remote_st = (remote_rd >> 12) & 0xF;

    while (cnt--) {
        local_rd = axiado_ltpi_read32(
                ltpi->ctrl_base,
                REG_LTPI_LINK_STATUS_ADRS_OFFSET); /// remote ltpi link state
        local_st = (local_rd >> 16) & 0xF;
        remote_rd = axiado_ltpi_read32(
                ltpi->ctrl_base,
                REG_LTPI_LINK_STATUS_ADRS_OFFSET); /// remote ltpi link state
        remote_st = (remote_rd >> 12) & 0xF;

        if ((local_st == 0x4) && (remote_st == 0x4)) {
            printf("Local and Remote reach Operational state\r\n");

            ret = ltpi_clear_error_flag(ltpi);
            if (ret) {
                printf("Cannot clear error flags\n");
                return ret;
            }

            return 0;
        }

        mdelay(1);
    }

    return -ETIMEDOUT;
}

static int ltpi_wait_for_link_aligned(struct ax_ltpi_ctrl_t *ltpi, uint32_t timeout)
{
    uint32_t reg;
    uint32_t cnt = timeout;

    while(cnt--) {
        reg = axiado_ltpi_read32(ltpi->ctrl_base, 0x0);

        if (reg & 0x1) {
            printf("Link is aligned\n");
            return 0;
        }

        mdelay(1);
    }
    if (cnt == 0) {
        printf("Link align timeout\n");
    }

    return -ETIMEDOUT;
}

static int ltpi_link_up(u32 timeout)
{
    int ret;
    struct ax_ltpi_ctrl_t *ltpi = &ltpi_base;

    ret = ltpi_wait_for_link_aligned(ltpi, timeout);
    if (ret) {
        printf("LTPI link aligned timeout\r\n");
        return ret;
    }
    printf("LTPI link aligned completed\r\n");

    /* Wait for link up */
    ret = ltpi_wait_for_linkup(&ltpi_base, timeout);
    if (ret) {
        printf("LTPI link up timeout\r\n");
        return ret;
    }
    printf("LTPI link up completed\r\n");

    /* Clear error flags */
    ret = ltpi_clear_error_flag(&ltpi_base);
    if (ret) {
        printf("Cannot clear error flags\n");
        return ret;
    }

    return 0;
}

struct ax_ltpi_ops ltpi = {
    .init = ltpi_init,
    .reset = ltpi_reset,
    .check_link = ltpi_link_up,
};
