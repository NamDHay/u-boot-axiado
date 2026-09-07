// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef DWC_SPI_H
#define DWC_SPI_H

#define SPI_MIN_TXFTLR 0x1 /* Minimum Fifo count to start transfer */
#define SPI_MIN_RXFTLR 0x1 /* Minimum Fifo receive count */
#define SPI_TX_BLOCK	  0x0100 /** < max bytes to send at once is 256 bytes */
#define SPI_RX_BLOCK	  0x0100 /** < max bytes to receive at once is 256 bytes */

/* Control Register 0 BIT definitions */
#define SPI_CTRL0_DFS_0	 (CTRLR0_DFS_BitAddressOffset) /**< DFS selection bit 0 */
#define SPI_CTRL0_DFS_1	 (CTRLR0_DFS_BitAddressOffset + 1) /**< DFS selection bit 1 */
#define SPI_CTRL0_DFS_2	 (CTRLR0_DFS_BitAddressOffset + 2) /**< DFS selection bit 2 */
#define SPI_CTRL0_DFS_3	 (CTRLR0_DFS_BitAddressOffset + 3) /**< DFS selection bit 3 */
#define SPI_CTRL0_DFS_4	 (CTRLR0_DFS_BitAddressOffset + 4) /**< DFS selection bit 4 */
#define SPI_CTRL0_SCPH	 (CTRLR0_SCPH_BitAddressOffset) /**< SPI Serial Clock Phase selection bit */
#define SPI_CTRL0_SPOL	 (CTRLR0_SCPOL_BitAddressOffset) /**< SPI Serial Clock Polarity selection bit */
#define SPI_CTRL0_TMOD_0 (CTRLR0_TMOD_BitAddressOffset) /* SPI serial transmit mode bit 0 */
#define SPI_CTRL0_TMOD_1 (CTRLR0_TMOD_BitAddressOffset + 1) /* SPI serial transmit mode bit 1 */
#define SPI_CTRL0_DWS	 (CTRLR0_SPI_DWS_EN_BitAddressOffset) /**< Enable Dynamic Wait State bit */
#define SPI_CTRL0_IS_MST                                                                                     \
	(CTRLR0_SSI_IS_MST_BitAddressOffset) /**< SPI in Controller/Target Mode selection bit */

/** Data Frame size 4 bit */
#define DFS_4_BIT (BIT(SPI_CTRL0_DFS_1) | BIT(SPI_CTRL0_DFS_0))
/** Data Frame size 8 bit */
#define DFS_8_BIT (BIT(SPI_CTRL0_DFS_2) | BIT(SPI_CTRL0_DFS_1) | BIT(SPI_CTRL0_DFS_0))
/** Data Frame size 16 bit */
#define DFS_16_BIT (BIT(SPI_CTRL0_DFS_3) | BIT(SPI_CTRL0_DFS_2) | BIT(SPI_CTRL0_DFS_1) | BIT(SPI_CTRL0_DFS_0))
/** Data Frame size 32 bit */
#define DFS_24_BIT (BIT(SPI_CTRL0_DFS_4) | BIT(SPI_CTRL0_DFS_2) | BIT(SPI_CTRL0_DFS_1) | BIT(SPI_CTRL0_DFS_0))
/**< Data Frame size 64 bit */
#define DFS_32_BIT                                                                                           \
	(BIT(SPI_CTRL0_DFS_4) | BIT(SPI_CTRL0_DFS_3) | BIT(SPI_CTRL0_DFS_2) | BIT(SPI_CTRL0_DFS_1) |             \
	 BIT(SPI_CTRL0_DFS_0))

/* SPI transmit mode selection */
#define TMOD_TXRX	0x0 /**< Transmit and Receive mode */
#define TMOD_TX		BIT(SPI_CTRL0_TMOD_0) /**< Transmit only */
#define TMOD_RX		BIT(SPI_CTRL0_TMOD_1) /**< Receive Only */
#define TMOD_EEPROM (BIT(SPI_CTRL0_TMOD_1) | BIT(SPI_CTRL0_TMOD_0)) /**< EEPROM Mode */

/** SPI Mode selection */
#define SPI_SPOL_SPHA_MODE0 0x0 /* SPI Mode 0 (CPOL = 0, CPHA = 0) */
#define SPI_SPOL_SPHA_MODE1 BIT(SPI_CTRL0_SCPH) /**<  SPI Mode 1 (CPOL = 0, CPHA = 1) */
#define SPI_SPOL_SPHA_MODE2 BIT(SPI_CTRL0_SPOL) /**<  SPI Mode 2 (CPOL = 1, CPHA = 0) */
#define SPI_SPOL_SPHA_MODE3                                                                                  \
	(BIT(SPI_CTRL0_SPOL) | BIT(SPI_CTRL0_SCPH)) /**<  SPI Mode 3 (CPOL = 1, CPHA = 1) */

#define SPI_CTRL0_DWS_EN			BIT(SPI_CTRL0_DWS) /**< Enable Dynamic Wait State */
#define SPI_CTRL0_IS_MST_CONTROLLER BIT(SPI_CTRL0_IS_MST) /**< Enable Controller Mode */
#define SPI_CTRL0_IS_MST_TARGET		0x0 /**< Enable Target Mode */

/** SPI Frame Format */
#define	SPI_STANDARD 	0x0
#define SPI_DUAL		0x1
#define SPI_QUAD		0x2
#define SPI_OCTAL		0x3

#define SPI_CTRL0_IS_STANDARD	(SPI_STANDARD 	<< CTRLR0_SPI_FRF_BitAddressOffset)
#define SPI_CTRL0_IS_DUAL		(SPI_DUAL 		<< CTRLR0_SPI_FRF_BitAddressOffset)	
#define SPI_CTRL0_IS_QUAD		(SPI_QUAD 		<< CTRLR0_SPI_FRF_BitAddressOffset)
#define SPI_CTRL0_IS_OCTAL		(SPI_OCTAL		<< CTRLR0_SPI_FRF_BitAddressOffset)

/* Status Register */
#define SPI_SR_BUSY BIT(SR_BUSY_BitAddressOffset) /**< SSI Busy Flag */
#define SPI_SR_TFE	BIT(SR_TFE_BitAddressOffset) /**< Transmit FIFO Empty */

/* Interrupt Mask Register */
#define SPI_IMR_DONEM BIT(IMR_DONEM_BitAddressOffset) /**< SSI Done Interrupt Mask */
#define SPI_IMR_ALL	  0xDFF /**< Enable All interrupts */

/* Interrupt Status Register */
#define SPI_ISR_TXEIS BIT(ISR_TXEIS_BitAddressOffset) /**< Transmit FIFO Empty Interrupt Status */

/* SPI_CTRLR0 - SPI Control Register */
#define SPI_CTRLR0_ADDR_L_0		 (SPI_CTRLR0_ADDR_L_BitAddressOffset)
#define SPI_CTRLR0_ADDR_L_1		 (SPI_CTRLR0_ADDR_L_BitAddressOffset + 1)
#define SPI_CTRLR0_ADDR_L_2		 (SPI_CTRLR0_ADDR_L_BitAddressOffset + 2)
#define SPI_CTRLR0_ADDR_L_3		 (SPI_CTRLR0_ADDR_L_BitAddressOffset + 3)
#define SPI_CTRLR0_INST_L_0		 (SPI_CTRLR0_INST_L_BitAddressOffset)
#define SPI_CTRLR0_INST_L_1		 (SPI_CTRLR0_INST_L_BitAddressOffset + 1)
#define SPI_CTRLR0_WAIT_CYCLES_0 (SPI_CTRLR0_WAIT_CYCLES_BitAddressOffset)
#define SPI_CTRLR0_WAIT_CYCLES_1 (SPI_CTRLR0_WAIT_CYCLES_BitAddressOffset + 1)
#define SPI_CTRLR0_WAIT_CYCLES_2 (SPI_CTRLR0_WAIT_CYCLES_BitAddressOffset + 2)
#define SPI_CTRLR0_WAIT_CYCLES_3 (SPI_CTRLR0_WAIT_CYCLES_BitAddressOffset + 3)

#define SPI_CTRLR0_ADDR_L_24	 (BIT(SPI_CTRLR0_ADDR_L_2) | BIT(SPI_CTRLR0_ADDR_L_1))
#define SPI_CTRLR0_ADDR_L_32	 (BIT(SPI_CTRLR0_ADDR_L_3))
#define SPI_CTRLR0_ADDR_L_40	 (BIT(SPI_CTRLR0_ADDR_L_3) | BIT(SPI_CTRLR0_ADDR_L_1))
#define SPI_CTRLR0_INST_L_8		 (BIT(SPI_CTRLR0_INST_L_1))
#define SPI_CTRLR0_INST_L_16	 (BIT(SPI_CTRLR0_INST_L_1) | BIT(SPI_CTRLR0_INST_L_0))
#define SPI_CTRLR0_WAIT_CYCLES_8 (BIT(SPI_CTRLR0_WAIT_CYCLES_3))

/**
 * @brief Macro to calculate number of data frame to receive (TMOD = RX only)
 * 
 * @param len size of data to be read/transmit including dummy bytes
 */
#define SPI_NDF_SIZE(len, dfs) ((len) ? (((len * 8) / (dfs + 1)) - 1) : 0)

/**
 * @brief Macro to calculate Data frame size
 * 
 */
#define SPI_DFS_SIZE(len)                                                                                    \
	((len >= 4) ? DFS_32_BIT : ((len == 3) ? DFS_24_BIT : ((len == 2) ? DFS_16_BIT : DFS_8_BIT)))

/**
 * @brief Macro's to select the mode of transfer for serial communication
 * 
 */
typedef enum spi_tmod {
	SPI_TMOD_TXRX = 0x0, /**< Transmit and Receive Mode */
	SPI_TMOD_TX, /**< Transmit only Mode */
	SPI_TMOD_RX, /**< Received only Mode */
	SPI_TMOD_EEPROM_READ, /**< EEPROM mode */
	SPI_INVALID_TMOD /**< Invalid Transmit Mode */
} SPI_TMOD;

/**
 * @brief SPI mode numbers (combinations of polarity and phases)
 */
typedef enum spi_mode {
	SPI_MODE_0 =
		0x0, /**< CPOL = 0, CPHA = 0, SCK held low in the inactive state and Sampling of data occurs at odd edges. */
	SPI_MODE_1, /**< CPOL = 0, CPHA = 1, SCK held low in the inactive state and Sampling of data occurs at even edges. */
	SPI_MODE_2, /**< CPOL = 1, CPHA = 0, SCK held high in the in active and Sampling of data occurs at odd edges. */
	SPI_MODE_3, /**< CPOL = 1, CPHA = 1, SCK held high in the in active Sampling of data occurs at even edges. */
	SPI_INVALID_MODE
} SPI_MODE;

/**
 * @brief Defines the data that are required for reconfiguration of SPI
 */
struct spi_dw_config_t {
	SPI_TMOD tmod; /**< SPI Transfer Mode */
	SPI_MODE mode; /**< SPI Clock Phase and Polarity */
	uint32_t tx_len; /**< Total data transmit length*/
	uint32_t rx_len; /**< Total data receive length */

	/* Following are valid for rx only mode */
	uint32_t adrs_len; /**< Length of Address to be transmitted */
	uint32_t inst_len; /**< Enhanced SPI mode instruction length in bits. */
	uint32_t wait_cycle; /**< Wait cycles in Enhanced SPI mode between control frames
transmit and data reception. */
};
#endif /*  DWC_SPI_H */
