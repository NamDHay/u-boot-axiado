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

int emmc_execute_hs200_tuning(struct arasan_emmc_ctrl_t *emmc);

int ax_emmc_change_clock_divider(struct arasan_emmc_ctrl_t *emmc, uint8_t clk_div)
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

int sd_issue_command(struct arasan_emmc_ctrl_t *emmc, uint32_t arg_2,
								  uint32_t size_count, uint32_t arg_1,
								  uint32_t transfer_cmd)
{
	uint32_t presentstate = 0;
	uint32_t interrupt_temp = 0;
	uint32_t delay_loop_count = 0;

	if (EMMC_CHECK_VALID(emmc))
	{
		printf("eMMC parameter address is invalid.\n");
		printf("eMMC sd issue command aborted. emmc: 0x%8p.\n",
				   emmc->reg_base);
		return -EINVAL;
	}

	/* Enable all Normal and Error interrupt status except Tuning error */
	arasan_reg_write32(emmc->reg_base, AX_EMMC_IRPT_STATUS,
				AX_EMMC_IRPT_STATUS_EN);
	arasan_reg_write32(
		emmc->reg_base, AX_EMMC_INTERRUPT,
		AX_EMMC_INTERRUPT_TRFR_COMPLETE); // reset Transfer Complete Interrupt
	delay_loop_count = DELAY_LOOP_COUNT;
	/* Check if any read/write transfer is active */
	/* As for now we added basic timeout logic with timer msleep function */
	do
	{
		presentstate =
			arasan_reg_read32(emmc->reg_base, AX_EMMC_STATUS);
		presentstate = presentstate & (AX_EMMC_STATUS_WR_TRANSFER_A |
									   AX_EMMC_STATUS_RD_TRANSFER_A);
		mdelay(MILLISECOND_1);
	} while ((--delay_loop_count > 0) &&
			 ((presentstate == AX_EMMC_STATUS_WR_TRANSFER_A) ||
			  (presentstate == AX_EMMC_STATUS_RD_TRANSFER_A)));
	if (delay_loop_count == 0)
	{
		printf("Timeout : Read/Write transfer is active state\n");
		return -ETIMEDOUT;
	}
	delay_loop_count = DELAY_LOOP_COUNT;
	/* Check if Command or Data inhibit is set */
	do
	{
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
	if (delay_loop_count == 0)
	{
		printf("Timeout : Command or Data inhibit is not set\n");
		return -ETIMEDOUT;
	}

	arasan_reg_write32(emmc->reg_base, AX_EMMC_ARG2, arg_2);
	arasan_reg_write32(emmc->reg_base, AX_EMMC_BLKSIZECNT, size_count);
	arasan_reg_write32(emmc->reg_base, AX_EMMC_ARG1, arg_1);
	arasan_reg_write32(emmc->reg_base, AX_EMMC_CMDTM, transfer_cmd);

	delay_loop_count = DELAY_LOOP_COUNT;
	/* Wait until command transfer is complete */
	do
	{
		interrupt_temp =
			arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
		/* If any error bit set -> fail */
		if (interrupt_temp & AX_EMMC_INTERRUPT_ERROR)
		{
			printf("eMMC interrupt error:0x%x\n",
					   interrupt_temp);
			/* clear the error bits that are set (W1C) before return */
			arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
						interrupt_temp &
							AX_EMMC_INTERRUPT_ERROR);
			return -EACCES;
		}

		if (interrupt_temp & AX_EMMC_INTERRUPT_CMD_COMPLETE)
		{
			// printf("[Before issue_cmd] AX_EMMC_INTERRUPT (0x30): 0x%x\r\n", arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT));
			arasan_reg_write32(
				emmc->reg_base, AX_EMMC_INTERRUPT,
				interrupt_temp &
					AX_EMMC_INTERRUPT_CMD_COMPLETE);
			// printf("[After issue cmd]  AX_EMMC_INTERRUPT (0x30): 0x%x\r\n", arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT));
			break;
		}
		mdelay(1);
	} while ((--delay_loop_count > 0));

	if (delay_loop_count == 0)
	{
		printf("Timeout : Command transfer is not completed\r\n");
		return -ETIMEDOUT;
	}

	return 0;
}

/*
 * Refer ARASAN Host Controller doc, page 254 / section 8.4
 * Enabling DLL and clock frequency change during speed mode switching (Applicable only for eMMC Phy)
 */
int emmc_phy_high_speed_switch(struct arasan_emmc_ctrl_t *emmc, uint32_t clk_div, uint32_t dll_en)
{
	int status;
	uint32_t reg;
	uint32_t timeout;

	// printf("PHY start switching speed mode\n");

	/*
	 *   1. Turn off SD clock and internal clock (clear bit Internal_Clock_Enable and SD_Clock_Enable)
	 */
	reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
	reg &= ~(AX_EMMC_CONTROL1_INT_CLK_EN | AX_EMMC_CONTROL1_SD_CLK_EN);
	arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1, reg);

	// Ensure internal clock stable bit is cleared (read-back must show 0)
	timeout = DELAY_LOOP_COUNT;
	while (timeout--)
	{
		reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
		if ((reg & AX_EMMC_CONTROL1_INT_CLK_STABLE) == 0)
			break;
		udelay(1);
	}
	if (timeout == 0)
	{
		printf("Can not turn off SD clock / Internal clock\n");
		return -ETIMEDOUT;
	}

	// Disabled DLL
	reg = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_1);
	reg &= ~(AX_EMMC_PHY_CTRL_1_ENDLL);
	arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_1, reg);

	/*
	 *   2. If desired mode supports DLL, prepare DLL registers; else set delay-selection bits
	 */
	if (dll_en)
	{

		// Write appropriate value to DLL TRM ICP --> Use default value 0b1000
		// Write appropriate value to frqsel --> refer table 100 in Arasan HC doc
		reg = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_3);
		reg &= ~(AX_EMMC_PHY_CTRL_3_FREQSEL_MASK);
		if ((emmc->speed_mode == EMMC_MODE_HIGH_SPEED) || (emmc->speed_mode == EMMC_MODE_DDR50))
		{
			reg |= (AX_EMMC_PHY_CTRL_3_FREQSEL_HS);
		}
		else if ((emmc->speed_mode == EMMC_MODE_HS200) || (emmc->speed_mode == EMMC_MODE_HS400))
		{
			reg |= (AX_EMMC_PHY_CTRL_3_FREQSEL_HS200); // same as hs400
		}
		arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_3, reg);

		// Keep seldlyTxclk and seldlyRxclk to "0" (Disable Delay chain for Tx/Rx clock, use DLL instead)
		reg = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_3); // 0x2FC1C00
		reg &= ~(AX_EMMC_PHY_SEL_DLY_TXCLK | AX_EMMC_PHY_SEL_DLY_RXCLK);
		arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_3, reg);
	}

	/*
	 *   3. Program the SDCLK divider and enable internal clock
	 */
	reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
	reg &= ~(0x3FF << 6);
	reg |= (clk_div << 8) | AX_EMMC_CONTROL1_INT_CLK_EN;
	arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1, reg);

	/*
	 *   4. Wait for internal clock stable
	 */
	timeout = DELAY_LOOP_COUNT;
	while (timeout--)
	{
		reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
		if (reg & AX_EMMC_CONTROL1_INT_CLK_STABLE)
			break;
	}
	if (timeout == 0)
	{
		printf("Timeout waiting for internal clock stable after enabling internal clock\n");
		return -ETIMEDOUT;
	}

	/*
	 *   5. Enable SD clock output (supply clock to card)
	 */
	reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
	reg |= AX_EMMC_CONTROL1_SD_CLK_EN;
	arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1, reg);

	/*
	 *   6. If mode uses DLL, enable it and wait for DLLRDY
	 */
	if (dll_en)
	{
		// Enable DLL
		reg = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_CTRL_1);
		reg |= AX_EMMC_PHY_CTRL_1_ENDLL;
		arasan_reg_write32(emmc->phy_base, AX_EMMC_PHY_CTRL_1, reg);

		// Wait for DLLRDY (PHY status register)
		timeout = DELAY_LOOP_COUNT;
		while (timeout--)
		{
			reg = arasan_reg_read32(emmc->phy_base, AX_EMMC_PHY_STATUS);
			if (reg & AX_EMMC_PHY_STATUS_DLLRDY)
				break;
			udelay(10);
		}
		if (timeout == 0)
		{
			printf("Timeout waiting for PHY DLL ready (DLLRDY)\n");
			return -ETIMEDOUT;
		}

		printf("DLL is ready\n");
	}

	printf("PHY entered speed mode: divider=0x%x, SDCLK enabled, DLL %s\n",
				 clk_div, dll_en ? "enabled" : "not used");
	return 0;
}

int emmc_read_ext_csd_reg(struct arasan_emmc_ctrl_t *emmc, uint32_t byte_start,
								uint32_t byte_end)
{
	int status;
	uint32_t i, temp;
	uint32_t stuff_bits = 0;
	uint32_t delay_loop_count;
	uint32_t ext_csd_reg_value[CONFIG_EMMC_BLOCK_SIZE / 4];
	uint32_t block_count = 0x1; // ext_csd_reg size = 1 block

	/* Issue CMD6 to set BUS_WIDTH to 8-bit in EXT CSD register*/
	printf("Read EXT_CSD reg ... \r\n");

	/* Issue CMD8 to Read Extended CSD Register */
	printf("CMD8: Read Extended CSD Register\n");
	status = sd_issue_command(
		emmc, stuff_bits, (AX_EMMC_BLOCK_SIZE | (block_count << 16)),
		stuff_bits,
		AX_EMMC_CMDTM_CMD8 | AX_EMMC_CMDTM_RESPONSE_R1 |
			AX_EMMC_CMDTM_DATA_PRESENT | AX_EMMC_CMDTM_DATA_READ);
	if (status)
	{
		printf("eMMC CMD8 transfer failed Status:0x%x\n", status);
		return status;
	}

	delay_loop_count = DELAY_LOOP_COUNT;
	do
	{
		temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
		temp = temp & AX_EMMC_INTERRUPT_RD_BUF_READY;
		mdelay(MILLISECOND_1);
	} while ((--delay_loop_count > 0) && ((temp == 0x00000000)));
	if (delay_loop_count == 0)
	{
		printf(
			"Timeout : Detect read buffer is not in ready state\n");
		return -ETIMEDOUT;
	}
	arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT, temp);

	/* Read data from Buffer data port register */
	for (i = 0; i < (CONFIG_EMMC_BLOCK_SIZE / 4); i = i + 1)
	{
		ext_csd_reg_value[i] =
			arasan_reg_read32(emmc->reg_base, AX_EMMC_DATA);
	}

	delay_loop_count = DELAY_LOOP_COUNT;
	do
	{
		temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);

		// Any error?
		if (temp & AX_EMMC_INTERRUPT_ERROR)
		{
			printf("Interrupt error reg 0x30: 0x%x\r\n", temp);
			arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
						temp & AX_EMMC_INTERRUPT_ERROR);
			return -EACCES;
		}

		// Transfer complete
		if (temp & AX_EMMC_INTERRUPT_TRFR_COMPLETE)
		{
			// Clear interrupt
			arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT,
						AX_EMMC_INTERRUPT_TRFR_COMPLETE);
			break;
		}

		mdelay(1);

	} while (--delay_loop_count > 0);

	if (delay_loop_count == 0)
	{
		printf("Timeout waiting for transfer complete\r\n");
		return -ETIMEDOUT;
	}

	// Extract EXT_CSD value from buffer
	uint8_t *ext_csd = (uint8_t *)ext_csd_reg_value;

	if (byte_end >= CONFIG_EMMC_BLOCK_SIZE || byte_start > byte_end)
	{
		printf("Invalid EXT_CSD byte range [%u:%u]\n", byte_start,
				   byte_end);
		return -EINVAL;
	}

	printf("EXT_CSD bytes [%u:%u]:\n", byte_start, byte_end);
	for (i = byte_start; i <= byte_end; ++i)
	{
		printf("Byte %d: 0x%X\r\n", i, ext_csd[i]);
	}

	return 0;
}

static int emmc_hci_high_speed_switch(struct arasan_emmc_ctrl_t *emmc,
											uint32_t hi_speed_en)
{
	uint32_t temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
	if (hi_speed_en)
	{
		temp |= (1 << 2); // Set High Speed Enable bit
		arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, temp);
	}
	else
	{
		temp &= ~(1 << 2); // Clear High Speed Enable bit
		arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, temp);
	}
	return 0;
}

static int emmc_device_high_speed_mode_switch(struct arasan_emmc_ctrl_t *emmc,
													emmc_speed_mode_t speed_mode)
{
	int status;
	uint32_t temp;
	uint32_t hs_timing_argument;

	switch (speed_mode)
	{
	case EMMC_MODE_HIGH_SPEED:
		hs_timing_argument = AX_EMMC_ARG1_HS_TIMING_HI_SPEED;
		break;
	case EMMC_MODE_HS200:
		hs_timing_argument = AX_EMMC_ARG1_HS_TIMING_HS200;
		break;
	case EMMC_MODE_HS400:
		hs_timing_argument = AX_EMMC_ARG1_HS_TIMING_HS400;
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

	printf("hs_timing_argument (CMD6) = 0x%x\n", hs_timing_argument);
	status = sd_issue_command(emmc, 0x0, 0x0, hs_timing_argument,
							  (AX_EMMC_CMDTM_CMD6 | AX_EMMC_CMDTM_RESPONSE_R1B));
	if (status)
	{
		printf("eMMC CMD6 transfer failed Status:0x%x\n", status);
		return status;
	}

	temp = arasan_reg_read32(emmc->reg_base, AX_EMMC_RESP0);

	// CHeck bit 7 - SWITCH_ERROR
	printf("RESP0 (0x10): 0x%x\n", temp);
	if ((temp & (1 << 7)))
	{
		printf("RESP0 (0x10): 0x%x\n", temp);
		return -EACCES;
	}

	printf("Device entered High speed mode\n");

	return 0;
}

int emmc_select_high_speed_mode(struct arasan_emmc_ctrl_t *emmc)
{
	int status;

	// Device config
	status = emmc_device_high_speed_mode_switch(emmc, EMMC_MODE_HIGH_SPEED);
	if (status)
	{
		printf("FAIL: Device cannot switch to HS mode\n");
		return status;
	}

	// Host controller config (this should implemented after send SWITCH command to device successfully)
	status = emmc_hci_high_speed_switch(
		emmc, EMMC_HI_SPEED_ENABLE); // change debug
	if (status)
	{
		printf("FAIL: Host controller cannot switch to HS mode\n");
		return status;
	}

	// Config clock divider
	
	status = ax_emmc_change_clock_divider(emmc, emmc->sdclk_freq_select);
    if (status) {
		printf("eMMC change clock failed\n");
		return -EINVAL;
	}

	return 0;
}

int emmc_select_hs200_mode(struct arasan_emmc_ctrl_t *emmc)
{
	int status;

	// Device config
	status = emmc_device_high_speed_mode_switch(emmc, EMMC_MODE_HS200);
	if (status)	{
		printf("FAIL: Device cannot switch to HS200 mode\n");
		return status;
	}

	// Host controller config (this should implemented after send SWITCH command to device successfully)
	status = emmc_hci_high_speed_switch(
		emmc, EMMC_HI_SPEED_ENABLE); // change debug
	if (status)	{
		printf("FAIL: Host controller cannot switch to HS200 mode\n");
		return status;
	}

	// Config clock divider
	status = ax_emmc_change_clock_divider(emmc, emmc->sdclk_freq_select);
    if (status)	{
		printf("eMMC change clock failed\n");
		return status;
	}

	// Issue tuning process
    /* status = emmc_execute_hs200_tuning(emmc); */
    /* if (status) { */
    /*     printf("HS200 Tuning executed FAILED\r\n"); */
    /*     return status; */
    /* } */
    /* printf("HS200 Tuning executed successfully\r\n"); */

    return 0;
}

int emmc_select_hs400_mode(struct arasan_emmc_ctrl_t *emmc)
{
    int status;
    uint32_t reg;
    uint32_t delay_loop_count;

    /* Set timing to HS200 for tuning */
    status = emmc_select_hs200_mode(emmc);
    if (status) {
        printf("FAIL: at HS200 mode switching\r\n");
        return status;
    }
    printf("[HS400_switch] HS200 entered\n");

    /* Set back to HS */
    status = emmc_device_high_speed_mode_switch(emmc, EMMC_MODE_HIGH_SPEED);
    if (status) {
        printf("FAIL: at Set back to HS\n");
        return status;
    }
    printf("[HS400_switch] HS entered\n");

    /* Decrease clock divider */
    status = ax_emmc_change_clock_divider(emmc, 0x4);
    if (status) {
        printf("FAIL: at Decrease clock divider\n");
        return status;
    }
    printf("[HS400_switch] Clock divider is changed to 0x4. 26MHz\n");

    /* Switch to DDR 8-bit bus width */
    status = sd_issue_command(emmc, 0x0, 0x0, 0x03B78600,
            (AX_EMMC_CMDTM_CMD6 | AX_EMMC_CMDTM_RESPONSE_R1B)); // BUS_WIDTH[183] 0x86 - using Enhanced Strobe
    if (status) {
        printf("eMMC CMD6 transfer failed Status:0x%x\n", status);
        return status;
    }
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_RESP0);
    if ((reg & (1 << 7))) {
        printf("RESP0 (0x10): 0x%x\n", reg);
        return -EINVAL;
    }
    printf("[HS400_switch] CMD6 (DDR8bit) is sent successfully\n");

    /* Set HS_TIMING for HS400 */
    status = emmc_device_high_speed_mode_switch(emmc, EMMC_MODE_HS400);
    if (status) {
        printf("FAIL: Device cannot switch to HS400 mode\n");
        return status;
    }
    printf("[HS400_switch] HS_TIMING = 0x03 is set.\n");

    // HS400 controller config
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL2);
    reg = reg & 0x0000FFFF;
    reg = reg | 0x800D0000; // HS400 mode & fixed clock
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL2, reg);

    // Enable enhanced strobe =====> meet timeout error for every next CMD
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_VENDOR);
    reg |= ((1 << 0));
    arasan_reg_write32(emmc->reg_base, AX_EMMC_VENDOR, reg);
    printf("[HS400_switch] Vendor register was modified\n");
    udelay(1);

    // Switching to DLL clock and change clock divider
    status = emmc_phy_high_speed_switch(emmc, emmc->sdclk_freq_select, 1);
    if (status) {
        printf("eMMC PLL PHY init failed\n");
        return -EACCES;
    }
    printf("[HS400_switch] eMMC PLL PHY init Completed\r\n");

    printf("[HS400_switch] HS400 switching done\n");

    return 0;
}

int emmc_select_normal_mode(struct arasan_emmc_ctrl_t *emmc)
{
    int status;

    // Host controller config
    status = emmc_hci_high_speed_switch(
            emmc, EMMC_HI_SPEED_DISABLE); // change debug
    if (status) {
        printf("FAIL: Host controller cannot switch to HS mode\n");
        return status;
    }

    // Config clock divider
    status = ax_emmc_change_clock_divider(emmc, emmc->sdclk_freq_select);
    if (status) {
        printf("eMMC change clock failed\n");
        return -EACCES;
    }

    return 0;
}

int emmc_select_ddr50_mode(struct arasan_emmc_ctrl_t *emmc)
{
    int status;
    uint32_t reg;

    // Device config, HS_TIMING must be set to “0x1” before setting BUS_WIDTH for dual data rate operation
    status = emmc_device_high_speed_mode_switch(emmc, EMMC_MODE_HIGH_SPEED);
    if (status) {
        printf("FAIL: Device cannot switch to HS mode\n");
        return status;
    }

    // Switch to DDR
    status = ax_emmc_set_bus_width(emmc->bus_width);
    if (status) {
        printf("FAIL: Switch DDR bus\n");
        return status;
    }

    // Disable clock and it will be re-enble in below function ax_emmc_change_clock_divider()
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL1);
    reg &= ~AX_EMMC_CONTROL1_SD_CLK_EN;
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL1, reg);

    // DDR50 mode config in HC2
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL2);
    reg &= ~(0xF << 16);
    reg |= (0xC << 16); // bit DDR50 and bit 1V8 signalling
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL2, reg);

    // Host controller config (this should implemented after send SWITCH command to device successfully)
    status = emmc_hci_high_speed_switch(emmc, EMMC_HI_SPEED_ENABLE);
    if (status) {
        printf("FAIL: Host controller cannot switch to HS mode\n");
        return status;
    }

    // Config clock divider
    status = ax_emmc_change_clock_divider(emmc, emmc->sdclk_freq_select);
    if (status) {
        printf("eMMC change clock failed\n");
        return -EACCES;
    }

    return 0;
}


int emmc_set_speed_mode(struct arasan_emmc_ctrl_t *emmc,
        emmc_speed_mode_t speed_mode)
{
    int status;

    switch (speed_mode)
    {
        case EMMC_MODE_NORMAL:
            status = emmc_select_normal_mode(emmc);
            if (status)
            {
                printf("Normal mode switching FAILED!!!\r\n");
                return status;
            }
            printf("NORMAL SPEED mode entered\r\n");
            break;

        case EMMC_MODE_HIGH_SPEED:
            status = emmc_select_high_speed_mode(emmc);
            if (status)
            {
                printf("High speed mode switching FAILED!!!\r\n");
                return status;
            }
            printf("HIGH SPEED mode entered\r\n");
            break;

        case EMMC_MODE_HS200:
            status = emmc_select_hs200_mode(emmc);
            if (status)
            {
                printf("HS200 mode switching FAILED!!!\r\n");
                return status;
            }
            printf("HS200 mode entered\r\n");
            break;

        case EMMC_MODE_HS400:
            status = emmc_select_hs400_mode(emmc);
            if (status)
            {
                printf("HS400 mode switching FAILED!!!\r\n");
                return status;
            }
            printf("HS400 mode entered\r\n");
            break;
        case EMMC_MODE_DDR50:
            status = emmc_select_ddr50_mode(emmc);
            if (status)
            {
                printf("DDR50 mode switching FAILED!!!\r\n");
                return status;
            }
            printf("DDR50 mode entered\r\n");
            break;

        default:
            printf("Unknown eMMC speed mode: %d\r\n", speed_mode);
            return -EINVAL;
            break;
    }
    emmc->speed_mode = speed_mode;

    return status;
}

int emmc_hci_bus_width_switch(struct arasan_emmc_ctrl_t *emmc,
        emmc_bus_width_t bus_width)
{
    uint32_t reg;
    if (bus_width == EMMC_BUS_1BIT)
    {
        printf("Switching to 1-BIT bus width\r\n");
        reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
        reg &= ~(AX_EMMC_CONTROL0_BUS_8BIT | AX_EMMC_CONTROL0_BUS_4BIT);
        arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, reg);
    }
    else if ((bus_width == EMMC_BUS_4BIT) || (bus_width == EMMC_BUS_4BIT_DDR))
    {
        printf("Switching to 4-BIT bus width\r\n");
        reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
        reg &= ~(AX_EMMC_CONTROL0_BUS_8BIT);
        reg |= AX_EMMC_CONTROL0_BUS_4BIT;
        arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, reg);
    }
    else if ((bus_width == EMMC_BUS_8BIT) || (bus_width == EMMC_BUS_8BIT_DDR))
    {
        printf("Switching to 8-BIT bus width\r\n");
        reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL0);
        reg |= AX_EMMC_CONTROL0_BUS_8BIT;
        arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL0, reg);
    }
    else
    {
        printf("SDHCI: Wrong bus width\n");
    }
    return 0;
}

int emmc_device_bus_width_switch(struct arasan_emmc_ctrl_t *emmc,
        emmc_bus_width_t bus_width)
{
    int status;
    uint32_t bus_width_argument;
    uint32_t reg;

    switch (bus_width)
    {
        case EMMC_BUS_1BIT:
            bus_width_argument = AX_EMMC_ARG1_BUS_WIDTH_1;
            break;

        case EMMC_BUS_4BIT:
            bus_width_argument = AX_EMMC_ARG1_BUS_WIDTH_4;
            break;

        case EMMC_BUS_8BIT:
            bus_width_argument = AX_EMMC_ARG1_BUS_WIDTH_8;
            break;
        case EMMC_BUS_4BIT_DDR:
            bus_width_argument = AX_EMMC_ARG1_BUS_WIDTH_4_DDR;
            break;
        case EMMC_BUS_8BIT_DDR:
            bus_width_argument = AX_EMMC_ARG1_BUS_WIDTH_8_DDR;
            break;
        default:
            printf("Invalid bus width value: %d\r\n", bus_width);
            return -EINVAL;
    }

    status = sd_issue_command(emmc, 0x0, 0x0, bus_width_argument,
            (AX_EMMC_CMDTM_CMD6 |
             AX_EMMC_CMDTM_RESPONSE_R1B));
    if (status) {
        printf("eMMC CMD6 transfer failed Status:0x%x\n", status);
        return status;
    }

    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_RESP0);
    // CHeck bit 7 - SWITCH_ERROR
    printf("RESP0 (0x10): 0x%x\n", reg);
    if ((reg & (1 << 7)))
    {
        printf("Sending CMD6 error, RESP0 (0x10): 0x%x\n", reg);
        return -EACCES;
    }

    return 0;
}

int emmc_send_tuning(struct arasan_emmc_ctrl_t *emmc)
{
    int status = 0;
    uint32_t reg, i;
    uint32_t block_size;
    uint32_t delay_loop_count;
    uint8_t read_buf[128];

    if (emmc->bus_width == EMMC_BUS_4BIT || emmc->bus_width == EMMC_BUS_4BIT_DDR)
    {
        block_size = 0x40;
    }
    else if (emmc->bus_width == EMMC_BUS_8BIT || emmc->bus_width == EMMC_BUS_8BIT_DDR)
    {
        block_size = 0x80;
    }
    /* Clear interrupts */
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
    arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT, reg);

    /* Check if any read/write transfer is active */
    delay_loop_count = DELAY_LOOP_COUNT;
    do
    {
        reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_STATUS);
        reg = reg & (AX_EMMC_STATUS_WR_TRANSFER_A | AX_EMMC_STATUS_RD_TRANSFER_A);
        mdelay(MILLISECOND_1);
    } while ((--delay_loop_count > 0) && ((reg == AX_EMMC_STATUS_WR_TRANSFER_A) ||
                (reg == AX_EMMC_STATUS_RD_TRANSFER_A)));
    if (delay_loop_count == 0)
    {
        printf("Timeout : Read/Write transfer is active state\n");
        return -ETIMEDOUT;
    }

    /* Check if Command or Data inhibit is set */
    delay_loop_count = DELAY_LOOP_COUNT;
    do
    {
        reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_STATUS);
        reg = reg & (AX_EMMC_STATUS_CMD_INHIBIT | AX_EMMC_STATUS_DAT_INHIBIT);
        mdelay(MILLISECOND_1);
    } while ((--delay_loop_count > 0) &&
            ((reg == AX_EMMC_STATUS_CMD_INHIBIT) || (reg == AX_EMMC_STATUS_DAT_INHIBIT) ||
             (reg == (AX_EMMC_STATUS_CMD_INHIBIT | AX_EMMC_STATUS_DAT_INHIBIT))));
    if (delay_loop_count == 0)
    {
        printf("Timeout : Command or Data inhibit is not set\n");
        return -ETIMEDOUT;
    }

    arasan_reg_write32(emmc->reg_base, AX_EMMC_BLKSIZECNT, block_size);
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CMDTM, (AX_EMMC_CMDTM_CMD21 | AX_EMMC_CMDTM_RESPONSE_R1 | AX_EMMC_CMDTM_DATA_PRESENT | AX_EMMC_CMDTM_DATA_READ));

    /* Wait for Buffer Read Ready */
    delay_loop_count = DELAY_LOOP_COUNT;
    do
    {
        reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_INTERRUPT);
        mdelay(MILLISECOND_1);
    } while ((--delay_loop_count > 0) && !(reg & AX_EMMC_INTERRUPT_RD_BUF_READY));
    if (delay_loop_count == 0)
    {
        printf("Timeout : waiting for Buffer Read Ready interrupt\n");
        return -ETIMEDOUT;
    }
    arasan_reg_write32(emmc->reg_base, AX_EMMC_INTERRUPT, reg);

    /* Read 128 bytes data pattern from Buffer data port register */
    for (i = 0; i < block_size; i += 4)
    {
        reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_DATA);
        read_buf[i] = (reg >> 0) & 0xFF;
        read_buf[i + 1] = (reg >> 8) & 0xFF;
        read_buf[i + 2] = (reg >> 16) & 0xFF;
        read_buf[i + 3] = (reg >> 24) & 0xFF;
    }

    return 0;
}

int emmc_execute_hs200_tuning(struct arasan_emmc_ctrl_t *emmc)
{
    int status = 0;
    uint32_t reg, temp, i;
    uint32_t stuff_bits = 0;
    uint32_t delay_loop_count;
    uint32_t tuning_loop_count = 100; // 40 is enough, in case not sure we use 100

    printf("Clear TUNED_CLK before starting tuning\n");
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL2);
    printf("TUNED_CLK before clearing: 0x%x\n", reg);
    reg &= ~AX_EMMC_CONTROL2_TUNED_CLK;
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL2, reg);
    printf("TUNED_CLK after clearing: 0x%x\n", reg);

    printf("Start HS200 Execute Tuning\n");
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL2);
    printf("EXEC_TUNING before triggering: 0x%x\n", reg);
    reg |= AX_EMMC_CONTROL2_EXEC_TUNING;
    arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL2, reg);
    printf("EXEC_TUNING after triggering: 0x%x\n", reg);

    delay_loop_count = tuning_loop_count;
    do
    {
        status = emmc_send_tuning(emmc);
        if (status) {
            continue;
        }
        reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL2);
        printf("Host Control 2 (0x3C) : 0x%x\n", reg);
        udelay(10);
    } while ((reg & AX_EMMC_CONTROL2_EXEC_TUNING) && (--delay_loop_count > 0));

    if (delay_loop_count == 0)
    {
        printf("Timeout waiting for Tuning complete\n");
        return -ETIMEDOUT;
    }

    // Check result
    reg = arasan_reg_read32(emmc->reg_base, AX_EMMC_CONTROL2);
    if (!(reg & AX_EMMC_CONTROL2_TUNED_CLK))
    {
        printf("HS200 Tuning failed\n");
        // Abort / reset TUNED_CLK
        reg &= ~AX_EMMC_CONTROL2_TUNED_CLK;
        arasan_reg_write32(emmc->reg_base, AX_EMMC_CONTROL2, reg);
        return -EACCES;
    }

    return 0;
}
