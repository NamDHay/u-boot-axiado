// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#ifndef __AXIADO_JTAG_H
#define __AXIADO_JTAG_H

#define AX3000_CSR_BASE_ADRS_DSUB_JTAG	(AX3000_CSR_BASE_ADRS_DSUB + 0x0000)

/*------------------------------------------------------------------------------*/
/**
* Name:DSUBCFG0
* Offset:0x000
* Type:default
* Comment:
*/
#define REG_DSUBCFG0_ADRS_OFFSET 0x000
#define REG_DSUBCFG0_REG_TYPE	 DEFAULT
/**
* Name: TEXFMTENABLE
* Bit Assignment: [31:0]
* Type:rw
* Initial_Value:0xffffffff
* Reference:
* Comment:Texture Formats enable. See GPU TRM Appendix B.
*/
#define REG_DSUBCFG0_FLD_TEXFMTENABLE_MSB	   31
#define REG_DSUBCFG0_FLD_TEXFMTENABLE_LSB	   0
#define REG_DSUBCFG0_FLD_TEXFMTENABLE_WIDTH	   32
#define REG_DSUBCFG0_FLD_TEXFMTENABLE_TYPE	   RW
#define REG_DSUBCFG0_FLD_TEXFMTENABLE_INIT_VAL 0xffffffff
/*------------------------------------------------------------------------------*/
/**
* Name:DSUBCFG1
* Offset:0x004
* Type:default
* Comment:
*/
#define REG_DSUBCFG1_ADRS_OFFSET 0x004
#define REG_DSUBCFG1_REG_TYPE	 DEFAULT
/**
* Name: GPU_DBGAUTH_EN
* Bit Assignment: [0:0]
* Type:rw
* Initial_Value:0x0
* Reference:
* Comment:Affect the amount of status information that is visible from GPUs in protected mode. 1 = job slot status and fault information is not restriced in protected mode.
*/
#define REG_DSUBCFG1_FLD_GPU_DBGAUTH_EN_MSB		 0
#define REG_DSUBCFG1_FLD_GPU_DBGAUTH_EN_LSB		 0
#define REG_DSUBCFG1_FLD_GPU_DBGAUTH_EN_WIDTH	 1
#define REG_DSUBCFG1_FLD_GPU_DBGAUTH_EN_TYPE	 RW
#define REG_DSUBCFG1_FLD_GPU_DBGAUTH_EN_INIT_VAL 0x0
/**
* Name: STRIPING_GRANULE
* Bit Assignment: [3:1]
* Type:rw
* Initial_Value:0x7
* Reference:
* Comment:Specify the GPU striping granule size
*/
#define REG_DSUBCFG1_FLD_STRIPING_GRANULE_MSB	   3
#define REG_DSUBCFG1_FLD_STRIPING_GRANULE_LSB	   1
#define REG_DSUBCFG1_FLD_STRIPING_GRANULE_WIDTH	   3
#define REG_DSUBCFG1_FLD_STRIPING_GRANULE_TYPE	   RW
#define REG_DSUBCFG1_FLD_STRIPING_GRANULE_INIT_VAL 0x7
/**
* Name: VGA_ENABLE
* Bit Assignment: [4:4]
* Type:rw
* Initial_Value:0x0
* Reference:
* Comment:Enable the VGA Interface. 0 = VGA outputs are tied low, 1 = VGA outputs are active
*/
#define REG_DSUBCFG1_FLD_VGA_ENABLE_MSB		 4
#define REG_DSUBCFG1_FLD_VGA_ENABLE_LSB		 4
#define REG_DSUBCFG1_FLD_VGA_ENABLE_WIDTH	 1
#define REG_DSUBCFG1_FLD_VGA_ENABLE_TYPE	 RW
#define REG_DSUBCFG1_FLD_VGA_ENABLE_INIT_VAL 0x0
/**
* Name: CNTVALUEB_ENABLE
* Bit Assignment: [5:5]
* Type:rw
* Initial_Value:0x1
* Reference:
* Comment:Enable the CNTVALUEB timer. 0 = Disabled, 1 = Enabled
*/
#define REG_DSUBCFG1_FLD_CNTVALUEB_ENABLE_MSB	   5
#define REG_DSUBCFG1_FLD_CNTVALUEB_ENABLE_LSB	   5
#define REG_DSUBCFG1_FLD_CNTVALUEB_ENABLE_WIDTH	   1
#define REG_DSUBCFG1_FLD_CNTVALUEB_ENABLE_TYPE	   RW
#define REG_DSUBCFG1_FLD_CNTVALUEB_ENABLE_INIT_VAL 0x1
/**
* Name: GPU_AXIM_UPPER
* Bit Assignment: [15:8]
* Type:rw
* Initial_Value:0x00
* Reference:
* Comment:System Base Offset Supplement. Provides the upper 8 bits for the GPU AXIm address bus.
*/
#define REG_DSUBCFG1_FLD_GPU_AXIM_UPPER_MSB		 15
#define REG_DSUBCFG1_FLD_GPU_AXIM_UPPER_LSB		 8
#define REG_DSUBCFG1_FLD_GPU_AXIM_UPPER_WIDTH	 8
#define REG_DSUBCFG1_FLD_GPU_AXIM_UPPER_TYPE	 RW
#define REG_DSUBCFG1_FLD_GPU_AXIM_UPPER_INIT_VAL 0x00
/**
* Name: DISP_AXIM_UPPER
* Bit Assignment: [31:16]
* Type:rw
* Initial_Value:0x0000
* Reference:
* Comment:System Base Offset Supplement. Provides the upper 16 bits for the display controller AXIm address bus.
*/
#define REG_DSUBCFG1_FLD_DISP_AXIM_UPPER_MSB	  31
#define REG_DSUBCFG1_FLD_DISP_AXIM_UPPER_LSB	  16
#define REG_DSUBCFG1_FLD_DISP_AXIM_UPPER_WIDTH	  16
#define REG_DSUBCFG1_FLD_DISP_AXIM_UPPER_TYPE	  RW
#define REG_DSUBCFG1_FLD_DISP_AXIM_UPPER_INIT_VAL 0x0000
/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
/**
* Name:DSUBCFG3
* Offset:0x00c
* Type:default
* Comment:
*/
#define REG_DSUBCFG3_ADRS_OFFSET 0x00c
#define REG_DSUBCFG3_REG_TYPE	 DEFAULT
/**
* Name: CNTVALUEB_SEED_LSB
* Bit Assignment: [31:0]
* Type:rw
* Initial_Value:0x00000000
* Reference:
* Comment:32 lower bits for the cntvalueb_timer seed
*/
#define REG_DSUBCFG3_FLD_CNTVALUEB_SEED_LSB_MSB		 31
#define REG_DSUBCFG3_FLD_CNTVALUEB_SEED_LSB_LSB		 0
#define REG_DSUBCFG3_FLD_CNTVALUEB_SEED_LSB_WIDTH	 32
#define REG_DSUBCFG3_FLD_CNTVALUEB_SEED_LSB_TYPE	 RW
#define REG_DSUBCFG3_FLD_CNTVALUEB_SEED_LSB_INIT_VAL 0x00000000
/*------------------------------------------------------------------------------*/
/**
* Name:DSUBCFG4
* Offset:0x010
* Type:default
* Comment:
*/
#define REG_DSUBCFG4_ADRS_OFFSET 0x010
#define REG_DSUBCFG4_REG_TYPE	 DEFAULT
/**
* Name: CNTVALUEB_SEED_MSB
* Bit Assignment: [31:0]
* Type:rw
* Initial_Value:0x00000000
* Reference:
* Comment:32 upper bits for the cntvalueb_timer seed
*/
#define REG_DSUBCFG4_FLD_CNTVALUEB_SEED_MSB_MSB		 31
#define REG_DSUBCFG4_FLD_CNTVALUEB_SEED_MSB_LSB		 0
#define REG_DSUBCFG4_FLD_CNTVALUEB_SEED_MSB_WIDTH	 32
#define REG_DSUBCFG4_FLD_CNTVALUEB_SEED_MSB_TYPE	 RW
#define REG_DSUBCFG4_FLD_CNTVALUEB_SEED_MSB_INIT_VAL 0x00000000
/*------------------------------------------------------------------------------*/
#define REG_HOST_JTAG_ADRS_OFFSET 0x014
#define REG_HOST_JTAG_REG_TYPE	  DEFAULT

#define REG_HOST_JTAG_FLD_ENABLE_MSB	  0
#define REG_HOST_JTAG_FLD_ENABLE_LSB	  0
#define REG_HOST_JTAG_FLD_ENABLE_WIDTH	  1
#define REG_HOST_JTAG_FLD_ENABLE_TYPE	  RW
#define REG_HOST_JTAG_FLD_ENABLE_INIT_VAL 0x0

#define REG_HOST_JTAG_FLD_TCK_SEL_MSB	   5
#define REG_HOST_JTAG_FLD_TCK_SEL_LSB	   4
#define REG_HOST_JTAG_FLD_TCK_SEL_WIDTH	   2
#define REG_HOST_JTAG_FLD_TCK_SEL_TYPE	   RW
#define REG_HOST_JTAG_FLD_TCK_SEL_INIT_VAL 0x0
/*------------------------------------------------------------------------------*/

#endif /* __AXIADO_JTAG_H */

