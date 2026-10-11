/** Copyright (c) 2021-22 Axiado Corporation (or its affiliates). All rights reserved.
 * Use, modification and redistribution of this file is subject to your possession
 * of a valid End User License Agreement (EULA) for the Axiado Product of which
 * these sources are part of and your compliance with all applicable terms and
 * conditions of such licence agreement.
 */
/**kirkwood_base_adrs.h*/
#ifndef _AX_HAL_AX3000_BASE_ADDR_H
#define _AX_HAL_AX3000_BASE_ADDR_H

/**
 * @file ax3000_base_addr.h
 * 
 * @brief This file contains the Axiado Base Address registers
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** <Start and End Addresses for DDR_0 */
#define AX3000_MEM_BASE_ADRS_DDR_LO 0x0
#define AX3000_MEM_END_ADRS_DDR_LO	0x7FFFFFFF

/** <Start and End Addresses for CLKRST */
#define AX3000_CSR_BASE_ADRS_CLKRST 0x80000000
#define AX3000_CSR_END_ADRS_CLKRST	0x80001FFF

/** <Start and End Addresses for DDRCTRL */
#define AX3000_CSR_BASE_ADRS_DDRCTRL 0x80010000
#define AX3000_CSR_END_ADRS_DDRCTRL	 0x80010FFF

/** <Start and End Addresses for DDRPHY */
#define AX3000_CSR_BASE_ADRS_DDRPHY 0x80020000
#define AX3000_CSR_END_ADRS_DDRPHY	0x80020FFF

/** <Start and End Addresses for NOC */
#define AX3000_CSR_BASE_ADRS_NOC 0x80100000
#define AX3000_CSR_END_ADRS_NOC	 0x801FFFFF

/** <Start and End Addresses for SRAM */
#define AX3000_MEM_BASE_ADRS_SRAM 0x80200000
#define AX3000_MEM_END_ADRS_SRAM  0x802FFFFF

/** <Start and End Addresses for GIC */
#define AX3000_CSR_BASE_ADRS_GIC 0x80300000
#define AX3000_CSR_END_ADRS_GIC	 0x803FFFFF

/** <Start and End Addresses for SP_SMMU */
#define AX3000_CSR_BASE_SP_SMMU 0x80400000
#define AX3000_CSR_END_SP_SMMU	0x8040FFFF

/** <Start and End Addresses for Slow peripheral 0 */
#define AX3000_CSR_BASE_ADRS_SP_0 0x80500000
#define AX3000_CSR_END_ADRS_SP_0  0x805FFFFF

/** <Start and End Addresses for Slow peripheral 1 */
#define AX3000_CSR_BASE_ADRS_SP_1 0x80600000
#define AX3000_CSR_END_ADRS_SP_1  0x806FFFFF

/** <Start and End Addresses for Slow peripheral 2 */
#define AX3000_CSR_BASE_ADRS_SP_2 0x80700000
#define AX3000_CSR_END_ADRS_SP_2  0x807FFFFF

/** <Start and End Addresses for Slow peripheral 3 */
#define AX3000_CSR_BASE_ADRS_SP_3 0x80800000
#define AX3000_CSR_END_ADRS_SP_3  0x808FFFFF

/** <Start and End Addresses for GPU*/
#define AX3000_CSR_BASE_ADRS_GPU 0x80900000
#define AX3000_CSR_END_ADRS_GPU	 0x8090FFFF

/** <Start and End Addresses for DISP */
#define AX3000_CSR_BASE_ADRS_DISP 0x80C00000
#define AX3000_CSR_END_ADRS_DISP  0x80C0FFFF

/** <Start and End Addresses for DISP_SUB */
#define AX3000_CSR_BASE_ADRS_DSUB 0x80C10000
#define AX3000_CSR_END_ADRS_DSUB  0x80C11FFF

/** <Start and End Addresses for DISP_SMMU */
#define AX3000_CSR_BASE_DISP_SMMU 0x80C20000
#define AX3000_CSR_END_DISP_SMMU  0x80C2FFFF

/** <Start and End Addresses for SMMU */
#define AX3000_CSR_BASE_ADRS_SMMU 0x80D00000
#define AX3000_CSR_END_ADRS_SMMU  0x80DFFFFF

/** <Start and End Addresses for DMA */
#define AX3000_CSR_BASE_ADRS_DMA 0x80E00000
#define AX3000_CSR_END_ADRS_DMA	 0x80E00FFF

/** <Start and End Addresses for USB2P_0 */
#define AX3000_CSR_BASE_ADRS_USB2P0_0 0x81000000
#define AX3000_CSR_END_ADRS_USB2P0_0  0x810FFFFF

/** <Start and End Addresses for USB2P_1 */
#define AX3000_CSR_BASE_ADRS_USB2P0_1 0x81100000
#define AX3000_CSR_END_ADRS_USB2P0_1  0x811FFFFF

/** <Start and End Addresses for USB2P_2 */
#define AX3000_CSR_BASE_ADRS_USB2P0_2 0x81200000
#define AX3000_CSR_END_ADRS_USB2P0_2  0x812FFFFF

/** <Start and End Addresses for USB2P_3 */
#define AX3000_CSR_BASE_ADRS_USB2P0_3 0x81300000
#define AX3000_CSR_END_ADRS_USB2P0_3  0x813FFFFF

/** <Start and End Addresses for USB3P_0 */
#define AX3000_CSR_BASE_ADRS_USB3P0_0 0x81400000
#define AX3000_CSR_END_ADRS_USB3P0_0  0x814FFFFF

/** <Start and End Addresses for USB3P_1 */
#define AX3000_CSR_BASE_ADRS_USB3P0_1 0x81500000
#define AX3000_CSR_END_ADRS_USB3P0_1  0x815FFFFF

/** <Start and End Addresses for USB_SMMU */
#define AX3000_CSR_BASE_USB_SMMU 0x81600000
#define AX3000_CSR_END_USB_SMMU	 0x8160FFFF

/** <Start and End Addresses for VLT */
#if CONFIG_HAPSEC_BUILD
#define AX3000_CSR_BASE_ADRS_VLT 0x42000000
#define AX3000_CSR_END_ADRS_VLT	 0x420FFFFF
#else
#define AX3000_CSR_BASE_ADRS_VLT 0x82000000
#define AX3000_CSR_END_ADRS_VLT	 0x820FFFFF
#endif

/** <Start and End Addresses for EIP */
#define AX3000_CSR_BASE_ADRS_EIP 0x83000000
#define AX3000_CSR_END_ADRS_EIP	 0x830FFFFF

/** <Start and End Addresses for HCP */
#define AX3000_CSR_BASE_ADRS_HCP 0x83100000
#define AX3000_CSR_END_ADRS_HCP	 0x83103FFF

/** <Start and End Addresses for HCP_SMMU */
#define AX3000_CSR_BASE_HCP_SMMU 0x83110000
#define AX3000_CSR_END_HCP_SMMU	 0x8311FFFF

/** <Start and End Addresses for FNP_FWL */
#define AX3000_CSR_BASE_ADRS_FNPFWL 0x84100000
#define AX3000_CSR_END_ADRS_FNPFWL	0x841FFFFF

/** <Start and End Addresses for FWL_SMMU */
#define AX3000_CSR_BASE_FWL_SMMU 0x84200000
#define AX3000_CSR_END_FWL_SMMU	 0x8420FFFF

/** <Start and End Addresses for FWL_M55 */
#define AX3000_CSR_BASE_ADRS_FWL_M55 0x84210000
#define AX3000_CSR_END_ADRS_FWL_M55	 0x84210FFF

/** <Start and End Addresses for FWL_SRAM */
#define AX3000_MEM_BASE_ADRS_FWL_SRAM 0x84300000
#define AX3000_MEM_END_ADRS_FWL_SRAM  0x8437FFFF

/** <Start and End Addresses for PCIE_X2 */
#define AX3000_CSR_BASE_ADRS_PCIE_X2 0x85000000
#define AX3000_CSR_END_ADRS_PCIE_X2	 0x8503FFFF

/** <Start and End Addresses for PCIE_EXT_X2 */
#define AX3000_CSR_BASE_ADRS_PCIE_EXT_X2 0x85040000
#define AX3000_CSR_END_ADRS_PCIE_EXT_X2	 0x85040FFF

/** <Start and End Addresses for  PCIE_MBX_X2 */
#define AX3000_CSR_BASE_ADRS_PCIE_MBX_X2 0x85080000
#define AX3000_CSR_END_ADRS_PCIE_MBX_X2	 0x850FFFFF

/** <Start and End Addresses for PCI_PHY_X2 */
#define AX3000_CSR_BASE_ADRS_PCIE_PHY_X2 0x85100000
#define AX3000_CSR_END_ADRS_PCIE_PHY_X2	 0x85103FFF

/** <Start and End Addresses for PCIE_X2_SMMU */
#define AX3000_CSR_BASE_PCIE_X2_SMMU 0x85110000
#define AX3000_CSR_END_PCIE_X2_SMMU	 0x8511FFFF

/** <Start and End Addresses for PCIE_X1 */
#define AX3000_CSR_BASE_ADRS_PCIE_X1 0x85200000
#define AX3000_CSR_END_ADRS_PCIE_X1	 0x8523FFFF

/** <Start and End Addresses for PCIE_EXT_X1 */
#define AX3000_CSR_BASE_ADRS_PCIE_EXT_X1 0x85240000
#define AX3000_CSR_END_ADRS_PCIE_EXT_X1	 0x85240FFF

/** <Start and End Addresses for PCIE_MBX_X1 */
#define AX3000_CSR_BASE_ADRS_PCIE_MBX_X1 0x85280000
#define AX3000_CSR_END_ADRS_PCIE_MBX_X1	 0x852FFFFF

/** <Start and End Addresses for PCIE_PHY_X1 */
#define AX3000_CSR_BASE_ADRS_PCIE_PHY_X1 0x85300000
#define AX3000_CSR_END_ADRS_PCIE_PHY_X1	 0x85301FFF

/** <Start and End Addresses for PCIE_X1_SMMU */
#define AX3000_CSR_BASE_PCIE_X1_SMMU 0x85310000
#define AX3000_CSR_END_PCIE_X1_SMMU	 0x8531FFFF

/** <Start and End Addresses for EMMC */
#define AX3000_CSR_BASE_ADRS_AHBS_EMMC 0x86000000
#define AX3000_CSR_END_ADRS_AHBS_EMMC  0x8600FFFF

/** <Start and End Addresses for ETH_PHY */
#define AX3000_CSR_BASE_ADRS_ETH_PHY 0x87000000
#define AX3000_CSR_END_ADRS_ETH_PHY	 0x8707FFFF

/** <Start and End Addresses for USBPHY8 */
#define AX3000_CSR_BASE_ADRS_USBPHY8 0x87080000
#define AX3000_CSR_END_ADRS_USBPHY8	 0x870FFFFF

/** <Start and End Addresses for MCE */
#define AX3000_CSR_BASE_ADRS_MCE 0x87100000
#define AX3000_CSR_END_ADRS_MCE	 0x871003FF

/** <Start and End Addresses for SDIO */
#define AX3000_CSR_BASE_ADRS_AHBS_SDIO 0x87200000
#define AX3000_CSR_END_ADRS_AHBS_SDIO  0x8720FFFF

/** <Start and End Addresses for ML_0 */
#define AX3000_CSR_BASE_ADRS_ML_0 0x88000000
#define AX3000_CSR_END_ADRS_ML_0  0x88001FFF

/** <Start and End Addresses for ML_1 */
#define AX3000_CSR_BASE_ADRS_ML_1 0x88002000
#define AX3000_CSR_END_ADRS_ML_1  0x88003FFF

/** <Start and End Addresses for ML_2 */
#define AX3000_CSR_BASE_ADRS_ML_2 0x88004000
#define AX3000_CSR_END_ADRS_ML_2  0x88005FFF

/** <Start and End Addresses for ML_3 */
#define AX3000_CSR_BASE_ADRS_ML_3 0x88006000
#define AX3000_CSR_END_ADRS_ML_3  0x88007FFF

/** <Start and End Addresses for CPU  */
#define AX3000_CSR_BASE_ADRS_CPU 0x88014000
#define AX3000_CSR_END_ADRS_CPU	 0x88015FFF

/** <Start and End Addresses for ML_SRAM_0 */
#define AX3000_MEM_BASE_ADRS_ML_SRAM_0 0x88100000
#define AX3000_MEM_END_ADRS_ML_SRAM_0  0x8817FFFF

/** <Start and End Addresses for ML_SRAM_1 */
#define AX3000_MEM_BASE_ADRS_ML_SRAM_1 0x88200000
#define AX3000_MEM_END_ADRS_ML_SRAM_1  0x8827FFFF

/** <Start and End Addresses for ML_SRAM_2 */
#define AX3000_MEM_BASE_ADRS_ML_SRAM_2 0x88300000
#define AX3000_MEM_END_ADRS_ML_SRAM_2  0x8837FFFF

/** <Start and End Addresses for ML_SRAM_3 */
#define AX3000_MEM_BASE_ADRS_ML_SRAM_3 0x88400000
#define AX3000_MEM_END_ADRS_ML_SRAM_3  0x8847FFFF

/** <Start and End Addresses for ML0_SMMU */
#define AX3000_CSR_BASE_ML0_SMMU 0x88500000
#define AX3000_CSR_END_ML0_SMMU	 0x8850FFFF

/** <Start and End Addresses for ML1_SMMU */
#define AX3000_CSR_BASE_ML1_SMMU 0x88510000
#define AX3000_CSR_END_ML1_SMMU	 0x8851FFFF

/** <Start and End Addresses for ML2_SMMU */
#define AX3000_CSR_BASE_ML2_SMMU 0x88520000
#define AX3000_CSR_END_ML2_SMMU	 0x8852FFFF

/** <Start and End Addresses for ML3_SMMU */
#define AX3000_CSR_BASE_ML3_SMMU 0x88530000
#define AX3000_CSR_END_ML3_SMMU	 0x8853FFFF

/** <Start and End Addresses for CPU_SH_DBG */
#define AX3000_CSR_BASE_ADRS_CPU_SH_DBG 0x8A000000
#define AX3000_CSR_END_ADRS_CPU_SH_DBG	0x8BFFFFFF

/** <Start and End Addresses for VLT_EXT */
#define AX3000_MEM_BASE_ADRS_VLTEXT 0x100000000llu
#define AX3000_MEM_END_ADRS_VLTEXT	0x13FFFFFFFllu

/** <Start and End Addresses for PCIE_X2  */
#define AX3000_MEM_BASE_ADRS_PCIE_X2 0x200000000llu
#define AX3000_MEM_END_ADRS_PCIE_X2	 0x2FFFFFFFFllu

/** <Start and End Addresses for PCIE_X1 */
#define AX3000_MEM_BASE_ADRS_PCIE_X1 0x300000000llu
#define AX3000_MEM_END_ADRS_PCIE_X1	 0x3FFFFFFFFllu

/** <Start and End Addresses for DDR_HI_1 */
#define AX3000_MEM_BASE_ADRS_DDR_HI 0x400000000llu
#define AX3000_MEM_END_ADRS_DDR_HI	0x47FFFFFFFllu

/** <Start and End Addresses for DDR_HI_2 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_2 0x480000000llu
#define AX3000_MEM_END_ADRS_DDR_HI_2  0x4FFFFFFFFllu

/** <Start and End Addresses for DDR_HI_3 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_3 0x500000000llu
#define AX3000_MEM_END_ADRS_DDR_HI_3  0x57FFFFFFFllu

/** <Start and End Addresses for DDR_HI_4 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_4 0x580000000llu
#define AX3000_MEM_END_ADRS_DDR_HI_4  0x5FFFFFFFFllu

/** <Start and End Addresses for DDR_HI_5 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_5 0x600000000llu
#define AX3000_MEM_END_ADRS_DDR_HI_5  0x67FFFFFFFllu

/** <Start and End Addresses for DDR_HI_6 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_6 0x680000000llu
#define AX3000_MEM_END_ADRS_DDR_HI_6  0x6FFFFFFFFllu

/** <Start and End Addresses for DDR_HI_7 */
#define AX3000_MEM_BASE_ADRS_DDR_HI_7 0x700000000llu
#define AX3000_MEM_END_ADRS_DDR_HI_7  0x77FFFFFFFllu

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AX_HAL_AX3000_BASE_ADDR_H */
