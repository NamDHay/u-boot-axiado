/* SPDX-License-Identifier: GPL-2.0-or-later */

/* Copyright (c) 2021-25 Axiado Corporation (or its affiliates). All rights reserved.
 * Use, modification and redistribution of this file is subject to your possession
 * of a valid End User License Agreement (EULA) for the Axiado Product of which
 * these sources are part of and your compliance with all applicable terms and
 * conditions of such licence agreement.
 */

/* ax3000 sgpio B0_512bits_FPGA register offset */
#define REG_SGPIO_SLICE_MUX_CFG0_ADRS_OFFSET_512	0x000
#define REG_SGPIO_PRESET0_ADRS_OFFSET_512		0x1dc
#define REG_SGPIO_COUNT0_ADRS_OFFSET_512		0x1f0
#define REG_SGPIO_POS0_ADRS_OFFSET_512			0x204

#define REG_SGPIO_SLICE_MUX_CFG1_ADRS_OFFSET_512	0x004
#define REG_SGPIO_REG_LD_ADRS_OFFSET_512		0x014
#define REG_SGPIO_REG_SS_LD_W_ADRS_OFFSET_512		0x0d8
#define REG_SGPIO_PRESET1_ADRS_OFFSET_512		0x1e0
#define REG_SGPIO_COUNT1_ADRS_OFFSET_512		0x1f4
#define REG_SGPIO_POS1_ADRS_OFFSET_512			0x208

#define REG_SGPIO_SLICE_MUX_CFG2_ADRS_OFFSET_512	0x008
#define REG_SGPIO_REG_DO_ADRS_OFFSET_512		0x054
#define REG_SGPIO_REG_SS_DO_ADRS_OFFSET_512		0x158
#define REG_SGPIO_PRESET2_ADRS_OFFSET_512		0x1e4
#define REG_SGPIO_COUNT2_ADRS_OFFSET_512		0x1f8
#define REG_SGPIO_POS2_ADRS_OFFSET_512			0x20c

#define REG_SGPIO_SLICE_MUX_CFG3_ADRS_OFFSET_512	0x00c
#define REG_SGPIO_PRESET3_ADRS_OFFSET_512		0x1e8
#define REG_SGPIO_COUNT3_ADRS_OFFSET_512		0x1fc
#define REG_SGPIO_POS3_ADRS_OFFSET_512			0x210

#define REG_SGPIO_SLICE_MUX_CFG4_ADRS_OFFSET_512	0x010
#define REG_SGPIO_REG_OE_ADRS_OFFSET_512		0x0d4
#define REG_SGPIO_REG_SS_OE_ADRS_OFFSET_512		0x1d8
#define REG_SGPIO_PRESET4_ADRS_OFFSET_512		0x1ec
#define REG_SGPIO_COUNT4_ADRS_OFFSET_512		0x200
#define REG_SGPIO_POS4_ADRS_OFFSET_512			0x214

#define REG_SGPIO_MASK_ADRS_OFFSET_512			0x224
#define REG_SGPIO_CTRL_ENABLE_ADRS_OFFSET_512		0x218
#define REG_SGPIO_CTRL_ENABLE_POS_ADRS_OFFSET_512	0x21c

#define REG_SGPIO_REG_SS_DI_ADRS_OFFSET_512		0x198

#define REG_SGPIO_STATUS_ADRS_OFFSET_512		0x228


/* ax3000 sgpio B0 128 bits_FPGA register offset */
#define REG_SGPIO_SLICE_MUX_CFG0_ADRS_OFFSET_128            0x000
#define REG_SGPIO_PRESET0_ADRS_OFFSET_128                   0x08c
#define REG_SGPIO_COUNT0_ADRS_OFFSET_128                    0x0a0
#define REG_SGPIO_POS0_ADRS_OFFSET_128                      0x0b4

#define REG_SGPIO_SLICE_MUX_CFG1_ADRS_OFFSET_128            0x004
#define REG_SGPIO_REG_LD_ADRS_OFFSET_128                    0x014
#define REG_SGPIO_REG_SS_LD_W_ADRS_OFFSET_128               0x048
#define REG_SGPIO_PRESET1_ADRS_OFFSET_128                   0x090
#define REG_SGPIO_COUNT1_ADRS_OFFSET_128                    0x0a4
#define REG_SGPIO_POS1_ADRS_OFFSET_128                      0x0b8

#define REG_SGPIO_SLICE_MUX_CFG2_ADRS_OFFSET_128            0x008
#define REG_SGPIO_REG_DO_ADRS_OFFSET_128                    0x024
#define REG_SGPIO_REG_SS_DO_ADRS_OFFSET_128                 0x068
#define REG_SGPIO_PRESET2_ADRS_OFFSET_128                   0x094
#define REG_SGPIO_COUNT2_ADRS_OFFSET_128                    0x0a8
#define REG_SGPIO_POS2_ADRS_OFFSET_128                      0x0bc

#define REG_SGPIO_SLICE_MUX_CFG3_ADRS_OFFSET_128            0x00c
#define REG_SGPIO_PRESET3_ADRS_OFFSET_128                   0x098
#define REG_SGPIO_COUNT3_ADRS_OFFSET_128                    0x0ac
#define REG_SGPIO_POS3_ADRS_OFFSET_128                      0x0c0

#define REG_SGPIO_SLICE_MUX_CFG4_ADRS_OFFSET_128            0x010
#define REG_SGPIO_REG_OE_ADRS_OFFSET_128                    0x044
#define REG_SGPIO_REG_SS_OE_ADRS_OFFSET_128                 0x088
#define REG_SGPIO_PRESET4_ADRS_OFFSET_128                   0x09c
#define REG_SGPIO_COUNT4_ADRS_OFFSET_128                    0x0b0
#define REG_SGPIO_POS4_ADRS_OFFSET_128                      0x0c4

#define REG_SGPIO_MASK_ADRS_OFFSET_128                      0x0d4
#define REG_SGPIO_CTRL_ENABLE_ADRS_OFFSET_128               0x0c8
#define REG_SGPIO_CTRL_ENABLE_POS_ADRS_OFFSET_128           0x0cc

#define REG_SGPIO_REG_SS_DI_ADRS_OFFSET_128                 0x078

#define REG_SGPIO_STATUS_ADRS_OFFSET_128                    0x0d8

#define MASK_CLR			0x0
#define MASK_EN				0xDFFF
#define STATUS_SHIFT		16
#define OFFSET(bank, pos)	(pos + (bank * 32))
#define GET_BANK(a)			(a / 32)
#define GET_POS(a)			(a - (32 * GET_BANK(a)))
#define SET_TILL_POS(reg, pos)		((reg) |= ((1 << (pos)) - 1))
#define CLEAR_BITS_UP_TO(reg, pos)	((reg) &= ~((1U << (pos)) - 1))
#define IS_BIT_SET(reg, bit_position)	((reg  & (1 << bit_position)) != 0)

/* Below values are the maximum for the current design */
#define MAX_SGPIO_PINS		512
#define MAX_OFFSET_REG		16
#define MAX_SLICE_COUNT		4

#define NUM_WORDS(bits)   (((bits) + 31) / 32)
#define START_OFFSET(bits)   (16 - NUM_WORDS(bits))
#define END_OFFSET(bits)     (15)

struct ax3000_sgpio {
	uint32_t preset_value;
	uint32_t count_value;
	int bus_frequency;
	int apb_frequency;
	uint32_t pos;
	uint32_t pos_reset;
	uint32_t pos_reg;
	int ngpios;
	int max_sgpio_pins;
	int max_offset_regs;
	int fpga_flag;
	uint32_t slice_enable;
	uint32_t mask_status[MAX_SGPIO_PINS]; /* 1 - Mask & 0 - Unmask */
	uint32_t irq_number[MAX_SGPIO_PINS]; /* To store the irq number of each bit */
	void __iomem *membase;
	struct sgpio_reg_offsets *regs;
};

/* Register access functions */
static inline u32 sgpio_reg_read(struct ax3000_sgpio *sgpio, u32 offset)
{
	u32 val = 0;

	if (sgpio->membase)
		val = readl(sgpio->membase + offset);

	return val;
}

static inline void sgpio_reg_write(struct ax3000_sgpio *sgpio, u32 offset,
				   u32 val)
{
	if (sgpio->membase)
		writel(val, sgpio->membase + offset);

}

struct ax3000_slice_info {
	uint32_t out_mux;
	uint32_t sgpio_mux;
	uint32_t slice_mux;
	uint32_t reg[MAX_OFFSET_REG];
	uint32_t reg_ss[MAX_OFFSET_REG];
	uint32_t preset;
	uint32_t count;
	uint32_t pos;
};

struct sgpio_reg_offsets {
	uint32_t  slice_mux_0;
	uint32_t  slice_preset_0;
	uint32_t  slice_count_0;
	uint32_t  slice_pos_0;
	uint32_t  slice_mux_1;
	uint32_t  slice_ld;
	uint32_t  slice_ld_ss;
	uint32_t  slice_preset_1;
	uint32_t  slice_count_1;
	uint32_t  slice_pos_1;
	uint32_t  slice_mux_2;
	uint32_t  slice_dout;
	uint32_t  slice_dout_ss;
	uint32_t  slice_preset_2;
	uint32_t  slice_count_2;
	uint32_t  slice_pos_2;
	uint32_t  slice_mux_3;
	uint32_t  slice_preset_3;
	uint32_t  slice_count_3;
	uint32_t  slice_pos_3;
	uint32_t  slice_mux_4;
	uint32_t  slice_oe;
	uint32_t  slice_oe_ss;
	uint32_t  slice_preset_4;
	uint32_t  slice_count_4;
	uint32_t  slice_pos_4;
	uint32_t  slice_mask;
	uint32_t  slice_ctrl_en;
	uint32_t  slice_ctrl_en_pos;
	uint32_t  slice_din_ss;
	uint32_t  slice_status;
};

static const struct sgpio_reg_offsets design_512_bit_regs = {
	.slice_mux_0 = REG_SGPIO_SLICE_MUX_CFG0_ADRS_OFFSET_512,
	.slice_preset_0 = REG_SGPIO_PRESET0_ADRS_OFFSET_512,
	.slice_count_0 = REG_SGPIO_COUNT0_ADRS_OFFSET_512,
	.slice_pos_0 = REG_SGPIO_POS0_ADRS_OFFSET_512,
	.slice_mux_1 = REG_SGPIO_SLICE_MUX_CFG1_ADRS_OFFSET_512,
	.slice_ld = REG_SGPIO_REG_LD_ADRS_OFFSET_512,
	.slice_ld_ss = REG_SGPIO_REG_SS_LD_W_ADRS_OFFSET_512,
	.slice_preset_1 = REG_SGPIO_PRESET1_ADRS_OFFSET_512,
	.slice_count_1 = REG_SGPIO_COUNT1_ADRS_OFFSET_512,
	.slice_pos_1 = REG_SGPIO_POS1_ADRS_OFFSET_512,
	.slice_mux_2 = REG_SGPIO_SLICE_MUX_CFG2_ADRS_OFFSET_512,
	.slice_dout = REG_SGPIO_REG_DO_ADRS_OFFSET_512,
	.slice_dout_ss = REG_SGPIO_REG_SS_DO_ADRS_OFFSET_512,
	.slice_preset_2 = REG_SGPIO_PRESET2_ADRS_OFFSET_512,
	.slice_count_2 = REG_SGPIO_COUNT2_ADRS_OFFSET_512,
	.slice_pos_2 = REG_SGPIO_POS2_ADRS_OFFSET_512,
	.slice_mux_3 = REG_SGPIO_SLICE_MUX_CFG3_ADRS_OFFSET_512,
	.slice_preset_3 = REG_SGPIO_PRESET3_ADRS_OFFSET_512,
	.slice_count_3 = REG_SGPIO_COUNT3_ADRS_OFFSET_512,
	.slice_pos_3 = REG_SGPIO_POS3_ADRS_OFFSET_512,
	.slice_mux_4 = REG_SGPIO_SLICE_MUX_CFG4_ADRS_OFFSET_512,
	.slice_oe = REG_SGPIO_REG_OE_ADRS_OFFSET_512,
	.slice_oe_ss = REG_SGPIO_REG_SS_OE_ADRS_OFFSET_512,
	.slice_preset_4 = REG_SGPIO_PRESET4_ADRS_OFFSET_512,
	.slice_count_4 = REG_SGPIO_COUNT4_ADRS_OFFSET_512,
	.slice_pos_4 = REG_SGPIO_POS4_ADRS_OFFSET_512,
	.slice_mask = REG_SGPIO_MASK_ADRS_OFFSET_512,
	.slice_ctrl_en = REG_SGPIO_CTRL_ENABLE_ADRS_OFFSET_512,
	.slice_ctrl_en_pos = REG_SGPIO_CTRL_ENABLE_POS_ADRS_OFFSET_512,
	.slice_din_ss = REG_SGPIO_REG_SS_DI_ADRS_OFFSET_512,
	.slice_status = REG_SGPIO_STATUS_ADRS_OFFSET_512,
};

static const struct sgpio_reg_offsets design_128_bit_regs = {
	.slice_mux_0 = REG_SGPIO_SLICE_MUX_CFG0_ADRS_OFFSET_128,
	.slice_preset_0 = REG_SGPIO_PRESET0_ADRS_OFFSET_128,
	.slice_count_0 = REG_SGPIO_COUNT0_ADRS_OFFSET_128,
	.slice_pos_0 = REG_SGPIO_POS0_ADRS_OFFSET_128,
	.slice_mux_1 = REG_SGPIO_SLICE_MUX_CFG1_ADRS_OFFSET_128,
	.slice_ld = REG_SGPIO_REG_LD_ADRS_OFFSET_128,
	.slice_ld_ss = REG_SGPIO_REG_SS_LD_W_ADRS_OFFSET_128,
	.slice_preset_1 = REG_SGPIO_PRESET1_ADRS_OFFSET_128,
	.slice_count_1 = REG_SGPIO_COUNT1_ADRS_OFFSET_128,
	.slice_pos_1 = REG_SGPIO_POS1_ADRS_OFFSET_128,
	.slice_mux_2 = REG_SGPIO_SLICE_MUX_CFG2_ADRS_OFFSET_128,
	.slice_dout = REG_SGPIO_REG_DO_ADRS_OFFSET_128,
	.slice_dout_ss = REG_SGPIO_REG_SS_DO_ADRS_OFFSET_128,
	.slice_preset_2 = REG_SGPIO_PRESET2_ADRS_OFFSET_128,
	.slice_count_2 = REG_SGPIO_COUNT2_ADRS_OFFSET_128,
	.slice_pos_2 = REG_SGPIO_POS2_ADRS_OFFSET_128,
	.slice_mux_3 = REG_SGPIO_SLICE_MUX_CFG3_ADRS_OFFSET_128,
	.slice_preset_3 = REG_SGPIO_PRESET3_ADRS_OFFSET_128,
	.slice_count_3 = REG_SGPIO_COUNT3_ADRS_OFFSET_128,
	.slice_pos_3 = REG_SGPIO_POS3_ADRS_OFFSET_128,
	.slice_mux_4 = REG_SGPIO_SLICE_MUX_CFG4_ADRS_OFFSET_128,
	.slice_oe = REG_SGPIO_REG_OE_ADRS_OFFSET_128,
	.slice_oe_ss = REG_SGPIO_REG_SS_OE_ADRS_OFFSET_128,
	.slice_preset_4 = REG_SGPIO_PRESET4_ADRS_OFFSET_128,
	.slice_count_4 = REG_SGPIO_COUNT4_ADRS_OFFSET_128,
	.slice_pos_4 = REG_SGPIO_POS4_ADRS_OFFSET_128,
	.slice_mask = REG_SGPIO_MASK_ADRS_OFFSET_128,
	.slice_ctrl_en = REG_SGPIO_CTRL_ENABLE_ADRS_OFFSET_128,
	.slice_ctrl_en_pos = REG_SGPIO_CTRL_ENABLE_POS_ADRS_OFFSET_128,
	.slice_din_ss = REG_SGPIO_REG_SS_DI_ADRS_OFFSET_128,
	.slice_status = REG_SGPIO_STATUS_ADRS_OFFSET_128,
};

extern struct ax3000_slice_info slice[MAX_SLICE_COUNT];

