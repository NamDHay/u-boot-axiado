/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2022-2025 Axiado Corporation.
 */

#ifndef __AXIADO_HDMI_H__
#define __AXIADO_HDMI_H__
#include <config.h>
#include <i2c.h>
#define DDC_SEGMENT_ADDR 0x30

enum PWR_MODE {
	NORMAL,
	LOWER_PWR,
};
struct ax_hdmi_data_info {
	int vic;
	bool sink_is_hdmi;
	bool sink_has_audio;
	unsigned int enc_in_format;
	unsigned int enc_out_format;
	unsigned int colorimetry;
};

struct axiado_hdmi_i2c {
	struct i2c_adapter adap;

	u8 ddc_addr;
	u8 segment_addr;
};
struct axiado_hdmi {
	struct udevice *dev;
	struct drm_device *drm_dev;
	struct gpio_desc *ax_hdmi_pwren;

	struct clk *pclk;
	void __iomem *regs;
	void __iomem *data_reg;

	struct axiado_hdmi_i2c *i2c;
	struct i2c_adapter *ddc;

	unsigned int tmds_rate;

	struct ax_hdmi_data_info hdmi_data;
};
struct axiado_connector {
	char name[32];
	struct {
		u32 width;
		u32 height;
		bool disconnected;
	} mode_hint;
};

#define AXIADO_OUT_MODE_P888 0

#define HDMI_SCL_RATE (100 * 1000)
#define DDC_BUS_FREQ_L 0x12C
#define DDC_BUS_FREQ_H 0x130

/*HDMI Control register */
#define HDMI_SYS_CTRL 0x000
#define m_RST_ANALOG (1 << 6)
#define v_RST_ANALOG (0 << 6)
#define v_NOT_RST_ANALOG (1 << 6)
#define m_RST_DIGITAL (1 << 5)
#define v_RST_DIGITAL (0 << 5)
#define v_NOT_RST_DIGITAL (1 << 5)
#define m_REG_CLK_INV (1 << 4)
#define v_REG_CLK_NOT_INV (0 << 4)
#define v_REG_CLK_INV (1 << 4)
#define m_VCLK_INV (1 << 3)
#define v_VCLK_NOT_INV (0 << 3)
#define v_VCLK_INV (1 << 3)
#define m_REG_CLK_SOURCE (1 << 2)
#define v_REG_CLK_SOURCE_TMDS (0 << 2)
#define v_REG_CLK_SOURCE_SYS (1 << 2)
#define m_POWER (1 << 1)
#define v_PWR_ON (0 << 1)
#define v_PWR_OFF (1 << 1)
#define m_INT_POL (1 << 0)
#define v_INT_POL_HIGH 1
#define v_INT_POL_LOW 0

#define HDMI_SYS_PWR_ON 0x63
#define HDMI_SYS_PWR_LOW 0x61
#define HDMI_SYS_CTRL1 0x004
#define HDMI_SYS_CTRL2 0x008
#define HDMI_CTRL2_VAL 0x34
/* Input video format control register */
#define HDMI_VIDEO_CONTRL1 0x004
#define m_VIDEO_INPUT_FORMAT (7 << 1)
#define m_DE_SOURCE (1 << 0)
#define v_VIDEO_INPUT_FORMAT(n) (n << 1)
#define v_DE_EXTERNAL 1
#define v_DE_INTERNAL 0

#define HDMI_VDE_SRC 0x01
#define HDMI_VDE_SRC2 0x34

/* video color space register */
#define HDMI_VIDEO_CONTRL2 0x008
#define m_VIDEO_OUTPUT_COLOR (3 << 6)
#define m_VIDEO_INPUT_BITS (3 << 4)
#define m_VIDEO_INPUT_CSP (1 << 0)
#define v_VIDEO_OUTPUT_COLOR(n) (((n)&0x3) << 6)
#define v_VIDEO_INPUT_BITS(n) (n << 4)
#define v_VIDEO_INPUT_CSP(n) (n << 0)

/* Color space converter */
#define HDMI_VIDEO_CONTRL 0x010
#define m_VIDEO_AUTO_CSC (1 << 7)
#define v_VIDEO_AUTO_CSC(n) (n << 7)
#define m_VIDEO_C0_C2_SWAP (1 << 0)
#define v_VIDEO_C0_C2_SWAP(n) (n << 0)

#define HDMI_VIDEO_CONTRL3 0x010
#define m_COLOR_DEPTH_NOT_INDICATED (1 << 4)
#define m_SOF (1 << 3)
#define m_COLOR_RANGE (1 << 2)
#define m_CSC (1 << 0)
#define v_COLOR_DEPTH_NOT_INDICATED(n) ((n) << 4)
#define v_SOF_ENABLE (0 << 3)
#define v_SOF_DISABLE (1 << 3)
#define v_COLOR_RANGE_FULL (1 << 2)
#define v_COLOR_RANGE_LIMITED (0 << 2)
#define v_CSC_ENABLE 1
#define v_CSC_DISABLE 0

#define HDMI_AV_MUTE 0x014
#define m_AVMUTE_CLEAR (1 << 7)
#define m_AVMUTE_ENABLE (1 << 6)
#define m_AUDIO_MUTE (1 << 1)
#define m_VIDEO_BLACK (1 << 0)
#define v_AVMUTE_CLEAR(n) (n << 7)
#define v_AVMUTE_ENABLE(n) (n << 6)
#define v_AUDIO_MUTE(n) (n << 1)
#define v_VIDEO_MUTE(n) (n << 0)

#define HDMI_VIDEO_TIMING_CTL 0x020
#define v_HSYNC_POLARITY(n) (n << 3)
#define v_VSYNC_POLARITY(n) (n << 2)
#define v_INETLACE(n) (n << 1)
#define v_EXTERANL_VIDEO(n) (n << 0)

#define HDMI_VIDEO_EXT_HTOTAL_L 0x024
#define HDMI_VIDEO_EXT_HTOTAL_H 0x028
#define HDMI_VIDEO_EXT_HBLANK_L 0x02c
#define HDMI_VIDEO_EXT_HBLANK_H 0x030
#define HDMI_VIDEO_EXT_HDELAY_L 0x034
#define HDMI_VIDEO_EXT_HDELAY_H 0x038
#define HDMI_VIDEO_EXT_HDURATION_L 0x03c
#define HDMI_VIDEO_EXT_HDURATION_H 0x040
#define HDMI_VIDEO_EXT_VTOTAL_L 0x044
#define HDMI_VIDEO_EXT_VTOTAL_H 0x048
#define HDMI_VIDEO_EXT_VBLANK 0x04c
#define HDMI_VIDEO_EXT_VDELAY 0x050
#define HDMI_VIDEO_EXT_VDURATION 0x054

/* PLL as per 1280x720 resolution */
#define HDMI_PLL0 0x680
#define HDMI_PLL0_VAL 0x01
#define HDMI_PLL1 0x684
#define HDMI_PLL1_VAL 0x01
#define HDMI_PLL2 0x688
#define HDMI_PLL2_VAL 0xf0
#define HDMI_PLL3 0x68C
#define HDMI_PLL3_VAL 0x63
#define m_PLL3_DIVIDER_CTRL_5 (1 << 5)
#define m_PLL3_DIVIDER_CTRL_4 (1 << 4)
#define m_PLL3_DIVIDER_CTRL_3 (1 << 3)
#define m_PLL3_DIVIDER_CTRL_2 (1 << 2)
#define m_PLL3_DIVIDER_CTRL_1 (1 << 1)
#define m_PLL3_DIVIDER_CTRL_0 (1 << 0)
#define m_PLL3                                           \
	(m_PLL3_DIVIDER_CTRL_0 | m_PLL3_DIVIDER_CTRL_1 | \
	 m_PLL3_DIVIDER_CTRL_2 | m_PLL3_DIVIDER_CTRL_4 | \
	 m_PLL3_DIVIDER_CTRL_5)

#define HDMI_PLL4 0x690
#define HDMI_PLL4_VAL 0x15
#define m_PLL4_DIVIDER_CTRL_5 (1 << 5)
#define m_PLL4_DIVIDER_CTRL_3 (1 << 3)
#define m_PLL4_DIVIDER_CTRL_2 (1 << 2)
#define m_PLL4_DIVIDER_CTRL_1 (1 << 1)
#define m_PLL4_DIVIDER_CTRL_0 (1 << 0)
#define m_PLL4                                           \
	(m_PLL4_DIVIDER_CTRL_0 | m_PLL4_DIVIDER_CTRL_1 | \
	 m_PLL4_DIVIDER_CTRL_2 | m_PLL4_DIVIDER_CTRL_3)
#define m_PLL4_640                                       \
	(m_PLL4_DIVIDER_CTRL_0 | m_PLL4_DIVIDER_CTRL_1 | \
	 m_PLL4_DIVIDER_CTRL_2 | m_PLL4_DIVIDER_CTRL_3 | \
	 m_PLL4_DIVIDER_CTRL_5)

#define HDMI_PLL5 0x694
#define HDMI_PLL5_VAL 0x41
#define m_PLL5_DIVIDER_CTRL_5 (1 << 5)
#define m_PLL5_DIVIDER_CTRL_3 (1 << 3)
#define m_PLL5_DIVIDER_CTRL_2 (1 << 2)
#define m_PLL5_DIVIDER_CTRL_1 (1 << 1)
#define m_PLL5_DIVIDER_CTRL_0 (1 << 0)
#define m_PLL5                                           \
	(m_PLL5_DIVIDER_CTRL_0 | m_PLL5_DIVIDER_CTRL_1 | \
	 m_PLL5_DIVIDER_CTRL_3 | m_PLL5_DIVIDER_CTRL_5)

#define HDMI_PLL6 0x698
#define HDMI_PLL6_VAL 0x42
#define m_PLL6_DIVIDER_CTRL_5 (1 << 5)
#define m_PLL6_DIVIDER_CTRL_2 (1 << 2)
#define m_PLL6_DIVIDER_CTRL_1 (1 << 1)
#define m_PLL6 \
	(m_PLL6_DIVIDER_CTRL_1 | m_PLL6_DIVIDER_CTRL_2 | m_PLL6_DIVIDER_CTRL_5)

#define HDMI_PLL7 0x744
#define HDMI_PLL7_VAL 0x00
#define HDMI_PLL8 0x748
#define HDMI_PLL8_VAL 0x00
#define HDMI_PLL9 0x74c
#define HDMI_PLL9_VAL 0x00
#define HDMI_PLL10 0x680
#define m_PLL10_DIVIDER_CTRL_0 (1 << 0)

#define HDMI_PLL11 0x6a8
#define m_PLL11_DIVIDER_CTRL_0 (1 << 0)
#define HDMI_PLL11_VAL 0x0F

#define HDMI_PLL12 0x6ac

#define HDMI_PLL13 0x6b0
#define HDMI_PLL13_VAL 0x14
#define m_PLL13_DIVIDER_CTRL_6 (1 << 6)
#define m_PLL13_DIVIDER_CTRL_2 (1 << 2)
#define m_PLL13 (m_PLL13_DIVIDER_CTRL_2 | m_PLL13_DIVIDER_CTRL_6)

#define HDMI_PLL14 0x6b4
#define m_PLL14_DIVIDER_CTRL_2 (1 << 2)
#define HDMI_PLL14_VAL 0x09

#define HDMI_PRE_LOCK 0x6a4
#define HDMI_POST_LOCK 0x6bc

/*Setting LDO registers */
#define HDMI_LDO 0x6d0
#define m_LDO_CTRL_DATA_CHANEEL2 (1 << 2)
#define m_LDO_CTRL_DATA_CHANEEL1 (1 << 1)
#define m_LDO_CTRL_DATA_CHANEEL0 (1 << 0)
#define m_HDMI_LDO                                             \
	(m_LDO_CTRL_DATA_CHANEEL2 | m_LDO_CTRL_DATA_CHANEEL1 | \
	 m_LDO_CTRL_DATA_CHANEEL0)

#define HDMI_SERIAL 0x6f8
#define m_SERIAL_CTRL_DATA_CHANEEL2 (1 << 6)
#define m_SERIAL_CTRL_DATA_CHANEEL1 (1 << 5)
#define m_SERIAL_CTRL_DATA_CHANEEL0 (1 << 4)
#define m_HDMI_SERIAL                                                \
	(m_SERIAL_CTRL_DATA_CHANEEL2 | m_SERIAL_CTRL_DATA_CHANEEL1 | \
	 m_SERIAL_CTRL_DATA_CHANEEL0)

#define HDMI_SYS_DEF1 0x6FC
#define HDMI_SYS_DEF2 0x700
#define HDMI_SYS_DEF3 0x6D4
#define HDMI_SYS_DEF4 0x6CC
#define HDMI_SYS_DEF5 0x6EC
#define HDMI_SYS_RESET1 0x0714
#define HDMI_SYS_RESET2 0x0720

#define HDMI_RESET_VAL 0x00
#define HDMI_RESET_VAL1 0x01
#define HDMI_RESET_VAL2 0x1F
#define HDMI_REG_OFFSET 0x04


/* sEtting HDMI VBIST */
#define HDMI_VBIST 0x324
#define VBIST_DISABLE 0x00
#define VBIST_ENABLE 0x01

#define HDMI_VIDEO_CSC_COEF 0x18

#define HDMI_AUDIO_CTRL1 0x0d4

#define v_CTS_SOURCE(n) (n << 7)

#define v_DOWN_SAMPLE(n) (n << 5)

#define v_AUDIO_SOURCE(n) (n << 3)

#define v_MCLK_ENABLE(n) (n << 2)

#define v_MCLK_RATIO(n) (n)

#define AUDIO_SAMPLE_RATE 0x0dc

#define v_I2S_CHANNEL(n) ((n) << 2)

#define v_I2S_MODE(n) (n)

#define AUDIO_I2S_MAP 0x0e4
#define AUDIO_I2S_SWAPS_SPDIF 0x0e8
#define v_SPIDF_FREQ(n) (n)

#define N_32K 0x1000
#define N_441K 0x1880
#define N_882K 0x3100
#define N_1764K 0x6200
#define N_48K 0x1800
#define N_96K 0x3000
#define N_192K 0x6000

#define HDMI_AUDIO_CHANNEL_STATUS 0x3e
#define m_AUDIO_STATUS_NLPCM (1 << 7)
#define m_AUDIO_STATUS_USE (1 << 6)
#define m_AUDIO_STATUS_COPYRIGHT (1 << 5)
#define m_AUDIO_STATUS_ADDITION (3 << 2)
#define m_AUDIO_STATUS_CLK_ACCURACY (2 << 0)
#define v_AUDIO_STATUS_NLPCM(n) ((n & 1) << 7)
#define AUDIO_N_H 0x0fc
#define AUDIO_N_M 0x100
#define AUDIO_N_L 0x101

#define HDMI_AUDIO_CTS_H 0x114
#define HDMI_AUDIO_CTS_M 0x118
#define HDMI_AUDIO_CTS_L 0x11c

#define HDMI_DDC_CLK_L 0x12c
#define HDMI_DDC_CLK_H 0x130

#define HDMI_EDID_SEGMENT_POINTER 0x134
#define HDMI_EDID_WORD_ADDR 0x138
#define HDMI_EDID_FIFO_OFFSET 0x13c
#define HDMI_EDID_FIFO_ADDR 0x140

#define HDMI_PACKET_SEND_MANUAL 0x270
#define HDMI_PACKET_SEND_AUTO 0x278
#define m_PACKET_GCP_EN (1 << 7)
#define m_PACKET_MSI_EN (1 << 6)
#define m_PACKET_SDI_EN (1 << 5)
#define m_PACKET_VSI_EN (1 << 4)
#define v_PACKET_GCP_EN(n) ((n & 1) << 7)
#define v_PACKET_MSI_EN(n) ((n & 1) << 6)
#define v_PACKET_SDI_EN(n) ((n & 1) << 5)
#define v_PACKET_VSI_EN(n) ((n & 1) << 4)

#define HDMI_CONTROL_PACKET_BUF_INDEX 0x27c
enum {
	INFOFRAME_VSI = 0x05,
	INFOFRAME_AVI = 0x06,
	INFOFRAME_AAI = 0x08,
};

#define HDMI_CONTROL_PACKET_ADDR 0x280
#define HDMI_MAXIMUM_INFO_FRAME_SIZE 9
#define m_HDMI_DVI (1 << 1)
#define v_HDMI_DVI(n) (n << 1)

#define HDMI_INTERRUPT_MASK1 0x300
#define HDMI_INTERRUPT_STATUS1 0x304
#define m_INT_ACTIVE_VSYNC (1 << 5)
#define m_INT_EDID_READY (1 << 2)

#define HDMI_INTERRUPT_MASK2 0x308
#define HDMI_INTERRUPT_STATUS2 0x30c
#define m_INT_HDCP_ERR (1 << 7)
#define m_INT_BKSV_FLAG (1 << 6)
#define m_INT_HDCP_OK (1 << 4)

#define HDMI_STATUS 0x320
#define m_HOTPLUG (1 << 7)
#define m_MASK_INT_HOTPLUG (1 << 5)
#define m_INT_HOTPLUG (1 << 1)
#define v_MASK_INT_HOTPLUG(n) ((n & 0x1) << 5)

#define HDMI_COLORBAR 0x324

#define HDMI_TMDS_SYNC 0x338
#define HDMI_TMDS_SYS_CTL 0x6c8
#define m_TMDS_CLK_SOURCE (1 << 5)
#define v_TMDS_FROM_PLL (0 << 5)
#define v_TMDS_FROM_GEN (1 << 5)
#define m_PHASE_CLK (1 << 4)
#define v_DEFAULT_PHASE (0 << 4)
#define v_SYNC_PHASE (1 << 4)
#define m_TMDS_CURRENT_PWR (1 << 3)
#define v_TURN_ON_CURRENT (0 << 3)
#define v_CAT_OFF_CURRENT (1 << 3)
#define m_BANDGAP_PWR (1 << 2)
#define v_BANDGAP_PWR_UP (0 << 2)
#define v_BANDGAP_PWR_DOWN (1 << 2)
#define m_PLL_PWR (1 << 1)
#define v_PLL_PWR_UP (0 << 1)
#define v_PLL_PWR_DOWN (1 << 1)
#define m_TMDS_CHG_PWR (1 << 0)
#define v_TMDS_CHG_PWR_UP (0 << 0)
#define v_TMDS_CHG_PWR_DOWN (1 << 0)
#define TMDS_SYNC_VAL0 0x00
#define TMDS_SYNC_VAL1 0x01
#define TMDS_SYNC_VAL2 0x80

#define HDMI_PHY_CHG_PWR 0xe1
#define v_CLK_CHG_PWR(n) ((n & 1) << 3)
#define v_DATA_CHG_PWR(n) ((n & 7) << 0)

#define HDMI_PHY_DRIVER 0xe2
#define v_CLK_MAIN_DRIVER(n) (n << 4)
#define v_DATA_MAIN_DRIVER(n) (n << 0)

#define HDMI_PHY_PRE_EMPHASIS 0xe3
#define v_PRE_EMPHASIS(n) ((n & 7) << 4)
#define v_CLK_PRE_DRIVER(n) ((n & 3) << 2)
#define v_DATA_PRE_DRIVER(n) ((n & 3) << 0)

#define HDMI_PHY_FEEDBACK_DIV_RATIO_LOW 0xe7
#define v_FEEDBACK_DIV_LOW(n) (n & 0xff)
#define HDMI_PHY_FEEDBACK_DIV_RATIO_HIGH 0xe8
#define v_FEEDBACK_DIV_HIGH(n) (n & 1)

#define HDMI_PHY_PRE_DIV_RATIO 0xed
#define v_PRE_DIV_RATIO(n) (n & 0x1f)

#define HDMI_CEC_CTRL 0x340
#define m_ADJUST_FOR_HISENSE (1 << 6)
#define m_REJECT_RX_BROADCAST (1 << 5)
#define m_BUSFREETIME_ENABLE (1 << 2)
#define m_REJECT_RX (1 << 1)
#define m_START_TX (1 << 0)

#define HDMI_CEC_DATA 0x344
#define HDMI_CEC_TX_OFFSET 0x348
#define HDMI_CEC_RX_OFFSET 0x34c
#define HDMI_CEC_CLK_H 0x350
#define HDMI_CEC_CLK_L 0x354
#define HDMI_CEC_TX_LENGTH 0x358
#define HDMI_CEC_RX_LENGTH 0x35c
#define HDMI_CEC_TX_INT_MASK 0x360
#define m_TX_DONE (1 << 3)
#define m_TX_NOACK (1 << 2)
#define m_TX_BROADCAST_REJ (1 << 1)
#define m_TX_BUSNOTFREE (1 << 0)

#define HDMI_CEC_RX_INT_MASK 0x364
#define m_RX_LA_ERR (1 << 4)
#define m_RX_GLITCH (1 << 3)
#define m_RX_DONE (1 << 0)

#define HDMI_CEC_TX_INT 0x368
#define HDMI_CEC_RX_INT 0x36c
#define HDMI_CEC_BUSFREETIME_L 0x370
#define HDMI_CEC_BUSFREETIME_H 0x374
#define HDMI_CEC_LOGICADDR 0x378

/* Setting BIAS current registers */
#define HDMI_BIAS_CIRCUIT 0x6C0
#define m_BIAS_CONTROL (1 << 2)
#define HDMI_BIAS_DEF 0x08

#define HDMI_RX_SENSE 0x730
#define m_RX_SENSE_CLOCK_CHANNEL (1 << 3)
#define m_RX_SENSE_DATA_CHANNEL_2 (1 << 2)
#define m_RX_SENSE_DATA_CHANNEL_1 (1 << 1)
#define m_RX_SENSE_DATA_CHANNEL_0 (1 << 0)
#define m_HDMI_RX_SENSE                                         \
	(m_RX_SENSE_CLOCK_CHANNEL | m_RX_SENSE_DATA_CHANNEL_2 | \
	 m_RX_SENSE_DATA_CHANNEL_1 | m_RX_SENSE_DATA_CHANNEL_0)

/* HDMI video information code(VIC) formats */
#define VIC_FMT_720x480i4_3 6
#define VIC_FMT_720x480i16_9 7
#define VIC_FMT_720x576i4_3 21
#define VIC_FMT_720x576i16_9 22
#define VIC_FMT_720x480p4_3 2
#define VIC_FMT_720x480p16_9 3
#define VIC_FMT_720x576p4_3 17
#define VIC_FMT_720x576p16_9 18

#define gpio_load 346
#define gpio_disp 347

/* Macros for the Control Path */
#define AX_HDMI_EDID_REG_OFFSET 0x00 /* EDID Register offset */

#endif /* __AXIADO_HDMI_H__ */
