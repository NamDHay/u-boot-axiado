#ifndef __MAC_DEFINES_H__
#define __MAC_DEFINES_H__

//
// MAC Register MAP Definitions
//

// COMMAND_CONFIG Register Bits

        // bit numbers
#define MAC_CC_TX_ENA_bit        0  
#define MAC_CC_RX_ENA_bit        1
#define MAC_CC_ETH_unused2_bit   2        // unused/reserved
#define MAC_CC_ETH_SPEED_bit     3 
#define MAC_CC_PROMIS_EN_bit     4
#define MAC_CC_PAD_EN_bit        5
#define MAC_CC_CRC_FWD_bit       6
#define MAC_CC_PAUSE_FWD_bit     7
#define MAC_CC_PAUSE_IGNORE_bit  8
#define MAC_CC_TX_ADDR_INS_bit   9
#define MAC_CC_LOOP_ENA_bit      10
#define MAC_CC_TX_PAD_EN_bit     11       // reserved
#define MAC_CC_SW_RESET_bit      12
#define MAC_CC_CNTL_FRM_ENA_bit  13
#define MAC_CC_RX_ERR_DISC_bit   14
#define MAC_CC_PHY_TXENA_bit     15
#define MAC_CC_SEND_IDLE_bit     16
#define MAC_CC_NO_LGTH_CHECK_bit 17
#define MAC_CC_COL_CNT_EXT_bit   18
#define MAC_CC_PFC_MODE_bit      19
#define MAC_CC_PAUSE_PFC_COMP_bit      20
#define MAC_CC_SFD_ANY_bit       21
#define MAC_CC_TX_FLUSH_bit      22
#define MAC_CC_TX_LOWP_ENA_bit   23
#define MAC_CC_REG_LOWP_RXEMPTY_bit   24
#define MAC_CC_SHORT_DISCARD_bit 25
#define MAC_CC_TX_FIFO_RESET_bit 26
#define MAC_CC_DISABLE_FLT_HDL_bit  27

        // bit masks
#define MAC_CC_TX_ENA           (1 << 0)
#define MAC_CC_RX_ENA           (1 << 1)
#define MAC_CC_ETH_unused2      (1 << 2)        // unused/reserved
#define MAC_CC_ETH_SPEED        (1 << 3) 
#define MAC_CC_PROMIS_EN        (1 << 4)
#define MAC_CC_PAD_EN           (1 << 5)
#define MAC_CC_CRC_FWD          (1 << 6)
#define MAC_CC_PAUSE_FWD        (1 << 7)
#define MAC_CC_PAUSE_IGNORE     (1 << 8)
#define MAC_CC_TX_ADDR_INS      (1 << 9)
#define MAC_CC_LOOP_ENA         (1 << 10)
#define MAC_CC_TX_PAD_EN        (1 << 11)       // reserved
#define MAC_CC_SW_RESET         (1 << 12)
#define MAC_CC_CNTL_FRM_ENA     (1 << 13)
#define MAC_CC_RX_ERR_DISC      (1 << 14)
#define MAC_CC_PHY_TXENA        (1 << 15)       // CONV_ENA
#define MAC_CC_SEND_IDLE        (1 << 16)       // CONV_ENA
#define MAC_CC_NO_LGTH_CHECK    (1 << 17)       // disable RX payload length checking
#define MAC_CC_COL_CNT_EXT      (1 << 18)       // extend LF/RF column counter
#define MAC_CC_PFC_MODE         (1 << 19)       // Set Priority Flow Control mode (Class Based Pause)
#define MAC_CC_PAUSE_PFC_COMP   (1 << 20)       // Link Pause Compatibility with PFC Mode
#define MAC_CC_SFD_ANY          (1 << 21)       // allow any SFD character
#define MAC_CC_TX_FLUSH         (1 << 22)
#define MAC_CC_TX_LOWP_ENA      (1 << 23)       // transmit low power idle
#define MAC_CC_REG_LOWP_RXEMPTY (1 << 24)       // mask reg_lowp with rx fifo empty
#define MAC_CC_SHORT_DISCARD    (1 << 25)       // discard short frames in FIFO
#define MAC_CC_TX_FIFO_RESET    (1 << 26)       // separate TX FIFO reset
#define MAC_CC_DISABLE_FLT_HDL  (1 << 27)       // disable fault handling

//#define MAC_CC_ENA_EXT_CONFIG   (1 << 22)
//#define MAC_CC_LINE_LOOPBACK    (1 << 25)
//#define MAC_CC_PRBL_ENA         (1 << 27)

// Register offsets

#define R_REVISION              0x000
#define R_SCRATCH               0x004
#define R_COMMAND_CONFIG        0x008
#define R_MAC_0                 0x00c
#define R_MAC_1                 0x010
#define R_FRM_LENGTH            0x014
#define R_PAUSE_QUANT           0x018
#define R_RX_FIFO_SECTIONS      0x01c
#define R_TX_FIFO_SECTIONS      0x020
#define R_RX_FIFO_ALMOST_F_E    0x024
#define R_TX_FIFO_ALMOST_F_E    0x028
#define R_HASHTABLE_LOAD        0x02c
#define R_MDIO_CFG_STATUS       0x030
#define R_MDIO_COMMAND          0x034
#define R_MDIO_DATA             0x038
#define R_MDIO_REGADDR          0x03c
#define R_STATUS                0x040
#define R_TX_IPG_LENGTH         0x044
#define R_CREDIT_TRIGGER        0x048
#define R_INIT_CREDIT           0x04c
#define R_CREDIT_REG            0x050
#define R_CL01_PAUSE_QUANTA     0x054
#define R_CL23_PAUSE_QUANTA     0x058
#define R_CL45_PAUSE_QUANTA     0x05c
#define R_CL67_PAUSE_QUANTA     0x060
#define R_CL89_PAUSE_QUANTA     0x084 // Used in newer MACs
#define R_CL1011_PAUSE_QUANTA   0x088 // Used in newer MACs
#define R_CL1213_PAUSE_QUANTA   0x08c // Used in newer MACs
#define R_CL1415_PAUSE_QUANTA   0x090 // Used in newer MACs
#define R_CL01_QUANTA_THRESH    0x064
#define R_CL23_QUANTA_THRESH    0x068
#define R_CL45_QUANTA_THRESH    0x06c
#define R_CL67_QUANTA_THRESH    0x070
#define R_CL89_QUANTA_THRESH    0x094 // Used in newer MACs
#define R_CL1011_QUANTA_THRESH  0x098 // Used in newer MACs
#define R_CL1213_QUANTA_THRESH  0x09c // Used in newer MACs
#define R_CL1415_QUANTA_THRESH  0x0a0 // Used in newer MACs
#define R_RX_PAUSE_STATUS       0x074

#define TX_PACKET_FIFO_0        0x100
#define TX_PACKET_FIFO_1        0x104
#define TX_PACKET_FIFO_WDATA    0x108

#define RX_PACKET_FIFO_0        0x110
#define RX_PACKET_FIFO_1        0x114
#define RX_PACKET_FIFO_2        0x118
#define RX_PACKET_FIFO_RDATA    0x11C

// FIFO Control/status registers
#define TX_FIFO_RST			 0
#define TX_FIFO_FULL		 1
#define TX_FIFO_OVF			 2
#define TX_FIFO_RDY			 3
#define TX_FIFO_OVF_IRQEN	 8
#define TX_FIFO_OVF_MON		 9
#define TX_FIFO_LVL			 16
#define TX_FIFO_SOP			 0
#define TX_FIFO_EOP			 1
#define TX_FIFO_MOD			 4
#define TX_FIFO_WDATA		 0
#define RX_FIFO_RST			 0
#define RX_FIFO_FULL		 1
#define RX_FIFO_OVF			 2
#define RX_FIFO_DAV			 3
#define RX_FIFO_OVF_IRQ_EN	 8
#define RX_FIFO_DAV_IRQ_EN	 9
#define RX_FIFO_OVF_MON		 10
#define RX_FIFO_DAV_MON		 11
#define RX_FIFO_LVL			 16
#define RX_FIFO_SEL			 4

#define RX_FIFO_SOP			 0
#define RX_FIFO_EOP			 1
#define RX_FIFO_MOD			 4
#define RX_FIFO_FRMLEN		 16
#define RX_FIFO_FRMSTAT		 0
#define RX_FIFO_RDATA		 0

#define R_TS_TIMESTAMP          0x07c
#define R_XIF_MODE              0x080

// (P2-6) removed GMII0..3_MAC_REG / XGMII_MAC_REG -- they duplicate the
// same offsets as eth_reg.h's GMII0..3_MAC_BASE / XGMII_MAC_BASE and had
// no in-tree usage.

// RAM based statistics module
#define R_STATN_CONFIG          0x0e0
#define R_STATN_CLEARVALUELO    0x0e4
#define R_STATN_CLEARVALUEHI    0x0e8
#define MAC_STC_SATURATE        (1 << 0)        // config bits
#define MAC_STC_CLEARONREAD     (1 << 1)
#define MAC_STC_CLEARALL        (1 << 2)

// MII/GMII/XGMII RS module included
#define R_IF_MODE               0x300
#define R_IF_STATUS             0x304

#define IFM_MODE_XGMII          0x0000
#define IFM_MODE_GMII           0x0002
#define IFM_MODE_MII            0x0001
#define IFM_MODE_GMII_HD        0x0022       // GMII with Halfduplex enabled (for clock-enable based modes)
#define IFM_MODE_MII_HD         0x0021       // MII with Halfduplex


// SGMII PCS Layer module included
#define R_PCS_CONTROL           0x300
#define R_PCS_STATUS            0x304
#define R_PCS_PHYID1            0x308
#define R_PCS_PHYID2            0x30c
#define R_PCS_DEV_ABILITY       0x310
#define R_PCS_PARTNER_ABILITY   0x314
#define R_PCS_AN_EXPANSION      0x318
#define R_PCS_LINK_TIMER_LO     0x348
#define R_PCS_LINK_TIMER_HI     0x34c
#define R_PCS_IF_MODE           0x350

// Counter Numbers
#define CNT_OCTETS              0
#define CNT_OCTETSOK            1
#define CNT_ALIGNERR            2
#define CNT_PAUSERX             3
#define CNT_FRAMESOK            4
#define CNT_CRCERR              5
#define CNT_VLANOK              6
#define CNT_IFERRORS            7
#define CNT_IFUCAST             8
#define CNT_IFMCAST             9
#define CNT_IFBCAST             10
#define CNT_STATSDROP           11
#define CNT_STATSPKTS           12
#define CNT_STATSUNDER          13
#define CNT_STATSPKT64          14
#define CNT_STATSPKT65          15
#define CNT_STATSPKT128         16
#define CNT_STATSPKT256         17
#define CNT_STATSPKT512         18
#define CNT_STATSPKT1024        19
#define CNT_STATSPKT1519        20
#define CNT_STATSOVER           21
#define CNT_STATSJABBER         22
#define CNT_STATSFRAGMENTS      23
#define CNT_CTRLFRAMES          24
#define CNT_TOOLONG             25      // optional: sum of jabber+over
#define CNT_RANGEERR            26      // optional

// special
#define CNT_ERROCTETS           31

#endif /* __MAC_DEFINES_H__ */