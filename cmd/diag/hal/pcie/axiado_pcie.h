// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#ifndef __AXIADO_PCIE_H
#define __AXIADO_PCIE_H

#define AX_PCIE_TL_CLOCK_FREQ_MHZ  500
#define IS_LANE_REVERSAL_EN 1
#define IS_COMMON_SLOT_CLOCK_SET 1

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
#define PCIE_PHY_POR_RESET 0x3
#define PCIE_ATR_PCIE_WIN0 0x600
#define PCIE_ATR_AXI4_SLV0 0x800
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

#define REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET 0x15c
#define REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET 0x1cc

/* REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET / REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET*/
#define REG_PCIE_RESET_CTRL_PCIE_RSTN_GET(val) ((uint32_t)val & 0x1)
#define REG_PCIE_RESET_CTRL_HOLD_PIPE_RST_GET(val) (((uint32_t)val & 0x2) >> 1)
#define REG_PCIE_RESET_CTRL_PCIE_RP_PERST_GET(val) (((uint32_t)val & 0x4) >> 2)

#define REG_PCIE_RESET_CTRL_PCIE_RSTN_SET(dst, val) (((uint32_t)dst & ~0x1) | (((uint32_t)val & 0x1)))
#define REG_PCIE_RESET_CTRL_HOLD_PIPE_RST_SET(dst, val) (((uint32_t)dst & ~0x2) | (((uint32_t)val << 1) & 0x2))
#define REG_PCIE_RESET_CTRL_PCIE_RP_PERST_SET(dst, val) (((uint32_t)dst & ~0x4) | (((uint32_t)val << 2) & 0x4))

#define PCIE_TYPE1_TIMER_SUB_SEC_PRI_OFF 0x18
#define PCIE_TYPE1_PRI_BUS_SET(dst, val) (((uint32_t)dst & ~0xFF) | ((uint32_t)val & 0xFF))
#define PCIE_TYPE1_SEC_BUS_SET(dst, val) (((uint32_t)dst & ~0xFF00) | (((uint32_t)val << 8) & 0xFF00))
#define PCIE_TYPE1_SUB_BUS_SET(dst, val) (((uint32_t)dst & ~0xFF0000) | (((uint32_t)val << 16) & 0xFF0000))

#define PCIE_X1_PLL_LOCK_OFFSET 0x13c0 /**< PCIe PLL lock offset*/
#define PCIE_X2_PLL_LOCK_OFFSET 0x23c0 /**< PCIe PLL lock offset*/

#define REG_PCIE_X1_IP_CTRL_ADRS_OFFSET 0x118
#define REG_PCIE_X2_IP_CTRL_ADRS_OFFSET 0x188

#define REG_PCIE_GEN_SETTINGS_ADRS_OFFSET 0x0000 + 0x0080
#define REG_PCIE_PEX_DEV_ADRS_OFFSET 0x0000 + 0x00C0
#define REG_PCIE_PEX_SPC_ADRS_OFFSET 0x0000 + 0x00D4
#define REG_PCIE_IMASK_LOCAL_ADRS_OFFSET 0x0180 + 0x0000
#define REG_PCIE_PHYMAC_CFG_ADRS_OFFSET (0x0300 + 0x003C)
#define REG_PCIE_EQ_TUNING_31_0_ADRS_OFFSET (0x0300 + 0x005C)
#define REG_PCIE_EQ_TUNING_63_32_ADRS_OFFSET (0x0300 + 0x0060)
#define REG_PCIE_EQ_PRESET_8G_31_0_ADRS_OFFSET (0x0000 + 0x0100)
#define REG_PCIE_EQ_PRESET_16G_31_0_ADRS_OFFSET (0x0000 + 0x0150)
#define REG_PCIE_PCI_IDS_63_32_ADRS_OFFSET (0x0000 + 0x009C)

#define PCIE_PHY_CONF_CTRL_LTSSM 0xFFFFFFFB
#define REG_PCIE_CFGCTRL_ADRS_OFFSET 0x0084

#define REG_PCIE_BASIC_STATUS_ADRS_OFFSET 0x0000 + 0x0018
#define LINK_STATUS_LOW_POWER_ADDR 0x000003dc
#define PCIE_ATR_TRSLID_PCIE_MEMORY 0x0
#define PCIE_ATR_TRSLID_PCIE_CFG 0x1
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
#define PCIE_ATR_TRSL_MASK_OFFSET 0x18
#define PCIE_ATR_TABLE_OFFSET 0x20
#define ATR_TRSL_MASK_SET(dst, val) (((uint32_t)dst & ~0xFFFFFFFF) | ((uint32_t)val & 0xFFFFFFFF))

#define REG_PCIE_COMMAND_STATUS_ADRS_OFFSET (0x0000 + 0x4)
/* -------------------------------------------------- */
/* ------------ Axiado PCIE External Reg ------------ */
/* -------------------------------------------------- */

/* REG_PCIE_X1_IP_CTRL_ADRS_OFFSET / REG_PCIE_X2_IP_CTRL_ADRS_OFFSET */
#define REG_PCIE_IP_CTRL_RP_NEP_GET(val) ((uint32_t)val & 0x1)
#define REG_PCIE_IP_CTRL_FREQ_GET(val) (((uint32_t)val & 0x7FFFFE) >> 1)
#define REG_PCIE_IP_CTRL_TL_CLK_GATE_EN_GET(val) (((uint32_t)val & 0x800000) >> 23)

#define REG_PCIE_IP_CTRL_RP_NEP_SET(dst, val) (((uint32_t)dst & ~0x1) | (((uint32_t)val & 0x1)))
#define REG_PCIE_IP_CTRL_FREQ_SET(dst, val) (((uint32_t)dst & ~0x7FFFFE) | (((uint32_t)val << 1) & 0x7FFFFE))
#define REG_PCIE_IP_CTRL_TL_CLK_GATE_EN_SET(dst, val) (((uint32_t)dst & ~0x800000) | (((uint32_t)val << 23) & 0x800000))

/* REG_PCIE_X2_IATU_PF0_BAR0_CONFIG_ADRS_OFFSET */
#define EXT_PCIE_X2_IATU_PF_CONFIG_ENABLE_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_PCIE_X2_IATU_PF_CONFIG_BASE_ADDR_SIZE_SET(dst, val) BIT_SET_VAL(5, 1, dst, val)

/* REG_PCIE_X2_IATU_PF0_BAR0_BASE_ADRS_OFFSET */
#define EXT_PCIE_X2_IATU_PF_BASE_ADDR_SET(dst, val) BIT_SET_VAL(27, 0, dst, val)

/* REG_PCIE_X2_IATU_PF0_VF_BAR0_CONFIG_ADRS_OFFSET */
#define EXT_PCIE_X2_IATU_PF_VF_CONFIG_ENABLE_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_PCIE_X2_IATU_PF_VF_CONFIG_BASE_ADDR_SIZE_SET(dst, val) BIT_SET_VAL(5, 1, dst, val)
#define EXT_PCIE_X2_IATU_PF_VF_CONFIG_GRANULARITY_SET(dst, val) BIT_SET_VAL(10, 6, dst, val)

/* REG_PCIE_X2_OFM_PF0_CONFIG_ADRS_OFFSET */
#define EXT_PCIE_X2_OFM_PF_CONFIG_ENABLE_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_PCIE_X2_OFM_PF_CONFIG_BASE_ADRS_SIZE_SET(dst, val) BIT_SET_VAL(5, 1, dst, val)
#define EXT_PCIE_X2_OFM_PF_CONFIG_PF_ID_SET(dst, val) BIT_SET_VAL(8, 6, dst, val)
#define EXT_PCIE_X2_OFM_PF_CONFIG_PCIE_X2_SPACE_SET(dst, val) BIT_SET_VAL(31, 12, dst, val)

/* REG_PCIE_X2_OFM_PF0_VF_CONFIG_ADRS_OFFSET */
#define EXT_PCIE_X2_OFM_VF_CONFIG_ENABLE_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_PCIE_X2_OFM_VF_CONFIG_BASE_ADRS_SIZE_SET(dst, val) BIT_SET_VAL(5, 1, dst, val)
#define EXT_PCIE_X2_OFM_VF_CONFIG_GRAN_SET(dst, val) BIT_SET_VAL(10, 6, dst, val)
#define EXT_PCIE_X2_OFM_VF_CONFIG_PCIE_X2_SPACE_SET(dst, val) BIT_SET_VAL(31, 12, dst, val)

/* REG_PCIE_X2_IATU_PF0_ROM_EXP_CONFIG_ADRS_OFFSET */
#define EXT_PCIE_X2_ROM_EXP_CONFIG_ENABLE_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_PCIE_X2_ROM_EXP_CONFIG_BASE_ADRS_SIZE_SET(dst, val) BIT_SET_VAL(5, 1, dst, val)

/* REG_PCIE_X2_IATU_PF0_ROM_EXP_BASE_ADRS_OFFSET */
#define EXT_PCIE_X2_ROM_EXP_BASE_SET(dst, val) BIT_SET_VAL(27, 0, dst, val)

/* REG_PCIE_X1_KUP_KEYS_ACCESS_ADRS_OFFSET / REG_PCIE_X2_KUP_KEYS_ACCESS_ADRS_OFFSET */

/* REG_PCIE_X1_KUP_CTRL_STS_ADRS_OFFSET / REG_PCIE_X2_KUP_CTRL_STS_ADRS_OFFSET */
#define EXT_KUP_CTRL_STS_START_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_KUP_CTRL_STS_KEY_SEL_SET(dst, val) BIT_SET_VAL(1, 1, dst, val)
#define EXT_KUP_CTRL_STS_TX_RX_SET(dst, val) BIT_SET_VAL(2, 2, dst, val)
#define EXT_KUP_CTRL_STS_LINK_SEL_SET(dst, val) BIT_SET_VAL(3, 3, dst, val)
#define EXT_KUP_CTRL_STS_LOCK_SET(dst, val) BIT_SET_VAL(5, 4, dst, val)
#define EXT_KUP_CTRL_STS_LOCK_ERR_SET(dst, val) BIT_SET_VAL(6, 6, dst, val)
#define EXT_KUP_CTRL_STS_KEY_MODE_SET(dst, val) BIT_SET_VAL(7, 7, dst, val)

/* REG_PCIE_X1_DOE_INFO_ADRS_OFFSET / REG_PCIE_X2_DOE_INFO_ADRS_OFFSET */
#define EXT_DOE_INFO_BUSY_GET(val) BIT_GET_VAL(0, 0, val)
#define EXT_DOE_INFO_READY_GET(val) BIT_GET_VAL(1, 1, val)
#define EXT_DOE_INFO_ERROR_GET(val) BIT_GET_VAL(2, 2, val)
#define EXT_DOE_INFO_ICM_LEVEL_GET(val) BIT_GET_VAL(10, 4, val)
#define EXT_DOE_INFO_OGM_LEVEL_GET(val) BIT_GET_VAL(17, 11, val)
#define EXT_DOE_INFO_CUR_READY_STS_GET(val) BIT_GET_VAL(18, 18, val)
#define EXT_DOE_INFO_TL_CTRL12_GET(val) BIT_GET_VAL(20, 19, val)
#define EXT_DOE_INFO_ICM_FLUSH_GET(val) BIT_GET_VAL(21, 21, val)
#define EXT_DOE_INFO_OGM_FLUSH_GET(val) BIT_GET_VAL(22, 22, val)
#define EXT_DOE_INFO_OGM_EOP_GET(val) BIT_GET_VAL(23, 23, val)

#define EXT_DOE_INFO_BUSY_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_DOE_INFO_READY_SET(dst, val) BIT_SET_VAL(1, 1, dst, val)
#define EXT_DOE_INFO_ERROR_SET(dst, val) BIT_SET_VAL(2, 2, dst, val)
#define EXT_DOE_INFO_ICM_FLUSH_SET(dst, val) BIT_SET_VAL(21, 21, dst, val)
#define EXT_DOE_INFO_OGM_FLUSH_SET(dst, val) BIT_SET_VAL(22, 22, dst, val)
#define EXT_DOE_INFO_OGM_EOP_SET(dst, val) BIT_SET_VAL(23, 23, dst, val)

/* REG_PCIE_X1_DOE_INTR_STATUS_ADRS_OFFSET / REG_PCIE_X2_DOE_INTR_STATUS_ADRS_OFFSET */
#define EXT_DOE_INTR_STATUS_GET(val) BIT_GET_VAL(7, 0, val)
#define EXT_DOE_INTR_STATUS_SET(dst, val) BIT_SET_VAL(7, 0, dst, val)

/* REG_PCIE_X1_DOE_INTR_MASK_ADRS_OFFSET / REG_PCIE_X2_DOE_INTR_MASK_ADRS_OFFSET */
#define EXT_DOE_INTR_MASK_GET(val) BIT_GET_VAL(7, 0, val)
#define EXT_DOE_INTR_MASK_SET(dst, val) BIT_SET_VAL(7, 0, dst, val)

/* REG_PCIE_X1_DOE_ICM_FIFO_THRES_ADRS_OFFSET / REG_PCIE_X2_DOE_ICM_FIFO_THRES_ADRS_OFFSET */
#define EXT_DOE_ICM_FIFO_THRESS_GET(val) BIT_GET_VAL(6, 0, val)
#define EXT_DOE_ICM_FIFO_THRESS_SET(dst, val) BIT_SET_VAL(6, 0, dst, val)

/* REG_PCIE_X1_DOE_OGM_FIFO_THRES_ADRS_OFFSET / REG_PCIE_X2_DOE_OGM_FIFO_THRES_ADRS_OFFSET */
#define EXT_DOE_OGM_FIFO_THRESS_GET(val) BIT_GET_VAL(6, 0, val)
#define EXT_DOE_OGM_FIFO_THRESS_SET(dst, val) BIT_SET_VAL(6, 0, dst, val)

/* REG_PCIE_X2_WR_MON_CTRL_STS_ADRS_OFFSET */
#define EXT_WR_MON_CTRL_ENABLE_GET(val) BIT_GET_VAL(0, 0, val)
#define EXT_WR_MON_CTRL_MASK_GET(val) BIT_GET_VAL(15, 8, val)
#define EXT_WR_MON_CTRL_STS_GET(val) BIT_GET_VAL(23, 16, val)
#define EXT_WR_MON_CTRL_STS_MP_MASK_GET(val) BIT_GET_VAL(24, 24, val)
#define EXT_WR_MON_CTRL_STS_MP_GET(val) BIT_GET_VAL(25, 25, val)
#define EXT_WR_MON_CTRL_STS_MV_MASK_GET(val) BIT_GET_VAL(26, 26, val)
#define EXT_WR_MON_CTRL_STS_MV_GET(val) BIT_GET_VAL(27, 27, val)

#define EXT_WR_MON_CTRL_ENABLE_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_WR_MON_CTRL_MASK_SET(dst, val) BIT_SET_VAL(15, 8, dst, val)
#define EXT_WR_MON_CTRL_STS_SET(dst, val) BIT_SET_VAL(23, 16, dst, val)
#define EXT_WR_MON_CTRL_STS_MP_MASK_SET(dst, val) BIT_SET_VAL(24, 24, dst, val)
#define EXT_WR_MON_CTRL_STS_MP_SET(dst, val) BIT_SET_VAL(25, 25, dst, val)
#define EXT_WR_MON_CTRL_STS_MV_MASK_SET(dst, val) BIT_SET_VAL(26, 26, dst, val)
#define EXT_WR_MON_CTRL_STS_MV_SET(dst, val) BIT_SET_VAL(27, 27, dst, val)

/* REG_PCIE_X2_WR_MON0_ADRS_MSBS_ADRS_OFFSET */
#define EXT_WR_MON_ADRS_MSBS_LOW_ADRS_GET(val) BIT_GET_VAL(7, 0, val)
#define EXT_WR_MON_ADRS_MSBS_HIGH_ADRS_GET(val) BIT_GET_VAL(15, 8, val)

#define EXT_WR_MON_ADRS_MSBS_LOW_ADRS_SET(dst, val) BIT_SET_VAL(7, 0, dst, val)
#define EXT_WR_MON_ADRS_MSBS_HIGH_ADRS_SET(dst, val) BIT_SET_VAL(15, 8, dst, val)

/* REG_PCIE_X1_MSG_ATU_CTRL_STATUS_ADRS_OFFSET / REG_PCIE_X2_MSG_ATU_CTRL_STATUS_ADRS_OFFSET */
#define EXT_MSG_ATU_CTRL_STATUS_MEM_EN_GET(val) BIT_GET_VAL(0, 0, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_STOP_GET(val) BIT_GET_VAL(1, 1, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_RESET_GET(val) BIT_GET_VAL(2, 2, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_THRES_GET(val) BIT_GET_VAL(3, 3, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_HW_SW_OFF_GET(val) BIT_GET_VAL(4, 4, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_REACH_END_GET(val) BIT_GET_VAL(5, 5, val)

#define EXT_MSG_ATU_CTRL_STATUS_MEM_EN_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_STOP_SET(dst, val) BIT_SET_VAL(1, 1, dst, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_RESET_SET(dst, val) BIT_SET_VAL(2, 2, dst, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_THRES_SET(dst, val) BIT_SET_VAL(3, 3, dst, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_HW_SW_OFF_SET(dst, val) BIT_SET_VAL(4, 4, dst, val)
#define EXT_MSG_ATU_CTRL_STATUS_MEM_REACH_END_SET(dst, val) BIT_SET_VAL(5, 5, dst, val)

/* REG_PCIE_X1_MSG_INTR_STS_ADRS_OFFSET / REG_PCIE_X2_MSG_INTR_STS_ADRS_OFFSET */
// 1–interrupt 0–no interrupt
#define EXT_MSG_INTR_STS_SOP_GET(val) BIT_GET_VAL(0, 0, val)
#define EXT_MSG_INTR_STS_THRES_GET(val) BIT_GET_VAL(1, 1, val)
#define EXT_MSG_INTR_STS_END_ADDR_GET(val) BIT_GET_VAL(2, 2, val)

/* REG_PCIE_X1_MSG_INTR_MASK_ADRS_OFFSET / REG_PCIE_X2_MSG_INTR_MASK_ADRS_OFFSET */
#define EXT_MSG_INTR_MASK_SOP_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define EXT_MSG_INTR_MASK_THRES_SET(dst, val) BIT_SET_VAL(1, 1, dst, val)
#define EXT_MSG_INTR_MASK_END_ADDR_SET(dst, val) BIT_SET_VAL(2, 2, dst, val)

/* -------------------------------------------------- */
/* ------------ PCIE XpressRich Internal ------------ */
/* -------------------------------------------------- */
/* REG_PCIE_X2_IATU_PF0_VF_BAR0_BASE_ADRS_OFFSET */
#define EXT_PCIE_X2_IATU_PF_VF_BASE_ADDR_SET(dst, val) BIT_SET_VAL(27, 0, dst, val)

/* REG_PCIE_X1_SET_SETTINGS_ADRS_OFFSET / REG_PCIE_X2_SET_SETTINGS_ADRS_OFFSET
 */
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

/* REG_PCIE_X1_RESET_CTRL_ADRS_OFFSET / REG_PCIE_X2_RESET_CTRL_ADRS_OFFSET*/
#define REG_PCIE_RESET_CTRL_PCIE_RSTN_GET(val) ((uint32_t)val & 0x1)
#define REG_PCIE_RESET_CTRL_HOLD_PIPE_RST_GET(val) (((uint32_t)val & 0x2) >> 1)
#define REG_PCIE_RESET_CTRL_PCIE_RP_PERST_GET(val) (((uint32_t)val & 0x4) >> 2)

#define REG_PCIE_RESET_CTRL_PCIE_RSTN_SET(dst, val) (((uint32_t)dst & ~0x1) | (((uint32_t)val & 0x1)))
#define REG_PCIE_RESET_CTRL_HOLD_PIPE_RST_SET(dst, val) (((uint32_t)dst & ~0x2) | (((uint32_t)val << 1) & 0x2))
#define REG_PCIE_RESET_CTRL_PCIE_RP_PERST_SET(dst, val) (((uint32_t)dst & ~0x4) | (((uint32_t)val << 2) & 0x4))

/* REG_PCIE_X1_PCIE_CFGCTRL_ADRS_OFFSET / REG_PCIE_X2_PCIE_CFGCTRL_ADRS_OFFSET
 */
#define PCIE_CFGCTRL_PHY_FUNC_CFG_NOT_RDY_GET(val) ((uint32_t)val & 0x1)
#define PCIE_CFGCTRL_VIR_FUNC_CFG_NOT_RDY_GET(val) (((uint32_t)val & 0x2) >> 1)
#define PCIE_CFGCTRL_DISABLE_LTSSM_GET(val) (((uint32_t)val & 0x4) >> 2)
#define PCIE_CFGCTRL_MARGIN_RDY_GET(val) (((uint32_t)val & 0x8) >> 3)
#define PCIE_CFGCTRL_MARGIN_SW_RDY_GET(val) (((uint32_t)val & 0x10) >> 4)

#define PCIE_CFGCTRL_PHY_FUNC_CFG_NOT_RDY_SET(dst, val) (((uint32_t)dst & ~0x1) | (((uint32_t)val & 0x1)))
#define PCIE_CFGCTRL_VIR_FUNC_CFG_NOT_RDY_SET(dst, val) (((uint32_t)dst & ~0x2) | (((uint32_t)val << 1) & 0x2))
#define PCIE_CFGCTRL_DISABLE_LTSSM_SET(dst, val) (((uint32_t)dst & ~0x4) | (((uint32_t)val << 2) & 0x4))
#define PCIE_CFGCTRL_MARGIN_RDY_SET(dst, val) (((uint32_t)dst & ~0x8) | (((uint32_t)val << 3) & 0x8))
#define PCIE_CFGCTRL_MARGIN_SW_RDY_SET(dst, val) (((uint32_t)dst & ~0x10) | (((uint32_t)val << 4) & 0x10))

#define PCIE_CFGCTRL_PRI_DEV_SET(dst, val) BIT_SET_VAL(23, 19, dst, val)
#define PCIE_CFGCTRL_PRI_BUS_SET(dst, val) BIT_SET_VAL(31, 24, dst, val)

/* REG_PCIE_X1_PHYMAC_CFG_ADRS_OFFSET / REG_PCIE_X2_PHYMAC_CFG_ADRS_OFFSET */
#define PCIE_PHYMAC_CFG_PER_EQ_PHASE_2_3_GET(val) BIT_GET_VAL(0, 0, val)

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
    (uint64_t)(((uint64_t)dst & ~0x3) | (((uint64_t)val << 0) & 0x3))
#define PCIE_EQ_TUNING_63_0_CONT_FINE_TUNE_EVEN_IF_NO_COEFF_SET(dst, val) \
    (uint64_t)(((uint64_t)dst & ~0x4) | (((uint64_t)val << 2) & 0x4))

/* [26:16]: Preset(s) to test at 8 GT (bit0: test Preset #0;,..; bit 11: test
 * Preset #11) */
#define PCIE_EQ_TUNING_63_0_GEN3_PRESET_SET(dst, val) \
    (uint64_t)(((uint64_t)dst & ~0x7FF0000) | (((uint64_t)val << 16) & 0x7FF0000))
/* [33:28]: Maximum number of fine-tuning iterations at 8GT */
#define PCIE_EQ_TUNING_63_0_GEN3_MAX_TUNING_ITER_SET(dst, val) \
    (uint64_t)(((uint64_t)dst & ~0x3F0000000) | (((uint64_t)val << 28) & 0x3F0000000))
/* [46:36]: Preset(s) to test at 16 GT (bit0: test Preset #0;,.. bit 11: test
 * Preset #11) */
#define PCIE_EQ_TUNING_63_0_GEN4_PRESET_SET(dst, val) \
    (uint64_t)(((uint64_t)dst & ~0x7FF000000000) | (((uint64_t)val << 36) & 0x7FF000000000))
/* [53:48]: Maximum number of fine-tuning iterations at 16GT (0 - 63)
 * (description as for bits [33:28]). */
#define PCIE_EQ_TUNING_63_0_GEN4_MAX_TUNING_ITER_SET(dst, val) \
    (uint64_t)(((uint64_t)dst & ~0x3F000000000000) | (((uint64_t)val << 48) & 0x3F000000000000))

/* REG_PCIE_X1_PCIE_EQ_TUNING_95_64_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_EQ_TUNING_95_64_ADRS_OFFSET */
/* [66:56]: Preset(s) to test at 32 GT (bit0: test Preset #0; - bit 10: test
 * Preset #10) */
/* [73:68]: Maximum number of fine-tuning iterations at 32GT (0 - 63)
 * (description as for bits [33:28]). */

/* REG_PCIE_X1_PCIE_EQ_PRESET8_31_0_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_EQ_PRESET8_31_0_ADRS_OFFSET*/
#define PCIE_EQ_PRESET8_15_0_TX_PRESET_SET(dst, val) (((uint32_t)dst & ~0xF) | ((uint32_t)val & 0xF))
#define PCIE_EQ_PRESET8_15_0_RX_PRESET_SET(dst, val) (((uint32_t)dst & ~0x70) | (((uint32_t)val << 4) & 0x70))

/* REG_PCIE_X1_PCIE_EQ_PRESET16_31_0_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_EQ_PRESET16_31_0_ADRS_OFFSET*/
#define PCIE_EQ_PRESET16_7_0_TX_PRESET_SET(dst, val) (((uint32_t)dst & ~0xF) | ((uint32_t)val & 0xF))
#define PCIE_EQ_PRESET16_7_0_RX_PRESET_SET(dst, val) (((uint32_t)dst & ~0xF0) | (((uint32_t)val << 4) & 0xF0))

/* REG_PCIE_X1_PHY_BIST_CTRL_ADRS_OFFSET */
#define PHY_BIST_X1_CTRL_LANE0_BIST_MODE_EN_SET(dst, val) (((uint32_t)dst & ~0x1) | ((uint32_t)val & 0x1))
#define PHY_BIST_X1_CTRL_TRIGGER_LANE0_SEQ_SET(dst, val) (((uint32_t)dst & ~0x2) | (((uint32_t)val << 1) & 0x2))

/* REG_PCIE_X2_PHY_BIST_CTRL_ADRS_OFFSET*/
#define PHY_BIST_X2_CTRL_LANE0_BIST_MODE_EN_SET(dst, val) (((uint32_t)dst & ~0x1) | ((uint32_t)val & 0x1))
#define PHY_BIST_X2_CTRL_LANE1_BIST_MODE_EN_SET(dst, val) (((uint32_t)dst & ~0x2) | (((uint32_t)val << 1) & 0x2))
#define PHY_BIST_X2_CTRL_TRIGGER_LANE0_SEQ_SET(dst, val) (((uint32_t)dst & ~0x4) | (((uint32_t)val << 2) & 0x4))
#define PHY_BIST_X2_CTRL_TRIGGER_LANE1_SEQ_SET(dst, val) (((uint32_t)dst & ~0x8) | (((uint32_t)val << 3) & 0x8))

/* REG_PCIE_X1_PHY_BIST_STS_ADRS_OFFSET */
#define PHY_BIST_X1_STATUS_LANE0_BIST_DONE_GET(val) ((uint32_t)val & 0x1)
#define PHY_BIST_X1_STATUS_LANE0_BIST_PASS_GET(val) (((uint32_t)val & 0x2) >> 1)

/* REG_PCIE_X2_PHY_BIST_STS_ADRS_OFFSET */
#define PHY_BIST_X2_STATUS_LANE0_BIST_DONE_GET(val) ((uint32_t)val & 0x1)
#define PHY_BIST_X2_STATUS_LANE1_BIST_DONE_GET(val) (((uint32_t)val & 0x2) >> 1)
#define PHY_BIST_X2_STATUS_LANE0_BIST_PASS_GET(val) (((uint32_t)val & 0x4) >> 2)
#define PHY_BIST_X2_STATUS_LANE1_BIST_PASS_GET(val) (((uint32_t)val & 0x8) >> 3)

/* REG_PCIE_X1_PCIE_PEX_DEV_ADRS_OFFSET / REG_PCIE_X2_PCIE_PEX_DEV_ADRS_OFFSET*/
#define PCIE_PEX_DEV_MAX_PAYLOAD_SET(dst, val) (((uint32_t)dst & ~0x7) | ((uint32_t)val & 0x7))

/* REG_PCIE_X1_PCIE_PEX_SPC_ADRS_OFFSET / REG_PCIE_X2_PCIE_PEX_SPC_ADRS_OFFSET
 */
#define PCIE_PEX_SPC_AER_MULTI_LOGGING_SET(dst, val) BIT_SET_VAL(11, 11, dst, val)
#define PCIE_PEX_SPC_SLOT_REG_IMPL_SET(dst, val) BIT_SET_VAL(12, 12, dst, val)
#define PCIE_PEX_SPC_SLOT_CLK_CONFIG_SET(dst, val) BIT_SET_VAL(13, 13, dst, val)
#define PCIE_PEX_SPC_LINK_SEL_DE_EMPHASIS_SET(dst, val) BIT_SET_VAL(14, 14, dst, val)
#define PCIE_PEX_SPC_ROOTPORT_RCB_SET(dst, val) BIT_SET_VAL(15, 15, dst, val)
#define PCIE_PEX_SPC_IMPL_MSI_X_CAP_VF_SET(dst, val) BIT_SET_VAL(21, 21, dst, val)
#define PCIE_PEX_SPC_DIS_MSI_CAP_VF_SET(dst, val) BIT_SET_VAL(22, 22, dst, val)
#define PCIE_PEX_SPC_NUM_MSI_MSG_SUPPORTED_VF_SET(dst, val) BIT_SET_VAL(25, 23, dst, val)
#define PCIE_PEX_SPC_IDE_KM_SUPPORTED_CAP_BIT_SET(dst, val) BIT_SET_VAL(26, 26, dst, val)
#define PCIE_PEX_SPC_DEV_SERIAL_NUM_EXT_CAP_SET(dst, val) BIT_SET_VAL(29, 29, dst, val)
#define PCIE_PEX_SPC_AER_IMPL_SET(dst, val) BIT_SET_VAL(31, 31, dst, val)

/* REG_PCIE_X1_PCIE_BAR_0_ADRS_OFFSET / REG_PCIE_X2_PCIE_BAR_0_ADRS_OFFSET*/
#define PCIE_PF_BAR0_TYPE_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
// If BAR type is IO
#define PCIE_PF_BAR0_IO_SIZE_MASK_SET(dst, val) BIT_SET_VAL(31, 2, dst, val)
// If BAR type is memory
#define PCIE_PF_BAR0_MEM_RESIZABLE_SET(dst, val) BIT_SET_VAL(1, 1, dst, val)
#define PCIE_PF_BAR0_MEM_64_BIT_ADDR_SPACE_SET(dst, val) BIT_SET_VAL(2, 2, dst, val)
#define PCIE_PF_BAR0_MEM_PREFETCHABLE_SET(dst, val) BIT_SET_VAL(3, 3, dst, val)
// If memory BAR is not resizable
// These registers is special, so dont use normal BIT_SET_VAL macros
#define PCIE_PF_BAR0_MEM_SIZE_MASK_SET(dst, val) ((uint32_t)dst & ~0xFFFFFFF0) | (((uint32_t)val & 0xFFFFFFF0))
// If memory BAR is resizable
#define PCIE_PF_BAR0_MEM_SUPPORTED_SIZE_SET(dst, val) ((uint32_t)dst & ~0xFFFFF0) | (((uint32_t)val & 0xFFFFF0))
#define PCIE_PF_BAR0_MEM_DEFAULT_SIZE_SET(dst, val) BIT_SET_VAL(29, 24, dst, val)

// If BAR0[2] is 1b, 64 bit address space
#define PCIE_PF_BAR1_MEM_SUPPORTED_SIZE_SET(dst, val) BIT_SET_VAL(23, 0, dst, val)

/* REG_PCIE_X1_PCIE_WINROM_ADRS_OFFSET / REG_PCIE_X2_PCIE_WINROM_ADRS_OFFSET */
// Bits [3:0] are reserved when the Core is in Endpoint mode.
// Bits [31:11] are reserved when the Core is in Rootport mode.
#define PCIE_WINROM_IO_IMPL_GET(val) BIT_GET_VAL(0, 0, val)
#define PCIE_WINROM_IO_32_BIT_ADDR_SUP_GET(val) BIT_GET_VAL(1, 1, val)
#define PCIE_WINROM_PREF_MEM_IMPL_GET(val) BIT_GET_VAL(2, 2, val)
#define PCIE_WINROM_PREF_MEM_64_BIT_SUP_GET(val) BIT_GET_VAL(3, 3, val)
#define PCIE_WINROM_EXP_ROM_SIZE_MASK_GET(val) BIT_GET_VAL(31, 11, val)

#define PCIE_WINROM_IO_IMPL_SET(dst, val) BIT_SET_VAL(0, 0, dst, val)
#define PCIE_WINROM_IO_32_BIT_ADDR_SUP_SET(dst, val) BIT_SET_VAL(1, 1, dst, val)
#define PCIE_WINROM_PREF_MEM_IMPL_SET(dst, val) BIT_SET_VAL(2, 2, dst, val)
#define PCIE_WINROM_PREF_MEM_64_BIT_SUP_SET(dst, val) BIT_SET_VAL(3, 3, dst, val)
#define PCIE_WINROM_EXP_ROM_SIZE_MASK_SET(dst, val) BIT_SET_VAL(31, 11, dst, val)

/* REG_PCIE_X1_PCIE_SRIOV_31_0_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_SRIOV_31_0_ADRS_OFFSET */
#define PCIE_SRIOV_VF_DEV_ID_SET(dst, val) BIT_SET_VAL(15, 0, dst, val)
#define PCIE_SRIOV_VF_SUB_SYS_DEV_ID_SET(dst, val) BIT_SET_VAL(31, 16, dst, val)

/* REG_PCIE_X1_PCIE_SRIOV_63_32_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_SRIOV_63_32_ADRS_OFFSET */
#define PCIE_SRIOV_VF_APP_SPECIFIC_CAP_OFF 0x40
#define PCIE_SRIOV_VF_SUP_PAGE_SIZE_SET(dst, val) BIT_SET_VAL(11, 0, dst, val)
#define PCIE_SRIOV_VF_10_BIT_TAG_SET(dst, val) BIT_SET_VAL(14, 14, dst, val)
#define PCIE_SRIOV_VF_APP_SPECIFIC_EXT_CAP_IMP_SET(dst, val) BIT_SET_VAL(15, 15, dst, val)
#define PCIE_SRIOV_VF_FUNC_DEPEND_LINK_SET(dst, val) BIT_SET_VAL(23, 16, dst, val)
#define PCIE_SRIOV_VF_APP_SPECIFIC_CAP_IMP_SET(dst, val) BIT_SET_VAL(31, 24, dst, val)

/* REG_PCIE_X1_PCIE_SRIOV_95_64_ADRS_OFFSET /
 * REG_PCIE_X2_PCIE_SRIOV_95_64_ADRS_OFFSET */
#define PCIE_SRIOV_VF_BAR0_RESIZABLE_SET(dst, val) BIT_SET_VAL(1, 1, dst, val)
#define PCIE_SRIOV_VF_BAR0_64_BIT_ADDR_SPACE_SET(dst, val) BIT_SET_VAL(2, 2, dst, val)
#define PCIE_SRIOV_VF_BAR0_PREFETCHABLE_SET(dst, val) BIT_SET_VAL(3, 3, dst, val)
// If VF BAR is not resizable
// These registers is special, so dont use normal BIT_SET_VAL macros
#define PCIE_SRIOV_VF_BAR0_MASK_SET(dst, val) ((uint32_t)dst & ~0xFFFFF000) | (((uint32_t)val & 0xFFFFF000))
// If VF BAR is resizable
#define PCIE_SRIOV_VF_BAR0_SUPPORTED_SIZE_SET(dst, val) ((uint32_t)dst & ~0xFFFFF0) | (((uint32_t)val & 0xFFFFF0))
#define PCIE_SRIOV_VF_BAR0_DEFAULT_SIZE_SET(dst, val) BIT_SET_VAL(29, 24, dst, val)
// If VF BAR0[2] is 1b, 64 bit address space
#define PCIE_SRIOV_VF_BAR1_SUPPORTED_SIZE_SET(dst, val) BIT_SET_VAL(23, 0, dst, val)

/* PCIE_PCI_IDS_31_0_ADRS_OFFSET */
#define PCIE_PCI_IDS_31_0_VENDOR_ID_SET(dst, val) BIT_SET_VAL(15, 0, dst, val)
#define PCIE_PCI_IDS_31_0_DEVICE_ID_SET(dst, val) BIT_SET_VAL(31, 16, dst, val)

/* REG_PCIE_X1_PCIE_PCI_IDS_63_32_ADRS_OFFSET / REG_PCIE_X2_PCIE_PCI_IDS_63_32_ADRS_OFFSET*/
#define PCIE_PCI_IDS_63_32_CLASS_CODE_SET(dst, val) BIT_SET_VAL(31, 8, dst, val)

/* PCIE_PCI_IRQ_PASID_MSIX_15_0_ADRS_OFFSET */
#define INT_PCIE_PCI_IRQ_INTR_PIN_GET(val) BIT_GET_VAL(2, 0, val)
#define INT_PCIE_PCI_IRQ_DISABLE_MSI_GET(val) BIT_GET_VAL(3, 3, val)
#define INT_PCIE_PCI_IRQ_NUM_MSI_MSG_GET(val) BIT_GET_VAL(6, 4, val)
#define INT_PCIE_PCI_IRQ_MSI_PER_VEC_MASK_SUP_GET(val) BIT_GET_VAL(7, 7, val)
#define INT_PCIE_PCI_PASID_SUP_GET(val) BIT_GET_VAL(8, 8, val)
#define INT_PCIE_PCI_PASID_EXEC_PERMIS_SUP_GET(val) BIT_GET_VAL(9, 9, val)
#define INT_PCIE_PCI_PASID_PRIVIL_MODE_SUP_GET(val) BIT_GET_VAL(10, 10, val)
#define INT_PCIE_PCI_PASID_MAX_WIDTH_GET(val) BIT_GET_VAL(15, 11, val)
#define INT_PCIE_PCI_MSIX_15_0_TABLE_SIZE_GET(val) BIT_GET_VAL(26, 16, val)
#define INT_PCIE_PCI_MSIX_15_0_IMPL_MSIX_CAP_PF_GET(val) BIT_GET_VAL(31, 31, val)

#define INT_PCIE_PCI_IRQ_INTR_PIN_SET(dst, val) BIT_SET_VAL(2, 0, dst, val)
#define INT_PCIE_PCI_IRQ_DISABLE_MSI_SET(dst, val) BIT_SET_VAL(3, 3, dst, val)
#define INT_PCIE_PCI_IRQ_NUM_MSI_MSG_SET(dst, val) BIT_SET_VAL(6, 4, dst, val)
#define INT_PCIE_PCI_IRQ_MSI_PER_VEC_MASK_SUP_SET(dst, val) BIT_SET_VAL(7, 7, dst, val)
#define INT_PCIE_PCI_PASID_SUP_SET(dst, val) BIT_SET_VAL(8, 8, dst, val)
#define INT_PCIE_PCI_PASID_EXEC_PERMIS_SUP_SET(dst, val) BIT_SET_VAL(9, 9, dst, val)
#define INT_PCIE_PCI_PASID_PRIVIL_MODE_SUP_SET(dst, val) BIT_SET_VAL(10, 10, dst, val)
#define INT_PCIE_PCI_PASID_MAX_WIDTH_SET(dst, val) BIT_SET_VAL(15, 11, dst, val)
#define INT_PCIE_PCI_MSIX_15_0_TABLE_SIZE_SET(dst, val) BIT_SET_VAL(26, 16, dst, val)
#define INT_PCIE_PCI_MSIX_15_0_IMPL_MSIX_CAP_PF_SET(dst, val) BIT_SET_VAL(31, 31, dst, val)

/* PCIE_PCI_MSIX_47_16_ADRS_OFFSET */
#define INT_PCIE_PCI_MSIX_47_16_TABLE_BIR_GET(val) BIT_GET_VAL(2, 0, val)
#define INT_PCIE_PCI_MSIX_47_16_TABLE_OFFSET_GET(val) BIT_GET_VAL(31, 3, val)

#define INT_PCIE_PCI_MSIX_47_16_TABLE_BIR_SET(dst, val) BIT_SET_VAL(2, 0, dst, val)
#define INT_PCIE_PCI_MSIX_47_16_TABLE_OFFSET_SET(dst, val) BIT_SET_VAL(31, 3, dst, val)

/* PCIE_PCI_MSIX_79_48_ADRS_OFFSET */
#define INT_PCIE_PCI_MSIX_79_48_PBA_BIR_GET(val) BIT_GET_VAL(2, 0, val)
#define INT_PCIE_PCI_MSIX_79_48_PBA_OFFSET_GET(val) BIT_GET_VAL(31, 3, val)

#define INT_PCIE_PCI_MSIX_79_48_PBA_BIR_SET(dst, val) BIT_SET_VAL(2, 0, dst, val)
#define INT_PCIE_PCI_MSIX_79_48_PBA_OFFSET_SET(dst, val) BIT_SET_VAL(31, 3, dst, val)

enum PCIE_CONTROLLER_MODE { PCIE_EP = 0, PCIE_RP, PCIE_INVALID };

enum PCIE_PAYLOAD_SIZE {
    PAYLOAD_128_BYTE = 0,
    PAYLOAD_256_BYTE,
    PAYLOAD_512_BYTE,
    PAYLOAD_1024_BYTE,
    PAYLOAD_2048_BYTE,
    PAYLOAD_4096_BYTE,
    INVALID_PAYLOAD_SIZE
};

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

struct axiado_pcie_atr {
	void __iomem *base;
	unsigned int size;
	unsigned int flags;
};

struct axiado_pcie {
	bool pcie_x1; /* Used for executing PCIe X1 specific task */
	bool pcie_x2; /* Used for executing PCIe X2 specific task */
    bool is_root_port;

    unsigned lanes;
    unsigned speed;

	void __iomem *csr; /* CSR memory region */
	void __iomem *phy; /* PHY memory region */
	void __iomem *cfg; /* PCIe 4K config space */
	void __iomem *bridge; /* INT bridge registers */
	void __iomem *mbx; /* mailbox memory region */
	void __iomem *ext; /* EXT memory region */

	void __iomem *ecam; /* ECAM base address */

    struct axiado_pcie_atr atr[2];

    struct axiado_pcie_atr bar[2];
};

enum PCIE_XPRESS_LTSSM {
    S_DETECT_QUIET = 0,
    S_DETECT_ACTIVE,
    S_POLLING_ACTIVE,
    S_POLLING_COMPLIANCE,
    S_POLLING_CFG,
    S_CFG_LINKWIDTHSTART,
    S_CFG_LINKWIDTHACCEPT,
    S_CFG_LANEN_WAIT,
    S_CFG_LANEN_ACCEPT,
    S_CFG_COMPLETE,
    S_CFG_IDLE,
    S_RECOVERY_RCV_LOCK,
    S_RECOVERY_EQ,
    S_RECOVERY_SPEED,
    S_RECOVERY_RCV_CFG,
    S_RECOVERY_IDLE,
    S_L0,
    S_L0S,
    S_L1_ENTRY,
    S_L1_IDLE,
    S_L2_IDLE_TRANSMITWAKE,
    S_RESERVED,
    S_DISABLE,
    S_LOOPBACK_ENTRY,
    S_LOOPBACK_ACTIVE,
    S_LOOPBACK_EXIT,
    S_HOTRESET
};

enum TRSF_ID_ENUM {
    PCIE_TX_TX_INF = 0,
    PCIE_CFG_IO_INF,
    AXI4_LITE_MSTR_INF,
    RESERVE1 = 3,
    AXI4_MSTR_NUM_0 = 4,
    AXI4_MSTR_NUM_1,
    AXI4_MSTR_NUM_2,
    AXI4_MSTR_NUM_3,
    AXI4_STREAM_NUM_0 = 8,
    AXI4_STREAM_NUM_1,
    AXI4_STREAM_NUM_2,
    AXI4_STREAM_NUM_3,
    BIDGE_INTERNAL_REG,
    ID_INVALID
};

enum ATR_PCIE_WIN { PCIE_WIN_0 = 0, PCIE_WIN_1 };

enum ATR_AXI4_SLV_NUM { AXI4_SLV_0 = 0, AXI4_SLV_1, AXI4_SLV_2, AXI4_SLV_3 };

enum ATR_TABLE_NUM { TABLE_0 = 0, TABLE_1, TABLE_2, TABLE_3, TABLE_4, TABLE_5, TABLE_6, TABLE_7 , TABLE_MAX };

#endif /* __AXIADO_PCIE_H */
