// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef SPI_DW_REGS_H
#define SPI_DW_REGS_H

// Offset SPI Monitor
#define SPI_MON_CFG   0x0
#define SPI_MON_CS    0x100

#define DWC_ssi_address_block_BaseAddress 0x0

/* Register CTRLR0 /
/ This register controls the serial data transfer. It is impossible to write to this register when the DWC_ssi is enabled. */
#define CTRLR0					  (DWC_ssi_address_block_BaseAddress + 0x0)
#define CTRLR0_RegisterSize		  32
#define CTRLR0_RegisterResetValue 0x80000007
#define CTRLR0_RegisterResetMask  0xffffffff

/* Register Field information for CTRLR0 */

/* Register CTRLR0 field DFS /
/ Data Frame Size.
Selects the data frame length. When the data frame size is programmed to be less than 32 bits, the receive data is automatically right-justified by the receive logic, with the upper bits of the receive FIFO zero-padded.
You must right-justify transmit data before writing into the transmit FIFO. The transmit logic ignores the upper unused bits when transmitting the data. 

Note: When SSIC_SPI_MODE is set to "Dual", "Quad" or "Octal" mode and SPI_FRF is not set to 2'b00:
DFS value must be a multiple of 2 if SPI_FRF = 01
DFS value must be multiple of 4 if SPI_FRF = 10
DFS value must be multiple of 8 if SPI_FRF = 11 */
#define CTRLR0_DFS_BitAddressOffset 0
#define CTRLR0_DFS_RegisterSize		5

/* Register CTRLR0 field RSVD_CTRLR0_5 /
/ Reserved bits - read as zero */
#define CTRLR0_RSVD_CTRLR0_5_BitAddressOffset 5
#define CTRLR0_RSVD_CTRLR0_5_RegisterSize	  1

/* Register CTRLR0 field FRF /
/ Frame Format.
Selects which serial protocol transfers the data. */
#define CTRLR0_FRF_BitAddressOffset 6
#define CTRLR0_FRF_RegisterSize		2

/* Register CTRLR0 field SCPH /
/ Serial Clock Phase.
Valid when the frame format (FRF) is set to Motorola SPI. The serial clock phase selects the relationship of the serial clock with the chip select signal.
When SCPH = 0, data are captured on the first edge of the serial clock. When SCPH = 1, the serial clock starts toggling one cycle after the chip select line is activated, and data are captured on the second edge of the serial clock. */
#define CTRLR0_SCPH_BitAddressOffset 8
#define CTRLR0_SCPH_RegisterSize	 1

/* Register CTRLR0 field SCPOL /
/ Serial Clock Polarity.
Valid when the frame format (FRF) is set to Motorola SPI. Used to select the polarity of the inactive serial clock, which is held inactive when the DWC_ssi controller is not actively transferring data on the serial bus. */
#define CTRLR0_SCPOL_BitAddressOffset 9
#define CTRLR0_SCPOL_RegisterSize	  1

/* Register CTRLR0 field TMOD /
/ Transfer Mode.
Selects the mode of transfer for serial communication. This field does not affect the transfer duplicity. Only indicates whether the receive or transmit data are valid.
In transmit-only mode, data received from the external device is not valid and is not stored in the receive FIFO memory; it is overwritten on the next transfer.
In receive-only mode, transmitted data are not valid. After the first write to the transmit FIFO, the same word is retransmitted for the duration of the transfer.
In transmit-and-receive mode, both transmit and receive data are valid. The transfer continues until the transmit FIFO is empty. Data received from the external device are stored into the receive FIFO memory, where it can be accessed by the host processor. */
#define CTRLR0_TMOD_BitAddressOffset 10
#define CTRLR0_TMOD_RegisterSize	 2

/* Register CTRLR0 field SLV_OE /
/ Target Output Enable.
Relevant only when the DWC_ssi is configured as a serial-target
device. When configured as a serial controller, this bit field has no functionality. This bit enables or disables the setting of the ssi_oe_n output from the DWC_ssi serial target. When SLV_OE = 1, the ssi_oe_n output can never be active. When the ssi_oe_n output controls the tri-state buffer on the txd output from the target, a high impedance state is always present on the target txd output when SLV_OE = 1.
This is useful when the controller transmits in broadcast mode (controller transmits data to all target devices). Only one target may respond with data on the controller rxd line. This bit is enabled after reset and must be disabled by software (when broadcast mode is used), if you do not want this device to respond with data.
When SSIC_SLV_SPI_MODE is set to 1 and SPI is programmed to work in Enhanced SPI mode, then for correct operation this bit should be programmed to 0. */
#define CTRLR0_SLV_OE_BitAddressOffset 12
#define CTRLR0_SLV_OE_RegisterSize	   1

/* Register CTRLR0 field SRL /
/ Shift Register Loop.
Used for testing purposes only. When internally
active, connects the transmit shift register output to the receive shift register input. Can be used in both serial-target and serial-controller modes. When the DWC_ssi is configured as a target in loopback mode, the ss_in_n and ssi_clk signals must be provided by an external source. In this mode, the target cannot generate these signals because there is nothing to which to loop back. */
#define CTRLR0_SRL_BitAddressOffset 13
#define CTRLR0_SRL_RegisterSize		1

/* Register CTRLR0 field SSTE /
/ Target Select Toggle Enable.
While operating in SPI mode with clock phase (SCPH) set to 0, this register controls the behavior of the chip select line (ss_*_n) between data frames. */
#define CTRLR0_SSTE_BitAddressOffset 14
#define CTRLR0_SSTE_RegisterSize	 1

/* Register CTRLR0 field RSVD_CTRLR0_15 /
/ Reserved bits - read as zero */
#define CTRLR0_RSVD_CTRLR0_15_BitAddressOffset 15
#define CTRLR0_RSVD_CTRLR0_15_RegisterSize	   1

/* Register CTRLR0 field CFS /
/ Control Frame Size.
Selects the length of the control word for the Microwire frame format. */
#define CTRLR0_CFS_BitAddressOffset 16
#define CTRLR0_CFS_RegisterSize		4

/* Register CTRLR0 field RSVD_CTRLR0_20_21 /
/ Reserved bits - read as zero */
#define CTRLR0_RSVD_CTRLR0_20_21_BitAddressOffset 20
#define CTRLR0_RSVD_CTRLR0_20_21_RegisterSize	  2

/* Register CTRLR0 field SPI_FRF /
/ SPI Frame Format
Selects data frame format for Transmitting/Receiving the data. Bits only valid when SSIC_SPI_MODE is either set to "Dual" or "Quad" or "Octal" mode. */
#define CTRLR0_SPI_FRF_BitAddressOffset 22
#define CTRLR0_SPI_FRF_RegisterSize		2

/* Register CTRLR0 field SPI_HYPERBUS_EN /
/ SPI Hyperbus Frame format enable.
Selects if data frame format for Transmitting/Receiving the data is in Hyperbus mode. This field is effective only when CTRLR0.FRF is set to SPI frame format. */
#define CTRLR0_SPI_HYPERBUS_EN_BitAddressOffset 24
#define CTRLR0_SPI_HYPERBUS_EN_RegisterSize		1

/* Register CTRLR0 field SPI_DWS_EN /
/ Enable Dynamic wait states in SPI mode of operation. This field is only applicable when CTRLR0.FRF is set to 0 (Motorola SPI Frame Format). */
#define CTRLR0_SPI_DWS_EN_BitAddressOffset 25
#define CTRLR0_SPI_DWS_EN_RegisterSize	   1

/* Register CTRLR0 field CLK_LOOP_EN /
/ Clock loop back enable bit. Once this bit is set to 1, DWC_ssi will use looped back clock (mst_sclk_in) to capture read data */
#define CTRLR0_CLK_LOOP_EN_BitAddressOffset 26
#define CTRLR0_CLK_LOOP_EN_RegisterSize		1

/* Register CTRLR0 field RSVD_CTRLR0_27_30 /
/ Reserved bits - read as zero */
#define CTRLR0_RSVD_CTRLR0_27_30_BitAddressOffset 27
#define CTRLR0_RSVD_CTRLR0_27_30_RegisterSize	  4

/* Register CTRLR0 field SSI_IS_MST /
/ This field selects if DWC_ssi is working in Controller or Target mode */
#define CTRLR0_SSI_IS_MST_BitAddressOffset 31
#define CTRLR0_SSI_IS_MST_RegisterSize	   1

/* End of Register Definition for CTRLR0 */

/* Register CTRLR1 /
/ This register exists only when the DWC_ssi is configured as a controller device. When the DWC_ssi is configured as a serial target, writing to this location has no effect; reading from this location returns 0. Control register 1 controls the end of serial transfers when in receive-only mode. It is impossible to write to this register when the DWC_ssi is enabled. */
#define CTRLR1					  (DWC_ssi_address_block_BaseAddress + 0x4)
#define CTRLR1_RegisterSize		  32
#define CTRLR1_RegisterResetValue 0x0
#define CTRLR1_RegisterResetMask  0xffffffff

/* Register Field information for CTRLR1 */

/* Register CTRLR1 field NDF /
/ Number of Data Frames.
When TMOD = 10 or TMOD = 11 , this register field sets the number of data frames to be continuously received by the DWC_ssi. The DWC_ssi continues to receive serial data until the number of data frames received is equal to this register value plus 1, which enables you to receive up to 256 KB of data in a continuous transfer.
 When SPI_CTRLR0.CLK_STRETCH_EN=1 and TMOD = 01, this register field sets the number of data frames to be continuously transmitted by DWC_ssi. If the Transmit FIFO goes empty in-between, DWC_ssi masks the serial clock (sclk_out) and wait for rest of the data until the programmed amount of frames are transferred successfully.
When the DWC_ssi is configured as a serial target, the transfer continues for as long as the target is selected. Therefore, this register serves no purpose and is not present when the DWC_ssi is configured as a serial target. */
#define CTRLR1_NDF_BitAddressOffset 0
#define CTRLR1_NDF_RegisterSize		16

/* Register CTRLR1 field RSVD_CTRLR1 /
/ Reserved bits - read as zero */
#define CTRLR1_RSVD_CTRLR1_BitAddressOffset 16
#define CTRLR1_RSVD_CTRLR1_RegisterSize		16

/* End of Register Definition for CTRLR1 */

/* Register SSIENR /
/ This register enables and disables the DWC_ssi. */
#define SSIENR					  (DWC_ssi_address_block_BaseAddress + 0x8)
#define SSIENR_RegisterSize		  32
#define SSIENR_RegisterResetValue 0x0
#define SSIENR_RegisterResetMask  0xffffffff

/* Register Field information for SSIENR */

/* Register SSIENR field SSIC_EN /
/ SSI Enable.
Enables and disables all DWC_ssi operations. When disabled, all serial transfers are halted immediately. Transmit and receive FIFO buffers are cleared when the device is disabled. It is impossible to program some of the DWC_ssi control registers when enabled. When disabled, the ssi sleep output is set (after delay) to inform the system that it is safe to remove the ssi_clk, thus saving power consumption in the system. */
#define SSIENR_SSIC_EN_BitAddressOffset 0
#define SSIENR_SSIC_EN_RegisterSize		1

/* Register SSIENR field RSVD_SSIENR /
/ Reserved bits - read as zero */
#define SSIENR_RSVD_SSIENR_BitAddressOffset 1
#define SSIENR_RSVD_SSIENR_RegisterSize		31

/* End of Register Definition for SSIENR */

/* Register MWCR /
/ This register controls the direction of the data word for the half-duplex Microwire serial protocol. It is impossible to write to this register when the DWC_ssi is enabled. */
#define MWCR					(DWC_ssi_address_block_BaseAddress + 0xc)
#define MWCR_RegisterSize		32
#define MWCR_RegisterResetValue 0x0
#define MWCR_RegisterResetMask	0xffffffff

/* Register Field information for MWCR */

/* Register MWCR field MWMOD /
/ Microwire Transfer Mode.
Defines whether the Microwire transfer is sequential or non-sequential. When sequential mode is used, only one control word is needed to transmit or receive a block of data words. When non-sequential mode is used, there must be a control word for each data word that is transmitted or received. */
#define MWCR_MWMOD_BitAddressOffset 0
#define MWCR_MWMOD_RegisterSize		1

/* Register MWCR field MDD /
/ Microwire Control.
Defines the direction of the data word when the Microwire serial protocol is used. When this bit is set to 0, the data word is received by the DWC_ssi MacroCell from the external serial device. When this bit is set to 1, the data word is transmitted from the DWC_ssi MacroCell to the external serial device. */
#define MWCR_MDD_BitAddressOffset 1
#define MWCR_MDD_RegisterSize	  1

/* Register MWCR field MHS /
/ Microwire Handshaking.
Relevant only when the DWC_ssi is configured as a serial-controller device. When configured as a serial target, this bit field has no functionality. Used to enable and disable the busy/ready handshaking interface for the Microwire protocol. When enabled, the DWC_ssi checks for a ready status from the target target, after the transfer of the last data/control bit, before clearing the BUSY status in the SR register. */
#define MWCR_MHS_BitAddressOffset 2
#define MWCR_MHS_RegisterSize	  1

/* Register MWCR field RSVD_MWCR /
/ Reserved bits - read as zero */
#define MWCR_RSVD_MWCR_BitAddressOffset 3
#define MWCR_RSVD_MWCR_RegisterSize		29

/* End of Register Definition for MWCR */

/* Register SER /
/ This register is valid only when the DWC_ssi is configured as a controller device. When the DWC_ssi is configured as a serial target, writing to this location has no effect; reading from this location returns 0. The register enables the individual chip select output lines from the DWC_ssi controller. Up to 16 chip-select output pins are available on the DWC_ssi controller. You cannot write to this register when DWC_ssi is busy and when SSIC_EN = 1. */
#define SER					   (DWC_ssi_address_block_BaseAddress + 0x10)
#define SER_RegisterSize	   32
#define SER_RegisterResetValue 0x0
#define SER_RegisterResetMask  0xffffffff

/* Register Field information for SER */

/* Register SER field SER /
/ Chip Select Enable Flag.
Each bit in this register corresponds to a chip select line (ss_x_n) from the DWC_ssi controller. When a bit in this register is set (1), the corresponding chip select line from the controller is activated when a serial transfer begins. It should be noted that setting or clearing bits in this register have no effect on the corresponding chip select outputs until a transfer is started. Before beginning a transfer, you should enable the bit in this register that corresponds to the target device with which the controller wants to communicate. When not operating in broadcast mode, only one bit in this field should be set. */
#define SER_SER_BitAddressOffset 0
#define SER_SER_RegisterSize	 4

/* Register SER field RSVD_SER /
/ Reserved bits - read as zero */
#define SER_RSVD_SER_BitAddressOffset 4
#define SER_RSVD_SER_RegisterSize	  28

/* End of Register Definition for SER */

/* Register BAUDR /
/ This register is valid only when the DWC_ssi is configured as a controller device. When the DWC_ssi is configured as a serial target, writing to this location has no effect; reading from this location returns 0. The register derives the frequency of the serial clock that regulates the data transfer. The 16-bit field in this register defines the ssi_clk divider value. It is impossible to write to this register when the DWC_ssi is enabled. */
#define BAUDR					 (DWC_ssi_address_block_BaseAddress + 0x14)
#define BAUDR_RegisterSize		 32
#define BAUDR_RegisterResetValue 0x2
#define BAUDR_RegisterResetMask	 0xffffffff

/* Register Field information for BAUDR */

/* Register BAUDR field RSVD_BAUDR_0 /
/ Reserved bits - read as zero */
#define BAUDR_RSVD_BAUDR_0_BitAddressOffset 0
#define BAUDR_RSVD_BAUDR_0_RegisterSize		1

/* Register BAUDR field SCKDV /
/ SSI Clock Divider.
The LSB for this field is always set to 0 and is unaffected by a write operation, which ensures an even value is held in this register. If the value is 0, the serial output clock (sclk_out) is disabled. The frequency of the sclk_out is derived from the following equation:

Fsclk_out = Fssi_clk/BAUDR
where BAUDR is any even value between 2 and 65534 (BAUDR = {SCKDV*2}). For example: for Fssi_clk = 3.6864MHz and BAUDR =2 Fsclk_out = 3.6864/2 = 1.8432MHz */
#define BAUDR_SCKDV_BitAddressOffset 1
#define BAUDR_SCKDV_RegisterSize	 15

/* Register BAUDR field RSVD_BAUDR_16_31 /
/ Reserved bits - read as zero */
#define BAUDR_RSVD_BAUDR_16_31_BitAddressOffset 16
#define BAUDR_RSVD_BAUDR_16_31_RegisterSize		16

/* End of Register Definition for BAUDR */

/* Register TXFTLR /
/ This register controls the threshold value for the transmit FIFO memory.. */
#define TXFTLR					  (DWC_ssi_address_block_BaseAddress + 0x18)
#define TXFTLR_RegisterSize		  32
#define TXFTLR_RegisterResetValue 0x0
#define TXFTLR_RegisterResetMask  0xffffffff

/* Register Field information for TXFTLR */

/* Register TXFTLR field TFT /
/ Transmit FIFO Threshold.
Controls the level of entries (or below) at which the transmit FIFO controller triggers an interrupt. The FIFO depth is configurable in the range 8-256; this register is sized to the number of address bits needed to access the FIFO. If you attempt to set this value greater than or equal to the depth of the FIFO, this field is not written and retains its current value. When the number of transmit FIFO entries is less than or equal to this value, the transmit FIFO empty interrupt is triggered. */
#define TXFTLR_TFT_BitAddressOffset 0
#define TXFTLR_TFT_RegisterSize		6

/* Register TXFTLR field RSVD_TXFTLR /
/ Reserved bits - read as zero */
#define TXFTLR_RSVD_TXFTLR_BitAddressOffset 6
#define TXFTLR_RSVD_TXFTLR_RegisterSize		10

/* Register TXFTLR field TXFTHR /
/ Transfer start FIFO level.
Used to control the level of entries in transmit FIFO above which transfer will start on serial line. This register can be used to ensure that sufficient data is present in transmit FIFO before starting a write operation on serial line.
In Internal DMA mode, this field sets the minimum amount of data frames present in the FIFO after which DWC_ssi starts the transfer.
This field is valid only for Controller mode of operation. */
#define TXFTLR_TXFTHR_BitAddressOffset 16
#define TXFTLR_TXFTHR_RegisterSize	   6

/* Register TXFTLR field RSVD_TXFTHR /
/ Reserved bits - read as zero */
#define TXFTLR_RSVD_TXFTHR_BitAddressOffset 22
#define TXFTLR_RSVD_TXFTHR_RegisterSize		10

/* End of Register Definition for TXFTLR */

/* Register RXFTLR /
/ This register controls the threshold value for the receive FIFO memory.. */
#define RXFTLR					  (DWC_ssi_address_block_BaseAddress + 0x1c)
#define RXFTLR_RegisterSize		  32
#define RXFTLR_RegisterResetValue 0x0
#define RXFTLR_RegisterResetMask  0xffffffff

/* Register Field information for RXFTLR */

/* Register RXFTLR field RFT /
/ Receive FIFO Threshold.
Controls the level of entries (or above) at which the receive FIFO controller triggers an interrupt. The FIFO depth is configurable in the range 8-256. This register is sized to the number of address bits needed to access the FIFO. If you attempt to set this value greater than the depth of the FIFO, this field is not written and retains its current value. When the number of receive FIFO entries is greater than or equal to this value + 1, the receive FIFO full interrupt is triggered. */
#define RXFTLR_RFT_BitAddressOffset 0
#define RXFTLR_RFT_RegisterSize		6

/* Register RXFTLR field RSVD_RXFTLR /
/ Reserved bits - read as zero */
#define RXFTLR_RSVD_RXFTLR_BitAddressOffset 6
#define RXFTLR_RSVD_RXFTLR_RegisterSize		26

/* End of Register Definition for RXFTLR */

/* Register TXFLR /
/ This register contains the number of valid data entries in the transmit FIFO memory. */
#define TXFLR					 (DWC_ssi_address_block_BaseAddress + 0x20)
#define TXFLR_RegisterSize		 32
#define TXFLR_RegisterResetValue 0x0
#define TXFLR_RegisterResetMask	 0xffffffff

/* Register Field information for TXFLR */

/* Register TXFLR field TXTFL /
/ Transmit FIFO Level.
Contains the number of valid data entries in the transmit FIFO. */
#define TXFLR_TXTFL_BitAddressOffset 0
#define TXFLR_TXTFL_RegisterSize	 7

/* Register TXFLR field RSVD_TXFLR /
/ Reserved bits - read as zero */
#define TXFLR_RSVD_TXFLR_BitAddressOffset 7
#define TXFLR_RSVD_TXFLR_RegisterSize	  25

/* End of Register Definition for TXFLR */

/* Register RXFLR /
/ This register contains the number of valid data entries in the receive FIFO memory. This register can be read at any time. */
#define RXFLR					 (DWC_ssi_address_block_BaseAddress + 0x24)
#define RXFLR_RegisterSize		 32
#define RXFLR_RegisterResetValue 0x0
#define RXFLR_RegisterResetMask	 0xffffffff

/* Register Field information for RXFLR */

/* Register RXFLR field RXTFL /
/ Receive FIFO Level.
Contains the number of valid data entries in the receive FIFO. */
#define RXFLR_RXTFL_BitAddressOffset 0
#define RXFLR_RXTFL_RegisterSize	 7

/* Register RXFLR field RSVD_RXFLR /
/ Reserved bits - read as zero */
#define RXFLR_RSVD_RXFLR_BitAddressOffset 7
#define RXFLR_RSVD_RXFLR_RegisterSize	  25

/* End of Register Definition for RXFLR */

/* Register SR /
/ This is a read-only register used to indicate the current transfer status, FIFO status, and any transmission/reception errors that may have occurred. The status register may be read at any time. None of the bits in this register request an interrupt. */
#define SR					  (DWC_ssi_address_block_BaseAddress + 0x28)
#define SR_RegisterSize		  32
#define SR_RegisterResetValue 0x6
#define SR_RegisterResetMask  0xffffffff

/* Register Field information for SR */

/* Register SR field BUSY /
/ SSI Busy Flag.
When set, indicates that a serial transfer is in progress; when cleared indicates that the DWC_ssi is idle or disabled. */
#define SR_BUSY_BitAddressOffset 0
#define SR_BUSY_RegisterSize	 1

/* Register SR field TFNF /
/ Transmit FIFO Not Full.
Set when the transmit FIFO contains one or more empty locations, and is cleared when the FIFO is full. */
#define SR_TFNF_BitAddressOffset 1
#define SR_TFNF_RegisterSize	 1

/* Register SR field TFE /
/ Transmit FIFO Empty.
When the transmit FIFO is completely empty, this bit is set. When the transmit FIFO contains one or more valid entries, this bit is cleared. This bit field does not request an interrupt. */
#define SR_TFE_BitAddressOffset 2
#define SR_TFE_RegisterSize		1

/* Register SR field RFNE /
/ Receive FIFO Not Empty.
Set when the receive FIFO contains one or more entries and is cleared when the receive FIFO is empty. This bit can be polled by software to completely empty the receive FIFO. */
#define SR_RFNE_BitAddressOffset 3
#define SR_RFNE_RegisterSize	 1

/* Register SR field RFF /
/ Receive FIFO Full.
When the receive FIFO is completely full, this bit is set. When the receive FIFO contains one or more empty location, this bit is cleared. */
#define SR_RFF_BitAddressOffset 4
#define SR_RFF_RegisterSize		1

/* Register SR field TXE /
/ Transmission Error.
Set if the transmit FIFO is empty when a transfer is started. This bit can be set only when the DWC_ssi is configured as a target device. Data from the previous transmission is resent on the txd line. This bit is cleared when read. */
#define SR_TXE_BitAddressOffset 5
#define SR_TXE_RegisterSize		1

/* Register SR field DCOL /
/ Data Collision Error.
Relevant only when the DWC_ssi is configured as a controller device. This bit will be set if ss_in_n input is asserted by other controller, when the DWC_ssi controller is in the middle of the transfer. This informs the processor that the last data transfer was halted before completion. This bit is cleared when read. */
#define SR_DCOL_BitAddressOffset 6
#define SR_DCOL_RegisterSize	 1

/* Register SR field RSVD_SR /
/ Reserved bits - read as zero */
#define SR_RSVD_SR_BitAddressOffset 7
#define SR_RSVD_SR_RegisterSize		8

/* Register SR field CMPLTD_DF /
/ Completed Data frames
This field indicates total data frames transferred in the previous internal DMA transfer */
#define SR_CMPLTD_DF_BitAddressOffset 15
#define SR_CMPLTD_DF_RegisterSize	  17

/* End of Register Definition for SR */

/* Register IMR /
/ This read/write register masks or enables all interrupts generated by the DWC_ssi. When the DWC_ssi is configured as a target device, the MSTIM bit field is not present. This changes the reset value from 0x3F for serial-controller configurations to 0x1F for serial-target configurations. */
#define IMR					   (DWC_ssi_address_block_BaseAddress + 0x2c)
#define IMR_RegisterSize	   32
#define IMR_RegisterResetValue 0x43f
#define IMR_RegisterResetMask  0xffffffff

/* Register Field information for IMR */

/* Register IMR field TXEIM /
/ Transmit FIFO Empty Interrupt Mask */
#define IMR_TXEIM_BitAddressOffset 0
#define IMR_TXEIM_RegisterSize	   1

/* Register IMR field TXOIM /
/ Transmit FIFO Overflow Interrupt Mask */
#define IMR_TXOIM_BitAddressOffset 1
#define IMR_TXOIM_RegisterSize	   1

/* Register IMR field RXUIM /
/ Receive FIFO Underflow Interrupt Mask */
#define IMR_RXUIM_BitAddressOffset 2
#define IMR_RXUIM_RegisterSize	   1

/* Register IMR field RXOIM /
/ Receive FIFO Overflow Interrupt Mask */
#define IMR_RXOIM_BitAddressOffset 3
#define IMR_RXOIM_RegisterSize	   1

/* Register IMR field RXFIM /
/ Receive FIFO Full Interrupt Mask
0 - ssi_rxf_intr interrupt is masked
1 - ssi_rxf_intr interrupt is not masked */
#define IMR_RXFIM_BitAddressOffset 4
#define IMR_RXFIM_RegisterSize	   1

/* Register IMR field MSTIM /
/ Multi-Controller Contention Interrupt Mask. This bit field is not present if the DWC_ssi is configured as a serial-controller device. */
#define IMR_MSTIM_BitAddressOffset 5
#define IMR_MSTIM_RegisterSize	   1

/* Register IMR field XRXOIM /
/ XIP Receive FIFO Overflow Interrupt Mask */
#define IMR_XRXOIM_BitAddressOffset 6
#define IMR_XRXOIM_RegisterSize		1

/* Register IMR field TXUIM /
/ Transmit FIFO Underflow Interrupt Mask */
#define IMR_TXUIM_BitAddressOffset 7
#define IMR_TXUIM_RegisterSize	   1

/* Register IMR field AXIEM /
/ AXI Error Interrupt Mask */
#define IMR_AXIEM_BitAddressOffset 8
#define IMR_AXIEM_RegisterSize	   1

/* Register IMR field RSVD_9_IMR /
/ Reserved bits - read as zero */
#define IMR_RSVD_9_IMR_BitAddressOffset 9
#define IMR_RSVD_9_IMR_RegisterSize		1

/* Register IMR field SPITEM /
/ SPI Transmit Error Interrupt Mask */
#define IMR_SPITEM_BitAddressOffset 10
#define IMR_SPITEM_RegisterSize		1

/* Register IMR field DONEM /
/ SSI Done Interrupt Mask */
#define IMR_DONEM_BitAddressOffset 11
#define IMR_DONEM_RegisterSize	   1

/* Register IMR field RSVD_12_31_IMR /
/ Reserved bits - read as zero */
#define IMR_RSVD_12_31_IMR_BitAddressOffset 12
#define IMR_RSVD_12_31_IMR_RegisterSize		20

/* End of Register Definition for IMR */

/* Register ISR /
/ This register reports the status of the DWC_ssi interrupts after they have been masked. */
#define ISR					   (DWC_ssi_address_block_BaseAddress + 0x30)
#define ISR_RegisterSize	   32
#define ISR_RegisterResetValue 0x0
#define ISR_RegisterResetMask  0xffffffff

/* Register Field information for ISR */

/* Register ISR field TXEIS /
/ Transmit FIFO Empty Interrupt Status */
#define ISR_TXEIS_BitAddressOffset 0
#define ISR_TXEIS_RegisterSize	   1

/* Register ISR field TXOIS /
/ Transmit FIFO Overflow Interrupt Status */
#define ISR_TXOIS_BitAddressOffset 1
#define ISR_TXOIS_RegisterSize	   1

/* Register ISR field RXUIS /
/ Receive FIFO Underflow Interrupt Status */
#define ISR_RXUIS_BitAddressOffset 2
#define ISR_RXUIS_RegisterSize	   1

/* Register ISR field RXOIS /
/ Receive FIFO Overflow Interrupt Status */
#define ISR_RXOIS_BitAddressOffset 3
#define ISR_RXOIS_RegisterSize	   1

/* Register ISR field RXFIS /
/ Receive FIFO Full Interrupt Status */
#define ISR_RXFIS_BitAddressOffset 4
#define ISR_RXFIS_RegisterSize	   1

/* Register ISR field MSTIS /
/ Multi-Controller Contention Interrupt Status. This bit field is not present
if the DWC_ssi is configured as a serial-target device. */
#define ISR_MSTIS_BitAddressOffset 5
#define ISR_MSTIS_RegisterSize	   1

/* Register ISR field XRXOIS /
/ XIP Receive FIFO Overflow Interrupt Status */
#define ISR_XRXOIS_BitAddressOffset 6
#define ISR_XRXOIS_RegisterSize		1

/* Register ISR field TXUIS /
/ Transmit FIFO Underflow Interrupt Status */
#define ISR_TXUIS_BitAddressOffset 7
#define ISR_TXUIS_RegisterSize	   1

/* Register ISR field AXIES /
/ AXI Error Interrupt Status */
#define ISR_AXIES_BitAddressOffset 8
#define ISR_AXIES_RegisterSize	   1

/* Register ISR field RSVD_9_RISR /
/ Reserved bits - read as zero */
#define ISR_RSVD_9_RISR_BitAddressOffset 9
#define ISR_RSVD_9_RISR_RegisterSize	 1

/* Register ISR field SPITES /
/ SPI Transmit Error Interrupt */
#define ISR_SPITES_BitAddressOffset 10
#define ISR_SPITES_RegisterSize		1

/* Register ISR field DONES /
/ SSI Done Interrupt Status */
#define ISR_DONES_BitAddressOffset 11
#define ISR_DONES_RegisterSize	   1

/* Register ISR field RSVD_12_31_RISR /
/ Reserved bits - read as zero */
#define ISR_RSVD_12_31_RISR_BitAddressOffset 12
#define ISR_RSVD_12_31_RISR_RegisterSize	 20

/* End of Register Definition for ISR */

/* Register RISR /
/ Raw Interrupt Status Register */
#define RISR					(DWC_ssi_address_block_BaseAddress + 0x34)
#define RISR_RegisterSize		32
#define RISR_RegisterResetValue 0x0
#define RISR_RegisterResetMask	0xffffffff

/* Register Field information for RISR */

/* Register RISR field TXEIR /
/ Transmit FIFO Empty Raw Interrupt Status */
#define RISR_TXEIR_BitAddressOffset 0
#define RISR_TXEIR_RegisterSize		1

/* Register RISR field TXOIR /
/ Transmit FIFO Overflow Raw Interrupt Status */
#define RISR_TXOIR_BitAddressOffset 1
#define RISR_TXOIR_RegisterSize		1

/* Register RISR field RXUIR /
/ Receive FIFO Underflow Raw Interrupt Status */
#define RISR_RXUIR_BitAddressOffset 2
#define RISR_RXUIR_RegisterSize		1

/* Register RISR field RXOIR /
/ Receive FIFO Overflow Raw Interrupt Status */
#define RISR_RXOIR_BitAddressOffset 3
#define RISR_RXOIR_RegisterSize		1

/* Register RISR field RXFIR /
/ Receive FIFO Full Raw Interrupt Status */
#define RISR_RXFIR_BitAddressOffset 4
#define RISR_RXFIR_RegisterSize		1

/* Register RISR field MSTIR /
/ Multi-Controller Contention Raw Interrupt Status. This bit field is not present if the DWC_ssi is configured as a serial-target device. */
#define RISR_MSTIR_BitAddressOffset 5
#define RISR_MSTIR_RegisterSize		1

/* Register RISR field XRXOIR /
/ XIP Receive FIFO Overflow Raw Interrupt Status */
#define RISR_XRXOIR_BitAddressOffset 6
#define RISR_XRXOIR_RegisterSize	 1

/* Register RISR field TXUIR /
/ Transmit FIFO Underflow Interrupt Raw Status */
#define RISR_TXUIR_BitAddressOffset 7
#define RISR_TXUIR_RegisterSize		1

/* Register RISR field AXIER /
/ AXI Error Interrupt Raw Status */
#define RISR_AXIER_BitAddressOffset 8
#define RISR_AXIER_RegisterSize		1

/* Register RISR field RSVD_9_RISR /
/ Reserved bits - read as zero */
#define RISR_RSVD_9_RISR_BitAddressOffset 9
#define RISR_RSVD_9_RISR_RegisterSize	  1

/* Register RISR field SPITER /
/ SPI Transmit Error Interrupt status.
This bit gets set, If SPI Controller fails to get a READY status from the target until the amount of time defined in SPI_CTRLR1.MAX_WS field, then it will stop the SPI transfer and the FIFO is flushed (in case of write operation). */
#define RISR_SPITER_BitAddressOffset 10
#define RISR_SPITER_RegisterSize	 1

/* Register RISR field DONER /
/ SSI Done Interrupt Raw Status */
#define RISR_DONER_BitAddressOffset 11
#define RISR_DONER_RegisterSize		1

/* Register RISR field RSVD_12_31_RISR /
/ Reserved bits - read as zero */
#define RISR_RSVD_12_31_RISR_BitAddressOffset 12
#define RISR_RSVD_12_31_RISR_RegisterSize	  20

/* End of Register Definition for RISR */

/* Register TXEICR /
/ Transmit FIFO Error Interrupt Clear Register */
#define TXEICR					  (DWC_ssi_address_block_BaseAddress + 0x38)
#define TXEICR_RegisterSize		  32
#define TXEICR_RegisterResetValue 0x0
#define TXEICR_RegisterResetMask  0xffffffff

/* Register Field information for TXEICR */

/* Register TXEICR field TXEICR /
/ Clear Transmit FIFO Overflow/Underflow Interrupt.
This register reflects the status of the interrupt. A read from this register clears the ssi_txo_intr/ssi_txu_intr interrupt; writing has no effect. */
#define TXEICR_TXEICR_BitAddressOffset 0
#define TXEICR_TXEICR_RegisterSize	   1

/* Register TXEICR field RSVD_TXEICR /
/ Reserved bits - read as zero */
#define TXEICR_RSVD_TXEICR_BitAddressOffset 1
#define TXEICR_RSVD_TXEICR_RegisterSize		31

/* End of Register Definition for TXEICR */

/* Register RXOICR /
/ Receive FIFO Overflow Interrupt Clear Register */
#define RXOICR					  (DWC_ssi_address_block_BaseAddress + 0x3c)
#define RXOICR_RegisterSize		  32
#define RXOICR_RegisterResetValue 0x0
#define RXOICR_RegisterResetMask  0xffffffff

/* Register Field information for RXOICR */

/* Register RXOICR field RXOICR /
/ Clear Receive FIFO Overflow Interrupt.
This register reflects the status of the interrupt. A read from this register clears the ssi_rxo_intr interrupt; writing has no effect. */
#define RXOICR_RXOICR_BitAddressOffset 0
#define RXOICR_RXOICR_RegisterSize	   1

/* Register RXOICR field RSVD_RXOICR /
/ Reserved bits - read as zero */
#define RXOICR_RSVD_RXOICR_BitAddressOffset 1
#define RXOICR_RSVD_RXOICR_RegisterSize		31

/* End of Register Definition for RXOICR */

/* Register RXUICR /
/ Receive FIFO Underflow Interrupt Clear Register */
#define RXUICR					  (DWC_ssi_address_block_BaseAddress + 0x40)
#define RXUICR_RegisterSize		  32
#define RXUICR_RegisterResetValue 0x0
#define RXUICR_RegisterResetMask  0xffffffff

/* Register Field information for RXUICR */

/* Register RXUICR field RXUICR /
/ Clear Receive FIFO Underflow Interrupt.
This register reflects the status of the interrupt. A read from this register clears the ssi_rxu_intr interrupt; writing has no effect. */
#define RXUICR_RXUICR_BitAddressOffset 0
#define RXUICR_RXUICR_RegisterSize	   1

/* Register RXUICR field RSVD_RXUICR /
/ Reserved bits - read as zero */
#define RXUICR_RSVD_RXUICR_BitAddressOffset 1
#define RXUICR_RSVD_RXUICR_RegisterSize		31

/* End of Register Definition for RXUICR */

/* Register MSTICR /
/ Multi-Controller Interrupt Clear Register */
#define MSTICR					  (DWC_ssi_address_block_BaseAddress + 0x44)
#define MSTICR_RegisterSize		  32
#define MSTICR_RegisterResetValue 0x0
#define MSTICR_RegisterResetMask  0xffffffff

/* Register Field information for MSTICR */

/* Register MSTICR field MSTICR /
/ Clear Multi-Controller Contention Interrupt.
This register reflects the status of the interrupt. A read from this register clears the ssi_mst_intr interrupt; writing has no effect. */
#define MSTICR_MSTICR_BitAddressOffset 0
#define MSTICR_MSTICR_RegisterSize	   1

/* Register MSTICR field RSVD_MSTICR /
/ Reserved bits - read as zero */
#define MSTICR_RSVD_MSTICR_BitAddressOffset 1
#define MSTICR_RSVD_MSTICR_RegisterSize		31

/* End of Register Definition for MSTICR */

/* Register ICR /
/ Interrupt Clear Register */
#define ICR					   (DWC_ssi_address_block_BaseAddress + 0x48)
#define ICR_RegisterSize	   32
#define ICR_RegisterResetValue 0x0
#define ICR_RegisterResetMask  0xffffffff

/* Register Field information for ICR */

/* Register ICR field ICR /
/ Clear Interrupts.
This register is set if any of the interrupts below are active. A read clears the ssi_done_intr, ssi_axie_intr, ssi_spite_intr, ssi_txo_intr, ssi_txu_intr, ssi_rxu_intr, ssi_xrxo_intr, ssi_rxo_intr and the ssi_mst_intr interrupts. Writing to this register has no effect. */
#define ICR_ICR_BitAddressOffset 0
#define ICR_ICR_RegisterSize	 1

/* Register ICR field RSVD_ICR /
/ Reserved bits - read as zero */
#define ICR_RSVD_ICR_BitAddressOffset 1
#define ICR_RSVD_ICR_RegisterSize	  31

/* End of Register Definition for ICR */

/* Register DMACR /
/ DMA Control Register.
This register is only valid when DWC_ssi is configured with a set of DMA Controller interface signals (SSIC_HAS_DMA = 1) or Internal DMA operation (SSIC_HAS_DMA = 2). When DWC_ssi is not configured for DMA operation, this register will not exist and writing to the register's address will have no effect; reading from this register address will return zero. */
#define DMACR					 (DWC_ssi_address_block_BaseAddress + 0x4c)
#define DMACR_RegisterSize		 32
#define DMACR_RegisterResetValue 0x0
#define DMACR_RegisterResetMask	 0xffffffff

/* Register Field information for DMACR */

/* Register DMACR field RDMAE /
/ Receive DMA Enable. This bit enables/disables the receive FIFO DMA channel. */
#define DMACR_RDMAE_BitAddressOffset 0
#define DMACR_RDMAE_RegisterSize	 1

/* Register DMACR field TDMAE /
/ Transmit DMA Enable. This bit enables/disables the transmit FIFO DMA channel. */
#define DMACR_TDMAE_BitAddressOffset 1
#define DMACR_TDMAE_RegisterSize	 1

/* Register DMACR field IDMAE /
/ Internal DMA Enable. This bit should be enabled only when CTRLR0.FRF = 0 (Motorola SPI) and CTRLR0.SPI_FRF > 0. */
#define DMACR_IDMAE_BitAddressOffset 2
#define DMACR_IDMAE_RegisterSize	 1

/* Register DMACR field ATW /
/ AXI transfer width for DMA transfers mapped to arsize/awsize. This value must be less than or equal to SSIC_AXI_DW. Values:

   0x0: 1 byte

   0x1: 2 bytes

   0x2: 4 bytes

   0x3: 8 bytes

 Note: When SSIC_AXI_DW is set to 32 bits, if user programs this field to 0x8(3 bytes). DWC_ssi will use 4 bytes as transfer size for the AXI transfers. */
#define DMACR_ATW_BitAddressOffset 3
#define DMACR_ATW_RegisterSize	   2

/* Register DMACR field RSVD_DMACR5 /
/ Reserved bits - read as zero */
#define DMACR_RSVD_DMACR5_BitAddressOffset 5
#define DMACR_RSVD_DMACR5_RegisterSize	   1

/* Register DMACR field AINC /
/ Address Increment. Indicates whether to increment the AXI address on every transfer.
   1 = Increment
   0 = No Change
 Note: Increment aligns the address to the next DMACR.ATW boundary */
#define DMACR_AINC_BitAddressOffset 6
#define DMACR_AINC_RegisterSize		1

/* Register DMACR field RSVD_DMACR7 /
/ Reserved bits - read as zero */
#define DMACR_RSVD_DMACR7_BitAddressOffset 7
#define DMACR_RSVD_DMACR7_RegisterSize	   1

/* Register DMACR field ACACHE /
/ AXI arcache/awcache signal value. */
#define DMACR_ACACHE_BitAddressOffset 8
#define DMACR_ACACHE_RegisterSize	  4

/* Register DMACR field APROT /
/ AXI arprot/awprot signal value. */
#define DMACR_APROT_BitAddressOffset 12
#define DMACR_APROT_RegisterSize	 3

/* Register DMACR field AID /
/ AXI awid/arid signal value. */
#define DMACR_AID_BitAddressOffset 15
#define DMACR_AID_RegisterSize	   6

/* Register DMACR field RSVD_DMACR /
/ Reserved bits - read as zero */
#define DMACR_RSVD_DMACR_BitAddressOffset 21
#define DMACR_RSVD_DMACR_RegisterSize	  11

/* End of Register Definition for DMACR */

/* Register DMATDLR /
/ This register is only valid when the DWC_ssi is configured with a set of DMA interface signals (SSIC_HAS_DMA = 1). When DWC_ssi is not configured for DMA operation, this register will not exist and writing to its address will have no effect; reading from its address will return zero. */
#define DMATDLR					   (DWC_ssi_address_block_BaseAddress + 0x50)
#define DMATDLR_RegisterSize	   32
#define DMATDLR_RegisterResetValue 0x0
#define DMATDLR_RegisterResetMask  0xffffffff

/* Register Field information for DMATDLR */

/* Register DMATDLR field DMATDL /
/ Transmit Data Level. This bit field controls the level at which a DMA request is made by the transmit logic. It is equal to the watermark level; that is, the dma_tx_req signal is generated when the number of valid data entries in the transmit FIFO is equal to or below this field value, and TDMAE = 1. */
#define DMATDLR_DMATDL_BitAddressOffset 0
#define DMATDLR_DMATDL_RegisterSize		6

/* Register DMATDLR field RSVD_DMATDLR /
/ Reserved bits - read as zero */
#define DMATDLR_RSVD_DMATDLR_BitAddressOffset 6
#define DMATDLR_RSVD_DMATDLR_RegisterSize	  26

/* End of Register Definition for DMATDLR */

/* Register DMARDLR /
/ This register is only valid when DWC_ssi is configured with a set of DMA interface signals (SSIC_HAS_DMA = 1). When DWC_ssi is not configured for DMA operation, this register will not exist and writing to its address will have no effect; reading from its address will return zero. */
#define DMARDLR					   (DWC_ssi_address_block_BaseAddress + 0x54)
#define DMARDLR_RegisterSize	   32
#define DMARDLR_RegisterResetValue 0x0
#define DMARDLR_RegisterResetMask  0xffffffff

/* Register Field information for DMARDLR */

/* Register DMARDLR field DMARDL /
/ Receive Data Level. This bit field controls the level at which a DMA request is made by the receive logic. The watermark level = DMARDL+1; that is, dma_rx_req is generated when the number of valid data entries in the receive FIFO is equal to or above this field value + 1, and RDMAE=1. */
#define DMARDLR_DMARDL_BitAddressOffset 0
#define DMARDLR_DMARDL_RegisterSize		6

/* Register DMARDLR field RSVD_DMARDLR /
/ Reserved bits - read as zero */
#define DMARDLR_RSVD_DMARDLR_BitAddressOffset 6
#define DMARDLR_RSVD_DMARDLR_RegisterSize	  26

/* End of Register Definition for DMARDLR */

/* Register IDR /
/ This register contains the peripherals identification code, which is written into the register at configuration time using coreConsultant. */
#define IDR					   (DWC_ssi_address_block_BaseAddress + 0x58)
#define IDR_RegisterSize	   32
#define IDR_RegisterResetValue 0xffffefef
#define IDR_RegisterResetMask  0xffffffff

/* Register Field information for IDR */

/* Register IDR field IDCODE /
/ Identification code. The register contains the identification code of the peripheral, which is written into the register at configuration time using CoreConsultant. */
#define IDR_IDCODE_BitAddressOffset 0
#define IDR_IDCODE_RegisterSize		32

/* End of Register Definition for IDR */

/* Register SSIC_VERSION_ID /
/ This read-only register stores the specific DWC_ssi component version. */
#define SSIC_VERSION_ID					   (DWC_ssi_address_block_BaseAddress + 0x5c)
#define SSIC_VERSION_ID_RegisterSize	   32
#define SSIC_VERSION_ID_RegisterResetValue 0x3230302a
#define SSIC_VERSION_ID_RegisterResetMask  0xffffffff

/* Register Field information for SSIC_VERSION_ID */

/* Register SSIC_VERSION_ID field SSIC_COMP_VERSION /
/ Contains the hex representation of the Synopsys component version. Consists of ASCII value for each number in the version, followed by . For example 31_30_33_2A represents the version 1.03. */
#define SSIC_VERSION_ID_SSIC_COMP_VERSION_BitAddressOffset 0
#define SSIC_VERSION_ID_SSIC_COMP_VERSION_RegisterSize	   32

/* End of Register Definition for SSIC_VERSION_ID */

/* Register DR0 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR0					   (DWC_ssi_address_block_BaseAddress + 0x60)
#define DR0_RegisterSize	   32
#define DR0_RegisterResetValue 0x0
#define DR0_RegisterResetMask  0xffffffff

/* Register Field information for DR0 */

/* Register DR0 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR0_DR_BitAddressOffset 0
#define DR0_DR_RegisterSize		32

/* End of Register Definition for DR0 */

/* Register DR1 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR1					   (DWC_ssi_address_block_BaseAddress + 0x64)
#define DR1_RegisterSize	   32
#define DR1_RegisterResetValue 0x0
#define DR1_RegisterResetMask  0xffffffff

/* Register Field information for DR1 */

/* Register DR1 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR1_DR_BitAddressOffset 0
#define DR1_DR_RegisterSize		32

/* End of Register Definition for DR1 */

/* Register DR2 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR2					   (DWC_ssi_address_block_BaseAddress + 0x68)
#define DR2_RegisterSize	   32
#define DR2_RegisterResetValue 0x0
#define DR2_RegisterResetMask  0xffffffff

/* Register Field information for DR2 */

/* Register DR2 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR2_DR_BitAddressOffset 0
#define DR2_DR_RegisterSize		32

/* End of Register Definition for DR2 */

/* Register DR3 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR3					   (DWC_ssi_address_block_BaseAddress + 0x6c)
#define DR3_RegisterSize	   32
#define DR3_RegisterResetValue 0x0
#define DR3_RegisterResetMask  0xffffffff

/* Register Field information for DR3 */

/* Register DR3 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR3_DR_BitAddressOffset 0
#define DR3_DR_RegisterSize		32

/* End of Register Definition for DR3 */

/* Register DR4 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR4					   (DWC_ssi_address_block_BaseAddress + 0x70)
#define DR4_RegisterSize	   32
#define DR4_RegisterResetValue 0x0
#define DR4_RegisterResetMask  0xffffffff

/* Register Field information for DR4 */

/* Register DR4 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR4_DR_BitAddressOffset 0
#define DR4_DR_RegisterSize		32

/* End of Register Definition for DR4 */

/* Register DR5 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR5					   (DWC_ssi_address_block_BaseAddress + 0x74)
#define DR5_RegisterSize	   32
#define DR5_RegisterResetValue 0x0
#define DR5_RegisterResetMask  0xffffffff

/* Register Field information for DR5 */

/* Register DR5 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR5_DR_BitAddressOffset 0
#define DR5_DR_RegisterSize		32

/* End of Register Definition for DR5 */

/* Register DR6 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR6					   (DWC_ssi_address_block_BaseAddress + 0x78)
#define DR6_RegisterSize	   32
#define DR6_RegisterResetValue 0x0
#define DR6_RegisterResetMask  0xffffffff

/* Register Field information for DR6 */

/* Register DR6 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR6_DR_BitAddressOffset 0
#define DR6_DR_RegisterSize		32

/* End of Register Definition for DR6 */

/* Register DR7 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR7					   (DWC_ssi_address_block_BaseAddress + 0x7c)
#define DR7_RegisterSize	   32
#define DR7_RegisterResetValue 0x0
#define DR7_RegisterResetMask  0xffffffff

/* Register Field information for DR7 */

/* Register DR7 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR7_DR_BitAddressOffset 0
#define DR7_DR_RegisterSize		32

/* End of Register Definition for DR7 */

/* Register DR8 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR8					   (DWC_ssi_address_block_BaseAddress + 0x80)
#define DR8_RegisterSize	   32
#define DR8_RegisterResetValue 0x0
#define DR8_RegisterResetMask  0xffffffff

/* Register Field information for DR8 */

/* Register DR8 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR8_DR_BitAddressOffset 0
#define DR8_DR_RegisterSize		32

/* End of Register Definition for DR8 */

/* Register DR9 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR9					   (DWC_ssi_address_block_BaseAddress + 0x84)
#define DR9_RegisterSize	   32
#define DR9_RegisterResetValue 0x0
#define DR9_RegisterResetMask  0xffffffff

/* Register Field information for DR9 */

/* Register DR9 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR9_DR_BitAddressOffset 0
#define DR9_DR_RegisterSize		32

/* End of Register Definition for DR9 */

/* Register DR10 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR10					(DWC_ssi_address_block_BaseAddress + 0x88)
#define DR10_RegisterSize		32
#define DR10_RegisterResetValue 0x0
#define DR10_RegisterResetMask	0xffffffff

/* Register Field information for DR10 */

/* Register DR10 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR10_DR_BitAddressOffset 0
#define DR10_DR_RegisterSize	 32

/* End of Register Definition for DR10 */

/* Register DR11 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR11					(DWC_ssi_address_block_BaseAddress + 0x8c)
#define DR11_RegisterSize		32
#define DR11_RegisterResetValue 0x0
#define DR11_RegisterResetMask	0xffffffff

/* Register Field information for DR11 */

/* Register DR11 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR11_DR_BitAddressOffset 0
#define DR11_DR_RegisterSize	 32

/* End of Register Definition for DR11 */

/* Register DR12 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR12					(DWC_ssi_address_block_BaseAddress + 0x90)
#define DR12_RegisterSize		32
#define DR12_RegisterResetValue 0x0
#define DR12_RegisterResetMask	0xffffffff

/* Register Field information for DR12 */

/* Register DR12 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR12_DR_BitAddressOffset 0
#define DR12_DR_RegisterSize	 32

/* End of Register Definition for DR12 */

/* Register DR13 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR13					(DWC_ssi_address_block_BaseAddress + 0x94)
#define DR13_RegisterSize		32
#define DR13_RegisterResetValue 0x0
#define DR13_RegisterResetMask	0xffffffff

/* Register Field information for DR13 */

/* Register DR13 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR13_DR_BitAddressOffset 0
#define DR13_DR_RegisterSize	 32

/* End of Register Definition for DR13 */

/* Register DR14 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR14					(DWC_ssi_address_block_BaseAddress + 0x98)
#define DR14_RegisterSize		32
#define DR14_RegisterResetValue 0x0
#define DR14_RegisterResetMask	0xffffffff

/* Register Field information for DR14 */

/* Register DR14 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR14_DR_BitAddressOffset 0
#define DR14_DR_RegisterSize	 32

/* End of Register Definition for DR14 */

/* Register DR15 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR15					(DWC_ssi_address_block_BaseAddress + 0x9c)
#define DR15_RegisterSize		32
#define DR15_RegisterResetValue 0x0
#define DR15_RegisterResetMask	0xffffffff

/* Register Field information for DR15 */

/* Register DR15 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR15_DR_BitAddressOffset 0
#define DR15_DR_RegisterSize	 32

/* End of Register Definition for DR15 */

/* Register DR16 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR16					(DWC_ssi_address_block_BaseAddress + 0xa0)
#define DR16_RegisterSize		32
#define DR16_RegisterResetValue 0x0
#define DR16_RegisterResetMask	0xffffffff

/* Register Field information for DR16 */

/* Register DR16 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR16_DR_BitAddressOffset 0
#define DR16_DR_RegisterSize	 32

/* End of Register Definition for DR16 */

/* Register DR17 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR17					(DWC_ssi_address_block_BaseAddress + 0xa4)
#define DR17_RegisterSize		32
#define DR17_RegisterResetValue 0x0
#define DR17_RegisterResetMask	0xffffffff

/* Register Field information for DR17 */

/* Register DR17 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR17_DR_BitAddressOffset 0
#define DR17_DR_RegisterSize	 32

/* End of Register Definition for DR17 */

/* Register DR18 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR18					(DWC_ssi_address_block_BaseAddress + 0xa8)
#define DR18_RegisterSize		32
#define DR18_RegisterResetValue 0x0
#define DR18_RegisterResetMask	0xffffffff

/* Register Field information for DR18 */

/* Register DR18 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR18_DR_BitAddressOffset 0
#define DR18_DR_RegisterSize	 32

/* End of Register Definition for DR18 */

/* Register DR19 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR19					(DWC_ssi_address_block_BaseAddress + 0xac)
#define DR19_RegisterSize		32
#define DR19_RegisterResetValue 0x0
#define DR19_RegisterResetMask	0xffffffff

/* Register Field information for DR19 */

/* Register DR19 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR19_DR_BitAddressOffset 0
#define DR19_DR_RegisterSize	 32

/* End of Register Definition for DR19 */

/* Register DR20 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR20					(DWC_ssi_address_block_BaseAddress + 0xb0)
#define DR20_RegisterSize		32
#define DR20_RegisterResetValue 0x0
#define DR20_RegisterResetMask	0xffffffff

/* Register Field information for DR20 */

/* Register DR20 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR20_DR_BitAddressOffset 0
#define DR20_DR_RegisterSize	 32

/* End of Register Definition for DR20 */

/* Register DR21 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR21					(DWC_ssi_address_block_BaseAddress + 0xb4)
#define DR21_RegisterSize		32
#define DR21_RegisterResetValue 0x0
#define DR21_RegisterResetMask	0xffffffff

/* Register Field information for DR21 */

/* Register DR21 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR21_DR_BitAddressOffset 0
#define DR21_DR_RegisterSize	 32

/* End of Register Definition for DR21 */

/* Register DR22 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR22					(DWC_ssi_address_block_BaseAddress + 0xb8)
#define DR22_RegisterSize		32
#define DR22_RegisterResetValue 0x0
#define DR22_RegisterResetMask	0xffffffff

/* Register Field information for DR22 */

/* Register DR22 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR22_DR_BitAddressOffset 0
#define DR22_DR_RegisterSize	 32

/* End of Register Definition for DR22 */

/* Register DR23 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR23					(DWC_ssi_address_block_BaseAddress + 0xbc)
#define DR23_RegisterSize		32
#define DR23_RegisterResetValue 0x0
#define DR23_RegisterResetMask	0xffffffff

/* Register Field information for DR23 */

/* Register DR23 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR23_DR_BitAddressOffset 0
#define DR23_DR_RegisterSize	 32

/* End of Register Definition for DR23 */

/* Register DR24 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR24					(DWC_ssi_address_block_BaseAddress + 0xc0)
#define DR24_RegisterSize		32
#define DR24_RegisterResetValue 0x0
#define DR24_RegisterResetMask	0xffffffff

/* Register Field information for DR24 */

/* Register DR24 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR24_DR_BitAddressOffset 0
#define DR24_DR_RegisterSize	 32

/* End of Register Definition for DR24 */

/* Register DR25 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR25					(DWC_ssi_address_block_BaseAddress + 0xc4)
#define DR25_RegisterSize		32
#define DR25_RegisterResetValue 0x0
#define DR25_RegisterResetMask	0xffffffff

/* Register Field information for DR25 */

/* Register DR25 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR25_DR_BitAddressOffset 0
#define DR25_DR_RegisterSize	 32

/* End of Register Definition for DR25 */

/* Register DR26 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR26					(DWC_ssi_address_block_BaseAddress + 0xc8)
#define DR26_RegisterSize		32
#define DR26_RegisterResetValue 0x0
#define DR26_RegisterResetMask	0xffffffff

/* Register Field information for DR26 */

/* Register DR26 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR26_DR_BitAddressOffset 0
#define DR26_DR_RegisterSize	 32

/* End of Register Definition for DR26 */

/* Register DR27 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR27					(DWC_ssi_address_block_BaseAddress + 0xcc)
#define DR27_RegisterSize		32
#define DR27_RegisterResetValue 0x0
#define DR27_RegisterResetMask	0xffffffff

/* Register Field information for DR27 */

/* Register DR27 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR27_DR_BitAddressOffset 0
#define DR27_DR_RegisterSize	 32

/* End of Register Definition for DR27 */

/* Register DR28 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR28					(DWC_ssi_address_block_BaseAddress + 0xd0)
#define DR28_RegisterSize		32
#define DR28_RegisterResetValue 0x0
#define DR28_RegisterResetMask	0xffffffff

/* Register Field information for DR28 */

/* Register DR28 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR28_DR_BitAddressOffset 0
#define DR28_DR_RegisterSize	 32

/* End of Register Definition for DR28 */

/* Register DR29 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR29					(DWC_ssi_address_block_BaseAddress + 0xd4)
#define DR29_RegisterSize		32
#define DR29_RegisterResetValue 0x0
#define DR29_RegisterResetMask	0xffffffff

/* Register Field information for DR29 */

/* Register DR29 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR29_DR_BitAddressOffset 0
#define DR29_DR_RegisterSize	 32

/* End of Register Definition for DR29 */

/* Register DR30 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR30					(DWC_ssi_address_block_BaseAddress + 0xd8)
#define DR30_RegisterSize		32
#define DR30_RegisterResetValue 0x0
#define DR30_RegisterResetMask	0xffffffff

/* Register Field information for DR30 */

/* Register DR30 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR30_DR_BitAddressOffset 0
#define DR30_DR_RegisterSize	 32

/* End of Register Definition for DR30 */

/* Register DR31 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR31					(DWC_ssi_address_block_BaseAddress + 0xdc)
#define DR31_RegisterSize		32
#define DR31_RegisterResetValue 0x0
#define DR31_RegisterResetMask	0xffffffff

/* Register Field information for DR31 */

/* Register DR31 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR31_DR_BitAddressOffset 0
#define DR31_DR_RegisterSize	 32

/* End of Register Definition for DR31 */

/* Register DR32 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR32					(DWC_ssi_address_block_BaseAddress + 0xe0)
#define DR32_RegisterSize		32
#define DR32_RegisterResetValue 0x0
#define DR32_RegisterResetMask	0xffffffff

/* Register Field information for DR32 */

/* Register DR32 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR32_DR_BitAddressOffset 0
#define DR32_DR_RegisterSize	 32

/* End of Register Definition for DR32 */

/* Register DR33 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR33					(DWC_ssi_address_block_BaseAddress + 0xe4)
#define DR33_RegisterSize		32
#define DR33_RegisterResetValue 0x0
#define DR33_RegisterResetMask	0xffffffff

/* Register Field information for DR33 */

/* Register DR33 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR33_DR_BitAddressOffset 0
#define DR33_DR_RegisterSize	 32

/* End of Register Definition for DR33 */

/* Register DR34 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR34					(DWC_ssi_address_block_BaseAddress + 0xe8)
#define DR34_RegisterSize		32
#define DR34_RegisterResetValue 0x0
#define DR34_RegisterResetMask	0xffffffff

/* Register Field information for DR34 */

/* Register DR34 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR34_DR_BitAddressOffset 0
#define DR34_DR_RegisterSize	 32

/* End of Register Definition for DR34 */

/* Register DR35 /
/ The DWC_ssi data register is a 32-bit read/write buffer for the transmit/receive FIFOs. When the register is read, data in the receive FIFO buffer is accessed. When it is written to, data are moved into the transmit FIFO buffer; a write can occur only when SSIC_EN = 1. FIFOs are reset when SSIC_EN = 0.
Note The DR register in the DWC_ssi occupies thirty-six 32-bit address locations of the memory map to facilitate AHB burst transfers. Writing to any of these address locations has the same effect as pushing the data from the pwdata bus into the transmit FIFO. Reading from any of these locations has the same effect as popping data from the receive FIFO onto the hrdata bus. The FIFO buffers on the DWC_ssi are not addressable. */
#define DR35					(DWC_ssi_address_block_BaseAddress + 0xec)
#define DR35_RegisterSize		32
#define DR35_RegisterResetValue 0x0
#define DR35_RegisterResetMask	0xffffffff

/* Register Field information for DR35 */

/* Register DR35 field DR /
/ Data Register. When writing to this register, you must right-justify the data. Read data are automatically right-justified.
Read = Receive FIFO buffer
Write = Transmit FIFO buffer. */
#define DR35_DR_BitAddressOffset 0
#define DR35_DR_RegisterSize	 32

/* End of Register Definition for DR35 */

/* Register RX_SAMPLE_DELAY /
/ This register is only valid when the DWC_ssi is configured with rxd sample delay logic (SSIC_HAS_RX_SAMPLE_DELAY!=0). When the DWC_ssi is not configured with rxd sample delay logic, this register will not exist and writing to its address location will have no effect; reading from its address will return zero.
This register control the number of ssi_clk cycles that are delayed (from the default sample time) before the actual sample of the rxd input occurs. It is impossible to write to this register when the DWC_ssi is enabled. */
#define RX_SAMPLE_DELAY					   (DWC_ssi_address_block_BaseAddress + 0xf0)
#define RX_SAMPLE_DELAY_RegisterSize	   32
#define RX_SAMPLE_DELAY_RegisterResetValue 0x0
#define RX_SAMPLE_DELAY_RegisterResetMask  0xffffffff

/* Register Field information for RX_SAMPLE_DELAY */

/* Register RX_SAMPLE_DELAY field RSD /
/ Receive Data (rxd) Sample Delay. This register is used to delay the sample of the rxd input port. Each value
represents a single ssi_clk delay on the sample of rxd.
Note; If this register is programmed with a value that exceeds the depth of the internal shift registers (SSIC_RX_DLY_SR_DEPTH) zero delay will be applied to the rxd sample. */
#define RX_SAMPLE_DELAY_RSD_BitAddressOffset 0
#define RX_SAMPLE_DELAY_RSD_RegisterSize	 8

/* Register RX_SAMPLE_DELAY field RSVD0_RX_SAMPLE_DELAY /
/ Reserved bits - read as zero */
#define RX_SAMPLE_DELAY_RSVD0_RX_SAMPLE_DELAY_BitAddressOffset 8
#define RX_SAMPLE_DELAY_RSVD0_RX_SAMPLE_DELAY_RegisterSize	   8

/* Register RX_SAMPLE_DELAY field SE /
/ Receive Data (rxd) Sampling Edge. This register is used to decide the sampling edge for RXD signal with ssi_clk. Then this bit is set to 1 then negative edge of ssi_clk will be used to sample the incoming data, otherwise positive edge will be used for sampling. */
#define RX_SAMPLE_DELAY_SE_BitAddressOffset 16
#define RX_SAMPLE_DELAY_SE_RegisterSize		1

/* Register RX_SAMPLE_DELAY field RSVD1_RX_SAMPLE_DELAY /
/ Reserved bits - read as zero */
#define RX_SAMPLE_DELAY_RSVD1_RX_SAMPLE_DELAY_BitAddressOffset 17
#define RX_SAMPLE_DELAY_RSVD1_RX_SAMPLE_DELAY_RegisterSize	   15

/* End of Register Definition for RX_SAMPLE_DELAY */

/* Register SPI_CTRLR0 /
/ This register is used to control the serial data transfer in enhanced SPI mode of operation. It is not possible to write to this register when the DWC_ssi is enabled (SSIC_EN=1). */
#define SPI_CTRLR0					  (DWC_ssi_address_block_BaseAddress + 0xf4)
#define SPI_CTRLR0_RegisterSize		  32
#define SPI_CTRLR0_RegisterResetValue 0x200
#define SPI_CTRLR0_RegisterResetMask  0xffffffff

/* Register Field information for SPI_CTRLR0 */

/* Register SPI_CTRLR0 field TRANS_TYPE /
/ Address and instruction transfer format.
Selects whether DWC_ssi will transmit instruction/address either in Standard SPI mode or the SPI mode selected in CTRLR0.SPI_FRF field. */
#define SPI_CTRLR0_TRANS_TYPE_BitAddressOffset 0
#define SPI_CTRLR0_TRANS_TYPE_RegisterSize	   2

/* Register SPI_CTRLR0 field ADDR_L /
/ This bit defines Length of Address to be transmitted. Only after this much bits are programmed in to the FIFO the transfer can begin. */
#define SPI_CTRLR0_ADDR_L_BitAddressOffset 2
#define SPI_CTRLR0_ADDR_L_RegisterSize	   4

/* Register SPI_CTRLR0 field RSVD_SPI_CTRLR0_6 /
/ Reserved bits - read as zero */
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_6_BitAddressOffset 6
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_6_RegisterSize	  1

/* Register SPI_CTRLR0 field XIP_MD_BIT_EN /
/ Mode bits enable in XIP mode. If this bit is set to 1, then in XIP mode of operation DWC_ssi will insert mode bits after the address phase. These bits are set in register XIP_MODE_BITS register. */
#define SPI_CTRLR0_XIP_MD_BIT_EN_BitAddressOffset 7
#define SPI_CTRLR0_XIP_MD_BIT_EN_RegisterSize	  1

/* Register SPI_CTRLR0 field INST_L /
/ Enhanced SPI mode instruction length in bits.
 */
#define SPI_CTRLR0_INST_L_BitAddressOffset 8
#define SPI_CTRLR0_INST_L_RegisterSize	   2

/* Register SPI_CTRLR0 field RSVD_SPI_CTRLR0_10 /
/ Reserved bits - read as zero */
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_10_BitAddressOffset 10
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_10_RegisterSize	   1

/* Register SPI_CTRLR0 field WAIT_CYCLES /
/ Wait cycles in Enhanced SPI mode between control frames transmit and data reception. Specified as number of SPI clock cycles. */
#define SPI_CTRLR0_WAIT_CYCLES_BitAddressOffset 11
#define SPI_CTRLR0_WAIT_CYCLES_RegisterSize		5

/* Register SPI_CTRLR0 field SPI_DDR_EN /
/ SPI DDR Enable bit. This will enable Dual-data rate transfers in Enhanced SPI frame formats of SPI. */
#define SPI_CTRLR0_SPI_DDR_EN_BitAddressOffset 16
#define SPI_CTRLR0_SPI_DDR_EN_RegisterSize	   1

/* Register SPI_CTRLR0 field INST_DDR_EN /
/ Instruction DDR Enable bit. This will enable Dual-data rate transfer for Instruction phase. */
#define SPI_CTRLR0_INST_DDR_EN_BitAddressOffset 17
#define SPI_CTRLR0_INST_DDR_EN_RegisterSize		1

/* Register SPI_CTRLR0 field SPI_RXDS_EN /
/ Read data strobe enable bit. Once this bit is set to 1 DWC_ssi will use Read data strobe (rxds) to capture read data. */
#define SPI_CTRLR0_SPI_RXDS_EN_BitAddressOffset 18
#define SPI_CTRLR0_SPI_RXDS_EN_RegisterSize		1

/* Register SPI_CTRLR0 field XIP_DFS_HC /
/ Fix DFS for XIP transfers. If this bit is set to 1 then data frame size for XIP transfers will be fixed to the programmed value in CTRLR0.DFS. The number of data frames to fetch will be determined by HSIZE and HBURST signals. If this bit is set to 0 then data frame size and number of data frames to fetch will be determined by HSIZE and HBURST signals */
#define SPI_CTRLR0_XIP_DFS_HC_BitAddressOffset 19
#define SPI_CTRLR0_XIP_DFS_HC_RegisterSize	   1

/* Register SPI_CTRLR0 field XIP_INST_EN /
/ XIP instruction enable bit. If this bit is set to 1 then XIP transfers will also have instruction phase. The instruction op-codes will be chosen from XIP_INCR_INST or XIP_WRAP_INST registers bases on AHB transfer type. */
#define SPI_CTRLR0_XIP_INST_EN_BitAddressOffset 20
#define SPI_CTRLR0_XIP_INST_EN_RegisterSize		1

/* Register SPI_CTRLR0 field SSIC_XIP_CONT_XFER_EN /
/ Enable continuous transfer in XIP mode. If this bit is set to 1 then continuous transfer mode in XIP will be enabled, in this mode DWC_ssi will keep target selected until a non-XIP transfer is detected on the AHB interface. */
#define SPI_CTRLR0_SSIC_XIP_CONT_XFER_EN_BitAddressOffset 21
#define SPI_CTRLR0_SSIC_XIP_CONT_XFER_EN_RegisterSize	  1

/* Register SPI_CTRLR0 field RSVD_SPI_CTRLR0_22 /
/ Reserved bits - read as zero */
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_22_BitAddressOffset 22
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_22_RegisterSize	   1

/* Register SPI_CTRLR0 field RXDS_VL_EN /
/ RXDS variable latency enable bit.
When this bit is set DWC_ssi waits for all the data to be sampled before stopping the SCLK_OUT clock. This enables the support for the devices which support variable latencies within the transfers for RXDS transfers. */
#define SPI_CTRLR0_RXDS_VL_EN_BitAddressOffset 23
#define SPI_CTRLR0_RXDS_VL_EN_RegisterSize	   1

/* Register SPI_CTRLR0 field SPI_DM_EN /
/ SPI data mask enable bit.
When this bit is enabled, the txd_dm signal is used to mask the data on the txd data line. This bit is enabled only when the SSIC_DM_EN parameter is set to 1. */
#define SPI_CTRLR0_SPI_DM_EN_BitAddressOffset 24
#define SPI_CTRLR0_SPI_DM_EN_RegisterSize	  1

/* Register SPI_CTRLR0 field SPI_RXDS_SIG_EN /
/ Enable rxds signaling during address and command phase of Hyperbus transfer.
This bit enables rxds signaling by Hyperbus target devices during Command-Address (CA) phase. If the rxds signal is set to 1 during the CA phase of transfer, DWC_ssi transmits (2*SPI_CTRLR0.WAIT_CYCLES-1) wait cycles after the address phase is complete. */
#define SPI_CTRLR0_SPI_RXDS_SIG_EN_BitAddressOffset 25
#define SPI_CTRLR0_SPI_RXDS_SIG_EN_RegisterSize		1

/* Register SPI_CTRLR0 field XIP_MBL /
/ XIP Mode bits length. Sets the length of mode bits in XIP mode of operation. These bits are valid only when SPI_CTRLR0.XIP_MD_BIT_EN is set to 1. */
#define SPI_CTRLR0_XIP_MBL_BitAddressOffset 26
#define SPI_CTRLR0_XIP_MBL_RegisterSize		2

/* Register SPI_CTRLR0 field RSVD_SPI_CTRLR0_28 /
/ Reserved bits - read as zero */
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_28_BitAddressOffset 28
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_28_RegisterSize	   1

/* Register SPI_CTRLR0 field XIP_PREFETCH_EN /
/ Enables XIP pre-fetch functionality in DWC_ssi.
Once enabled DWC_ssi will pre-fetch data frames from next contiguous location, to reduce the latency for the upcoming contiguous transfer. If the next XIP request is not contiguous then pre-fetched bits will be discarded. */
#define SPI_CTRLR0_XIP_PREFETCH_EN_BitAddressOffset 29
#define SPI_CTRLR0_XIP_PREFETCH_EN_RegisterSize		1

/* Register SPI_CTRLR0 field CLK_STRETCH_EN /
/ Enables clock stretching capability in SPI transfers.

In case of write, if the FIFO becomes empty DWC_ssi will stretch the clock until FIFO has enough data to continue the transfer.

In case of read, if the receive FIFO becomes full DWC_ssi will stop the clock until data has been read from the FIFO. */
#define SPI_CTRLR0_CLK_STRETCH_EN_BitAddressOffset 30
#define SPI_CTRLR0_CLK_STRETCH_EN_RegisterSize	   1

/* Register SPI_CTRLR0 field RSVD_SPI_CTRLR0 /
/ Reserved bits - read as zero */
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_BitAddressOffset 31
#define SPI_CTRLR0_RSVD_SPI_CTRLR0_RegisterSize		1

/* End of Register Definition for SPI_CTRLR0 */

/* Register DDR_DRIVE_EDGE /
/ This Register is valid only when SSIC_HAS_DDR is equal to 1. This register is used to control the driving edge of TXD register in DDR mode.
 It is not possible to write to this register when the DWC_ssi is enabled (SSIC_EN=1). */
#define DDR_DRIVE_EDGE					  (DWC_ssi_address_block_BaseAddress + 0xf8)
#define DDR_DRIVE_EDGE_RegisterSize		  32
#define DDR_DRIVE_EDGE_RegisterResetValue 0x0
#define DDR_DRIVE_EDGE_RegisterResetMask  0xffffffff

/* Register Field information for DDR_DRIVE_EDGE */

/* Register DDR_DRIVE_EDGE field TDE /
/ TXD Drive edge register which decided the driving edge of transmit data.
The maximum value of this register is = (BAUDR/2) -1. */
#define DDR_DRIVE_EDGE_TDE_BitAddressOffset 0
#define DDR_DRIVE_EDGE_TDE_RegisterSize		8

/* Register DDR_DRIVE_EDGE field RSVD_DDR_DRIVE_EDGE /
/ Reserved bits - read as zero */
#define DDR_DRIVE_EDGE_RSVD_DDR_DRIVE_EDGE_BitAddressOffset 8
#define DDR_DRIVE_EDGE_RSVD_DDR_DRIVE_EDGE_RegisterSize		24

/* End of Register Definition for DDR_DRIVE_EDGE */

#define DWC_ssi_address_block2_BaseAddress 0x100

/* Register SPI_CTRLR1 /
/ SPI Control register 1 is used to control serial transfers in SPI mode of operation. It is not possible to write to this register when the DWC_ssi is enabled (SSIC_EN=1). */
#define SPI_CTRLR1					  (DWC_ssi_address_block2_BaseAddress + 0x18)
#define SPI_CTRLR1_RegisterSize		  32
#define SPI_CTRLR1_RegisterResetValue 0x0
#define SPI_CTRLR1_RegisterResetMask  0xffffffff

/* Register Field information for SPI_CTRLR1 */

/* Register SPI_CTRLR1 field DYN_WS /
/ SPI Dynamic Wait states field.
This field is used to set the value for wait states which will be introduced when SPI target sends BUSY status to the Controller. The programmed value of wait States will be introduced before checking status again.
Number of wait states = DYN_WS+1 */
#define SPI_CTRLR1_DYN_WS_BitAddressOffset 0
#define SPI_CTRLR1_DYN_WS_RegisterSize	   3

/* Register SPI_CTRLR1 field RSVD_SPI_CTRLR1_3_7 /
/ Reserved bits - Read Only */
#define SPI_CTRLR1_RSVD_SPI_CTRLR1_3_7_BitAddressOffset 3
#define SPI_CTRLR1_RSVD_SPI_CTRLR1_3_7_RegisterSize		5

/* Register SPI_CTRLR1 field MAX_WS /
/ Maximum wait cycles allowed per transaction.
This field indicate, up to how many times SPI target could insert the wait states. The internal counter is incremented every time when DWC_ssi controller checks for the status from the target and receives wait response. */
#define SPI_CTRLR1_MAX_WS_BitAddressOffset 8
#define SPI_CTRLR1_MAX_WS_RegisterSize	   4

/* Register SPI_CTRLR1 field RSVD_SPI_CTRLR1_12_15 /
/ Reserved bits - Read Only */
#define SPI_CTRLR1_RSVD_SPI_CTRLR1_12_15_BitAddressOffset 12
#define SPI_CTRLR1_RSVD_SPI_CTRLR1_12_15_RegisterSize	  4

/* Register SPI_CTRLR1 field CS_MIN_HIGH /
/ Chip-Select Minimum HIGH period.
This field is used to set the minimum time period (in terms of ssi_clk) between two back to back SPI operations. */
#define SPI_CTRLR1_CS_MIN_HIGH_BitAddressOffset 16
#define SPI_CTRLR1_CS_MIN_HIGH_RegisterSize		4

/* Register SPI_CTRLR1 field RSVD_SPI_CTRLR1_20_31 /
/ Reserved bits - Read Only */
#define SPI_CTRLR1_RSVD_SPI_CTRLR1_20_31_BitAddressOffset 20
#define SPI_CTRLR1_RSVD_SPI_CTRLR1_20_31_RegisterSize	  12

/* End of Register Definition for SPI_CTRLR1 */

/* Register SPITECR /
/ SPI Transmit Error Interrupt Clear Register */
#define SPITECR					   (DWC_ssi_address_block2_BaseAddress + 0x1c)
#define SPITECR_RegisterSize	   32
#define SPITECR_RegisterResetValue 0x0
#define SPITECR_RegisterResetMask  0xffffffff

/* Register Field information for SPITECR */

/* Register SPITECR field SPITECR /
/ Clear SPI Transmit Error interrupt.
This register will reflect the status of the interrupt.
A read from this register clears the ssi_spite_intr interrupt.  Writing to this register has no effect. */
#define SPITECR_SPITECR_BitAddressOffset 0
#define SPITECR_SPITECR_RegisterSize	 1

/* Register SPITECR field RSVD_SPITECR /
/ Reserved bits - Read Only */
#define SPITECR_RSVD_SPITECR_BitAddressOffset 1
#define SPITECR_RSVD_SPITECR_RegisterSize	  31

/* End of Register Definition for SPITECR */

#endif /* SPI_DW_REGS_H */
