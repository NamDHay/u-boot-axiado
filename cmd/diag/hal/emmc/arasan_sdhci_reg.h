#ifndef _AX_HAL_AX_EMMC_REGS_H
#define _AX_HAL_AX_EMMC_REGS_H

/**
 * @file ax_emmc_regs.h
 *
 * @brief basic offset and bit value manages EMMC controller
 */

/* EMMC register description */

#define AX_EMMC_ARG2		   		0x0 /**< EMMC Argument2 register */
#define AX_EMMC_BLKSIZECNT	   		0x4 /**< EMMC Block size register */
#define AX_EMMC_ARG1		   		0x8 /**< EMMC Argument1 register */
#define AX_EMMC_CMDTM		   		0xC /**< EMMC Transfer mode register */
#define AX_EMMC_RESP0		   		0x10 /**< EMMC Response0 register */
#define AX_EMMC_RESP1		   		0x14 /**< EMMC Response1 register */
#define AX_EMMC_RESP2		   		0x18 /**< EMMC Response2 register */
#define AX_EMMC_RESP3		   		0x1C /**< EMMC Response3 register */
#define AX_EMMC_DATA		   		0x20 /**< EMMC Buffer data port register */
#define AX_EMMC_STATUS		   		0x24 /**< EMMC Present state register */
#define AX_EMMC_CONTROL0	   		0x28 /**< EMMC Host,Power,wakeup control register */
#define AX_EMMC_CONTROL_POWER      	0x29 /**< EMMC Power control register */
#define AX_EMMC_CONTROL1	   		0x2C /**< EMMC clock,timeout control register */
#define AX_EMMC_INTERRUPT	   		0x30 /**< EMMC Interrupt register */
#define AX_EMMC_IRPT_STATUS	   		0x34 /**< EMMC Interrupt status enable register */
#define AX_EMMC_IRPT_SIGNAL	   		0x38 /**< EMMC Interrupr signal enable register */
#define AX_EMMC_CONTROL2	   		0x3C /**< EMMC Host control2 register */
#define AX_EMMC_CAPABILITIES_0 		0x40 /**< EMMC Capabilities0 register */
#define AX_EMMC_CAPABILITIES_1 		0x44 /**< EMMC Capabilities1 register */
#define AX_EMMC_PHY_BIST_1          0x48
#define AX_EMMC_PHY_BIST_2          0x4C
#define AX_EMMC_ADMA_SYS_ADDR_LOW  	0x58 /**< EMMC ADMA system address register - 32-bit address */
#define AX_EMMC_ADMA_SYS_ADDR_HIGH 	0x5C /**< EMMC ADMA system address register - 64-bit address */
#define AX_EMMC_VENDOR		   		0x78 /**< EMMC vendor register */

/* EMMC register field descriptions */
#define AX_EMMC_CONTROL_POWER_ON		 0x10 /**< EMMC Power on */
#define AX_EMMC_CONTROL_POWER_OFF		 0x00 /**< EMMC Power	off */
#define AX_EMMC_ARG1_BUS_WIDTH_1		 0x03B70000 /**< Value to set bus width to 1 in EXT_CSD register */
#define AX_EMMC_ARG1_BUS_WIDTH_4		 0x03B70100 /**< Value to set bus width to 4 in EXT_CSD register */
#define AX_EMMC_ARG1_BUS_WIDTH_8		 0x03B70200 /**< Value to set bus width to 8 in EXT_CSD register */
#define AX_EMMC_ARG1_HS_TIMING_HI_SPEED	 0x03B90100 /**< Value to set HS_TIMING [185d] to High Speed in EXT_CSD register */
#define AX_EMMC_ARG1_HS_TIMING_HS200	 0x03B90200 /**< Value to set HS_TIMING [185d] to High Speed in EXT_CSD register */
#define AX_EMMC_ARG1_HS_TIMING_HS400	 0x03B90300 /**< Value to set HS_TIMING [185d] to High Speed in EXT_CSD register */
#define AX_EMMC_ARG1_RELATIVE_ADDR		 0xAABB0000 /**< Relative address */
#define AX_EMMC_PRE_IDLE_STATE			 0xF0F0F0F0 /**< Value to Device enter into pre IDLE STATE */
#define AX_EMMC_ARG1_HIGH_VTG_OCR		 0x40ff8080 /**< High voltage operation conditions register */
#define AX_EMMC_VENDOR_AUTO_CLK_GATE	 0x00000002 /**< Value to enable auto clock gate */
#define AX_EMMC_CONTROL1_INT_CLK_DEF_EN	 0x00002001 /**< Value to enable internal clock */
#define AX_EMMC_CONTROL1_INT_CLK_400K_EN 0x00000041 /**< Value to enable internal clock for 400KHz */
#define AX_EMMC_CONTROL1_DATA_TOUT		 0x000E0000 /**< Value to increase data line timeout */
#define AX_EMMC_CONTROL1_INT_CLK_EN_MASK 0xFFFF00FE /**< Mask value to enable internal clock */
#define AX_EMMC_CONTROL1_INT_CLK_EN		 0x00000001 /**< Internal Clock Enable*/
#define AX_EMMC_CONTROL1_INT_CLK_STABLE	 0x00000002 /**< Internal clock stable set */
#define AX_EMMC_CONTROL1_SD_CLK_EN		 0x00000004 /**< Value to enable SD clock */
#define AX_EMMC_CONTROL1_SD_CLK_EN_MASK	 0xFFFFFFFB /**< Mask value to enable SD clock */
#define AX_EMMC_CONTROL1_SW_RESET		 0x01000000 /**< Value to enable SW reset */
#define AX_EMMC_CONTROL1_SW_RESET_CMD	 0x02000000 /**< Software reset for Data line set */
#define AX_EMMC_CONTROL1_SW_RESET_DAT	 0x04000000 /**< Software reset for CMD line set */
#define AX_EMMC_CONTROL0_BUS_VTG_1V8	 0x00000B00 /**< Value to set Bus voltage 1.8v */
#define AX_EMMC_CONTROL0_BUS_VTG_3V3	 0x00000E00 /**< Value to set Bus voltage 3.3v */
#define AX_EMMC_CONTROL0_BUS_8BIT		 0x00000020 /**< @EMMC_BUS_WIDTH: Value to set Bus 8BIT mode */
#define AX_EMMC_CONTROL0_BUS_4BIT		 0x00000002 /**< @EMMC_BUS_WIDTH: Value to set Bus 4BIT mode */
#define AX_EMMC_CONTROL0_BUS_1BIT		 0x00000000 /**< @EMMC_BUS_WIDTH: Value to set Bus 1BIT mode */
#define AX_EMMC_CONTROL0_BUS_POWER_ON	 0x00000100 /**< Value to set Bus power on */
#define AX_EMMC_CONTROL0_BUS_VTG_MASK 	 0xFFFFF0FF /**< Mask value Bus voltage and Bus power */
#define AX_EMMC_CONTROL0_DMA_SDMA		 0x00000000 /**< SDMA is selected */
#define AX_EMMC_CONTROL0_DMA_ADMA1_32BIT 0x00000008 /**< 32-bit Address ADMA1 is selected */
#define AX_EMMC_CONTROL0_DMA_ADMA2_32BIT 0x00000010 /**< 32-bit Address ADMA2 is selected */
#define AX_EMMC_CONTROL0_DMA_ADMA2_64BIT 0x00000018 /**< 64-bit Address ADMA2 is selected */
#define AX_EMMC_CMDTM_DATA_PRESENT	  0x00200000 /**< Value to represent Data present bit */
#define AX_EMMC_CMDTM_DATA_READ		  0x00000010 /**< Data transfer direction set to Read */
#define AX_EMMC_CMDTM_MULTI_BLOCK_EN  0x00000022 /**< Value to enable multi block */
#define AX_EMMC_BLOCK_REG_MAX		  0xffff /**< Block count register maximum value */
#define AX_EMMC_CMDTM_CMD_VALUE		  0x37000000 /**< Mask value for EMMC command */
#define AX_EMMC_CMDTM_AUTOCMD_VALUE	  0x0000000C /**< Mask value for EMMC Auto CMD */
#define AX_EMMC_CMDTM_ACMD12_EN		  0x00000004 /**< ACMD12 is set */
#define AX_EMMC_CMDTM_ACMD23_EN		  0x00000008 /**< ACMD23 is set */
#define AX_EMMC_CMDTM_DMA_EN		  0x00000001 /**< DMA enable */
#define AX_EMMC_CONTROL2_EXEC_TUNING 	(0x40 << 16)
#define AX_EMMC_CONTROL2_TUNED_CLK 	 	(0x80 << 16)

#define AX_EMMC_CMDTM_CMD0 0x00000000 /**< EMMC command 0 - Resets the device to idle state */
#define AX_EMMC_CMDTM_CMD1 0x01000000 /**< EMMC command 1 - Send operating condition of the device*/
#define AX_EMMC_CMDTM_CMD2 0x02000000 /**< EMMC command 2 - Send CID of the device and sends the response*/
#define AX_EMMC_CMDTM_CMD3 0x03000000 /**< EMMC command 3 - Set the RCA to the device and send the response */
#define AX_EMMC_CMDTM_CMD4                                                                                   \
	0x04000000 /**< EMMC command 4 - Used to send CID of the device and sends the response */
#define AX_EMMC_CMDTM_CMD6                                                                                   \
	0x06000000 /**< EMMC command 6 - Used to send CID of the device and sends the response */
#define AX_EMMC_CMDTM_CMD7                                                                                   \
	0x07000000 /**< EMMC command 7 - Used to Select/Deselect the device and sends the response along with RCA */
#define AX_EMMC_CMDTM_CMD8 0x08000000 /**< EMMC command 8 - Send EXT_CSD */
#define AX_EMMC_CMDTM_CMD9                                                                                   \
	0x09000000 /**< EMMC command 9 - Send the device specific data on cmd line and sends the response */
#define AX_EMMC_CMDTM_CMD10                                                                                  \
	0x0A000000 /**< EMMC command 10 - Send the device identification on cmd line and sends the response */
#define AX_EMMC_CMDTM_CMD13 0x0D000000 /**< EMMC command 13 - Used to send the status*/
#define AX_EMMC_CMDTM_CMD15                                                                                  \
	0x0F000000 /**< EMMC command 15 - Set the device into inactive state and send the response */
#define AX_EMMC_CMDTM_CMD16                                                                                  \
	0x10000000 /**< EMMC command 16 - Used to set the blocklength for the data transfer commands and send theresponse*/
#define AX_EMMC_CMDTM_CMD17                                                                                  \
	0x11000000 /**< EMMC command 17 - Used to read single block data and send the response */
#define AX_EMMC_CMDTM_CMD18                                                                                  \
	0x12000000 /**< EMMC command 18 - Read multiple block data until interrupted by AutoCMD12/23 and send the response */
#define AX_EMMC_CMDTM_CMD19 0x13000000
#define AX_EMMC_CMDTM_CMD21 0x15000000
#define AX_EMMC_CMDTM_CMD23 0x17000000 /**< EMMC command 23 - Set Block Count */
#define AX_EMMC_CMDTM_CMD24                                                                                  \
	0x18000000 /**< EMMC command 24 - Writes single block data and send the response */
#define AX_EMMC_CMDTM_CMD25                                                                                  \
	0x19000000 /**< EMMC command 25 - Writes multiple block data until interruptedby AutoCMD12/23 and send theresponse */
#define AX_EMMC_CMDTM_CMD60 0x3C000000 /**< EMMC command 60 - AIO TS1315 Vendor Specific Command */
#define AX_EMMC_CMDTM_CMD61 0x3D000000 /**< EMMC command 61 - AIO TS1315 Vendor Specific Command */

#define AX_EMMC_CMDTM_RESPONSE_R1	0x001A0000 /**< Value for response type R1 (48-bit, CRC, IDX check) */
#define AX_EMMC_CMDTM_RESPONSE_R1B	0x001B0000 /**< Value for response type R1b (R1 + busy) */
#define AX_EMMC_CMDTM_RESPONSE_R2	0x00090000 /**< Value for response type R2 (136-bit, CRC check) */
#define AX_EMMC_CMDTM_RESPONSE_R3R4 0x00020000 /**< Value for response type R3,R4 (48-bit, no CRC/IDX) */

#define AX_EMMC_STATUS_CARD_INSERTED	0x00010000 /**< Detect EMMC card inserted */
#define AX_EMMC_STATUS_WR_TRANSFER_A	0x00000100 /**< Write transfer is active */
#define AX_EMMC_STATUS_RD_TRANSFER_A	0x00000200 /**< Read transfer is active */
#define AX_EMMC_STATUS_CMD_INHIBIT		0x00000001 /**< Inhibit state for command line */
#define AX_EMMC_STATUS_DAT_INHIBIT		0x00000002 /**< Inhibit state for Data line */
#define AX_EMMC_IRPT_STATUS_EN			0x13FFFFFF /**< Enable EMMC normal and error irpts */
#define AX_EMMC_IRPT_STATUS_CARD_EN		0x00000040 /**< Card Insertion Signal Enable */
#define AX_EMMC_IRPT_SIGNAL_CARD_EN		0x00000040 /**< Card Insertion Signal Enable */
#define AX_EMMC_INTERRUPT_ERROR			0x00008000 /**< EMMC error interrupt is set */
#define AX_EMMC_INTERRUPT_CMD_COMPLETE	0x00000001 /**< Command complete interrupt is set */
#define AX_EMMC_INTERRUPT_TRFR_COMPLETE 0x00000002 /**< Transfer complete interrupt is set */
#define AX_EMMC_INTERRUPT_WR_BUF_READY	0x00000010 /**< Detect Write buffer ready state */
#define AX_EMMC_INTERRUPT_RD_BUF_READY	0x00000020 /**< Detect read buffer ready state */
#define AX_EMMC_INTERRUPT_DMA			0x00000008 /**< DMA Interrupt */

#define AX_EMMC_BLKSIZECNT_SDMA_4KB 	0x0000 //000b - 4KB
#define AX_EMMC_BLKSIZECNT_SDMA_8KB 	0x1000 //001b - 8KB
#define AX_EMMC_BLKSIZECNT_SDMA_16KB 	0x2000 //010b - 16KB
#define AX_EMMC_BLKSIZECNT_SDMA_32KB 	0x3000 //011b - 32KB
#define AX_EMMC_BLKSIZECNT_SDMA_64KB 	0x4000 //100b - 64KB
#define AX_EMMC_BLKSIZECNT_SDMA_128KB 	0x5000 //101b - 128KB
#define AX_EMMC_BLKSIZECNT_SDMA_256KB 	0x6000 //110b - 256KB
#define AX_EMMC_BLKSIZECNT_SDMA_512KB 	0x7000 //111b - 512KB
#define AX_EMMC_BLKSIZECNT_512B 		0x0200 //512 Bytes - Block size

#define AX_EMMC_PHY_CTRL_1			  0x38
#define AX_EMMC_PHY_CTRL_2			  0x3C
#define AX_EMMC_PHY_CTRL_3			  0x40
#define AX_EMMC_PHY_STATUS			  0x50
#define AX_EMMC_PHY_CAP_REG_IN_S1_LSB 0x00
#define AX_EMMC_PHY_CAP_REG_IN_S1_MSB 0x04
#define AX_EMMC_PHY_CAP_REG_IN_S1_MSB_CLK_MULTIPLIER 0xC008E

/* eMMC PHY Control Bits */
#define AX_EMMC_PHY_RETB_ENBL	BIT(1)
#define AX_EMMC_PHY_ITAPDLY_WIN BIT(6)
#define AX_EMMC_PHY_ITAPDLY_EN	BIT(5)
#define AX_EMMC_PHY_ITAPDLY_SEL BIT(0)
#define AX_EMMC_PHY_ITAPDLY_VAL 0x2
#define AX_EMMC_PHY_RTRIM_EN	BIT(21)
#define AX_EMMC_PHY_PDB_ENBL	BIT(23)
#define AX_EMMC_PHY_DLL_ENBL	BIT(26)
#define AX_EMMC_PHY_REN_STRB	BIT(27)
#define AX_EMMC_PHY_REN_CMD		BIT(12)
#define AX_EMMC_PHY_REN_DAT0	BIT(13)
#define AX_EMMC_PHY_REN_DAT1	BIT(14)
#define AX_EMMC_PHY_REN_DAT2	BIT(15)
#define AX_EMMC_PHY_REN_DAT3	BIT(16)
#define AX_EMMC_PHY_REN_DAT4	BIT(17)
#define AX_EMMC_PHY_REN_DAT5	BIT(18)
#define AX_EMMC_PHY_REN_DAT6	BIT(19)
#define AX_EMMC_PHY_REN_DAT7	BIT(20)
#define AX_EMMC_PHY_REN_CMD_EN                                                                               \
	(AX_EMMC_PHY_REN_CMD | AX_EMMC_PHY_REN_DAT0 | AX_EMMC_PHY_REN_DAT1 | AX_EMMC_PHY_REN_DAT2 |              \
	 AX_EMMC_PHY_REN_DAT3 | AX_EMMC_PHY_REN_DAT4 | AX_EMMC_PHY_REN_DAT5 | AX_EMMC_PHY_REN_DAT6 |             \
	 AX_EMMC_PHY_REN_DAT7)
#define AX_EMMC_PHY_OD_CMD	BIT(2)
#define AX_EMMC_PHY_PU_CMD	BIT(3)
#define AX_EMMC_PHY_PU_DAT0 BIT(4)
#define AX_EMMC_PHY_PU_DAT1 BIT(5)
#define AX_EMMC_PHY_PU_DAT2 BIT(6)
#define AX_EMMC_PHY_PU_DAT3 BIT(7)
#define AX_EMMC_PHY_PU_DAT4 BIT(8)
#define AX_EMMC_PHY_PU_DAT5 BIT(9)
#define AX_EMMC_PHY_PU_DAT6 BIT(10)
#define AX_EMMC_PHY_PU_DAT7 BIT(11)
#define AX_EMMC_PHY_PU_CMD_EN                                                                                \
	(AX_EMMC_PHY_OD_CMD | AX_EMMC_PHY_PU_CMD | AX_EMMC_PHY_PU_DAT0 | AX_EMMC_PHY_PU_DAT1 |                   \
	 AX_EMMC_PHY_PU_DAT2 | AX_EMMC_PHY_PU_DAT3 | AX_EMMC_PHY_PU_DAT4 | AX_EMMC_PHY_PU_DAT5 |                 \
	 AX_EMMC_PHY_PU_DAT6 | AX_EMMC_PHY_PU_DAT7)
#define AX_EMMC_PHY_PU_CMD_EN_NEW                                                                            \
	(AX_EMMC_PHY_PU_CMD | AX_EMMC_PHY_PU_DAT0 | AX_EMMC_PHY_PU_DAT1 |                   \
	 AX_EMMC_PHY_PU_DAT2 | AX_EMMC_PHY_PU_DAT3 | AX_EMMC_PHY_PU_DAT4 | AX_EMMC_PHY_PU_DAT5 |                 \
	 AX_EMMC_PHY_PU_DAT6 | AX_EMMC_PHY_PU_DAT7) // Refer Linux driver
#define AX_EMMC_PHY_PUOD_CMD	  (AX_EMMC_PHY_OD_CMD | AX_EMMC_PHY_PU_CMD)
#define AX_EMMC_PHY_OTAPDLY_EN	  BIT(11)
#define AX_EMMC_PHY_SEL_DLY_RXCLK BIT(18)
#define AX_EMMC_PHY_SEL_DLY_TXCLK BIT(19)
#define AX_EMMC_PHY_CALDONE_MASK  0x40
#define AX_EMMC_PHY_DLL_RDY_MASK  0x1
#define AX_EMMC_PHY_MAX_CLK_BUF0  BIT(20)
#define AX_EMMC_PHY_MAX_CLK_BUF1  BIT(21)
#define AX_EMMC_PHY_MAX_CLK_BUF2  BIT(22)

/* eMMC PHY CTRL_2 */
#define AX_EMMC_PHY_OD_STRB BIT(18)
#define AX_EMMC_PHY_OD_DAT0 BIT(19)
#define AX_EMMC_PHY_OD_DAT1 BIT(20)
#define AX_EMMC_PHY_OD_DAT2 BIT(21)
#define AX_EMMC_PHY_OD_DAT3 BIT(22)
#define AX_EMMC_PHY_OD_DAT4 BIT(23)
#define AX_EMMC_PHY_OD_DAT5 BIT(24)
#define AX_EMMC_PHY_OD_DAT6 BIT(25)
#define AX_EMMC_PHY_OD_DAT7 BIT(26)
#define AX_EMMC_PHY_PU_STRB BIT(28)
#define AX_EMMC_PHY_OD_DATA_EN                                                                               \
	(AX_EMMC_PHY_OD_DAT0 | AX_EMMC_PHY_OD_DAT1 | AX_EMMC_PHY_OD_DAT2 | AX_EMMC_PHY_OD_DAT3 |                 \
	 AX_EMMC_PHY_OD_DAT4 | AX_EMMC_PHY_OD_DAT5 | AX_EMMC_PHY_OD_DAT6 | AX_EMMC_PHY_OD_DAT7)


#define AX_EMMC_PHY_CTRL_1_ENDLL	  		0x04000000 /**< Enable or disable the eMMC PHY DLL logic */
#define AX_EMMC_PHY_CTRL_3_FREQSEL_MASK		0x0E000000 
#define AX_EMMC_PHY_CTRL_3_FREQSEL_HS		0x08000000 // Refer table 100 / Arasan Host Controller datasheet
#define AX_EMMC_PHY_CTRL_3_FREQSEL_DDR50	0x08000000 // Refer table 100 / Arasan Host Controller datasheet
#define AX_EMMC_PHY_CTRL_3_FREQSEL_HS200	0x00000000 // Refer table 100 / Arasan Host Controller datasheet
#define AX_EMMC_PHY_CTRL_3_FREQSEL_HS400	0x00000000 // Refer table 100 / Arasan Host Controller datasheet
#define AX_EMMC_PHY_STATUS_DLLRDY			0x00000001 // DLL is ready

/* Timeout values */
#define AX_EMMC_PHY_LOOP_TIMEOUT 30000

/** @defgroup emmc_fw_update_args Command Arguments for AIO TS1315 eMMC FW Update
 * @{
 */

/* Arguments for CMD61 (Vendor Specific) */
#define ARG_VENDOR_CMD61_ENTER_ROM 0x50473152 /**< Argument for CMD61 to enter ROM mode */
#define ARG_CMD0_GO_IDLE		   0x00000000 /**< Argument for CMD0 to go idle */
#define ARG_SEND_OP_COND_PDF	   0x40FF8080 /**< Argument for CMD8 to send operation conditions */
#define ARG_RCA_DEFAULT			   0x00010000 /**< Default RCA used in PDF FW update sequences */

/* Arguments for CMD60 (Vendor Specific) */
#define ARG_VCMD60_GET_FW_INFO_1 0x20110616 /**< Argument 1 for CMD60 to get FW version info */
#define ARG_VCMD60_GET_FW_INFO_2 0x00000300 /**< Argument 2 for CMD60 to get FW version info */
#define ARG_VCMD60_TRANSFER_CST                                                                              \
	0x00000002 /**< Argument for CMD60 to transfer CST data (PKGTest and FullFW) */
#define ARG_VCMD60_TRANSFER_PKGTESTFW 0x00000005 /**< Argument for CMD60 to transfer PKGTestFW image */
#define ARG_VCMD60_RUN_PKGTESTFW	  0x00000006 /**< Argument for CMD60 to run PKGTestFW */
#define ARG_VCMD60_EN_DECRYPTION	  0x00000008 /**< Argument for CMD60 to enable decryption */
#define ARG_VCMD60_PROGRAM_NAND_EFUSE 0x00000910 /**< Argument for CMD60 to program NAND info to eFuse */
#define ARG_VCMD60_ALL_BLOCK_ERASE	  0x00000011 /**< Argument for CMD60 for All Block Erase */
#define ARG_VCMD60_TRANSFER_FULLFW	  0x00000003 /**< Argument for CMD60 to transfer Full FW image */
/** @} */ // end of emmc_fw_update_args


#define CONFIG_EMMC_STORAGE_SIZE 16
#define CONFIG_EMMC_BLOCK_SIZE 512

#define AX_EMMC_BLOCK_SIZE	0x00000200 /**< Value for Block size 512 bytes */
#define AX_EMMC_1BLOCK		0x00010000 /**< Value for num of blocks=1 */
#define AX_EMMC_CARD_PWR_UP 0x80000000 /**< EMMC card power up status set */
#define DELAY_LOOP_COUNT	200 /**< delay loop count value for 100 milliseconds with 1ms loop */
#define MILLISECOND_1		1 /**< Timer delay value for 1 msleep */
#define ONEGB_TO_BYTE_COUNT 0x40000000 /**< 1GB in byte value */
#define EMMC_TOTAL_BYTE_COUNT                                                                                \
	((long long)CONFIG_EMMC_STORAGE_SIZE * ONEGB_TO_BYTE_COUNT) /**< eMMC storage size in bytes */
#define AX_EMMC_BLOCK_COUNT (EMMC_TOTAL_BYTE_COUNT / CONFIG_EMMC_BLOCK_SIZE) /**< eMMC block count */

// Descriptor table
#define ADMA_DESC_MAX_LEN    0xFFFF   // 65535 bytes per descriptor
#define ADMA_ACT_TRAN        (0x2 << 4)  // 01b in bits 5:4
#define ADMA_ACT_LINK        (0x3 << 4)
#define ADMA_VALID           (0x1 << 0)
#define ADMA_END             (0x1 << 1)
#define ADMA_INT             (0x1 << 2)

#define EMMC_CHECK_VALID(emmc) (!(emmc) || (!(emmc->reg_base)))

struct arasan_emmc_ctrl_t {
	uint32_t version; /**< Version indicating compatibility with this API */
	void __iomem *reg_base; /**< base address of eMMC instance */
	void __iomem * phy_base; /**< PHY base address of eMMC instance */
	emmc_speed_mode_t speed_mode; /* @EMMC_SPEED_MODE */
	emmc_bus_width_t bus_width; /* @EMMC_BUS_WIDTH */
	uint8_t sdclk_freq_select;
};

typedef struct {
    uint16_t attr;    // Control + attributes (bits [7:0])
    uint16_t length;  // Length in bytes (max 65535)
    uint32_t addr;    // Data buffer address (4-byte aligned)
} adma_desc_t;

typedef struct {
    adma_desc_t line1;
    adma_desc_t line2;
} desp_t; // only 2 lines for IP testing

#define EMMC_HI_SPEED_ENABLE 	1
#define EMMC_HI_SPEED_DISABLE 	0
#define EMMC_DLL_ENABLE			1
#define EMMC_DLL_DISABLE		0
#endif /* _AX_HAL_AX_EMMC_REGS_H */

//#endif
