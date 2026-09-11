// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <malloc.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "ax_diag.h"
#include "ax_emmc.h"

#include "arasan_sdhci_reg.h"

struct arasan_emmc_ctrl_t emmc_dev = {
	.phy_base = (void __iomem *)AX3000_CSR_BASE_ADRS_APBS_EMMC,
	.reg_base = (void __iomem *)AX3000_CSR_BASE_ADRS_AHBS_EMMC,
	.version = 1,
};

static int sd_issue_command(struct ax_emmc_ctrl_t *emmc, uint32_t arg_2,
				  uint32_t size_count, uint32_t arg_1,
				  uint32_t transfer_cmd)
{
	uint32_t presentstate = 0;
	uint32_t interrupt_temp = 0;
	uint32_t delay_loop_count = 0;

	if (EMMC_CHECK_VALID(emmc)) {
		printf("eMMC parameter address is invalid.\n");
		printf("eMMC sd issue command aborted. emmc: 0x%08x.\n",
			   emmc->reg_base);
		return -EINVAL;
	}

	/* Enable all Normal and Error interrupt status except Tuning error */
	REG_WRITE32(emmc->reg_base, AX_EMMC_IRPT_STATUS,
			    AX_EMMC_IRPT_STATUS_EN);
	REG_WRITE32(
		emmc->reg_base, AX_EMMC_INTERRUPT,
		AX_EMMC_INTERRUPT_TRFR_COMPLETE); // reset Transfer Complete Interrupt
	delay_loop_count = DELAY_LOOP_COUNT;
	/* Check if any read/write transfer is active */
	/* As for now we added basic timeout logic with timer msleep function */
	do {
		presentstate =
			REG_READ32(emmc->reg_base, AX_EMMC_STATUS);
		presentstate = presentstate & (AX_EMMC_STATUS_WR_TRANSFER_A |
					       AX_EMMC_STATUS_RD_TRANSFER_A);
		mdelay(MILLISECOND_1);
	} while ((--delay_loop_count > 0) &&
		 ((presentstate == AX_EMMC_STATUS_WR_TRANSFER_A) ||
		  (presentstate == AX_EMMC_STATUS_RD_TRANSFER_A)));
	if (delay_loop_count == 0) {
		printf("Timeout : Read/Write transfer is active state\n");
		return -ETIMEDOUT;
	}
	delay_loop_count = DELAY_LOOP_COUNT;
	/* Check if Command or Data inhibit is set */
	do {
		presentstate =
			REG_READ32(emmc->reg_base, AX_EMMC_STATUS);
		presentstate = presentstate & (AX_EMMC_STATUS_CMD_INHIBIT |
					       AX_EMMC_STATUS_DAT_INHIBIT);
		mdelay(MILLISECOND_1);
	} while ((--delay_loop_count > 0) &&
		 ((presentstate == AX_EMMC_STATUS_CMD_INHIBIT) ||
		  (presentstate == AX_EMMC_STATUS_DAT_INHIBIT) ||
		  (presentstate ==
		   (AX_EMMC_STATUS_CMD_INHIBIT | AX_EMMC_STATUS_DAT_INHIBIT))));
	if (delay_loop_count == 0) {
		printf("Timeout : Command or Data inhibit is not set\n");
		return -ETIMEDOUT;
	}

	REG_WRITE32(emmc->reg_base, AX_EMMC_ARG2, arg_2);
	REG_WRITE32(emmc->reg_base, AX_EMMC_BLKSIZECNT, size_count);
	REG_WRITE32(emmc->reg_base, AX_EMMC_ARG1, arg_1);
	REG_WRITE32(emmc->reg_base, AX_EMMC_CMDTM, transfer_cmd);
	delay_loop_count = DELAY_LOOP_COUNT * 100;

	/* Wait until command transfer is complete */
	do {
		interrupt_temp =
			REG_READ32(emmc->reg_base, AX_EMMC_INTERRUPT);
		/* If any error bit set -> fail */
		if (interrupt_temp & AX_EMMC_INTERRUPT_ERROR) {
			printf("eMMC interrupt error:0x%x\n",
				   interrupt_temp);
			/* clear the error bits that are set (W1C) before return */
			REG_WRITE32(emmc->reg_base, AX_EMMC_INTERRUPT,
					    interrupt_temp &
						    AX_EMMC_INTERRUPT_ERROR);
			return -EACCES;
		}

		if (interrupt_temp & AX_EMMC_INTERRUPT_CMD_COMPLETE) {
			REG_WRITE32(
				emmc->reg_base, AX_EMMC_INTERRUPT,
				interrupt_temp &
					AX_EMMC_INTERRUPT_CMD_COMPLETE);
			break;
		}

	} while ((--delay_loop_count > 0));

	if (delay_loop_count == 0) {
		printf("Timeout : Command transfer is not completed\r\n");
		return -ETIMEDOUT;
	}

	return 0;
}

int arasan_emmc_phy_power_on(void)
{
	uint32_t val;
	int timedout = 0;

	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_1);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_1,
			    val | AX_EMMC_PHY_RETB_ENBL);

	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3,
			    val | AX_EMMC_PHY_PDB_ENBL);

	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_2);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_2,
			    val | AX_EMMC_PHY_OTAPDLY_EN | ((0x2) << 7));

	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC,
				 AX_EMMC_PHY_CTRL_2); // dummy read

	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_1);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_1,
			    val | ((0x8) << 22) | AX_EMMC_PHY_DLL_ENBL);

	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_STATUS, 0x0);

	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3,
			    val | ((0x1) << 25));

	while (1) {
		mdelay(MILLISECOND_1);

		if (REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_STATUS) &
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
int arasan_emmc_phy_init(void)
{
	uint32_t val;
	int timedout = 0;

	// printf("Init PHY Start !!!\n");

	/* Enable RETB and RTRIM in PHY_CTRL_1 */
	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_1);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_1,
			    val | AX_EMMC_PHY_RETB_ENBL | AX_EMMC_PHY_RTRIM_EN);

	/* Enable PDB in PHY_CTRL_3 */
	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3,
			    val | AX_EMMC_PHY_PDB_ENBL);

	/* Wait for calibration to complete (max 3000 ms) */
	while (1) {
		mdelay(MILLISECOND_1);

		if (REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_STATUS) &
		    AX_EMMC_PHY_CALDONE_MASK)
			break;

		if (timedout == AX_EMMC_PHY_LOOP_TIMEOUT) {
			printf(
				"eMMC-PHY: CALDONE_MASK bit is not cleared.\n");
			return -ETIMEDOUT;
		}
		timedout++;
	}


	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_1);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_1,
			    val | AX_EMMC_PHY_REN_CMD_EN |
				    AX_EMMC_PHY_PU_CMD_EN_NEW);

	/* Enable REN_STRB in PHY_CTRL_2 */
	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_2);

	/* Try to enable open drain data pin */
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_2,
			    val | AX_EMMC_PHY_REN_STRB);

	/* Enable MAX_CLK buffers in PHY_CTRL_3 */
	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3,
			    val | AX_EMMC_PHY_MAX_CLK_BUF0 |
				    AX_EMMC_PHY_MAX_CLK_BUF1 |
				    AX_EMMC_PHY_MAX_CLK_BUF2);

	/* Configure clock multiplier in CAP_REG_IN_S1_MSB */

	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CAP_REG_IN_S1_MSB,
			    AX_EMMC_PHY_CAP_REG_IN_S1_MSB_CLK_MULTIPLIER);

	/* Enable RX and TX clock delay selection in PHY_CTRL_3 */
	val = REG_READ32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3);
	REG_WRITE32(AX3000_CSR_BASE_ADRS_APBS_EMMC, AX_EMMC_PHY_CTRL_3,
			    val | AX_EMMC_PHY_SEL_DLY_RXCLK |
				    AX_EMMC_PHY_SEL_DLY_TXCLK);

	return 0;
}

/* API to initialize the eMMC device, see ax_emmc_api.h for more details */
int arasan_emmc_device_init(struct arasan_emmc_ctrl_t *emmc)
{
	int status;
	uint32_t temp = 0;
	uint32_t ocr = 0;
	uint32_t delay_loop_count = 0;

	if (EMMC_CHECK_VALID(emmc)) {
		printf("eMMC parameter address is invalid.\n");
		printf("eMMC init aborted. emmc: 0x%08x.\n",
			   emmc->reg_base);
		return -EINVAL;
	}

	/* Issue CMD0 with argument 0xF0F0F0F0 for device enter into pre IDLE state */
	status = sd_issue_command(emmc, 0, 0, AX_EMMC_PRE_IDLE_STATE, 0);
	if (status != int_SUCCESS) {
		printf(
			"eMMC CMD0 Pre IDLE state transfer failed Status:0x%x\n",
			status);
		return status;
	}

	/* Issue CMD0 with argument 0 to reset device to IDLE state */
	status = sd_issue_command(emmc, 0, 0, 0, 0);
	if (status != int_SUCCESS) {
		printf("eMMC CMD0 transfer failed Status:0x%x\n", status);
		return status;
	}
	ocr = AX_EMMC_ARG1_HIGH_VTG_OCR;

	delay_loop_count = DELAY_LOOP_COUNT;
	/* Issue CMD1 with R3,R4 response type
	 * Send OCR[Operations condition] to support high capacity and high voltage window
	 */
	do {
		status = sd_issue_command(emmc, 0x0, 0x0, ocr,
					  (AX_EMMC_CMDTM_CMD1 |
					   AX_EMMC_CMDTM_RESPONSE_R3R4));
		if (status != int_SUCCESS) {
			printf("eMMC CMD1 transfer failed Status:0x%x\n",
				   status);
			return status;
		}
		temp = REG_READ32(emmc->reg_base, 0x10);
		ocr = temp;
		mdelay(MILLISECOND_1);
	} while ((--delay_loop_count > 0) &&
		 ((temp & AX_EMMC_CARD_PWR_UP) == 0x0));
	if (delay_loop_count == 0) {
		printf("Timeout : EMMC card power up status not set \n");
		return -ETIMEDOUT;
	}
	/* Issue CMD2 to ask device to send CID[R2 response] */
	status = sd_issue_command(emmc, 0x0, 0x0, 0x0,
				  (AX_EMMC_CMDTM_CMD2 |
				   AX_EMMC_CMDTM_RESPONSE_R2));
	if (status != int_SUCCESS) {
		printf("eMMC CMD2 transfer failed Status:0x%x\n", status);
		return status;
	}

	/* Issue CMD3 to assign relative address to device[R1 response] */
	status = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
				  (AX_EMMC_CMDTM_CMD3 |
				   AX_EMMC_CMDTM_RESPONSE_R1));
	if (status != int_SUCCESS) {
		printf("eMMC CMD3 transfer failed Status:0x%x\n", status);
		return status;
	}

	/* Issue CMD9 with R2 response type ,Transfer block size 512bytes and 1block is sent */
	status = sd_issue_command(
		emmc, 0x0, (AX_EMMC_1BLOCK | AX_EMMC_BLOCK_SIZE),
		AX_EMMC_ARG1_RELATIVE_ADDR,
		(AX_EMMC_CMDTM_CMD9 | AX_EMMC_CMDTM_RESPONSE_R2));
	if (status != int_SUCCESS) {
		printf("eMMC CMD9 transfer failed Status:0x%x\n", status);
		return status;
	}

	/* Issue CMD7 to select the card */
	status = sd_issue_command(emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
				  (AX_EMMC_CMDTM_CMD7 |
				   AX_EMMC_CMDTM_RESPONSE_R1));
	if (status != int_SUCCESS) {
		printf("eMMC CMD7 transfer failed Status:0x%x\n", status);
		return status;
	}

	// Polling CDM13 for TRANSFER state
	do {
		status = sd_issue_command(
			emmc, 0x0, 0x0, AX_EMMC_ARG1_RELATIVE_ADDR,
			(AX_EMMC_CMDTM_CMD13 | AX_EMMC_CMDTM_RESPONSE_R1));
		if (status != int_SUCCESS) {
			printf("eMMC CMD13 transfer failed Status:0x%x\n",
				   status);
			return status;
		}
		temp = REG_READ32(emmc->reg_base, AX_EMMC_RESP0);

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
	int status;
	uint32_t temp = 0;
	uint32_t delay_loop_count = 0;

	if (EMMC_CHECK_VALID(emmc)) {
		printf("eMMC parameter address is invalid.\n");
		printf("eMMC init aborted. emmc: 0x%08x.\n",
			   emmc->reg_base);
		return -EINVAL;
	}

	REG_WRITE32(emmc->reg_base, AX_EMMC_IRPT_STATUS,
			    AX_EMMC_IRPT_STATUS_CARD_EN);
	REG_WRITE32(emmc->reg_base, AX_EMMC_IRPT_SIGNAL,
			    AX_EMMC_IRPT_STATUS_CARD_EN);

	/* Reset the eMMC controller */
	temp = REG_READ32(emmc->reg_base, AX_EMMC_CONTROL1);

	REG_WRITE32(emmc->reg_base, AX_EMMC_CONTROL1,
			    temp | AX_EMMC_CONTROL1_SW_RESET);

	/* Added basic timeout logic */
	delay_loop_count = DELAY_LOOP_COUNT;
	do {
		temp = REG_READ32(emmc->reg_base, AX_EMMC_CONTROL1) &
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
	temp = REG_READ32(emmc->reg_base, AX_EMMC_CONTROL0);

	temp = temp & AX_EMMC_CONTROL0_BUS_VTG_MASK;
	temp = temp |
	       (AX_EMMC_CONTROL0_BUS_VTG_1V8 | AX_EMMC_CONTROL0_BUS_8BIT);
	REG_WRITE32(emmc->reg_base, AX_EMMC_CONTROL0, temp);
	/* Enable auto clock gate */
	REG_WRITE32(emmc->reg_base, AX_EMMC_VENDOR,
			    AX_EMMC_VENDOR_AUTO_CLK_GATE);

	/* Enable internal clock and wait until it is stable */
	temp = REG_READ32(emmc->reg_base, AX_EMMC_CONTROL1);

	// printf("Set clock frequency and Internal clock enable\n");
	temp = ((temp & AX_EMMC_CONTROL1_INT_CLK_EN_MASK) | clock_frequency |
		AX_EMMC_CONTROL1_DATA_TOUT);
	REG_WRITE32(emmc->reg_base, AX_EMMC_CONTROL1, temp);
	delay_loop_count = DELAY_LOOP_COUNT;
	do {
		temp = REG_READ32(emmc->reg_base, AX_EMMC_CONTROL1);
		temp = temp & AX_EMMC_CONTROL1_INT_CLK_STABLE;
		mdelay(MILLISECOND_1);
	} while ((--delay_loop_count > 0) && (temp == 0x00000000));
	if (delay_loop_count == 0) {
		printf(
			"Timeout : Unable to reach internal clock stable state\n");
		return -ETIMEDOUT;
	}
	// sdhci_show_speed(emmc);

	REG_WRITE32(
		emmc->reg_base, AX_EMMC_CONTROL1,
		(REG_READ32(emmc->reg_base, AX_EMMC_CONTROL1) |
		 AX_EMMC_CONTROL1_SD_CLK_EN));

	/*Interrupt enable */
	REG_WRITE32(emmc->reg_base, AX_EMMC_IRPT_STATUS,
			    AX_EMMC_IRPT_STATUS_EN);
	REG_WRITE32(emmc->reg_base, AX_EMMC_IRPT_SIGNAL,
			    AX_EMMC_IRPT_STATUS_EN);

	return 0;
}
static int arasan_emmc_init(void)
{
    struct arasan_emmc_ctrl_t *dev = &emmc_dev;
    int ret;

    ret = arasan_emmc_phy_init();
    if (ret){
        printf("eMMC PHY init failed\n");
        return ret;
    }

    ret = arasan_emmc_controller_init(dev, AX_EMMC_CONTROL1_INT_CLK_400K_EN);
    if (ret){
        printf("eMMC controller init failed\n");
        return ret;
    }

    ret = arasan_emmc_device_init(dev);
    if (ret){
        printf("eMMC device init failed\n");
        return ret;
    }

	return 0;
}

static int arasan_emmc_reset(void)
{
	/* TODO: Reset eMMC controller */
	return 0;
}

static int arasan_emmc_hw_set_clock(uint32_t hz)
{
	/* TODO: Configure controller clock */
	return 0;
}

static int arasan_emmc_hw_set_bus_width(uint32_t width)
{
	/* TODO: Configure 1/4/8-bit bus */
	return 0;
}

static int arasan_emmc_hw_read(uint64_t start, uint32_t blkcnt,
			   void *dst)
{
	/* TODO: CMD17 / CMD18 + data transfer */
	return 0;
}

static int arasan_emmc_hw_write(uint64_t start, uint32_t blkcnt,
			    const void *src)
{
	/* TODO: CMD24 / CMD25 + data transfer */
	return 0;
}

static int arasan_emmc_hw_erase(uint64_t start, uint32_t blkcnt)
{
	/* TODO: CMD35 / CMD36 / CMD38 */
	return 0;
}

static uint32_t arasan_emmc_hw_get_block_size(void)
{
	return 512;
}

static uint64_t arasan_emmc_hw_get_block_count(void)
{
	/* TODO: Read CSD capacity */
	return 0;
}

struct ax_emmc_ops emmc = {
	.init = arasan_emmc_init,
	.reset = arasan_emmc_reset,
	.set_clock = arasan_emmc_hw_set_clock,
	.set_bus_width = arasan_emmc_hw_set_bus_width,
	.read = arasan_emmc_hw_read,
	.write = arasan_emmc_hw_write,
	.erase = arasan_emmc_hw_erase,
	.get_block_size = arasan_emmc_hw_get_block_size,
	.get_block_count = arasan_emmc_hw_get_block_count,
};
