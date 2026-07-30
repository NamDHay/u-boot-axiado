// SPDX-License-Identifier: GPL-2.0-or-later

/* Copyright (c) 2021-25 Axiado Corporation (or its affiliates). All rights reserved.
 * Use, modification and redistribution of this file is subject to your possession
 * of a valid End User License Agreement (EULA) for the Axiado Product of which
 * these sources are part of and your compliance with all applicable terms and
 * conditions of such licence agreement.
 *
 * Nguyen Nam Huy <axiado-2561@axiado.com>
 *
 * Implementation extracted from the Linux kernel and adapted for u-boot.
 */
#include <asm/io.h>
#include <asm/gpio.h>

#include <config.h>
#include <clk.h>
#include <dm.h>
#include <asm/io.h>
#include <linux/bug.h>
#include <linux/sizes.h>

#include "axiado-sgpio.h"

/* Define the global slice array */
struct ax3000_slice_info slice[MAX_SLICE_COUNT];

static void sgpio_hw_init(struct ax3000_sgpio *sgpio)
{
	uint32_t bank;
	uint32_t position;
	int i = 0;

	/* slice A0, Clock Pin - 0 */
	sgpio_reg_write(sgpio, sgpio->regs->slice_mux_0,
			0x306); /* sgpio_mux_config */
	sgpio_reg_write(sgpio, sgpio->regs->slice_preset_0,
			sgpio->preset_value); /* preset for countl */
	sgpio_reg_write(sgpio, sgpio->regs->slice_count_0,
			sgpio->count_value); /* count start value */
	sgpio_reg_write(sgpio, sgpio->regs->slice_pos_0,
			0x1f001f); /* pos and pos preset */

	/* Slice B1, Data Load Pin - 1 */
	bank = GET_BANK((sgpio->ngpios-1));
	position = GET_POS((sgpio->ngpios-1));

	sgpio_reg_write(sgpio, sgpio->regs->slice_mux_1,
			0x30c); /* slice_mux_config */

	for (i = 0; i < bank; i++) {
		sgpio_reg_write(sgpio, sgpio->regs->slice_ld + (i * 4),
				0xffffffff); /* shift reg */
		sgpio_reg_write(sgpio, sgpio->regs->slice_ld_ss + (i * 4),
				0xffffffff); /* shift reg ss */
	}

	if (position) {
		sgpio_reg_write(sgpio, sgpio->regs->slice_ld + (i * 4),
				SET_TILL_POS(slice[1].reg_ss[i],
					     position)); /* shift reg */
		sgpio_reg_write(sgpio, sgpio->regs->slice_ld_ss + (i * 4),
				SET_TILL_POS(slice[1].reg_ss[i],
					     position)); /* shift reg ss */
	}

	sgpio_reg_write(sgpio, sgpio->regs->slice_preset_1,
			sgpio->preset_value); /* preset for count */
	sgpio_reg_write(sgpio, sgpio->regs->slice_count_1,
			sgpio->count_value); /* count start value */
	sgpio_reg_write(sgpio, sgpio->regs->slice_pos_1,
			sgpio->pos_reg); /* pos and pos preset */

	/* Slice C2, Data Out Pin - 2 */
	bank = GET_BANK((sgpio->ngpios));
	position = GET_POS((sgpio->ngpios));

	sgpio_reg_write(sgpio, sgpio->regs->slice_mux_2,
			0x30c); //slice_mux_config

	for (i = 0; i < bank; i++) {
		sgpio_reg_write(sgpio, sgpio->regs->slice_dout + (i * 4),
				slice[2].reg_ss[i]); /* shift reg */
		sgpio_reg_write(sgpio, sgpio->regs->slice_dout_ss + (i * 4),
				slice[2].reg_ss[i]); /* shift reg ss */
	}

	if (position) {
		sgpio_reg_write(sgpio, sgpio->regs->slice_dout + (i * 4),
				slice[2].reg_ss[i]); /* shift reg */
		sgpio_reg_write(sgpio, sgpio->regs->slice_dout_ss + (i * 4),
				slice[2].reg_ss[i]); /* shift reg ss */
	}

	sgpio_reg_write(sgpio, sgpio->regs->slice_preset_2,
			sgpio->preset_value); /* preset for count */
	sgpio_reg_write(sgpio, sgpio->regs->slice_count_2,
			sgpio->count_value); /* count start value */
	sgpio_reg_write(sgpio, sgpio->regs->slice_pos_2,
			sgpio->pos_reg); /* pos and pos preset */

	/* Slice D3, Data In Pin - 3 */
	sgpio_reg_write(sgpio, sgpio->regs->slice_mux_3,
			0x14C); /* slice_mux_config */
	sgpio_reg_write(sgpio, sgpio->regs->slice_preset_3,
			sgpio->preset_value); /* preset for count */
	sgpio_reg_write(sgpio, sgpio->regs->slice_count_3,
			sgpio->count_value); /* count start value */
	sgpio_reg_write(sgpio, sgpio->regs->slice_pos_3,
			sgpio->pos_reg); /* pos and pos preset */

	/* Slice E4, Output Enable for respective pins */
	sgpio_reg_write(sgpio, sgpio->regs->slice_mux_4,
			0x10c); /* slice_mux_config */
	sgpio_reg_write(sgpio, sgpio->regs->slice_oe,
			0xffffffff); /* shift reg */
	sgpio_reg_write(sgpio, sgpio->regs->slice_oe_ss,
			0xffffffff); /* shift reg ss */
	sgpio_reg_write(sgpio, sgpio->regs->slice_preset_4,
			sgpio->preset_value); /* preset for count */
	sgpio_reg_write(sgpio, sgpio->regs->slice_count_4,
			sgpio->count_value); /* count start value */
	sgpio_reg_write(sgpio, sgpio->regs->slice_pos_4,
			0x1f001f); /* pos and pos preset */

	sgpio_reg_write(sgpio, sgpio->regs->slice_mask,
			0xdfff); /* Enabling the interrupt for new data */

	sgpio_reg_write(sgpio, sgpio->regs->slice_ctrl_en,
			0xffff); /* cntrl_enable 'b100001111  ckfl d bmgn a */
	sgpio_reg_write(sgpio, sgpio->regs->slice_ctrl_en_pos,
			0xffff); /* out_mux_config 'b1000000 */
}

static int axiado_sgpio_get_value(struct udevice *dev, unsigned int offset)
{
	uint32_t bank = GET_BANK(offset/2);;
	uint32_t position = GET_POS(offset/2);;
	int rc;

	if (!(offset % 2))
		rc = IS_BIT_SET(slice[3].reg_ss[bank], position);
	else
		rc = IS_BIT_SET(slice[2].reg_ss[bank], position);

	return rc;
}

static int
axiado_sgpio_set_value(struct udevice *dev, unsigned int offset, int value)
{
	uint32_t bank = GET_BANK(offset/2);
	uint32_t position = GET_POS(offset/2);
    struct ax3000_sgpio *sgpio = dev_get_priv(dev);

	if (value)
		slice[2].reg_ss[bank] |= 1 << position;
	else
		slice[2].reg_ss[bank] &= ~(1 << position);

	sgpio_reg_write(sgpio, sgpio->regs->slice_dout_ss + (bank * 4),
			slice[2].reg_ss[bank]);

	return 0;
}

static int axiado_sgpio_direction_input(struct udevice *dev, unsigned int offset) 
{
	if (!(offset % 2))
		return 0;
	else
		return -EINVAL;
	return 0;
}

static int axiado_sgpio_direction_output(struct udevice *dev, unsigned int offset,
                    int value) 
{
	if (offset % 2) {
		axiado_sgpio_set_value(dev, offset, value);
		return 0;
	} else
		return -EINVAL;
	return 0;
}

static struct dm_gpio_ops axiado_sgpio_ops = {
    .direction_input    = axiado_sgpio_direction_input,
    .direction_output   = axiado_sgpio_direction_output,
	.get_value		    = axiado_sgpio_get_value,
	.set_value		    = axiado_sgpio_set_value,
};

static int axiado_sgpio_probe(struct udevice *dev)
{
    struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
    struct ax3000_sgpio *sgpio = dev_get_priv(dev);
	const __be32 *prop;
	uint32_t variant;
	uint32_t dout_value;
	int dout_reverse;
    int i;
    int ret;

	uc_priv->bank_name = dev->name;
	sgpio->membase = devfdt_get_addr_ptr(dev);
    if (!sgpio->membase) {
        pr_err("Failed to probe device\n");
        return -EINVAL;
    }
	ret = ofnode_read_u32(dev_ofnode(dev), "ngpios", &uc_priv->gpio_count);
    if (ret < 0) {
        pr_err("Could not read ngpios property\n");
        return -EINVAL;
    }

	ret = ofnode_read_u32(dev_ofnode(dev), "design-variant", &variant);
    if (ret < 0) {
        pr_err("design-variant not specified in DT\n");
        return -EINVAL;
    }

	/* FPGA B0 SGPIO design variant 128 bit or 512 bit */
	if (variant == 128) {
		sgpio->regs = (struct sgpio_reg_offsets *)&design_128_bit_regs;
		sgpio->max_sgpio_pins = 128;
		sgpio->max_offset_regs = 4;
	} else if (variant == 512) {
		sgpio->regs = (struct sgpio_reg_offsets *)&design_512_bit_regs;
		sgpio->max_sgpio_pins = 512;
		sgpio->max_offset_regs = 16;
	} else {
		return -EINVAL;
	}

	if (sgpio->ngpios > sgpio->max_sgpio_pins) {
		pr_err("ngpio is greater than 512 pins\n");
		return -EINVAL;
	}

    ret = ofnode_read_u32(dev_ofnode(dev), "bus-frequency", &sgpio->bus_frequency);
    if (ret < 0) {
        pr_err("Could not read bus-frequency property\n");
        return -EINVAL;
    }

    ret = ofnode_read_u32(dev_ofnode(dev), "apb-frequency", &sgpio->apb_frequency);
    if (ret < 0) {
        pr_err("Could not read apb-frequency property\n");
        return -EINVAL;
    }

	sgpio->preset_value = (sgpio->apb_frequency / sgpio->bus_frequency) - 1;
	sgpio->count_value = sgpio->preset_value;

	sgpio->pos = sgpio->ngpios - 1;
	sgpio->pos_reset = sgpio->ngpios - 1;

	sgpio->pos_reg = (sgpio->pos_reset<<16) | sgpio->pos;

	prop = ofnode_get_property(dev_ofnode(dev), "dout-init", NULL);
	if (!prop) {
		pr_err("Failed to get dout-init\n");
		return -EINVAL;
	}

	for (i = 0; i < sgpio->max_offset_regs; i++) {
		slice[2].reg_ss[i] = 0;
		dout_value = be32_to_cpu(prop[i]);

		for (dout_reverse = 0; dout_reverse < 32; ++dout_reverse) {
			slice[2].reg_ss[i] <<= 1;
			slice[2].reg_ss[i] |= (dout_value & 1);
			dout_value >>= 1;
		}
	}

	sgpio_hw_init(sgpio);

    return 0;
}

static const struct udevice_id axiado_sgpio_ids[] = {
	{ .compatible = "axiado,sgpio",  },
	{ }
};

U_BOOT_DRIVER(sgpio_axiado) = {
	.name   = "sgpio-axiado",
	.id     = UCLASS_GPIO,
	.of_match = axiado_sgpio_ids,
	.ops    = &axiado_sgpio_ops,
	.probe  = axiado_sgpio_probe,
	.priv_auto = sizeof(struct ax3000_sgpio),
};


