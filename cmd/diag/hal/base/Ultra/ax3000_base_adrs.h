/*
 * Copyright (c) 2021-24 Axiado Corporation (or its affiliates). All rights reserved.
 *
 * Use, modification and redistribution of this file is subject to your possession
 * of a valid End User License Agreement (EULA) for the Axiado Product of which
 * these sources are part of and your compliance with all applicable terms and
 * conditions of such licence agreement.
 */
#ifndef _AX_HAL_AX3000_BASE_ADRS_H
#define _AX_HAL_AX3000_BASE_ADRS_H

/**
 * @file ax3000_base_adrs.h
 * @brief This file contains the Axiado Base Address registers for Rev B0
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AX3000_CSR_BASE_ADRS_SVC		0x30000000
#define AX3000_MEM_BASE_ADRS_R52_SRAM	0x30100000
#define AX3000_CSR_BASE_ADRS_R52_PERIPH 0x30160000
#define AX3000_CSR_BASE_ADRS_ALGO		0x30200000
#define AX3000_MEM_BASE_ADRS_VLTEXT		0x30280000
#define AX3000_MEM_BASE_ADRS_R52_TCM	0x31000000

/** <Start and End Addresses for Slow peripheral 0 */
#define AX3000_CSR_BASE_ADRS_SP_0 0x33000000

/** <Start and End Addresses for Slow peripheral 0 */
#define AX3000_CSR_BASE_ADRS_SP_1 0x33100000

/** <Start and End Addresses for Slow peripheral 0 */
#define AX3000_CSR_BASE_ADRS_SP_2 0x33200000

/** <Start and End Addresses for Slow peripheral 0 */
#define AX3000_CSR_BASE_ADRS_SP_3 0x33300000
#define AX3000_CSR_END_ADRS_SP_3  0x333FFFFF

#define AX3000_CSR_BASE_ADRS_R52_SMMU 0x33F00000

/** <Start and End Addresses for CLKRST */
#define AX3000_CSR_BASE_ADRS_CLKRST 0x40000000
#define AX3000_CSR_END_ADRS_CLKRST	0x40001FFF

/** <Start and End Addresses for DDRCTRL */
#define AX3000_CSR_BASE_ADRS_DDRCTRL 0x40010000
#define AX3000_CSR_END_ADRS_DDRCTRL	 0x40010FFF

/** <Start and End Addresses for DDRPHY */
#define AX3000_CSR_BASE_ADRS_DDRPHY 0x40020000
#define AX3000_CSR_END_ADRS_DDRPHY	0x40020FFF

/** <Start and End Addresses for NOC */
#define AX3000_CSR_BASE_ADRS_NOC 0x40100000
#define AX3000_CSR_END_ADRS_NOC	 0x401FFFFF

/** <Start and End Addresses for SRAM */
#define AX3000_MEM_BASE_ADRS_SRAM 0x40200000
#define AX3000_MEM_END_ADRS_SRAM  0x402FFFFF

/** <Start and End Addresses for SP_SMMU */
#define AX3000_CSR_BASE_SP_SMMU 0x40300000
#define AX3000_CSR_END_SP_SMMU	0x4040FFFF

/** <Start and End Addresses for GIC */
#define AX3000_CSR_BASE_ADRS_GIC 0x40400000
#define AX3000_CSR_END_ADRS_GIC	 0x403FFFFF

/** <Start and End Addresses for GPU*/
#define AX3000_CSR_BASE_ADRS_GPU 0x40900000
#define AX3000_CSR_END_ADRS_GPU	 0x4090FFFF

/** <Start and End Addresses for SPI */
#define AX3000_CSR_BASE_ADRS_SPI 0x40A00000
#define AX3000_CSR_END_ADRS_SPI	 0x40A03FFF

/** <Start and End Addresses for DISP */
#define AX3000_CSR_BASE_ADRS_DISP 0x40C00000
#define AX3000_CSR_END_ADRS_DISP  0x40C0FFFF

/** <Start and End Addresses for DISP_SUB */
#define AX3000_CSR_BASE_ADRS_DSUB 0x40C10000
#define AX3000_CSR_END_ADRS_DSUB  0x40C11FFF

/** <Start and End Addresses for DISP_SMMU */
#define AX3000_CSR_BASE_DISP_SMMU 0x40C20000
#define AX3000_CSR_END_DISP_SMMU  0x40C2FFFF

/** <Start and End Addresses for SMMU */
#define AX3000_CSR_BASE_ADRS_SMMU 0x40D00000
#define AX3000_CSR_END_ADRS_SMMU  0x40DFFFFF

/** <Start and End Addresses for DMA */
#define AX3000_CSR_BASE_ADRS_DMA 0x40E00000
#define AX3000_CSR_END_ADRS_DMA	 0x40E00FFF

/** <Start and End Addresses for USB2P_0 */
#define AX3000_CSR_BASE_ADRS_USB2P_0 0x41000000
#define AX3000_CSR_END_ADRS_USB2P_0	 0x410FFFFF

/** <Start and End Addresses for USB2P_1 */
#define AX3000_CSR_BASE_ADRS_USB2P0_1 0x41100000
#define AX3000_CSR_END_ADRS_USB2P0_1  0x411FFFFF

/** <Start and End Addresses for USB2P_1 */
#define AX3000_CSR_BASE_ADRS_USB2P0_2 0x41200000
#define AX3000_CSR_END_ADRS_USB2P0_2  0x412FFFFF

/** <Start and End Addresses for USB2P_1 */
#define AX3000_CSR_BASE_ADRS_USB2P0_3 0x41300000
#define AX3000_CSR_END_ADRS_USB2P0_3  0x413FFFFF

/** <Start and End Addresses for USB3P_0 */
#define AX3000_CSR_BASE_ADRS_USB3P0_0 0x41400000
#define AX3000_CSR_END_ADRS_USB3P0_0  0x414FFFFF

/** <Start and End Addresses for USB3P_1 */
#define AX3000_CSR_BASE_ADRS_USB3P0_1 0x41500000
#define AX3000_CSR_END_ADRS_USB3P0_1  0x415FFFFF

/** <Start and End Addresses for USB_SMMU */
#define AX3000_CSR_BASE_USB_SMMU 0x41600000
#define AX3000_CSR_END_USB_SMMU	 0x4160FFFF

/** <Start and End Addresses for VLT */
#define AX3000_CSR_BASE_ADRS_VLT 0x42000000
#define AX3000_CSR_END_ADRS_VLT	 0x420FFFFF

/** <Start and End Addresses for EIP */
#define AX3000_CSR_BASE_ADRS_EIP 0x43000000
#define AX3000_CSR_END_ADRS_EIP	 0x430FFFFF

/** <Start and End Addresses for HCP */
#define AX3000_CSR_BASE_ADRS_HCP 0x43100000
#define AX3000_CSR_END_ADRS_HCP	 0x43103FFF

/** <Start and End Addresses for HCP_SMMU */
#define AX3000_CSR_BASE_HCP_SMMU 0x43110000
#define AX3000_CSR_END_HCP_SMMU	 0x4311FFFF

/** <Start and End Addresses for FRM_MAIN_TO_FWL_MINI */
#define AX3000_MEM_BASE_ADRS_FRM_MAIN_TO_FWL_MINI 0x44000000

/** <Start and End Addresses for FNP_FWL */
#define AX3000_CSR_BASE_ADRS_FNPFWL 0x44100000
#define AX3000_CSR_END_ADRS_FNPFWL	0x441FFFFF

/** <Start and End Addresses for FWL_SMMU */
#define AX3000_CSR_BASE_FWL_SMMU 0x44200000
#define AX3000_CSR_END_FWL_SMMU	 0x4420FFFF

/** <Start and End Addresses for FWL_M55 */
#define AX3000_CSR_BASE_ADRS_FWL_M55 0x44210000
#define AX3000_CSR_END_ADRS_FWL_M55	 0x44210FFF

/** <Start and End Addresses for FWL_SRAM */
#define AX3000_MEM_BASE_ADRS_FWL_SRAM 0x44300000
#define AX3000_MEM_END_ADRS_FWL_SRAM  0x4437FFFF

/** <Start and End Addresses for PCIE_X2 */
#define AX3000_CSR_BASE_ADRS_PCIE_X2 0x45000000
#define AX3000_CSR_END_ADRS_PCIE_X2	 0x4503FFFF

/** <Start and End Addresses for PCIE_EXT_X2 */
#define AX3000_CSR_BASE_ADRS_PCIE_EXT_X2 0x45040000
#define AX3000_CSR_END_ADRS_PCIE_EXT_X2	 0x45040FFF

/** <Start and End Addresses for  PCIE_MBX_X2 */
#define AX3000_CSR_BASE_ADRS_PCIE_MBX_X2 0x45080000
#define AX3000_CSR_END_ADRS_PCIE_MBX_X2	 0x450FFFFF

/** <Start and End Addresses for PCI_PHY_X2 */
#define AX3000_CSR_BASE_ADRS_PCIE_PHY_X2 0x45100000
#define AX3000_CSR_END_ADRS_PCIE_PHY_X2	 0x45103FFF

/** <Start and End Addresses for PCIE_X2_SMMU */
#define AX3000_CSR_BASE_PCIE_X2_SMMU 0x45110000
#define AX3000_CSR_END_PCIE_X2_SMMU	 0x4511FFFF

/** <Start and End Addresses for PCIE_X1 */
#define AX3000_CSR_BASE_ADRS_PCIE_X1 0x45200000
#define AX3000_CSR_END_ADRS_PCIE_X1	 0x4523FFFF

/** <Start and End Addresses for PCIE_EXT_X1 */
#define AX3000_CSR_BASE_ADRS_PCIE_EXT_X1 0x45240000
#define AX3000_CSR_END_ADRS_PCIE_EXT_X1	 0x45240FFF

/** <Start and End Addresses for PCIE_MBX_X1 */
#define AX3000_CSR_BASE_ADRS_PCIE_MBX_X1 0x45280000
#define AX3000_CSR_END_ADRS_PCIE_MBX_X1	 0x452FFFFF

/** <Start and End Addresses for PCIE_PHY_X1 */
#define AX3000_CSR_BASE_ADRS_PCIE_PHY_X1 0x45300000
#define AX3000_CSR_END_ADRS_PCIE_PHY_X1	 0x45301FFF

/** <Start and End Addresses for PCIE_X1_SMMU */
#define AX3000_CSR_BASE_PCIE_X1_SMMU 0x45310000
#define AX3000_CSR_END_PCIE_X1_SMMU	 0x4531FFFF

/** <Start and End Addresses for EMMC */
#define AX3000_CSR_BASE_ADRS_AHBS_EMMC 0x46000000
#define AX3000_CSR_END_ADRS_AHBS_EMMC  0x4600FFFF

/** <Start and End Addresses for ETH_PHY */
#define AX3000_CSR_BASE_ADRS_ETH_PHY 0x47000000
#define AX3000_CSR_END_ADRS_ETH_PHY	 0x4707FFFF

/** <Start and End Addresses for USBPHY8 */
#define AX3000_CSR_BASE_ADRS_USBPHY8 0x47080000
#define AX3000_CSR_END_ADRS_USBPHY8	 0x470FFFFF

/** <Start and End Addresses for MCE */
#define AX3000_CSR_BASE_ADRS_MCE 0x47100000
#define AX3000_CSR_END_ADRS_MCE	 0x471003FF

/** <Start and End Addresses for SDIO */
#define AX3000_CSR_BASE_ADRS_AHBS_SDIO 0x47200000
#define AX3000_CSR_END_ADRS_AHBS_SDIO  0x4720FFFF

/** <Start and End Addresses for ML_0 */
#define AX3000_CSR_BASE_ADRS_ML_0 0x48000000
#define AX3000_CSR_END_ADRS_ML_0  0x48001FFF

/** <Start and End Addresses for ML_1 */
#define AX3000_CSR_BASE_ADRS_ML_1 0x48002000
#define AX3000_CSR_END_ADRS_ML_1  0x48003FFF

/** <Start and End Addresses for ML_2 */
#define AX3000_CSR_BASE_ADRS_ML_2 0x48004000
#define AX3000_CSR_END_ADRS_ML_2  0x48005FFF

/** <Start and End Addresses for ML_3 */
#define AX3000_CSR_BASE_ADRS_ML_3 0x48006000
#define AX3000_CSR_END_ADRS_ML_3  0x48007FFF

/** Start and End Addresses for CPU  */
#define AX3000_CSR_BASE_ADRS_CPU 0x48014000
#define AX3000_CSR_END_ADRS_CPU	 0x48017FFF

/** Start and End Addresses for ML_SRAM_0 */
#define AX3000_MEM_BASE_ADRS_ML_SRAM_0 0x48100000
#define AX3000_MEM_END_ADRS_ML_SRAM_0  0x4817FFFF

/** Start and End Addresses for ML_SRAM_1 */
#define AX3000_MEM_BASE_ADRS_ML_SRAM_1 0x48140000
#define AX3000_MEM_END_ADRS_ML_SRAM_1  0x4817FFFF

/** Start and End Addresses for ML_SRAM_2 */
#define AX3000_MEM_BASE_ADRS_ML_SRAM_2 0x48180000
#define AX3000_MEM_END_ADRS_ML_SRAM_2  0x481BFFFF

/** Start and End Addresses for ML_SRAM_3 */
#define AX3000_MEM_BASE_ADRS_ML_SRAM_3 0x481C0000
#define AX3000_MEM_END_ADRS_ML_SRAM_3  0x481C7FFF

/** Start and End Addresses for ML0_SMMU */
#define AX3000_CSR_BASE_ML0_SMMU 0x48500000
#define AX3000_CSR_END_ML0_SMMU	 0x4850FFFF

/** Start and End Addresses for ML1_SMMU */
#define AX3000_CSR_BASE_ML1_SMMU 0x48510000
#define AX3000_CSR_END_ML1_SMMU	 0x4851FFFF

/** Start and End Addresses for ML2_SMMU */
#define AX3000_CSR_BASE_ML2_SMMU 0x48520000
#define AX3000_CSR_END_ML2_SMMU	 0x4852FFFF

/** Start and End Addresses for ML3_SMMU */
#define AX3000_CSR_BASE_ML3_SMMU 0x48530000
#define AX3000_CSR_END_ML3_SMMU	 0x4853FFFF

/** Start and End Addresses for CPU_SH_DBG */
#define AX3000_CSR_BASE_ADRS_CPU_SH_DBG 0x4A000000
#define AX3000_CSR_END_ADRS_CPU_SH_DBG	0x4BFFFFFF

/** Start and End Addresses for PCIE_X2 */
#define AX3000_MEM_BASE_ADRS_PCIE_X2 0x500000000
#define AX3000_MEM_END_ADRS_PCIE_X2	 0x5FFFFFFFF

/** Start and End Addresses for PCIE_X1 */
#define AX3000_MEM_BASE_ADRS_PCIE_X1 0x700000000
#define AX3000_MEM_END_ADRS_PCIE_X1	 0x7FFFFFFFF

/** Start and End Addresses for PCIE_X2  */
#define AX3000_MEM_BASE_ADRS_PCIE_X2_1 0x600000000
#define AX3000_MEM_END_ADRS_PCIE_X2_1  0x6FFFFFFFF

#define AX3000_MEM_BASE_ADRS_DDR_LO 0x80000000
#define AX3000_MEM_END_ADRS_DDR_LO	0xFFFFFFFF

/** Start and End Addresses for DDR_HI */
#define AX3000_MEM_BASE_ADRS_DDR_HI 0x100000000
#define AX3000_MEM_END_ADRS_DDR_HI	0x17FFFFFFF

/** Start and End Addresses for DDR_HI_2 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_2 0x180000000
#define AX3000_MEM_END_ADRS_DDR_HI_2  0x1FFFFFFFF

/** Start and End Addresses for DDR_HI_3 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_3 0x200000000
#define AX3000_MEM_END_ADRS_DDR_HI_3  0x27FFFFFFF

/** Start and End Addresses for DDR_HI_4 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_4 0x280000000
#define AX3000_MEM_END_ADRS_DDR_HI_4  0x2FFFFFFFF

/** Start and End Addresses for DDR_HI_5 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_5 0x300000000
#define AX3000_MEM_END_ADRS_DDR_HI_5  0x37FFFFFFF

/** Start and End Addresses for DDR_HI_6 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_6 0x380000000
#define AX3000_MEM_END_ADRS_DDR_HI_6  0x3FFFFFFFF

/** Start and End Addresses for DDR_HI_7 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_7 0x400000000
#define AX3000_MEM_END_ADRS_DDR_HI_7  0x47FFFFFFF

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AX_HAL_AX3000_BASE_ADRS_H */
