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

int sd_issue_command(struct arasan_emmc_ctrl_t *emmc, uint32_t arg_2,
				  uint32_t size_count, uint32_t arg_1,
				  uint32_t transfer_cmd)
{
	uint32_t presentstate = 0;
	uint32_t interrupt_temp = 0;
	uint32_t delay_loop_count = 0;

	if (EMMC_CHECK_VALID(emmc)) {
		printf("eMMC parameter address is invalid.\n");
		printf("eMMC sd issue command aborted. emmc: 0x%8p.\n",
			   emmc->reg_base);
		return -EINVAL;
	}

	/* Enable all Normal and Error interrupt status except Tuning error */
    arasan_reg_write32(emmc->reg_base, AX_EMMC_IRPT_STATUS, AX_EMMC_IRPT_STATUS_EN);

	arasan_reg_write32(
		emmc->reg_base, AX_EMMC_INTERRUPT,
		AX_EMMC_INTERRUPT_TRFR_COMPLETE); // reset Transfer Complete Interrupt
	delay_loop_count = DELAY_LOOP_COUNT;
	/* Check if any read/write transfer is active */
	/* As for now we added basic timeout logic with timer msleep function */
	do {
		presentstate =
			arasan_reg_read32(emmc->reg_base, AX_EMMC_STATUS);
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
			arasan_reg_read32(emmc->reg_base, AX_EMMC_STATUS);
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

	arasan_reg_write32(emmc->reg_base, AX_EMMC_ARG2, arg_2);
	arasan_reg_write32(emmc->reg_base, AX_EMMC_BLKSIZECNT, size_count);
	arasan_reg_write32(emmc->reg_base, AX_EMMC_ARG1, arg_1);
	arasan_reg_write32(emmc->reg_base, AX_EMMC_CMDTM, transfer_cmd);
	delay_loop_count = DELAY_LOOP_COUNT * 100;

	/* Wait until command transfer is complete */
	do {
		interrupt_temp =
			arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
		/* If any error bit set -> fail */
		if (interrupt_temp & AX_EMMC_INTERRUPT_ERROR) {
			printf("eMMC interrupt error:0x%x\n",
				   interrupt_temp);
			/* clear the error bits that are set (W1C) before return */
			arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
					    interrupt_temp &
						    AX_EMMC_INTERRUPT_ERROR);
			return -EACCES;
		}

		if (interrupt_temp & AX_EMMC_INTERRUPT_CMD_COMPLETE) {
			arasan_reg_write32(
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

int emmc_read_ext_csd_reg(struct arasan_emmc_ctrl_t *emmc, uint32_t byte_start,
				uint32_t byte_end)
{
	int ret;
	uint32_t i, temp;
	uint32_t stuff_bits = 0;
	uint32_t delay_loop_count;
	uint32_t ext_csd_reg_value[CONFIG_EMMC_BLOCK_SIZE / 4];
	uint32_t block_count = 0x1; // ext_csd_reg size = 1 block

	printf("Read EXT_CSD reg ... \r\n");

	printf("CMD8: Read Extended CSD Register\n");
	ret = sd_issue_command(
		emmc, stuff_bits, (AX_EMMC_BLOCK_SIZE | (block_count << 16)),
		stuff_bits,
		AX_EMMC_CMDTM_CMD8 | AX_EMMC_CMDTM_RESPONSE_R1 |
			AX_EMMC_CMDTM_DATA_PRESENT | AX_EMMC_CMDTM_DATA_READ);
	if (ret) {
		printf("eMMC CMD8 transfer failed Status:0x%x\n", ret);
		return ret;
	}

	delay_loop_count = DELAY_LOOP_COUNT;
	do {
		temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
		temp = temp & AX_EMMC_INTERRUPT_RD_BUF_READY;
		mdelay(MILLISECOND_1);
	} while ((--delay_loop_count > 0) && ((temp == 0x00000000)));
	if (delay_loop_count == 0) {
		printf(
			"Timeout : Detect read buffer is not in ready state\n");
		return -ETIMEDOUT;
	}
	arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT, temp);

	/* Read  data from Buffer data port register */
	for (i = 0; i < (CONFIG_EMMC_BLOCK_SIZE / 4); i = i + 1) {
		ext_csd_reg_value[i] =
			arasan_reg_read32(emmc->reg_base, AX_EMMC_DATA);
	}

	delay_loop_count = DELAY_LOOP_COUNT;
	do {
		temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);

		// Any error?
		if (temp & AX_EMMC_INTERRUPT_ERROR) {
			printf("Interrupt error reg 0x30: 0x%x\r\n", temp);
			arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
					    temp & AX_EMMC_INTERRUPT_ERROR);
            return -EACCES;
		}

		// Transfer complete
		if (temp & AX_EMMC_INTERRUPT_TRFR_COMPLETE) {
			// Clear interrupt
			arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
					    AX_EMMC_INTERRUPT_TRFR_COMPLETE);
			break;
		}

		mdelay(1);

	} while (--delay_loop_count > 0);

	if (delay_loop_count == 0) {
		printf("Timeout waiting for transfer complete\r\n");
		return -ETIMEDOUT;
	}

	// Extract EXT_CSD value from buffer
	uint8_t *ext_csd = (uint8_t *)ext_csd_reg_value;

	if (byte_end >= CONFIG_EMMC_BLOCK_SIZE || byte_start > byte_end) {
		printf("Invalid EXT_CSD byte range [%u:%u]\n", byte_start,
			   byte_end);
		return -EINVAL;
	}

	printf("EXT_CSD bytes [%u:%u]:\n", byte_start, byte_end);
	for (i = byte_start; i <= byte_end; ++i) {
		printf("Byte %d: 0x%X\r\n", i, ext_csd[i]);
	}

	return 0;
}

static int emmc_hci_high_speed_switch(struct arasan_emmc_ctrl_t *emmc,
					    uint32_t hi_speed_en)
{
	uint32_t temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
	if (hi_speed_en) {
		temp |= (1 << 2); // Set High Speed Enable bit
		arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, temp);
	} else {
		temp &= ~(1 << 2); // Clear High Speed Enable bit
		arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, temp);
	}
	return 0;
}

static int arasan_emmc_change_clock_divider(struct arasan_emmc_ctrl_t *emmc, uint8_t clk_div)
{
	uint32_t reg;
	uint32_t delay_loop_count;

	if ((clk_div != 0x00) && (clk_div != 0x01) && (clk_div != 0x02) &&
	    (clk_div != 0x04) && (clk_div != 0x08) && (clk_div != 0x10) &&
	    (clk_div != 0x20) && (clk_div != 0x40) && (clk_div != 0x80)) {
		printf("Invalid clock divider value (0x%x). Use 0x20 instead\n",
			clk_div);
		clk_div = 0x20;
	}

	// Disable clock
	reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
	reg &= ~AX_EMMC_CONTROL1_SD_CLK_EN;
	arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1, reg);

	// Set new divider
	reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
	reg &= ~(0x3FF << 6);
	reg |= (clk_div << 8);
	arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1, reg);

	// Re-enable clock
	reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1) |
	      AX_EMMC_CONTROL1_SD_CLK_EN;
	arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1, reg);

	// Polling for clock stable
	delay_loop_count = DELAY_LOOP_COUNT;
	do {
		reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
		reg = reg & AX_EMMC_CONTROL1_INT_CLK_STABLE;
		mdelay(MILLISECOND_1);
	} while ((--delay_loop_count > 0) && (reg == 0x00000000));
	if (delay_loop_count == 0) {
        printf("Timeout : Unable to reach internal clock stable state\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int
emmc_device_high_speed_mode_switch(struct arasan_emmc_ctrl_t *emmc,
				   emmc_speed_mode_t speed_mode)
{
	int ret;
	uint32_t temp;
	uint32_t hs_timing_argument;

	switch (speed_mode) {
	case EMMC_MODE_HIGH_SPEED:
		hs_timing_argument = AX_EMMC_ARG1_HS_TIMING_HI_SPEED;
		break;
	case EMMC_MODE_HS200:
		hs_timing_argument = AX_EMMC_ARG1_HS_TIMING_HS200;
		break;
	case EMMC_MODE_HS400:
		printf("This speed mode is still in developing\n");
		return -EINVAL;
		break;
	case EMMC_MODE_DDR50:
		printf("This speed mode is still in developing\n");
		return -EINVAL;
		break;
	default:
		printf("Invalid speed mode\r\n");
		return -EINVAL;
		break;
	}

	ret = sd_issue_command(emmc, 0x0, 0x0, hs_timing_argument,
				  (AX_EMMC_CMDTM_CMD6 |
				   AX_EMMC_CMDTM_RESPONSE_R1B));
	if (ret) {
		printf("eMMC CMD6 transfer failed Status:0x%x\n", ret);
		return ret;
	}

	temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_RESP0);

	/* Check bit 7 - SWITCH_ERROR */
	if ((temp & (1 << 7))) {
		printf("Sending CMD6 error, RESP0 (0x10): 0x%x\n", temp);
		return -EACCES;
	}

	return 0;
}

int emmc_select_high_speed_mode(struct arasan_emmc_ctrl_t *emmc)
{
	int ret;

	/* Device config */
	ret = emmc_device_high_speed_mode_switch(emmc, EMMC_MODE_HIGH_SPEED);
	if (ret) {
		printf("FAIL: Device cannot switch to HS mode\n");
		return ret;
	}

	/* Host controller config (this should implemented after send SWITCH command to device successfully) */
	ret = emmc_hci_high_speed_switch(emmc, EMMC_HI_SPEED_ENABLE); // change debug
	if (ret) {
		printf("FAIL: Host controller cannot switch to HS mode\n");
		return ret;
	}

	// Config clock divider
	ret = arasan_emmc_change_clock_divider(emmc, emmc->sdclk_freq_select);
    if (ret) {
		printf("eMMC change clock failed\n");
		return ret;
	}

	return 0;
}

int emmc_select_hs200_mode(struct arasan_emmc_ctrl_t *emmc)
{
	int ret;

	// Device config
	ret = emmc_device_high_speed_mode_switch(emmc, EMMC_MODE_HS200);
	if (ret) {
		printf("FAIL: Device cannot switch to HS200 mode\n");
		return ret;
	}

	// Host controller config (this should implemented after send SWITCH command to device successfully)
	ret = emmc_hci_high_speed_switch(emmc, EMMC_HI_SPEED_ENABLE);
	if (ret) {
		printf(
			"FAIL: Host controller cannot switch to HS200 mode\n");
		return ret;
	}

	// Config clock divider
	ret = arasan_emmc_change_clock_divider(emmc, emmc->sdclk_freq_select);
    if (ret) {
		printf("eMMC change clock failed\n");
	}

	return 0;
}

int emmc_select_normal_mode(struct arasan_emmc_ctrl_t *emmc)
{
	int ret;

	// Host controller config
	ret = emmc_hci_high_speed_switch(emmc, EMMC_HI_SPEED_DISABLE);
	if (ret) {
		printf("FAIL: Host controller cannot switch to HS mode\n");
		return ret;
	}

	// Config clock divider
	ret = arasan_emmc_change_clock_divider(emmc, emmc->sdclk_freq_select);
    if (ret) {
		printf("eMMC change clock failed\n");
		return ret;
	}

	return 0;
}

int emmc_hci_bus_width_switch(struct arasan_emmc_ctrl_t *emmc,
					   emmc_bus_width_t bus_width)
{
	uint32_t reg;
	if (bus_width == EMMC_BUS_1BIT) {
		printf("Switching to 1-BIT bus width\r\n");
		reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
		reg &= ~(AX_EMMC_CONTROL0_BUS_8BIT | AX_EMMC_CONTROL0_BUS_4BIT);
		arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, reg);
	} else if ((bus_width == EMMC_BUS_4BIT) ||
		   (bus_width == EMMC_BUS_4BIT_DDR)) {
		printf("Switching to 4-BIT bus width\r\n");
		reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
		reg &= ~(AX_EMMC_CONTROL0_BUS_8BIT);
		reg |= AX_EMMC_CONTROL0_BUS_4BIT;
		arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, reg);
	} else if ((bus_width == EMMC_BUS_8BIT) ||
		   (bus_width == EMMC_BUS_8BIT_DDR)) {
		printf("Switching to 8-BIT bus width\r\n");
		reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
		reg |= AX_EMMC_CONTROL0_BUS_8BIT;
		arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, reg);
	} else {
		printf("SDHCI: Wrong bus width\n");
        return -EINVAL;
	}

	return 0;
}

int emmc_device_bus_width_switch(struct arasan_emmc_ctrl_t *emmc,
					      emmc_bus_width_t bus_width)
{
	int ret;
	uint32_t bus_width_argument;
	uint32_t reg;

	switch (bus_width) {
	case EMMC_BUS_1BIT:
		bus_width_argument = AX_EMMC_ARG1_BUS_WIDTH_1;
		break;

	case EMMC_BUS_4BIT:
		bus_width_argument = AX_EMMC_ARG1_BUS_WIDTH_4;
		break;

	case EMMC_BUS_8BIT:
		bus_width_argument = AX_EMMC_ARG1_BUS_WIDTH_8;
		break;

	default:
		printf("Invalid bus width value: %d\r\n", bus_width);
		return -EINVAL;
	}

	ret = sd_issue_command(emmc, 0x0, 0x0, bus_width_argument,
				  (AX_EMMC_CMDTM_CMD6 |
				   AX_EMMC_CMDTM_RESPONSE_R1B));
	if (ret) {
		printf("eMMC CMD6 transfer failed Status:0x%x\n", ret);
		return ret;
	}

	reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_RESP0);
	if ((reg & (1 << 7))) {
		printf("Sending CMD6 error, RESP0 (0x10): 0x%x\n", reg);
		return -EACCES;
	}

	return 0;
}

