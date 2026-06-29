/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2023-2025 Axiado Corporation.
 */

#ifndef __PCIE_AXIADO_H__
#define __PCIE_AXIADO_H__

#include <asm/io.h>

#define usleep_range(a, b) udelay((b))

#ifndef CONFIG_TARGET_SCM3005
#define REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET 0x09c
#define REG_PCIE_X1_IP_CTRL_ADRS_OFFSET 0x058
#define REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET 0x0fc
#else
#define REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET 0x15C
#define REG_PCIE_X1_IP_CTRL_ADRS_OFFSET 0x118
#define REG_PCIE_X1_GEN_OFF 0x10C
#define REG_PCIE_X1_GEN_VAL 0xFE
#define AX_PCIE_TL_CLOCK_FREQ_MHZ  500
#define REG_PCIE_X1_GEN_SETTINGS_ADRS_OFFSET 0x0000 + 0x0080
#define REG_PCIE_X1_PCIE_PEX_DEV_ADRS_OFFSET 0x0000 + 0x00C0
#define REG_PCIE_X1_PCIE_PEX_SPC_ADRS_OFFSET 0x0000 + 0x00D4
#define REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET 0x1cc
#endif
#define REG_PCIE_PCIE_CFGCTRL_ADRS_OFFSET 0x0084

#define REG_PCIE_X2_PF_MBX_INTR_MASK_ADRS_OFFSET 0x0a4
#define REG_PCIE_X2_PCIE_CFGCTRL_ADRS_OFFSET 0x0084

#define REG_PAD_CFG_REG_25_ADRS_OFFSET 0x070
#define REG_PCIE_GEN_SETTINGS_ADRS_OFFSET (0x0000 + 0x0080)
#define REG_PCIE_X1_PCIE_PCI_IDS_63_32_ADRS_OFFSET (0x0000 + 0x009C)
#define REG_PCIE_PCIE_PCI_IDS_63_32_ADRS_OFFSET (0x0000 + 0x009C)

#define REG_PAD_CFG_REG_26_ADRS_OFFSET 0x074
#ifdef CONFIG_TARGET_SCM3005
#define REG_PCIE_X2_IP_CTRL_ADRS_OFFSET 0x188
#else
#define REG_PCIE_X2_IP_CTRL_ADRS_OFFSET 0x0b8
#endif
#define REG_PCIE_X2_PCIE_BASIC_CONF_ADRS_OFFSET (0x0000 + 0x0014)
#define REG_PCIE_X2_GEN_SETTINGS_ADRS_OFFSET (0x0000 + 0x0080)
#define REG_PCIE_X2_PCIE_PCI_IDS_63_32_ADRS_OFFSET (0x0000 + 0x009C)
#define REG_PCIE_X1_COMMAND_STATUS_ADRS_OFFSET (0x0000 + 0x4)

#define PCIE_ATR_PCIE_WIN0 0x600
#define REG_PCIE_ATR_PCIE_WIN0_TAB0_TRSL_ADDR_31_0_ADRS_OFFSET (0x600 + 0x0008)
#define REG_PCIE_ATR_PCIE_WIN0_TAB1_TRSL_ADDR_31_0_ADRS_OFFSET (0x600 + 0x0028)
#define REG_PCIE_ATR_PCIE_WIN0_TAB2_TRSL_ADDR_31_0_ADRS_OFFSET (0x600 + 0x0048)
#define REG_PCIE_ATR_PCIE_WIN0_TAB3_TRSL_ADDR_31_0_ADRS_OFFSET (0x600 + 0x0068)
#define REG_PCIE_ATR_PCIE_WIN0_TAB4_TRSL_ADDR_31_0_ADRS_OFFSET (0x600 + 0x0088)
#define REG_PCIE_ATR_PCIE_WIN0_TAB5_TRSL_ADDR_31_0_ADRS_OFFSET (0x600 + 0x00a8)
#define REG_PCIE_ATR_PCIE_WIN0_TAB6_TRSL_ADDR_31_0_ADRS_OFFSET (0x600 + 0x00C8)
#define REG_PCIE_X1_BAR_SIZE_OFFSET 0xEC

#define REG_PCIE_PHYMAC_CFG_ADRS_OFFSET (0x0300 + 0x003C)
#define REG_PCIE_EQ_TUNING_31_0_ADRS_OFFSET (0x0300 + 0x005C)
#define REG_PCIE_EQ_TUNING_63_32_ADRS_OFFSET (0x0300 + 0x0060)
#define REG_PCIE_EQ_PRESET_8G_31_0_ADRS_OFFSET (0x0000 + 0x0100)
#define REG_PCIE_EQ_PRESET_16G_31_0_ADRS_OFFSET (0x0000 + 0x0150)
#define REG_PCIE_DMA_BASE  (0x400)
#define REG_PCIE_DMA0_CONFIG_SPACE_DMA_SRCPARAM_ADRS_OFFSET       (0x00)
#define REG_PCIE_DMA0_CONFIG_SPACE_DMA_DESTPARAM_ADRS_OFFSET      (0x04)
#define REG_PCIE_DMA0_CONFIG_SPACE_DMA_SRCADDR_31_0_ADRS_OFFSET   (0x08)
#define REG_PCIE_DMA0_CONFIG_SPACE_DMA_SRCADDR_63_32_ADRS_OFFSET  (0x0C)
#define REG_PCIE_DMA0_CONFIG_SPACE_DMA_DESTADDR_31_0_ADRS_OFFSET  (0x10)
#define REG_PCIE_DMA0_CONFIG_SPACE_DMA_DESTADDR_63_32_ADRS_OFFSET (0x14)
#define REG_PCIE_DMA0_CONFIG_SPACE_DMA_LENGTH_ADRS_OFFSET         (0x18)
#define REG_PCIE_DMA0_CONFIG_SPACE_DMA_CONTROL_ADRS_OFFSET        (0x1C)
#define REG_PCIE_DMA0_CONFIG_SPACE_DMA_STATUS_ADRS_OFFSET         (0x20)
#define REG_PCIE_PCIE_VC_CRED_31_0_ADRS_OFFSET 0x0090
#define REG_PCIE_PCIE_VC_CRED_63_32_ADRS_OFFSET 0x0094
#define REG_PCIE_PCIE_SRIOV_31_0_ADRS_OFFSET 0x0120
#define REG_PCIE_PCIE_SRIOV_63_32_ADRS_OFFSET 0x0124
#define REG_PCIE_AXI_MST0_VF_APER_LO_LSBS_N3_ADRS_OFFSET 0x064
#define REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_ADRS_OFFSET 0x070
#define REG_PCIE_AXI_MST0_VF_APER_HI_LSBS_N3_ADRS_OFFSET 0x07c
#define REG_PCIE_AXI_MST0_VF_APER_HI_MSBS_N3_ADRS_OFFSET 0x0
#define REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_FLD_REMAP_DIS_LSB 27
#define REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_FLD_APER_SIZE_LSB 16
#define REG_PCIE_AXI_MST0_VF_APER_LO_MSBS_N3_FLD_ADRS_LO_LSB 0
#define REG_PCIE_X1_PF_CTRL_STATUS_ADRS_OFFSET 0x044
#define REG_PCIE_X2_PF_CTRL_STATUS_ADRS_OFFSET 0x094
#define REG_PCIE_X2_VF_MSI_TRG_N2_ADRS_OFFSET 0x054
#define REG_PCIE_ISTATUS_HOST_ADRS_OFFSET 0x018c
#define REG_PCIE_X2_CSR_INT_OCM_OFFSET 0x03f8


#ifdef CONFIG_TARGET_SCM3005
#define REG_PCIE_X1_MSGBOX_ACCESS_ADRS_OFFSET 0x0
#define REG_PCIE_X1_MSGBOX_CSR_ADRS_OFFSET 0x100
#define REG_PCIE_X1_MSGBOX_INTR_STATUS_ADRS_OFFSET 0x108
#define REG_PCIE_X1_MSGBOX_INTR_MASK_ADRS_OFFSET 0x10c
#define REG_PCIE_X1_MAILBOX_INTR_STATUS_ADRS_OFFSET 0x110
#define REG_PCIE_X1_MAILBOX_INTR_MASK_ADRS_OFFSET 0x114
#define REG_PCIE_X1_MAILBOX0_ACCESS_ADDR_OFFSET 0x0
#define REG_PCIE_X1_MAILBOX0_CSR_ADDR_OFFSET 0x10
#define REG_PCIE_X1_MAILBOX1_ACCESS_ADDR_OFFSET 0x4
#define REG_PCIE_X1_MAILBOX1_CSR_ADDR_OFFSET 0x14
#define REG_PCIE_X1_MAILBOX2_ACCESS_ADDR_OFFSET 0x8
#define REG_PCIE_X1_MAILBOX2_CSR_ADDR_OFFSET 0x18
#define REG_PCIE_X1_MAILBOX3_ACCESS_ADDR_OFFSET 0xc
#define REG_PCIE_X1_MAILBOX3_CSR_ADDR_OFFSET 0x1c
#define REG_PCIE_X2_PF_MAILBOX_INTR_MASK_ADRS_OFFSET 0x174
#define REG_PCIE_X2_PF_MAILBOX_INTR_STATUS_ADRS_OFFSET 0x170
#define REG_PCIE_X2_VF_MAILBOX_INTR_MASK_ADRS_OFFSET 0x180
#define REG_PCIE_X2_VF_MAILBOX_INTR_STATUS_ADRS_OFFSET 0x178
#define REG_PCIE_X1_IP_CTLR_OFFSET 0x118
#define REG_PCIE_X2_IP_CTRL_OFFSET 0x188
#endif

#define LINK_STATUS_LOW_POWER_ADDR 0x000003dc
#define PCIE_ATR_TRSLID_PCIE_MEMORY 0x0
#define PCIE_ATR_TRSLID_AXI4_MASTER_0 0x4
#define PCIE_ATR_TRSL_DIR BIT(22)
#define PCIE_ATR_AXI4_SLV0 0x800
#define PCIE_ATR_MAX_TABLE_NUM 8
#define PCIE_ATR_SRC_WIN_SIZE_SHIFT 1
#define PCIE_ATR_SRC_ADDR_MASK 0xfffff000
#define PCIE_ATR_TRSL_ADDR_MASK 0xfffff000
#define PCIE_ATR_SRC_ADDR_LOW 0x0
#define PCIE_ATR_SRC_ADDR_HIGH 0x4
#define PCIE_ATR_TRSL_ADDR_LOW 0x8
#define PCIE_ATR_TRSL_ADDR_HIGH 0xc
#define PCIE_ATR_TRSL_PARAM 0x10
#define PCIE_ATR_TABLE_OFFSET 0x20

#define INT_PCI_MSI_NR (1 * 32)
#define PCIE_PERST_PULLUP_EN 1 /* PCIe RESET Enabling Settings */
#define PCIE_SERIAL_MODE 1 /* Enabling the MAC and Equalization Settings */
#define PCIE_LINKUP_TIMEOUT 2000 /* Timeout for Link status check */
#define PCIE_RP_DISABLE 0x0 /* DTS value to disable RP Mode */

#define PCIE_ETHERNET_CLASS_CODE \
	0x02000000 /* PCIe Class Code for network in EP Mode */
#define PCIE_VGA_CLASS_CODE 0x03000000 /* PCIe Class Code for VGA in EP Mode */
#define PCIE_BRIDGE_CLASS_CODE 0x06040000 /* PCIe Class Code for Host Mode */
#define PCIE_USB_CLASS_CODE             0x0C03FE00 /* PCIe Class Code for USB Mode */
#define PCIE_STORAGE_CLASS_CODE         0x01800000 /* PCIe Class Code for Storage Mode */
#define PCIE_TRUSTEDS_CLASS_CODE        0x0B800000 /* PCIe Class Code for Trusted Services Mode */
#define PCIE_PERST_PULLUP_SET (0x1 << 22)
#define PCIE_PHY_GEN1 (0x1 << 12)
#define PCIE_PHY_GEN2 (0x3 << 12)
#define PCIE_PHY_GEN3 (0x7 << 12)
#define PCIE_PHY_PLL_CTRL1_SET (0x03 << 8)
#define PCIE_PHY_PLL_CTRL2_SET (0x04 << 16)
#define PCIE_PHY_PLL_VGA_SET (0x1f << 8)
#define PCIE_PHY_PLL_CTLE_SET (0x4 << 4)
#define PCIE_PHY_FREQ_SET (0xFA << 1)
#define PCIE_PHY_PLL_CTRL1_MASK 0xFFFFF000
#define PCIE_X1_PHY_PLL_CTRL1_REG 0x10B8
#define PCIE_X2_PHY_PLL_CTRL1_REG 0x20B8
#define PCIE_PHY_CTRL_CONF1_MASK 0xFFFFE0FF
#define PCIE_PHY_CTRL_CONF2_MASK 0xFFFFEF0F
#define PCIE_X1_PHY_PLL_CTRL2_REG 0x10C4
#define PCIE_X2_PHY_PLL_CTRL2_REG 0x20C4
#define PCIE_PHY_LANE_CTRL_MASK 0xFFF8FFFF
#define PCIE_PHY_LANE0_CTRL_REG 0x0024
#define PCIE_PHY_LANE1_CTRL_REG 0x1024
#define PCIE_PHY_VGA0_GAIN_REG 0x0804
#define PCIE_PHY_VGA1_GAIN_REG 0x1804
#define PCIE_X1_PHY_PLL_LOCK_REG 0x13C0
#define PCIE_X2_PHY_PLL_LOCK_REG 0x23C0
#define PCIE_PHY_SPEED_MASK 0xFFFFF0FF /* PHY speed mask */
#define PCIE_PHY_EQ_ENABLE 0x1
#define PCIE_PHY_EQ_TUNING_SET 0x1
#define PCIE_PHY_EQ_TUNING_MASK 0xFFC0FFFF
#define PCIE_PHY_PRESET_MASK 0xFFFF80FF
#define PCIE_PHY_CONF_CTRL_LTSSM 0xFFFFFFFB
#define PCIE_PHY_FREQ_MASK 0xFF800001
#define PCIE_CFG_MEM_SPACE_EN 0x2
#define PCIE_PHY_POR_RESET 0x3
#define PCIE_CFG_BUS_MASTER_EN 0x6
#define PCIE_ATR_AXI4_SLV0 0x800
#define PCIE_ATR_TRSLID_PCIE_MEMORY 0x0
#define PCIE_BAR_MASK                  0xFFFFFA
#define PCIE_BAR_64_MASK		0xFFFFFE
#define IMASK_LOCAL 0x180
#define ISTATUS_LOCAL 0x184
#define IMSI_ADDR 0x190
#define ISTATUS_MSI 0x194
#define PCIE_INT_VAL_MASK 0xffffffff
#define INT_AXI_POST_ERROR BIT(16)
#define INT_AXI_FETCH_ERROR BIT(17)
#define INT_AXI_DISCARD_ERROR BIT(18)
#define INT_PCIE_POST_ERROR BIT(20)
#define INT_PCIE_FETCH_ERROR BIT(21)
#define INT_PCIE_DISCARD_ERROR BIT(22)

#define INT_ERRORS                                                          \
	(INT_AXI_POST_ERROR | INT_AXI_FETCH_ERROR | INT_AXI_DISCARD_ERROR | \
	 INT_PCIE_POST_ERROR | INT_PCIE_FETCH_ERROR | INT_PCIE_DISCARD_ERROR)

#define INTA_OFFSET 24
#define INTA BIT(24)
#define INTB BIT(25)
#define INTC BIT(26)
#define INTD BIT(27)
#define INT_MSI BIT(28)
#define INT_INTX_MASK (INTA | INTB | INTC | INTD)
#define INT_MASK (INT_INTX_MASK | INT_MSI | INT_ERRORS)

/* Bus offset for root port enumeration */
#define PCIE_WITH_BUS_OFFSET 0x100000
/* PCIe Root Port Adrressing space size */
#define PCIE_X2_MAP 0x10000000
#define PCIE_X1_MAP 0x8000000

#define PCIE_LINK_SPEED_1 0x1 /**< PCIe link speed 1*/
#define PCIE_LINK_SPEED_2 0x2 /**< PCIe link speed 2*/
#define PCIE_LINK_SPEED_3 0x3 /**< PCIe link speed 3*/
#define PCIE_LINK_SPEED_4 0x4 /**< PCIe link speed 4*/

/* PCIe bridge internal Address translation */
#define PCIE_BAR_01_OFFSET		0xE4
#define PCIE_BAR_23_OFFSET		0xEC
#define PCIE_BAR_45_OFFSET		0xF4

#define MAX_DMA_LIST (16)
#define PCIE_INTF 0x00000000
#define AXI4_M0   0x00000004
#define AX_PCIE_DMA_VDM_TRSF_PARAM              (4)
#define EP_LINK_POLL_MS 1000

#define BIT_SET_VAL(end_bit, start_bit, dst, set_val)                                                            \
    (~(((u32)(0x1 << start_bit) - 1) ^ ((u32)(0x1 << (end_bit + 1)) - 1)) & (u32)dst) | \
        ((((u32)(0x1 << start_bit) - 1) ^ ((u32)(0x1 << (end_bit + 1)) - 1)) &               \
         ((u32)set_val << start_bit))

#define BIT_GET_VAL(end_bit, start_bit, dst) \
    ((((u32)(0x1 << start_bit) - 1) ^ ((u32)(0x1 << (end_bit + 1)) - 1)) & dst) >> start_bit

#define REG_PCIE_IP_CTRL_RP_NEP_SET(dst, val) (((u32)dst & ~0x1) | (((u32)val & 0x1)))
#define REG_PCIE_IP_CTRL_FREQ_SET(dst, val) (((u32)dst & ~0x7FFFFE) | (((u32)val << 1) & 0x7FFFFE))
#define REG_PCIE_IP_CTRL_TL_CLK_GATE_EN_SET(dst, val) (((u32)dst & ~0x800000) | (((u32)val << 23) & 0x800000))

#define REG_PCIE_RESET_CTRL_PCIE_RSTN_SET(dst, val) (((u32)dst & ~0x1) | (((u32)val & 0x1)))
#define REG_PCIE_RESET_CTRL_HOLD_PIPE_RST_SET(dst, val) (((u32)dst & ~0x2) | (((u32)val << 1) & 0x2))
#define REG_PCIE_RESET_CTRL_PCIE_RP_PERST_SET(dst, val) (((u32)dst & ~0x4) | (((u32)val << 2) & 0x4))

#define PCIE_K_SET_PCIE_PORT_TYPE_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define PCIE_K_SET_PCIE_VERSION_SET(dst, val) BIT_SET_VAL(4, 1, dst, val)
#define PCIE_K_SET_PCIE_LINKUP_CONFIG_SET(dst, val) BIT_SET_VAL(6, 6, dst, val)
#define PCIE_K_SET_LANE_REVERSAL_EN_SET(dst, val) BIT_SET_VAL(7, 7, dst, val)
#define PCIE_K_SET_LINK_WIDTH_X2_SET(dst, val) BIT_SET_VAL(8, 8, dst, val)
#define PCIE_K_SET_LINK_WIDTH_X4_SET(dst, val) BIT_SET_VAL(9, 9, dst, val)
#define PCIE_K_SET_LINK_WIDTH_X8_SET(dst, val) BIT_SET_VAL(10, 10, dst, val)
#define PCIE_K_SET_LINK_WIDTH_X16_SET(dst, val) BIT_SET_VAL(11, 11, dst, val)
#define PCIE_K_SET_LINK_SPEED_SET(dst, val) BIT_SET_VAL(15, 12, dst, val)
#define PCIE_K_SET_TX_ERR_MANAGEMENT_MODE_SET(dst, val) BIT_SET_VAL(20, 19, dst, val)
#define PCIE_K_SET_SRIS_MODE_EN_SET(dst, val) BIT_SET_VAL(21, 21, dst, val)
#define PCIE_K_SET_EXT_INTR_MODE_SET(dst, val) BIT_SET_VAL(22, 22, dst, val)


#define PCIE_PHYMAC_CFG_PER_EQ_PHASE_2_3_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define PCIE_PHYMAC_CFG_RXELECIDLE_SET(dst, val) BIT_SET_VAL(1, 1, dst, val)
#define PCIE_PHYMAC_CFG_RX_VALID_FILTER_EN_SET(dst, val) BIT_SET_VAL(2, 2, dst, val)
#define PCIE_PHYMAC_CFG_ENHANCE_EQ_SET(dst, val) BIT_SET_VAL(4, 3, dst, val)
#define PCIE_PHYMAC_CFG_TX_PRECODING_REQUEST_SET(dst, val) BIT_SET_VAL(5, 5, dst, val)
#define PCIE_PHYMAC_CFG_CLOCK_IS_COMMON_SET(dst, val) BIT_SET_VAL(6, 6, dst, val)

/* REG_PCIE_X1_PCIE_EQ_TUNING_31_0_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_EQ_TUNING_31_0_ADRS_OFFSET */
/* REG_PCIE_X1_PCIE_EQ_TUNING_63_32_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_EQ_TUNING_63_32_ADRS_OFFSET */
/* [1:0]: Specifies behavior when an error is detected during the
 * fine-tuning process. If the PHY gives direction-change
 * instructions that would lead to illegal coefficients, then the
 * module does one of the following:
 */
#define PCIE_EQ_TUNING_63_0_PRESET_TUNING_SETTINGS_SET(dst, val) \
    (u64)(((u64)dst & ~0x3) | (((u64)val << 0) & 0x3))
#define PCIE_EQ_TUNING_63_0_CONT_FINE_TUNE_EVEN_IF_NO_COEFF_SET(dst, val) \
    (u64)(((u64)dst & ~0x4) | (((u64)val << 2) & 0x4))

/* [26:16]: Preset(s) to test at 8 GT (bit0: test Preset #0;,..; bit 11: test
 * Preset #11) */
#define PCIE_EQ_TUNING_63_0_GEN3_PRESET_SET(dst, val) \
    (u64)(((u64)dst & ~0x7FF0000) | (((u64)val << 16) & 0x7FF0000))
/* [33:28]: Maximum number of fine-tuning iterations at 8GT */
#define PCIE_EQ_TUNING_63_0_GEN3_MAX_TUNING_ITER_SET(dst, val) \
    (u64)(((u64)dst & ~0x3F0000000) | (((u64)val << 28) & 0x3F0000000))
/* [46:36]: Preset(s) to test at 16 GT (bit0: test Preset #0;,.. bit 11: test
 * Preset #11) */
#define PCIE_EQ_TUNING_63_0_GEN4_PRESET_SET(dst, val) \
    (u64)(((u64)dst & ~0x7FF000000000) | (((u64)val << 36) & 0x7FF000000000))
/* [53:48]: Maximum number of fine-tuning iterations at 16GT (0 - 63)
 * (description as for bits [33:28]). */
#define PCIE_EQ_TUNING_63_0_GEN4_MAX_TUNING_ITER_SET(dst, val) \
    (u64)(((u64)dst & ~0x3F000000000000) | (((u64)val << 48) & 0x3F000000000000))


#define PCIE_GEN3_MAX_TUNING_ITER 40
#define PCIE_GEN3_TX_PRESET 0x1
#define PCIE_GEN3_RX_PRESET 0x1
#define PCIE_GEN3_PRESET_VECTOR 0x9F

// XpressRICH-AXI PCIe 5.0 Reference Manual
// Recommended Preset and RxPresetHint values are typically
// found in PHY user manuals; however, if a device has no special
// requirements, then it is recommended to use Preset #4 as the
// default value
#define PCIE_GEN4_MAX_TUNING_ITER 40
#define PCIE_GEN4_TX_PRESET 0x4
#define PCIE_GEN4_RX_PRESET 0x4
#define PCIE_GEN4_PRESET_VECTOR 0x7FF

// 0x0: last good coefficients are kept and fine-tuning iterations continue
// 0x1: last good coefficients are kept and fine-tuning iterations are stopped
// 0x2: best preset is re-applied and fine-tuning iterations continue from there
// (# of fine tuning iterations is reset) 0x3: best preset is reapplied and
// fine-tuning iterations are stopped.
enum PCIE_EQ_TUNING_SETTING {
    KEPT_LAST_GOOD_COEFF_CONT = 0,
    KEPT_LAST_GOOD_COEFF_STOP,
    BEST_PRESET_REAPPLIED_CONT,
    BEST_PRESET_REAPPLIED_STOP,
    TUNING_SETTING_INVALID
};

// Bit [4:3]: Enhanced equalization behavior:
// • 00: Equalization can be bypassed to the highest NRZ
// (non-return-to-zero) speed.
// • 01: Perform equalization at all speeds before reaching the
// highest NRZ speed.
// • 10: No equalization is needed.
// This setting is not used and must be 00 if the device does not
// support 32.0 GT/s or higher speed.
enum PCIE_PHYMAC_ENHANCED_EQ {
    EQ_BYPASS_TO_HIGHEST_NRZ = 0,
    PERFORM_EQ_ALL_SPEED_BEFORE_NRZ,
    NO_EQ_NEEDED,
    PHYMAC_ENHANCED_INVALID
};

enum PCIE_PRESET_NUM {
    PCIE_PRESET_0 = 0,
    PCIE_PRESET_1,
    PCIE_PRESET_2,
    PCIE_PRESET_3,
    PCIE_PRESET_4,
    PCIE_PRESET_5,
    PCIE_PRESET_6,
    PCIE_PRESET_7,
    PCIE_PRESET_8,
    PCIE_PRESET_9,
    PCIE_PRESET_MAX
};


/* REG_PCIE_X1_PCIE_EQ_TUNING_95_64_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_EQ_TUNING_95_64_ADRS_OFFSET */
/* [66:56]: Preset(s) to test at 32 GT (bit0: test Preset #0; - bit 10: test
 * Preset #10) */
/* [73:68]: Maximum number of fine-tuning iterations at 32GT (0 - 63)
 * (description as for bits [33:28]). */

/* REG_PCIE_X1_PCIE_EQ_PRESET8_31_0_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_EQ_PRESET8_31_0_ADRS_OFFSET*/
#define PCIE_EQ_PRESET8_15_0_TX_PRESET_SET(dst, val) (((u32)dst & ~0xF) | ((u32)val & 0xF))
#define PCIE_EQ_PRESET8_15_0_RX_PRESET_SET(dst, val) (((u32)dst & ~0x70) | (((u32)val << 4) & 0x70))

/* REG_PCIE_X1_PCIE_EQ_PRESET16_31_0_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_EQ_PRESET16_31_0_ADRS_OFFSET*/
#define PCIE_EQ_PRESET16_7_0_TX_PRESET_SET(dst, val) (((u32)dst & ~0xF) | ((u32)val & 0xF))
#define PCIE_EQ_PRESET16_7_0_RX_PRESET_SET(dst, val) (((u32)dst & ~0xF0) | (((u32)val << 4) & 0xF0))


// 1 lane need 16-bit config
// 2 lane need 2*8 = 32-bit config ==> 32-bit is the size of a singe register
// access
#define PCIE_GEN3_RX_TX_PRESET_GET(g3_rx_pre, g3_tx_pre) \
    (u32)(((g3_rx_pre << 4) | g3_tx_pre) | (((g3_rx_pre << 4) | g3_tx_pre) << 16))
#define PCIE_GEN3_RX_TX_PRESET PCIE_GEN3_RX_TX_PRESET_GET(PCIE_GEN3_RX_PRESET, PCIE_GEN3_TX_PRESET)

// 1 lane need 8-bit config
// 4 lane need 4*8 = 32-bit config ==> 32-bit is the size of a singe register
// access
#define PCIE_GEN4_RX_TX_PRESET_GET(g4_rx_pre, g4_tx_pre)                                \
    (u32)(((g4_rx_pre << 4) | g4_tx_pre) | (((g4_rx_pre << 4) | g4_tx_pre) << 8) | \
               (((g4_rx_pre << 4) | g4_tx_pre) << 16) | (((g4_rx_pre << 4) | g4_tx_pre) << 24))
#define PCIE_GEN4_RX_TX_PRESET PCIE_GEN4_RX_TX_PRESET_GET(PCIE_GEN4_RX_PRESET, PCIE_GEN4_TX_PRESET)

struct axiado_pcie {
	struct udevice *dev;
	int irq;
	int mailbox_irq;
	int msgbox_irq;
	int scm_version;

	/* Device Class */
	bool is_root_port; /* If set, enables Root Port mode */
	bool is_vga; /* If set, initialises as a VGA device */
	bool is_eth; /* If set, initialises as a Network device */
	bool is_usb; /* If set, initialises as a USB device */
	bool is_storage; /* If set, initialises as a Storage device */
	bool is_trusts; /* If set, initialises as a Trust Services device */

	bool pcie_x1; /* Used for executing PCIe X1 specific task */
	bool pcie_x2; /* Used for executing PCIe X2 specific task */

	/* Memory Regions to be mapped */
	void __iomem *csr; /* CSR memory region */
	void __iomem *phy; /* PHY memory region */
	void __iomem *cfg; /* PCIe 4K config space */
	void __iomem *bridge; /* INT bridge registers */
	void __iomem *mbx; /* mailbox memory region */
	void __iomem *ioctl; /* IOCTL memory region */
	void __iomem *ext; /* EXT memory region */
	void __iomem *base; /* RP base address */
	u64 phys_bar2; /* BAR2 region */
	u32 phys_bar2_size; /* BAR2 size */

	DECLARE_BITMAP(msi_irq_in_use, INT_PCI_MSI_NR);
	struct mutex map_lock;
	struct mutex lock;

	struct list_head ports;
	unsigned int lanes; /* Number of lanes */
	unsigned int bar;
	unsigned int bar0;
	unsigned int bar1;
	unsigned int bar2;
	unsigned int bar3;
	unsigned int bar4;
	unsigned int bar5;
	unsigned int reconfig;
	unsigned int bar0_addr;
	unsigned int bar1_addr;
	unsigned int bar2_addr;
	unsigned int bar3_addr;
	unsigned int bar4_addr;
	unsigned int bar5_addr;
	int atr_table_num;

	/* Endpoint BAR config parsed from DTS ranges */
	u64 ep_bar0_addr;
	u64 ep_bar0_size;
	u32 ep_bar0_flags;
	u64 ep_bar1_addr;
	u64 ep_bar1_size;
	u32 ep_bar1_flags;

	/* EP link monitor */
	bool ep_link_up;
	u64 ep_bar0_pci;
	u64 ep_bar2_pci;

	void *sg_buffer_desc_src[MAX_DMA_LIST];
	dma_addr_t sg_buffer_dma_desc_src[MAX_DMA_LIST];
	void *sg_buffer_desc_dest[MAX_DMA_LIST];
	dma_addr_t sg_buffer_dma_desc_dest[MAX_DMA_LIST];
};

struct axiado_pcie_port {
	struct axiado_pcie *pcie;
	ofnode np;
    struct fdt_resource regs;
	void __iomem *base;

	unsigned int index;

	struct list_head list;
};

/* Common Function Declaration */
inline u32 axiado_pcie_ioread(void __iomem *base, u32 offset);
inline void axiado_pcie_iowrite(void __iomem *base, u32 offset, u32 val);
int axiado_pcie_pll_wait(struct axiado_pcie *pcie, unsigned long timeout_ms);
int axiado_pcie_port_check_link(struct axiado_pcie_port *port, unsigned long timeout_ms);

struct axiado_pcie;
// struct udevice;

#define MAX_DMA_LIST (16)
#define MAX_PF (1)
#define MAX_VF (64)
#define MAX_PF_MBX (4)
#define MAX_MBX (64)
#define MAX_PORT 2

/* PCIE port number */
enum axiado_pcie_port_num { PCIE_X1, PCIE_X2 };

/* PCIE BAR numbers */
enum axiado_pcie_bars { BAR_01 = 0, BAR_23 = 2, BAR_45 = 4 };

struct axiado_pcie_func_info {
	enum axiado_pcie_port_num port;
	u8 pf_num;
	u8 vf_num;
};

/* PCIE DMA Channel numbers */
enum axiado_pcie_dma_channel {
	DMA_CHANNEL_0,
	DMA_CHANNEL_1,
	DMA_CHANNEL_2,
	DMA_CHANNEL_3,
	DMA_CHANNEL_4,
	DMA_CHANNEL_5,
	DMA_CHANNEL_6,
	DMA_CHANNEL_7
};

/* PCIE DMA modes */
enum axiado_pcie_dma_mode {
	DIRECT_MODE,
	SCATTER_GATHER_00,
	SCATTER_GATHER_01,
	SCATTER_GATHER_10,
	SCATTER_GATHER_11,
};

enum ax_pcie_dma_dir {
	AX_EP_MEMORY_HOST_TO_PDEV, /** On EP, host memory read, host -> phy.func. */
	AX_EP_MEMORY_PDEV_TO_HOST, /** On EP, host memory write, phy.func. -> host */
	AX_RC_VDM_HOST_TO_PDEV, /** On RC, VDM transmit, host -> phy.func. */
	AX_EP_VDM_PDEV_TO_HOST, /** On EP, VDM transmit, phy.func. -> host */
};

/* PCIE DMA request structure */
struct axiado_pcie_dma_request {
	enum axiado_pcie_dma_channel chan_num;
	enum axiado_pcie_dma_mode dma_mode;
	enum ax_pcie_dma_dir dma_dir;
	u8 source_list_size;
	u8 dest_list_size;
	u32 dma_size;
	u64 dma_source_ptr[MAX_DMA_LIST];
	u64 dma_dest_ptr[MAX_DMA_LIST];
	u64 dma_source_size[MAX_DMA_LIST];
	u64 dma_dest_size[MAX_DMA_LIST];
};

struct dma_cfg {
	u32 src_inf;
	u32 dest_inf;
	u32 src_addr_31_0;
	u32 src_addr_63_32;
	u32 dest_addr_31_0;
	u32 dest_addr_63_32;
	u32 length;
	u32 se_cond; // start and end conditions
	u32 irq; // interrupt enable
};

u64 axiado_pcie_get_bar(struct udevice *dev,
			struct axiado_pcie_func_info func_info,
			enum axiado_pcie_bars bar_num, u32 *bar_size);
int axiado_pcie_is_dma_busy(struct udevice *dev,
			    enum axiado_pcie_port_num port,
			    enum axiado_pcie_dma_channel chan_num);
int axiado_pcie_dma_xfer(struct udevice *dev,
			 enum axiado_pcie_port_num port,
			 struct axiado_pcie_dma_request *dma_request);
int axiado_pcie_trigger_msi(struct udevice *dev,
			    struct axiado_pcie_func_info func_info,
			    u8 vector_no);
unsigned int axiado_pcie_msgbox_read(struct udevice *dev, u32 offset);
void axiado_pcie_msgbox_write(struct udevice *dev, u32 offset,
				      u32 val);
unsigned int axiado_pcie_ocm_csr_read(struct udevice *dev);
void axiado_pcie_ocm_csr_write(struct udevice *dev, u32 val);

#endif /** __PCIE_AXIADO_H__ */

