#ifndef __HDMI_H__
#define __HDMI_H__

#define AX3000_CSR_BASE_ADRS_DSUB_LOCAL  (void __iomem *)AX3000_CSR_BASE_ADRS_DSUB + 0x1C00llu
#define AX3000_CSR_BASE_ADRS_DSUB_DPHY   (void __iomem *)AX3000_CSR_BASE_ADRS_DSUB + 0x1800llu
#define AX3000_CSR_BASE_ADRS_DSUB_HDMI   (void __iomem *)AX3000_CSR_BASE_ADRS_DSUB + 0x1000llu
#define AX3000_CSR_BASE_ADRS_DSUB_DSI2   (void __iomem *)AX3000_CSR_BASE_ADRS_DSUB

#define FORMAT16
 
#ifdef FORMAT1
  #define TB_HDMITX_8BIT_25P2M
  #define VIC 0x01
#else
  #ifdef FORMAT4
    #define TB_HDMITX_8BIT_74P25M
    #define VIC 0x04
  #else  // FORMAT16
    #define TB_HDMITX_8BIT_148P5M
    #define VIC 0x10
  #endif
#endif
 
/**
 * @brief Power on relate register of INNO HDMI1.4 TRANSMITTER
 *
 */
#define HDMI_VBIST        (0xc9 << 2)
 
#define HDMI_SYNC_REG     (0x0ce << 2)
#define HDMI_PLL0         (0x1a0 << 2)
#define HDMI_PLL11        (0x1aa << 2)
#define HDMI_LDO          (0x1b4 << 2)
#define HDMI_BIAS_CIRCUIT (0x1b0 << 2)
#define HDMI_SERIAL       (0x1be << 2)
#define HDMI_RX_SENSE     (0x1cc << 2)
#define HDMI_RX_SENSE_STATUS      (0x1cd << 2)
#define HDMI_TMDS_SYS_CTL (0x1b2 << 2)
 
/*HDMI Control register */
#define HDMI_SYS_CTRL           0x000
#define m_RST_ANALOG            (1 << 6)
#define v_RST_ANALOG            (0 << 6)
#define v_NOT_RST_ANALOG        (1 << 6)
#define m_RST_DIGITAL           (1 << 5)
#define v_RST_DIGITAL           (0 << 5)
#define v_NOT_RST_DIGITAL       (1 << 5)
#define m_REG_CLK_INV           (1 << 4)
#define v_REG_CLK_NOT_INV       (0 << 4)
#define v_REG_CLK_INV           (1 << 4)
#define m_VCLK_INV          (1 << 3)
#define v_VCLK_NOT_INV          (0 << 3)
#define v_VCLK_INV          (1 << 3)
#define m_REG_CLK_SOURCE        (1 << 2)
#define v_REG_CLK_SOURCE_TMDS       (0 << 2)
#define v_REG_CLK_SOURCE_SYS        (1 << 2)
#define m_POWER             (1 << 1)
#define v_PWR_ON            (0 << 1)
#define v_PWR_OFF           (1 << 1)
#define m_INT_POL           (1 << 0)
#define v_INT_POL_HIGH          1
#define v_INT_POL_LOW           0
 
#define HDMI_SYS_PWR_ON                 0x63
#define HDMI_SYS_PWR_LOW                0x61
 
//------------------------------------------------//
/**< setting the required bit control display controller */
#define DB9000_CR1      (0x000) /**< LCD Controller Control Register 1 */
#define DB9000_HTR      (0x008) /**< Horizontal Timing Register */
#define DB9000_VTR1     (0x00C) /**< Vertical Timing Register 1 */
#define DB9000_VTR2     (0x010) /**< Vertical Timing Register 2 */
#define DB9000_PCTR     (0x014) /**< Pixel Clock Timing Register */
#define DB9000_ISR      (0x018) /**< Interrupt Status Register */
#define DB9000_IMR      (0x01C) /**< Interrupt Mask Register */
#define DB9000_IVR      (0x020) /**< Interrupt Vector Register */
#define DB9000_ISCR     (0x024) /**< Interrupt Scan Compare Register */
#define DB9000_DBAR     (0x028) /**< DMA Base Address Register */
#define DB9000_DEAR     (0x030) /**< DMA End Address Register */
#define DB9000_HVTER    (0x044) /**< DMA Horizontal and Vertical Timing Extension Register */
#define DB9000_CIR1     (0x1F8) /**< Core Identification Register-1 */
#define DB9000_DFBAR    (0x03C) /**< DMA Frame Descriptor Branch Address Register */
#define DB9000_DFLAR    (0x040) /**< DMA Frame Descriptor Last Address Register */
#define DB9000_MRR      (0xFFC) /**< Multiple Memory Reads Request Register  */
 
//----------------------------------------------//
/* PLL registers */
#define PLL_PRE_REG                 (0x680) /**< HDMI pre pll setting Registers */
#define PLL_PRE_DIV_CTRL            (0x684) /**< HDMI Pre-PLL pre-div divider control Registers */
#define PLL_PRE_SSC_CTRL            (0x688) /**< HDMI Pre-PLL SSC Mode control Registers */
#define PLL_PRE_FDBK_DIV            (0x68c) /**< HDMI Pre-PLL Feedback Divider Registers */
#define PLL_PRE_TMDS_CLK_DIV        (0x690) /**< HDMI Pre-PLL TMDS and Link Clock Divider Registers */
#define PLL_PRE_MAIN_AUX_CLK_DIV    (0x694) /**< HDMI Pre-PLL Main and Aux Clock Divider Registers */
#define PLL_PRE_PIXEL_CLK_DIV       (0x698) /**< HDMI Pre-PLL Repeat and Pixel Clock Divider Registers */
#define PLL_PRE_LOCK_STAT_REG       (0x6a4) /**< HDMI Pre-PLL lock Status Registers */
#define PLL_POST_REG                (0x6a8) /**< HDMI post pll setting Registers */
#define PLL_POST_DIV_PRE_CTRL       (0x6ac) /**< HDMI Post-PLL pre-div divider control Registers */
#define PLL_POST_FDBK_DIV           (0x6b0) /**< HDMI Post-PLL feedback divider Registers */
#define PLL_POST_OUTPUT_CLK_DIV     (0x6b4) /**< HDMI Post-PLL output clock divider Registers */
#define PLL_POST_LOCK_STAT_REG      (0x6bc) /**< HDMI Post-PLL lock status Registers */
#define PLL_PRE_FRACTIONAL_DIV_H    (0x744) /**< HDMI Pre-PLL fractional divider register (23:16 bits of fractional divider) */
#define PLL_PRE_FRACTIONAL_DIV_M    (0x748) /**< HDMI Pre-PLL fractional divider register (15:8 bits of fractional divider) */
#define PLL_PRE_FRACTIONAL_DIV_L    (0x74c) /**< HDMI Pre-PLL fractional divider register (7:0 bits of fractional divider) */
 
//-------------------------------------------------------------------------------------------------------------------------------//
/* PLL register values for 1080P*/
 
#define PLL_PRE_TMDS_CLK_DIV_VAL_1080P     0x15 /**< Value to set the Link TMDS clocks for 1080p*/
#define PLL_POST_FDBK_VAL_1080P            0x0A /**< Post-PLL feedback divider value */
#define PLL_POST_OUTPUT_CLK_VAL_1080P      0x08 /**< Post-PLL output clock divider value */
#define PLL_PRE_PIXEL_CLK_DIV_VAL_1080P    0x42 /**< Pre-PLL Repeat and Pixel Clock Divider value*/
 
/* PLL register values for 480P*/
 
#define PLL_PRE_TMDS_CLK_DIV_VAL_480P     0x15 /**< Value to set the Link TMDS clocks for 1080p*/
#define PLL_POST_FDBK_VAL_480P            0x0A /**< Post-PLL feedback divider value */
#define PLL_POST_OUTPUT_CLK_VAL_480P      0x08 /**< Post-PLL output clock divider value */
#define PLL_PRE_PIXEL_CLK_DIV_VAL_480P    0x42 /**< Pre-PLL Repeat and Pixel Clock Divider value*/
 
/* PLL register values */
#define PLL_PRE_REG_SET              0x00 /**< setting Pre PLL */
#define PLL_PRE_REG_CLEAR            0x02 /**< clearing Pre PLL */
#define PLL_PRE_DIV                  0x01 /**< Value to set the Pre-PLL divider value to 1 */
#define PLL_PRE_POWER_DOWN           0x01 /**< Value for the Pre-PLL Power Down mode */
#define PLL_PRE_SSC_MODE             0xf0 /**< Value to set the SSC mode */
#define PLL_PRE_FDBK_VAL             0x63 /**< Pre-PLL feedback value */
#define PLL_PRE_TMDS_CLK_DIV_VAL     0x1a /**< Value to set the Link TMDS clocks */
#define PLL_PRE_MAIN_AUX_CLK_DIV_VAL 0x41 /**< Pre-PLL Main and Aux Clock Divider values */
 
#define PLL_PRE_PIXEL_CLK_DIV_VAL    0x64 /**< Pre-PLL Repeat and Pixel Clock Divider value*/
 
#define PLL_PRE_FRACTIONAL_DIV_VAL   0x0 /**< Pre-PLL fractional divider value */
#define PLL_POST_REG_SET             0x0e /**< setting Post PLL */
#define PLL_POST_REG_CLEAR           0x02 /**< clearing Post PLL */
#define PLL_POST_DIV_PRE_VAL         0x01 /**< Post-PLL pre-div divider value */
#define PLL_POST_FDBK_VAL            0x14 /**< Post-PLL feedback divider value */
#define PLL_POST_OUTPUT_CLK_VAL      0x09 /**< Post-PLL output clock divider value */
 
//-------------------------------------------------------------------------------------------------------------------------------//
 
#define LOCKED_STATE                 0x01
 
#define LDO_TURN_REG    (0x6d0) /**< HDMI LDO turn on register address */
 
#define SERIALIZER_REG  (0x6f8) /**< HDMI serializer setting Registers */
 
/* LDO register values */
#define LDO_TURN_REG_ON  0x07 /**< turning on LDO */
#define LDO_TURN_REG_OFF 0x00 /**< turning off LDO */
 
/* Serializer register values */
#define SERIALIZER_SET_REG   0x71 /**< setting serializer on */
#define SERIALIZER_CLEAR_REG 0x00 /**< setting serializer off */
 
#define RX_SENSE_DET (0x730) /**< HDMI rx sense detect Registers */
 
/* Rxsense register values */
#define RX_SENSE_DET_YES 0x0f /**< writing to rx sense detect */
#define RX_SENSE_DET_NO  0x00 /**< clearing rx sense detect */
 
#define BIAS_CKT    (0x6c0) /**< HDMI turn on bias circuit Registers */
 
/* Current Bias register values */
#define TURN_ON_BIAS_CKT  0x0c /**< turning on bias ckt */
#define TURN_OFF_BIAS_CKT 0x00 /**< turning off bias ckt */
 
/**< HDMI Video Input Configuration register values */
#define IN_VIDEO_RGB_Y4_4_4   0x00 /**< To set the input video format as RGB and YCbCr 4:4:4 */
#define IN_VIDEO_Y4_2_2       0x01 /**< To set the input video format as YCbCr 4:2:2 */
#define VIDEO_FORMAT          0x34 /**< Register set values for 1280x720 Resolution*/
#define AVI_INFO_HB0          0x82 /**< HB0, HB1, HB3 Values for Video Input Configuration for Sink Device */
#define AVI_INFO_HB1          0x02
#define AVI_INFO_HB3          0x0d
#define AVI_INFO_PB0          0x00 /**< PB0, PB1, PB2, PB3 Values for Video Input Configuration for Sink Device */
#define AVI_INFO_PB1          0x00
#define AVI_INFO_PB2          0x08
#define AVI_INFO_PB3          0x70
#define AVI_INFO_VID_680_480p   0x01  // 640x480p @ 59.94  /60Hz
#define AVI_INFO_VID_1280_720   0x04  // 1280x720p @ 59.94 /60Hz
#define AVI_INFO_VID_1920_1080  0x10  // 1920x1080p @ 59.94/60Hz
#define AVI_INFO_PB5          0x40 /**< PB5 Value for Video Input Configuration for Sink Device */
#define TMDS_SYNC_DISABLE     0x00 /**< Value to disable the TMDS sync */
#define TMDS_SYNC_ENABLE      0x01 /**< Value to enable the TMDS sync */
 
/**< HDMI Video Input Configuration registers */
#define IN_VIDEO_FORMAT  (0x004) /**< Input video format control */
#define VIDEO_FORMAT_REG (0x008) /**< Video input and output format */
#define REG_HB0          (0x280) /**< HB0, HB1, HB3 (Video Input Configuration for Sink Device) */
#define REG_HB1          (0x284)
#define REG_HB3          (0x288)
#define REG_PB0          (0x28c) /**< PB0-PB5 (Video Input Configuration for Sink Device) */
#define REG_PB1          (0x290)
#define REG_PB2          (0x294)
#define REG_PB3          (0x298)
#define REG_PB4          (0x29c) /**< Register to configure the video resolution. Write 0x04 for 1280x720 mode */
#define REG_PB5          (0x2a0)
#define TMDS_SYNC_REG    (0x338) /**< Register for the manual synchronization of TMDS channel */
 
#define PKT_OFFSET_REG   (0x27c) /* HDMI Packet Offset registers */
 
/* HDMI Packet Offset register values */
#define PKT_GENERIC              0x00 /**< Generic packet */
#define PKT_ACP                  0x01 /**< ACP packet */
#define PKT_ISRC1                0x02 /**< ISRC1 packet */
#define PKT_ISRC2                0x03 /**< ISRC2 packet */
#define PKT_GAMUT_METADATA       0x04 /**< Gamut metadata packet */
#define PKT_VENDOR_SPECIFIC_IF   0x05 /**< Vendor specific InfoFrame */
#define PKT_AVI_IF               0x06 /**< AVI InfoFrame */
#define PKT_SRC_PROCUCT_DESCR_IF 0x07 /**< Source productdescriptorInfoFramepacket */
#define PKT_AUDIO_IF             0x08 /**< Audio InfoFrame packet */
#define PKT_MPEG_SRC_IF          0x09 /**< MPEG source InfoFrame */
 
#define DRIVER_TURN_STAT (0x6c8) /**< HDMI Driver turn off/on Registers */
 
/* HDMI driver enable/disable register values */
#define DRIVER_ENABLE        0x8f /**< Value to enable the HDMI Driver */
#define DRIVER_DISABLE       0x00 /**< Value to disable the HDMI Driver */
#define DRIVER_TURN_STAT_OFF 0x00 /**< turn off driver ckt */
//----------------------------------------------------------------------------//
#define CLR_REG_PTRN 0x00000000 /**< Default Register Pattern */
 
#define CR1_SAU_SET     BIT(26) /**< Simultaneous Address Register Update Enable */
 
#define CR1_HSS_SET(x) ((x) << 24) /**< Horizontal Sync Shift */
#define CR1_DFE_SET     BIT(23) /**DMA Frame Descriptor Enable */
#define CR1_DFB_SET     BIT(22) /**DMA Frame Descriptor Branch Address Enable*/
 
#define CR1_FBP_SET    BIT(19) /**<  Frame Buffer 24 bpp Packed Word*/
#define CR1_FDW_SET(x) ((x) << 16) /**< FIFO DMA Request Words */
#define CR1_DEP_SET    BIT(8) /**< Data Enable Polarity */
#define CR1_BPP_SET(x) ((x) << 2) /**< LCD Bits Per Pixel */
#define CR1_LPE_SET    BIT(1) /**< LCD Power Enable */
#define CR1_LCE_SET    BIT(0) /**< LCD Controller Enable */

#endif
