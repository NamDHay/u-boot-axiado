// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <malloc.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <time.h>

#include "ax_diag.h"
#include "ax_emmc.h"
#include "arasan_sdhci_reg.h"

struct arasan_emmc_ctrl_t emmc_dev = {
    .phy_base = (void __iomem *)AX3000_CSR_BASE_ADRS_APBS_EMMC,
    .reg_base = (void __iomem *)AX3000_CSR_BASE_ADRS_AHBS_EMMC,
    .version = 1,
};

int arasan_emmc_phy_power_on(struct arasan_emmc_ctrl_t *emmc)
{
    uint32_t val;
    int timedout = 0;

    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_1);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_1,
            val | AX_EMMC_PHY_RETB_ENBL);

    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_3);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_3,
            val | AX_EMMC_PHY_PDB_ENBL);

    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_2);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_2,
            val | AX_EMMC_PHY_OTAPDLY_EN | ((0x2) << 7));

    val = arasan_reg_read32(emmc->phy_base,
            AX_EMMC_PHY_CTRL_2); // dummy read

    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_1);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_1,
            val | ((0x8) << 22) | AX_EMMC_PHY_DLL_ENBL);

    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_STATUS, 0x0);

    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_3);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_3,
            val | ((0x1) << 25));

    while (1) {
        mdelay(MILLISECOND_1);

        if (arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_STATUS) &
                AX_EMMC_PHY_DLL_RDY_MASK)
            break;

        if (timedout == AX_EMMC_PHY_LOOP_TIMEOUT) {
            printf(
                    "eMMC-PHY: DLL_RDY_MASK bit is not cleared.\n");
            return -ETIMEDOUT;
        }
        timedout++;
    }

    return 0;
}

/* API to initialize the eMMC PHY interface, see ax_emmc_api.h for more details */
int arasan_emmc_phy_init(struct arasan_emmc_ctrl_t *emmc)
{
    uint32_t val;
    int timedout = 0;

    printf("Init PHY Start !!!\n");

    /* Enable RETB and RTRIM in PHY_CTRL_1 */
    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_1);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_1,
            val | AX_EMMC_PHY_RETB_ENBL | AX_EMMC_PHY_RTRIM_EN);

    /* Enable PDB in PHY_CTRL_3 */
    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_3);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_3,
            val | AX_EMMC_PHY_PDB_ENBL);

    /* Wait for calibration to complete (max 3000 ms) */
    while (1) {
        mdelay(MILLISECOND_1);

        if (arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_STATUS) &
                AX_EMMC_PHY_CALDONE_MASK)
            break;

        if (timedout == AX_EMMC_PHY_LOOP_TIMEOUT) {
            printf(
                    "eMMC-PHY: CALDONE_MASK bit is not cleared.\n");
            return -ETIMEDOUT;
        }
        timedout++;
    }


    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_1);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_1,
            val | AX_EMMC_PHY_REN_CMD_EN |
            AX_EMMC_PHY_PU_CMD_EN_NEW);

    /* Enable REN_STRB in PHY_CTRL_2 */
    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_2);

    /* Try to enable open drain data pin */
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_2,
            val | AX_EMMC_PHY_REN_STRB);

    /* Enable MAX_CLK buffers in PHY_CTRL_3 */
    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_3);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_3,
            val | AX_EMMC_PHY_MAX_CLK_BUF0 |
            AX_EMMC_PHY_MAX_CLK_BUF1 |
            AX_EMMC_PHY_MAX_CLK_BUF2);

    /* Configure clock multiplier in CAP_REG_IN_S1_MSB */

    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CAP_REG_IN_S1_MSB,
            AX_EMMC_PHY_CAP_REG_IN_S1_MSB_CLK_MULTIPLIER);

    /* Enable RX and TX clock delay selection in PHY_CTRL_3 */
    val = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_3);
    arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_3,
            val | AX_EMMC_PHY_SEL_DLY_RXCLK |
            AX_EMMC_PHY_SEL_DLY_TXCLK);

    return 0;
}

/* API to initialize the eMMC device, see ax_emmc_api.h for more details */
int arasan_emmc_device_init(struct arasan_emmc_ctrl_t *emmc)
{
    int ret;
    uint32_t temp = 0;
    uint32_t ocr = 0;
    uint32_t delay_loop_count = 0;

    if (EMMC_CHECK_VALID(emmc)) {
        printf("eMMC parameter address is invalid.\n");
        printf("eMMC init aborted. emmc: 0x%8p.\n",
                emmc->reg_base);
        return -EINVAL;
    }

    /* Issue CMD0 with argument 0xF0F0F0F0 for device enter into pre IDLE state */
    ret = sd_issue_command(emmc, 0, 0, AX_EMMC_PRE_IDLE_STATE, 0);
    if (ret) {
        printf(
                "eMMC CMD0 Pre IDLE state transfer failed Status:0x%x\n",
                ret);
        return ret;
    }

    /* Issue CMD0 with argument 0 to reset device to IDLE state */
    ret = sd_issue_command(emmc, 0, 0, 0, 0);
    if (ret) {
        printf("eMMC CMD0 transfer failed Status:0x%x\n", ret);
        return ret;
    }
    ocr = AX_EMMC_ARG1_HIGH_VTG_OCR;

    delay_loop_count = DELAY_LOOP_COUNT;
    /* Issue CMD1 with R3,R4 response type
     * Send OCR[Operations condition] to support high capacity and high voltage window
     */
    do {
        ret = sd_issue_command(emmc, 0x0, 0x0, ocr,
                (AX_EMMC_CMDTM_CMD1 |
                 AX_EMMC_CMDTM_RESPONSE_R3R4));
        if (ret) {
            printf("eMMC CMD1 transfer failed Status:0x%x\n",
                    ret);
            return ret;
        }
        temp = arasan_reg_read32(emmc->reg_base, 0x10);
        ocr = temp;
        mdelay(MILLISECOND_1);
    } while ((--delay_loop_count > 0) &&
            ((temp & AX_EMMC_CARD_PWR_UP) == 0x0));
    if (delay_loop_count == 0) {
        printf("Timeout : EMMC card power up status not set \n");
        return -ETIMEDOUT;
    }
    /* Issue CMD2 to ask device to send CID[R2 response] */
    ret = sd_issue_command(emmc, 0x0, 0x0, 0x0,
            (AX_EMMC_CMDTM_CMD2 |
             AX_EMMC_CMDTM_RESPONSE_R2));
    if (ret) {
        printf("eMMC CMD2 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    /* Issue CMD3 to assign relative address to device[R1 response] */
    ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
            (AX_EMMC_CMDTM_CMD3 |
             AX_EMMC_CMDTM_RESPONSE_R1));
    if (ret) {
        printf("eMMC CMD3 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    /* Issue CMD9 with R2 response type ,Transfer block size 512bytes and 1block is sent */
    ret = sd_issue_command(
            emmc, 0x0, (AX_EMMC_1BLOCK | AX_EMMC_BLOCK_SIZE),
            AX_EMMC_ARG1_RELATIVE_ADDR,
            (AX_EMMC_CMDTM_CMD9 | AX_EMMC_CMDTM_RESPONSE_R2));
    if (ret) {
        printf("eMMC CMD9 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    /* Issue CMD7 to select the card */
    ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
            (AX_EMMC_CMDTM_CMD7 |
             AX_EMMC_CMDTM_RESPONSE_R1));
    if (ret) {
        printf("eMMC CMD7 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    // Polling CDM13 for TRANSFER state
    do {
        ret = sd_issue_command(
                emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
                (AX_EMMC_CMDTM_CMD13 | AX_EMMC_CMDTM_RESPONSE_R1));
        if (ret) {
            printf("eMMC CMD13 transfer failed Status:0x%x\n",
                    ret);
            return ret;
        }
        temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_RESP0);

        /* READY_FOR_DATA (bit 8) must be 1 and CURRENT_STATE (bits [12:9]) must be TRAN (4) */
        if ((temp & (1 << 8)) && (((temp >> 9) & 0xF) == 4)) {
            return 0;
        }
        mdelay(MILLISECOND_1);
    } while (--delay_loop_count > 0);

    if (delay_loop_count == 0) {
        printf("Timeout: Card did not return to TRAN state\r\n");
        return -EACCES;
    }

    return 0;
}

/* API to initialize the eMMC controller, see ax_emmc_api.h for more details */
int arasan_emmc_controller_init(struct arasan_emmc_ctrl_t *emmc,
        uint32_t clock_frequency)
{
    uint32_t temp = 0;
    uint32_t delay_loop_count = 0;

    if (EMMC_CHECK_VALID(emmc)) {
        printf("eMMC parameter address is invalid.\n");
        printf("eMMC init aborted. emmc: 0x%8p.\n",
                emmc->reg_base);
        return -EINVAL;
    }

    arasan_reg_write32(emmc->reg_base, AX_EMMC_IRPT_STATUS,
            AX_EMMC_IRPT_STATUS_CARD_EN);
    arasan_reg_write32(emmc->reg_base, AX_EMMC_IRPT_SIGNAL,
            AX_EMMC_IRPT_STATUS_CARD_EN);

    /* Reset the eMMC controller */
    temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);

    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1,
            temp | AX_EMMC_CONTROL1_SW_RESET);

    /* Added basic timeout logic */
    delay_loop_count = DELAY_LOOP_COUNT;
    do {
        temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1) &
            AX_EMMC_CONTROL1_SW_RESET;
        mdelay(MILLISECOND_1);
    } while ((--delay_loop_count > 0) &&
            (temp == AX_EMMC_CONTROL1_SW_RESET));
    if (delay_loop_count == 0) {
        printf("Timeout : Software reset is not done\n");
        return -ETIMEDOUT;
    }

    // printf("Power enable and 1bits mode\n");
    /* Select SD Bus Voltage to 1.8 V */
    temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);

    temp = temp & AX_EMMC_CONTROL0_BUS_VTG_MASK;
    temp = temp |
        (AX_EMMC_CONTROL0_BUS_VTG_1V8 | AX_EMMC_CONTROL0_BUS_8BIT);
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, temp);
    /* Enable auto clock gate */
    arasan_reg_write32(emmc->reg_base, AX_EMMC_VENDOR,
            AX_EMMC_VENDOR_AUTO_CLK_GATE);

    /* Enable internal clock and wait until it is stable */
    temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);

    // printf("Set clock frequency and Internal clock enable\n");
    temp = ((temp & AX_EMMC_CONTROL1_INT_CLK_EN_MASK) | clock_frequency |
            AX_EMMC_CONTROL1_DATA_TOUT);
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1, temp);
    delay_loop_count = DELAY_LOOP_COUNT;
    do {
        temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
        temp = temp & AX_EMMC_CONTROL1_INT_CLK_STABLE;
        mdelay(MILLISECOND_1);
    } while ((--delay_loop_count > 0) && (temp == 0x00000000));
    if (delay_loop_count == 0) {
        printf(
                "Timeout : Unable to reach internal clock stable state\n");
        return -ETIMEDOUT;
    }
    // sdhci_show_speed(emmc);

    arasan_reg_write32(
            emmc->reg_base, AX_EMMC_CONTROL1,
            (arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1) |
             AX_EMMC_CONTROL1_SD_CLK_EN));

    /*Interrupt enable */
    arasan_reg_write32(emmc->reg_base, AX_EMMC_IRPT_STATUS,
            AX_EMMC_IRPT_STATUS_EN);
    arasan_reg_write32(emmc->reg_base, AX_EMMC_IRPT_SIGNAL,
            AX_EMMC_IRPT_STATUS_EN);

    return 0;
}

static void arasan_sdhci_dma_select(struct arasan_emmc_ctrl_t *emmc, u32 dma_mode)
{
    u32 reg;
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
    reg &= ~(0x18); /* clear bit [4:3] */
    reg |= (dma_mode & 0x18); /* set new DMA mode */
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, reg);
}

static int arasan_emmc_init(void)
{
    struct arasan_emmc_ctrl_t *emmc;
    int ret;

    emmc = &emmc_dev;

    ret = arasan_emmc_phy_init(emmc);
    if (ret){
        printf("eMMC PHY init failed\n");
        return ret;
    }

    ret = arasan_emmc_controller_init(emmc, AX_EMMC_CONTROL1_INT_CLK_400K_EN);
    if (ret){
        printf("eMMC controller init failed\n");
        return ret;
    }

    ret = arasan_emmc_device_init(emmc);
    if (ret){
        printf("eMMC device init failed\n");
        return ret;
    }

    arasan_sdhci_dma_select(emmc, AX_EMMC_CONTROL0_DMA_ADMA2_32BIT);

    printf("eMMC init complete, mode ADMA2 32-bit\n");

    return 0;
}

static int arasan_emmc_reset(void)
{
    struct arasan_emmc_ctrl_t *emmc;
    uint32_t delay_loop_count = 0;
    uint32_t temp = 0;

    emmc = &emmc_dev;

    if (EMMC_CHECK_VALID(emmc)) {
        printf("eMMC parameter address is invalid.\n");
        printf("eMMC deinit aborted. emmc: 0x%8p.\n",
                emmc->reg_base);
        return -EINVAL;
    }

    printf("software reset\n");
    /* Reset the eMMC controller */
    temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
    temp |= AX_EMMC_CONTROL1_SW_RESET;
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1, temp);

    /* Added basic timeout logic */
    printf("Wait software reset done\n");
    delay_loop_count = DELAY_LOOP_COUNT;
    do {
        temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1) &
            AX_EMMC_CONTROL1_SW_RESET;
        mdelay(MILLISECOND_1);
    } while ((--delay_loop_count > 0) &&
            (temp == AX_EMMC_CONTROL1_SW_RESET));
    if (delay_loop_count == 0) {
        printf("Timeout : Software reset is not done\n");
        return -ETIMEDOUT;
    }
    printf("eMMC device moved to IDEL state\n");

    return 0;
}

static int arasan_emmc_set_speed(emmc_speed_mode_t speed_mode)
{
    int ret;
    struct arasan_emmc_ctrl_t *emmc = &emmc_dev;

    emmc->sdclk_freq_select = 0x1; /* Clock div = 2, SDCLK = 100MHz */

    switch (speed_mode) {
        case EMMC_MODE_NORMAL:
            ret = emmc_select_normal_mode(emmc);
            if (ret) {
                printf("Normal mode switching FAILED!!!\r\n");
                return ret;
            }
            printf("NORMAL SPEED mode entered\r\n");
            break;

        case EMMC_MODE_HIGH_SPEED:
            ret = emmc_select_high_speed_mode(emmc);
            if (ret) {
                printf("High speed mode switching FAILED!!!\r\n");
                return ret;
            }
            printf("HIGH SPEED mode entered\r\n");
            break;

        case EMMC_MODE_HS200:
            ret = emmc_select_hs200_mode(emmc);
            if (ret) {
                printf("HS200 mode switching FAILED!!!\r\n");
                return ret;
            }
            break;

        case EMMC_MODE_HS400:
            printf("HS400 still in debugging\r\n");
            break;

        default:
            printf("Unknown eMMC speed mode: %d\r\n", speed_mode);
            return -EINVAL;
    }
    emmc->speed_mode = speed_mode;

    return 0;
}

static int arasan_emmc_set_bus_width(emmc_bus_width_t bus_width)
{
    struct arasan_emmc_ctrl_t *emmc = &emmc_dev;
    int ret;

    ret = emmc_hci_bus_width_switch(emmc, bus_width);
    if (ret) {
        printf("Host controller switch bus width failed!!!\n");
        return ret;
    }

    ret = emmc_device_bus_width_switch(emmc, bus_width);
    if (ret) {
        printf("Device switch bus width failed!!!\n");
        return ret;
    }

    return 0;
}

static __maybe_unused int arasan_emmc_write(uint32_t start_block_number, uint32_t blkcnt,
        const void *write_buf)
{
    struct arasan_emmc_ctrl_t *emmc = &emmc_dev;
    int ret;
    uint32_t i = 0;
    uint32_t temp = 0;
    uint32_t blk_size_cnt = 0;
    uint32_t delay_loop_count = 0;
    const uint32_t *src;

    if (EMMC_CHECK_VALID(emmc) || write_buf == NULL) {
        printf("eMMC parameter address is invalid.\n");
        return -EINVAL;
    }

    if (start_block_number > (AX_EMMC_BLOCK_COUNT - 1)) {
        printf("The start block number or end block number is out of range\n");
        return -ERANGE;
    }
    if (blkcnt > AX_EMMC_BLOCK_REG_MAX) {
        printf("Total number of block count value is out of block count register range\n");
        return -ERANGE;
    }

    src = (const uint32_t *)write_buf;

    printf("CMD13\n");
    /* Issue CMD13 to Device sends its status register */
    ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
            (AX_EMMC_CMDTM_CMD13 |
             AX_EMMC_CMDTM_RESPONSE_R1));
    if (ret) {
        printf("eMMC CMD13 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    if (!((emmc->speed_mode = EMMC_MODE_HS400) ||
                (emmc->speed_mode = EMMC_MODE_DDR50))) {
        printf("CMD16\n");
        /* Issue CMD16 to set Block length of 512 bytes */
        ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_BLOCK_SIZE,
                (AX_EMMC_CMDTM_CMD16 |
                 AX_EMMC_CMDTM_RESPONSE_R1));
        if (ret) {
            printf("eMMC CMD16 transfer failed Status:0x%x\n",
                    ret);
            return ret;
        }
    }

    blk_size_cnt = (AX_EMMC_BLOCK_SIZE | (blkcnt << 16));
    printf("CMD25\n");
    /* Issue CMD25 to write multiple blocks of data */
    ret = sd_issue_command(
            emmc, 0x0, blk_size_cnt, start_block_number,
            (AX_EMMC_CMDTM_CMD25 | AX_EMMC_CMDTM_RESPONSE_R1 |
             AX_EMMC_CMDTM_DATA_PRESENT | AX_EMMC_CMDTM_MULTI_BLOCK_EN |
             AX_EMMC_CMDTM_ACMD12_EN));
    if (ret) {
        printf("eMMC CMD25 transfer failed Status:0x%x\n", ret);
        return ret;
    }
    for (uint32_t block = 0;
            block <= blkcnt; block++) {
        delay_loop_count = DELAY_LOOP_COUNT;
        /* This loop will wait for Buffer Read Ready */
        do {
            temp = arasan_reg_read32(emmc->reg_base,
                    AX_EMMC_INTERRUPT);
            temp = temp & AX_EMMC_INTERRUPT_WR_BUF_READY;
            if (temp == 0x0) {
                mdelay(MILLISECOND_1);
            } else {
                break;
            }
        } while (--delay_loop_count > 0);
        if (delay_loop_count == 0) {
            printf("Timeout : Detect write buffer is not in ready state\n");
            return -ETIMEDOUT;
        }
        arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT, temp);
        /* Read  data from Buffer data port register */
        for (i = 0; i < (CONFIG_EMMC_BLOCK_SIZE / 4); i++) {
            arasan_reg_write32(
                    emmc->reg_base, AX_EMMC_DATA,
                    src[((block * CONFIG_EMMC_BLOCK_SIZE) / 4) +
                    i]);
        }
    }
    delay_loop_count = DELAY_LOOP_COUNT;
    do {
        temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
        temp = temp & (AX_EMMC_INTERRUPT_ERROR |
                AX_EMMC_INTERRUPT_CMD_COMPLETE |
                AX_EMMC_INTERRUPT_TRFR_COMPLETE);
        if (temp == AX_EMMC_INTERRUPT_ERROR) {
            printf("eMMC interrupt error:0x%x\n",
                    AX_EMMC_INTERRUPT_ERROR);
            return -EACCES;
        }
        if (temp != AX_EMMC_INTERRUPT_TRFR_COMPLETE) {
            mdelay(MILLISECOND_1);
        } else {
            break;
        }
    } while (--delay_loop_count > 0);
    if (delay_loop_count == 0) {
        printf("Timeout : Transfer complete interrupt is not set\n");
        return -ETIMEDOUT;
    }
    arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT, temp);

    return 0;
}

static __maybe_unused int arasan_emmc_read(uint32_t start_block_number, uint32_t blkcnt,
        void *read_buf)
{
    struct arasan_emmc_ctrl_t *emmc = &emmc_dev;
    uint32_t blk_size_cnt = 0;
    uint32_t delay_loop_count = 0;
    uint32_t i = 0;
    uint32_t temp = 0;
    uint32_t *dst;
    int ret;

    if (EMMC_CHECK_VALID(emmc) || read_buf == NULL) {
        printf("eMMC parameter address is invalid.\n");
        return -EINVAL;
    }

    if (start_block_number > (AX_EMMC_BLOCK_COUNT - 1)) {
        printf("The start block number or  end block number is out of range\n");
        return -ERANGE;
    }
    if (blkcnt > AX_EMMC_BLOCK_REG_MAX) {
        printf("Total number of block count value is out of block count register range\n");
        return -ERANGE;
    }

    dst = (uint32_t *)read_buf;

    printf("CMD13\n");
    /* Issue CMD13 to Device sends its status register */
    ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
            (AX_EMMC_CMDTM_CMD13 |
             AX_EMMC_CMDTM_RESPONSE_R1));
    if (ret) {
        printf("eMMC CMD13 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    printf("CMD16\n");
    /* Issue CMD16 to set Block length of 512 bytes */
    ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_BLOCK_SIZE,
            (AX_EMMC_CMDTM_CMD16 |
             AX_EMMC_CMDTM_RESPONSE_R1));
    if (ret) {
        printf("eMMC CMD16 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    blk_size_cnt = AX_EMMC_BLOCK_SIZE | (blkcnt << 16);
    printf("CMD18\n");
    /* Issue CMD18 to read multiple blocks of data */
    ret = sd_issue_command(
            emmc, 0x0, blk_size_cnt, start_block_number,
            (AX_EMMC_CMDTM_CMD18 | AX_EMMC_CMDTM_RESPONSE_R1 |
             AX_EMMC_CMDTM_DATA_PRESENT | AX_EMMC_CMDTM_MULTI_BLOCK_EN |
             AX_EMMC_CMDTM_DATA_READ | AX_EMMC_CMDTM_ACMD12_EN));

    if (ret) {
        printf("eMMC CMD18 transfer failed Status:0x%x\n", ret);
        return ret;
    }
    for (uint32_t block = 0;
            block <= blkcnt; block++) {
        delay_loop_count = DELAY_LOOP_COUNT;
        /* This loop will wait for Buffer Read Ready */
        do {
            temp = arasan_reg_read32(emmc->reg_base,
                    AX_EMMC_INTERRUPT);
            temp = temp & AX_EMMC_INTERRUPT_RD_BUF_READY;
            if (temp == 0x0) {
                mdelay(MILLISECOND_1);
            } else {
                break;
            }
        } while (--delay_loop_count > 0);
        if (delay_loop_count == 0) {
            printf("Timeout : Detect read buffer is not in ready state\n");
            return -ETIMEDOUT;
        }
        arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT, temp);
        /* Read  data from Buffer data port register */
        for (i = 0; i < (CONFIG_EMMC_BLOCK_SIZE / 4); i++) {
            dst[((block * CONFIG_EMMC_BLOCK_SIZE) / 4) + i] =
                arasan_reg_read32(emmc->reg_base,
                        AX_EMMC_DATA);
        }
    }
    delay_loop_count = DELAY_LOOP_COUNT;
    do {
        temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
        temp = temp & (AX_EMMC_INTERRUPT_ERROR |
                AX_EMMC_INTERRUPT_CMD_COMPLETE |
                AX_EMMC_INTERRUPT_TRFR_COMPLETE);
        if (temp == AX_EMMC_INTERRUPT_ERROR) {
            printf("eMMC interrupt error:0x%x\n",
                    AX_EMMC_INTERRUPT_ERROR);
            return -EACCES;
        }
        if (temp != AX_EMMC_INTERRUPT_TRFR_COMPLETE) {
            mdelay(MILLISECOND_1);
        } else {
            break;
        }
    } while (--delay_loop_count > 0);
    if (delay_loop_count == 0) {
        printf("Timeout : Transfer complete interrupt is not set\n");
        return -ETIMEDOUT;
    }
    arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT, temp);

    return 0;
}


static int arasan_emmc_dma_write(uint32_t start_block_number, uint32_t blkcnt,
        const void *write_buf)
{
    struct arasan_emmc_ctrl_t *emmc = &emmc_dev;
    struct desp_t tx_desp_table;
    uint32_t blk_size_cnt = 0;
    uint16_t total_size = blkcnt * AX_EMMC_BLOCK_COUNT;
    uint32_t delay_loop_count = 0;
    uint32_t dma_mode;
    uint32_t dma_addr = 0;
    uint32_t next_dma_addr = 0;
    uint32_t temp = 0;
    int ret;

    if (EMMC_CHECK_VALID(emmc) || write_buf == NULL) {
        printf("eMMC parameter address is invalid.\n");
        return -EINVAL;
    }

    if (start_block_number > (AX_EMMC_BLOCK_COUNT - 1)) {
        printf(
                "The start block number or end block number is out of range\n");
        return -ERANGE;
    }
    if (blkcnt > AX_EMMC_BLOCK_REG_MAX) {
        printf(
                "Total number of block count value is out of block count register range\n");
        return -ERANGE;
    }

    tx_desp_table.line.addr = (uint32_t)(uintptr_t)write_buf;
    tx_desp_table.line.length = total_size;
    tx_desp_table.line.attr = ADMA_VALID | ADMA_ACT_TRAN | ADMA_END;

    /* Issue CMD13 to Device sends its status register */
    ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
            (AX_EMMC_CMDTM_CMD13 |
             AX_EMMC_CMDTM_RESPONSE_R1));
    if (ret) {
        printf("eMMC CMD13 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    /* Issue CMD16 to set Block length of 512 bytes */
    if (!((emmc->speed_mode == EMMC_MODE_HS400) ||
                (emmc->speed_mode == EMMC_MODE_DDR50))) {
        ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_BLOCK_SIZE,
                (AX_EMMC_CMDTM_CMD16 |
                 AX_EMMC_CMDTM_RESPONSE_R1));
        if (ret) {
            printf("eMMC CMD16 transfer failed Status:0x%x\n",
                    ret);
            return ret;
        }
    }

    blk_size_cnt = (AX_EMMC_BLOCK_SIZE | (blkcnt << 16));

    /* Configure DMA */
    dma_mode = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
    dma_mode &= (0x3 << 3);
    if (dma_mode == AX_EMMC_CONTROL0_DMA_SDMA) {
        // AX_LOG_DBG("SDMA mode\n");
        blk_size_cnt |= (uint32_t)(uintptr_t)&tx_desp_table;
        dma_addr = (uint32_t)(uintptr_t)write_buf;
    } else if (dma_mode == AX_EMMC_CONTROL0_DMA_ADMA2_32BIT) {
        // AX_LOG_DBG("ADMA mode\n");
        arasan_reg_write32(emmc->reg_base, AX_EMMC_ADMA_SYS_ADDR_LOW,
                (uint32_t)(uintptr_t)&tx_desp_table);
        arasan_reg_write32(emmc->reg_base, AX_EMMC_ADMA_SYS_ADDR_HIGH,
                0x0);
    } else {
        printf("INCORRECT DMA MODE SELECTED, REG (0x28): 0x%x\n",
                arasan_reg_read32(emmc->reg_base,
                    AX_EMMC_CONTROL0));
        return -EINVAL;
    }

    /* Issue CMD25 to write multiple blocks of data */
    ret = sd_issue_command(
            emmc, dma_addr, blk_size_cnt, start_block_number,
            (AX_EMMC_CMDTM_CMD25 | AX_EMMC_CMDTM_RESPONSE_R1 |
             AX_EMMC_CMDTM_DATA_PRESENT | AX_EMMC_CMDTM_MULTI_BLOCK_EN |
             AX_EMMC_CMDTM_ACMD12_EN | AX_EMMC_CMDTM_DMA_EN));
    if (ret) {
        printf("eMMC CMD25 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    delay_loop_count = DELAY_LOOP_COUNT;
    do {
        temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
        // Handle DMA boundary interrupt
        if ((dma_mode == AX_EMMC_CONTROL0_DMA_SDMA) &&
                (temp & AX_EMMC_INTERRUPT_DMA)) {
            // Read updated SDMA address (next position)
            next_dma_addr = arasan_reg_read32(
                    emmc->reg_base,
                    AX_EMMC_ARG2); // Read current SDMA System Address Register

            // Resume SDMA by writing the MSB of address
            arasan_reg_write32(
                    emmc->reg_base, AX_EMMC_ARG2,
                    next_dma_addr); // Assign again current SDMA System Address Register

            // Clear DMA interrupt bit (W1C)
            arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
                    AX_EMMC_INTERRUPT_DMA);
        }

        // Transfer complete
        if (temp & AX_EMMC_INTERRUPT_TRFR_COMPLETE) {
            // Clear interrupt
            arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
                    AX_EMMC_INTERRUPT_TRFR_COMPLETE);
            return 0;
        }

        // Any error?
        if (temp & AX_EMMC_INTERRUPT_ERROR) {
            printf("SDMA/ADMA interrupt error reg 0x30: 0x%x\r\n", temp);
            arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
                    temp & AX_EMMC_INTERRUPT_ERROR);
            return -EACCES;
        }

        mdelay(1);

    } while (--delay_loop_count > 0);

    printf("Timeout waiting for SDMA/ADMA transfer complete\r\n");
    return -ETIMEDOUT;
}

static int arasan_emmc_dma_read(uint32_t start_block_number, uint32_t blkcnt,
        void *read_buf)
{
    struct arasan_emmc_ctrl_t *emmc = &emmc_dev;
    struct desp_t rx_desp_table;
    uint32_t blk_size_cnt = 0;
    uint32_t delay_loop_count = 0;
    uint16_t total_size = blkcnt * AX_EMMC_BLOCK_COUNT;
    uint32_t dma_mode;
    uint32_t dma_addr = 0;
    uint32_t next_dma_addr = 0;
    uint32_t temp = 0;
    int ret;

    if (EMMC_CHECK_VALID(emmc) || read_buf == NULL) {
        printf("eMMC parameter address is invalid.\n");
        return -EINVAL;
    }

    if (start_block_number > (AX_EMMC_BLOCK_COUNT - 1)) {
        printf("The start block number or  end block number is out of range\n");
        return -ERANGE;
    }
    if (blkcnt > AX_EMMC_BLOCK_REG_MAX) {
        printf("Total number of block count value is out of block count register range\n");
        return -ERANGE;
    }

    rx_desp_table.line.addr = (uint32_t)(uintptr_t)read_buf;
    rx_desp_table.line.length = total_size;
    rx_desp_table.line.attr = ADMA_VALID | ADMA_ACT_TRAN | ADMA_END;

    /* Issue CMD13 to Device sends its status register */
    ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
            (AX_EMMC_CMDTM_CMD13 |
             AX_EMMC_CMDTM_RESPONSE_R1));
    if (ret) {
        printf("eMMC CMD13 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    /* Issue CMD16 to set Block length of 512 bytes */
    ret = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_BLOCK_SIZE,
            (AX_EMMC_CMDTM_CMD16 |
             AX_EMMC_CMDTM_RESPONSE_R1));
    if (ret) {
        printf("eMMC CMD16 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    blk_size_cnt = AX_EMMC_BLOCK_SIZE | (blkcnt << 16);

    /* Configure DMA */
    dma_mode = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
    dma_mode &= (0x3 << 3);
    if (dma_mode == AX_EMMC_CONTROL0_DMA_SDMA) {
        blk_size_cnt |= ((uint32_t)(uintptr_t)&rx_desp_table);
        dma_addr = (uint32_t)(uintptr_t)read_buf;
    } else if (dma_mode == AX_EMMC_CONTROL0_DMA_ADMA2_32BIT) {
        arasan_reg_write32(emmc->reg_base, AX_EMMC_ADMA_SYS_ADDR_LOW,
                (uint32_t)(uintptr_t)&rx_desp_table);
        arasan_reg_write32(emmc->reg_base, AX_EMMC_ADMA_SYS_ADDR_HIGH,
                0x0);
    } else {
        printf("INCORRECT DMA MODE SELECTED, REG (0x28): 0x%x\n",
                arasan_reg_read32(emmc->reg_base,
                    AX_EMMC_CONTROL0));
        return -EINVAL;
    }

    /* Issue CMD18 to read multiple blocks of data */
    ret = sd_issue_command(
            emmc, dma_addr, blk_size_cnt, start_block_number,
            (AX_EMMC_CMDTM_CMD18 | AX_EMMC_CMDTM_RESPONSE_R1 |
             AX_EMMC_CMDTM_DATA_PRESENT | AX_EMMC_CMDTM_MULTI_BLOCK_EN |
             AX_EMMC_CMDTM_DATA_READ | AX_EMMC_CMDTM_ACMD12_EN |
             AX_EMMC_CMDTM_DMA_EN));
    if (ret) {
        printf("eMMC CMD18 transfer failed Status:0x%x\n", ret);
        return ret;
    }

    delay_loop_count = DELAY_LOOP_COUNT;
    do {
        temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
        // Handle DMA boundary interrupt
        if ((dma_mode == AX_EMMC_CONTROL0_DMA_SDMA) &&
                (temp & AX_EMMC_INTERRUPT_DMA)) {
            // Read updated SDMA address (next position)
            next_dma_addr = arasan_reg_read32(emmc->reg_base, AX_EMMC_ARG2); // Read current SDMA System Address Register

            // Resume SDMA by writing the MSB of address
            arasan_reg_write32(emmc->reg_base, AX_EMMC_ARG2, next_dma_addr); // Assign again current SDMA System Address Register

            // Clear DMA interrupt bit (W1C)
            arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
                    AX_EMMC_INTERRUPT_DMA);
        }

        // Transfer complete
        if (temp & AX_EMMC_INTERRUPT_TRFR_COMPLETE) {
            arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
                    AX_EMMC_INTERRUPT_TRFR_COMPLETE);
            // AX_LOG_DBG("SDMA/ADMA Read Transfer Complete\r\n\n");
            return 0;
        }

        // Any error?
        if (temp & AX_EMMC_INTERRUPT_ERROR) {
            printf("SDMA/ADMA interrupt error reg 0x30: 0x%x\r\n", temp);
            arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
                    temp & AX_EMMC_INTERRUPT_ERROR);
            return -EACCES;
        }

        mdelay(1);

    } while (--delay_loop_count > 0);

    printf("Timeout waiting for SDMA/ADMA transfer complete\r\n");
    return -ETIMEDOUT;
}

static int arasan_emmc_erase(uint32_t start, uint32_t blkcnt)
{
    struct arasan_emmc_ctrl_t *emmc = &emmc_dev;
    int ret;

    if (EMMC_CHECK_VALID(emmc)) {
        printf("eMMC parameter address is invalid.\n");
        return -EINVAL;
    }
    printf("Performing All Block Erase...\n");

    /* Vendor CMD (0x00000011) for All Block Erase */
    printf("CMD60: %x\n", ARG_VCMD60_ALL_BLOCK_ERASE);
    ret = sd_issue_command(emmc, 0, 0, ARG_VCMD60_ALL_BLOCK_ERASE,
            AX_EMMC_CMDTM_CMD60 |
            AX_EMMC_CMDTM_RESPONSE_R1B);
    if (ret) {
        printf("Vendor CMD (ALL_BLOCK_ERASE) failed. Status: 0x%x\n",
                ret);
        return ret;
    }
    printf("CMD60 return: 0x%x\n",
            arasan_reg_read32(emmc->reg_base, 0x10));
    /* Note: R1b response implies a busy state. sd_issue_command should handle waiting for command completion. */
    /* Erase operations can take a significant amount of time. The timeout in sd_issue_command might need adjustment */
    /* or this command might need a custom wait loop checking status until erase is complete if R1b doesn't cover it. */

    /* CMD13 (SEND_STATUS) */
    ret = sd_issue_command(emmc, 0, 0, ARG_RCA_DEFAULT,
            AX_EMMC_CMDTM_CMD13 |
            AX_EMMC_CMDTM_RESPONSE_R1);
    if (ret) {
        printf("CMD13 (SEND_STATUS after All Block Erase) failed. Status: 0x%x\n",
                ret);
    }
    printf("CMD13 return: 0x%x\n",
            arasan_reg_read32(emmc->reg_base, 0x10));

    printf("All Block Erase command sent.\n");
    return 0;
}

static uint32_t arasan_emmc_get_block_size(void)
{
    return AX_EMMC_BLOCK_SIZE;
}

static uint32_t arasan_emmc_get_block_count(void)
{
    return AX_EMMC_BLOCK_COUNT;
}

struct ax_emmc_ops emmc = {
    .init = arasan_emmc_init,
    .reset = arasan_emmc_reset,
    .set_speed = arasan_emmc_set_speed,
    .set_bus_width = arasan_emmc_set_bus_width,
    /* .read = arasan_emmc_read, */
    /* .write = arasan_emmc_write, */
    .read = arasan_emmc_dma_read,
    .write = arasan_emmc_dma_write,
    .erase = arasan_emmc_erase,
    .get_block_size = arasan_emmc_get_block_size,
    .get_block_count = arasan_emmc_get_block_count,
};
