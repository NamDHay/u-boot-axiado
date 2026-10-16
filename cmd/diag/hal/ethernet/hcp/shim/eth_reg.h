#ifndef __ETH_REGS_H__
#define __ETH_REGS_H__
#include <dm.h>
#include <asm/io.h>
#include <stdio.h>

#include "ax_diag.h"

#define AXI4S_EIP_BASE          AX3000_CSR_BASE_ADRS_EIP

#define AXILS_HCP_BASE          AX3000_CSR_BASE_ADRS_HCP

#define XGMII_MAC_BASE          AXILS_HCP_BASE+0x400
#define GMII0_MAC_BASE          AXILS_HCP_BASE+0x800
#define GMII1_MAC_BASE          AXILS_HCP_BASE+0xC00
#define GMII2_MAC_BASE          AXILS_HCP_BASE+0x1000
#define GMII3_MAC_BASE          AXILS_HCP_BASE+0x1400

#define SGMII0_BASE             AX3000_CSR_BASE_ADRS_ETH_PHY
#define SGMII1_BASE             AX3000_CSR_BASE_ADRS_ETH_PHY + 0x10000
#define SGMII2_BASE             AX3000_CSR_BASE_ADRS_ETH_PHY + 0x20000
#define SGMII3_BASE             AX3000_CSR_BASE_ADRS_ETH_PHY + 0x30000
#define RXAUI_BASE	            AX3000_CSR_BASE_ADRS_ETH_PHY + 0x40000

#define GMII_PORT_CONTROL_OFFSET 	        4
#define REG_XGMII_PORT_CONTROL_ADRS_OFFSET 	0x0000
#define REG_GMII0_PORT_CONTROL_ADRS_OFFSET 	0x0004
#define REG_GMII1_PORT_CONTROL_ADRS_OFFSET 	0x0008
#define REG_GMII2_PORT_CONTROL_ADRS_OFFSET 	0x000c
#define REG_GMII3_PORT_CONTROL_ADRS_OFFSET 	0x0010

#define XGE_SHIM_FIFO 	0x0040
#define GE0_SHIM_FIFO 	0x0044
#define GE1_SHIM_FIFO 	0x0048
#define GE2_SHIM_FIFO 	0x004c
#define GE3_SHIM_FIFO 	0x0050

#define SHIM_MAC_TX_OVFLOW_BIT 7
#define SHIM_MAC_RX_OVFLOW_BIT 3
#define BIT_RX_RST 0
#define BIT_TX_RST 4
#define BIT_TX_EMPTY 6
#define BIT_RX_EMPTY 2

#define XGE_SHIM_RX_PKTS 	0x0060
#define GE0_SHIM_RX_PKTS 	0x0064
#define GE1_SHIM_RX_PKTS 	0x0068
#define GE2_SHIM_RX_PKTS 	0x006c
#define GE3_SHIM_RX_PKTS 	0x0070

#define XGE_SHIM_RX_GOOD_PKTS 	0x0080
#define GE0_SHIM_RX_GOOD_PKTS 	0x0084
#define GE1_SHIM_RX_GOOD_PKTS 	0x0088
#define GE2_SHIM_RX_GOOD_PKTS 	0x008c
#define GE3_SHIM_RX_GOOD_PKTS 	0x00A0

#define XGE_SHIM_RX_BAD_PKTS 	0x00A0
#define GE0_SHIM_RX_BAD_PKTS 	0x00A4
#define GE1_SHIM_RX_BAD_PKTS 	0x00A8
#define GE2_SHIM_RX_BAD_PKTS 	0x00AC
#define GE3_SHIM_RX_BAD_PKTS 	0x00B0


/*for lb and traffic test*/
#define HOST_TX_FIFO_EN	   1
#define XGMII_PORT_CONTROL 0x0000
#define GMII_PORT_CONTROL  0x0004

/* Total number of maximum MAC */
#define MAX_MAC_CNT 5

/* Total number of maximum RMII MAC */
#define MAX_RMII_MAC_CNT 3

/* base offset for five MAC. base is shim_phy_base_addr */
#define MAC_BASE_OFFSET		0x400
#define MAC_CONFIG_BYTE_CNT 0x400


/* base offset for mac-flow config regs. */
#define MAC_FLOW_CONFIG_BASE_OFFSET 0x0
#define MAC_FLOW_CONFIG_BYTE_CNT	0x4

#define MAC_1G_R_RX_FIFO_VAL	0x03f00008
#define MAC_1G_R_TX_FIFO_VAL	0x03f00008
#define MAC_1G_R_FRM_LENGTH_VAL 9600

#define ETH_JUMBO_PAYLOAD	    9000

/* 0x01 : MII mode*/
#define MAC_MII_IF_MODE_VAL 0x01
/* 0x02 : GMII mode*/
#define MAC_1G_R_IF_MODE_VAL 0x02
#define MAC_1G_REVISION_VAL	 0x10136
#define MAC_CC_NULL			 0x00000000

/* Non-lookback flow : 0x00000000; Loopback flow: 0x00000001 */
#define MAC_1G_0_FLOW_VAL 0x000000
#define MAC_1G_1_FLOW_VAL 0x000000
#define MAC_1G_2_FLOW_VAL 0x000000
#define MAC_1G_3_FLOW_VAL 0x000000

/* Setting Mac address as 00:11:22:33:44:55 */
#define XG_R_MAC_1_VAL			0x00005544
#define XG_R_MAC_0_VAL			0x33221100
#define XG_MAC_R_RX_FIFO_VAL	0x04
#define XG_MAC_R_TX_FIFO_VAL	0x06
#define XG_MAC_R_FRM_LENGTH_VAL 9600
#define XG_MAC_R_IF_MODE_VAL	0x0

#define AX3000_CSR_ETH_PHY_SIZE 0x50000
#define SGMII_CONFIG_RANGE		0x10000

#define GMII_PORT_COUNT		0x4
#define PORT_CONTROL_OFFSET 0x4

/*TODO Verify this revision value from package file*/
#define MAC_XG_REVISION_VAL 0x10136

/* Non-lookback flow : 0x00000000; Loopback flow: 0x00000001 */
#define MAC_XG_FLOW_VAL     0x00000000

/* Config for Cmd Descriptor token */

/* Set 1/0 for enabling/disabling for lookup mode.
 * If lookup is disabled, shim will use a static Transform Record.
 */
#define TRANSFORM_LOOKUP_ENABLE     1

/* Destination Interface to be used in static Transform Record.
 * If the value is more than 0, multi-interface support must be
 * enabled in EIP-DDK driver.
 */
#define DEST_IF_ID  0

/* COMMAND TOKEN WORDS VALUE FOR CMD DESCRIPTOR */
/* Refer Firmware Reference Manual of EIP - page_23 */
#define CMD_TOKEN_WORD_0_VAL    0xF0000000
#define CMD_TOKEN_WORD_1_VAL    0x00200200

/* TR ptr(low). For lookup mode set it to 0xFFFFFFFC */
#define CMD_TOKEN_WORD_2_VAL    0xFFFFFFFC

/* TR ptr(high). For lookup mode set it to 0xFFFFFFFF */
#define CMD_TOKEN_WORD_3_VAL    0xFFFFFFFF

#define CMD_TOKEN_WORD_4_VAL    0x02000000
#define CMD_TOKEN_WORD_5_VAL    0x00000e00
#define CMD_TOKEN_WORD_6_VAL    0x00000000
#define CMD_TOKEN_WORD_7_VAL    0x00000000

/* VALUE FOR TDEST MASK*/
#define TDEST_MASK_VALUE_XG	    0xFFFF0A00
#define TDEST_MASK_VALUE_1G_0   0xFFFF0200
#define TDEST_MASK_VALUE_1G_1   0xFFFF0400
#define TDEST_MASK_VALUE_1G_2   0xFFFF0600
#define TDEST_MASK_VALUE_1G_3   0xFFFF0800

/* Offset for MAC for command token words*/
#define SHIM_CD_TOKEN_CONFIG_BASE_OFFSET    0x300
#define SHIM_CD_TOKEN_CONFIG_BYTE_CNT	    0x20

/* Offset for token word within a Cmd Token */
#define CMD_TOKEN_WORD_0_OFFSET     0x0

/* offset for each MAC for tdest mask reg. */
#define TDEST_MASK_BASE_OFFSET  0x20
#define TDEST_MASK_BYTE_CNT	    0x4

/* shim/mac stats offset */

#define AXILS_SHIM_MAC_BASE		   (AXILS_HCP_BASE + MAC_BASE_OFFSET)
#define AXILS_SHIM_BASE			   (AXILS_HCP_BASE)
#define SHIM_STATS_TX_MAC_TOTAL	   0x260
#define SHIM_STATS_TX_MAC_GOOD	   0x220
#define SHIM_STATS_RX_MAC_TOTAL	   0x160
#define SHIM_STATS_RX_MAC_GOOD	   0x120
#define SHIM_STATS_CRCERRORS	   0x128
#define SHIM_STATS_RX_VLANOK	   0x130
#define SHIM_STATS_RX_DROP_EVENTS  0x158
#define SHIM_STATS_RX_LNTHERROR	   0x1d0
#define SHIM_STATS_RX_UNDERSIZE_PKTS	   0x168
#define SHIM_STATS_TX_UNDERSIZE_PKTS	   0x268
#define SHIM_STATS_RX_OVERSIZE_PKTS	   0x1a8
#define SHIM_STATS_RX_JABBER_PKTS	   0x1b0
#define SHIM_STATS_RX_FRAGMENTS_PKTS   0x1b8
#define SHIM_STATS_TX_VLANOK	       0x230
#define SHIM_STATS_RX_PAUSEFRAME	   0x118

#define SHIM_STATS_MAC_CTRL_FRAME_RX    0x1c0
#define SHIM_STATS_MAC_CTRL_FRAME_TX	0x2c0

#define SHIM_STATS_TX_PAUSEFRAME	   0x218

#define SHIM_STATS_RX_PFC_FRM_0	       0x380
#define SHIM_STATS_RX_PFC_FRM_1	       0x388
#define SHIM_STATS_RX_PFC_FRM_2	       0x390
#define SHIM_STATS_RX_PFC_FRM_3	       0x398
#define SHIM_STATS_RX_PFC_FRM_4	       0x3a0
#define SHIM_STATS_RX_PFC_FRM_5	       0x3a8
#define SHIM_STATS_RX_PFC_FRM_6	       0x3b0
#define SHIM_STATS_RX_PFC_FRM_7	       0x3b8

#define SHIM_STATS_RX_UNICAST	   0x140
#define SHIM_STATS_TX_UNICAST	   0x240

#define SHIM_STATS_RX_MULTICAST	   0x148
#define SHIM_STATS_TX_MULTICAST	   0x248

#define SHIM_STATS_RX_BROADCAST	   0x150
#define SHIM_STATS_TX_BROADCAST	   0x250

#define SHIM_STATS_RX_OCTETS	    0x100
#define SHIM_STATS_RX_OCTETS_OK     0x108

#define SHIM_STATS_RX_PKTS64OCTETS     0x170
#define SHIM_STATS_TX_PKTS64OCTETS     0x270

#define SHIM_STATS_RX_PKTS65_127OCTETS     0x178
#define SHIM_STATS_TX_PKTS65_127OCTETS     0x278

#define SHIM_STATS_RX_PKTS128_255OCTETS     0x180
#define SHIM_STATS_TX_PKTS128_255OCTETS     0x280

#define SHIM_STATS_RX_PKTS256_511OCTETS     0x188
#define SHIM_STATS_TX_PKTS256_511OCTETS     0x288

#define SHIM_STATS_RX_PKTS512_1023OCTETS     0x190
#define SHIM_STATS_TX_PKTS512_1023OCTETS     0x290

#define SHIM_STATS_RX_PKTS1024_1518OCTETS     0x198
#define SHIM_STATS_TX_PKTS1024_1518OCTETS     0x298

#define SHIM_STATS_RX_PKTSOVER1518OCTETS     0x1a0
#define SHIM_STATS_TX_PKTSOVER1518OCTETS     0x2a0

#define SHIM_STATS_SHIM_TOTAL	   0x60
#define SHIM_STATS_SHIM_GOOD	   0x80

/*------------------------------------------------------------------------------*/
#define REG_SHIM_CSR_ADRS_OFFSET    0x03a0
/**
 * @file mdio.h
 * 
 * @brief Defined the MDIO access function APIs
 */

#define MDIO_CFG_STATUS_BIT		0 /**< Status BIT */
#define MDIO_CMD_PORT_ADRS_LSB	5 /**< PORT address lsb bit in MDIO_COMMAND */
#define MDIO_CMD_PORT_ADRS_MSB	9 /**< PORT address msb bit in MDIO_COMMAND */
#define MDIO_CMD_READ_INC_BIT	14 /**< Read with address post-increment transaction bit in MDIO_COMMAND */
#define MDIO_CMD_READ_BIT		15 /**< Read transaction bit in MDIO_COMMAND */
#define MDIO_WAIT_TIME			10 /**< Wait time before reading the MDIO device status again */
#define MDIO_READ_INC_CMD_COUNT 4 /**< Maximum number of consecutive read */

/* Helper macros for MDIO_CFG_STATUS */

#define MDIO_BUSY_MASK		   (1U << 0)
#define MDIO_READ_ERROR_MASK   (1U << 1)
#define MDIO_HOLD_TIME_MASK	   (0x7U << 2)
#define MDIO_DISABLE_PREAMBLE  (1U << 5)
#define MDIO_CLAUSE_45_SUPPORT (1U << 6)
#define MDIO_CLK_DIV_MASK	   (0x1FFU << 7)

#define MDIO_CFG_CLAUSE_22 0x0B0C /**< Value to config clause 22 */
#define MDIO_CFG_CLAUSE_45 (MDIO_CLAUSE_45_SUPPORT | MDIO_CLK_DIV_MASK) /**< Value to config clause 45 */

/**
 * @file phy_defs.h
 *
 * @brief Defines phy manipulation routines/definitions.
 */

/* Phy Model number for Marvel. Ref. doc
- MARV_88e1512_S_A0009147335_1-2576139.pdf, Page-76 */
#define PHY_MDL_NUM_MARVEL 0x1D

/* Phy Model number for VITESSE. Ref. doc
- VMDS-10264.pdf, page-33 */
#define PHY_MDL_NUM_VITESSE 0x26

/* Common phy registers across models */
#define PHY_MDIO_BUSY			   0x00000001
#define PHY_MDIO_READ_ERROR		   0x00000002
#define PHY_MDIO_SUCCESS		   0x0
#define PHY_MDIO_SLEEP			   20
#define PHY_MDIO_RETRY			   50
#define PHY_MDIO_C22_PAGE_SELECTOR 0x16
#define PHY_C22_PHY_IDENT1_REG	   0x2
#define PHY_C22_PHY_IDENT2_REG	   0x3
#define PHY_DEFAULT_PAGE		   0x0
#define PHY_DEFAULT_REG			   0x0

#define SGMII_CSR_BASE             (AXILS_HCP_BASE + 0x120)
#define SGMII_CSR_OFFSET           4
#define SGMII_CSR0 0x0120
#define SGMII_CSR1 0x0124
#define SGMII_CSR2 0x0128
#define SGMII_CSR3 0x012c

#define SYNC_STATUS             0
#define AUTO_NEG_COMPLETE       1

#define SGMII_MON_BASE             (AXILS_HCP_BASE + 0x134)
#define SGMII_MON_OFFSET           4

#define REG_XGE_MON_ADRS_OFFSET 0x0130
#define REG_GE_MON0_ADRS_OFFSET 0x0134
#define REG_GE_MON1_ADRS_OFFSET 0x0138
#define REG_GE_MON2_ADRS_OFFSET 0x013c
#define REG_GE_MON3_ADRS_OFFSET 0x0140

#define REG_GE0_TS_CSR          0x230
#define REG_GE1_TS_CSR          0x240
#define REG_GE2_TS_CSR          0x250
#define REG_GE3_TS_CSR          0x260

#define REG_GE0_TS_NS_MAX       0x234
#define REG_GE1_TS_NS_MAX       0x244
#define REG_GE2_TS_NS_MAX       0x254
#define REG_GE3_TS_NS_MAX       0x264

#define REG_GE0_TS_NS_CTR       0x238
#define REG_GE1_TS_NS_CTR       0x248
#define REG_GE2_TS_NS_CTR       0x258
#define REG_GE3_TS_NS_CTR       0x268

#define REG_GE0_TS_1S_CTR       0x23c
#define REG_GE1_TS_1S_CTR       0x24c
#define REG_GE2_TS_1S_CTR       0x25c
#define REG_GE3_TS_1S_CTR       0x26c

#define REG_IRQ_GE0_CSR         0x208

#define LOCAL_LPBK_CTRL   0x1C00
/* GE/SGMII monitor and HPI IRQ bits, used by the monitor tests */
// #define BIT(n) (1 << (n))
#define SGMII_MON 17
#define GE_MON 16
#define SGMII_IRQ_EN 1
#define GE_MON_BASE (AXILS_HCP_BASE + 0x134)
#define AUTO_NEG_MON 1
#define AUTO_NEG 17
#define SGMII_RESET_REGISTER (AXILS_HCP_BASE + 0x0004)
#define SGMII_RESET 4
#define IRQ_GE_CSR_BASE 0x0208
#define HPI_CSR                 0x014
#define HPI_MON                 1
#define GE_HPI_IRQ_EN             3

/* mac_idx -> base-address helpers: centralizes the per-MAC
 * addressing arithmetic that used to be duplicated at every call site.
 */
static inline u32 sgmii_phy_base(u8 mac_idx)
{
	return SGMII0_BASE + ((mac_idx - 1) * SGMII_CONFIG_RANGE);
}

static inline u32 gmii_port_ctrl_offset(u8 mac_idx)
{
	return mac_idx * GMII_PORT_CONTROL_OFFSET;
}

static inline u32 sgmii_stride(u8 mac_idx)
{
	return (mac_idx - 1) * SGMII_CSR_OFFSET;
}
#endif /* __ETH_REGISTERS_H__ */