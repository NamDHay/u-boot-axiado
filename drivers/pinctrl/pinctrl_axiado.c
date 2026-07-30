// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024-2025 Axiado Corporation.
 *
 * Author: Nguyen Nam Huy <axiado-2561@axiado.com>
 *
 */

#include <dm.h>
#include <errno.h>
#include <log.h>
#include <asm/io.h>
#include <dm/pinctrl.h>
#include <asm/gpio.h>
#include <linux/delay.h>

#define I3C_I2C_MUX_OFFSET 0x1B4
#define SPI_TPM_MUX_OFFSET    0x120

struct axiado_pinctrl_priv {
    void __iomem *base;
};

static int axiado_pinctrl_probe(struct udevice *dev)
{
	struct axiado_pinctrl_priv *priv = dev_get_priv(dev);
    priv->base = devfdt_get_addr_ptr(dev);
	return 0;
}

struct axiado_group_config {
	char *group_name;
	unsigned offset;
	u32 ctrl_bit_mask;
};

static const struct axiado_group_config axiado_groups[] = {
	{ "I3C4",   I3C_I2C_MUX_OFFSET, (1 << 0) },
	{ "I3C5",   I3C_I2C_MUX_OFFSET, (1 << 1) },
	{ "I3C6",   I3C_I2C_MUX_OFFSET, (1 << 2) },
	{ "I3C7",   I3C_I2C_MUX_OFFSET, (1 << 3) },
	{ "I3C8",   I3C_I2C_MUX_OFFSET, (1 << 0) },
	{ "I3C9",   I3C_I2C_MUX_OFFSET, (1 << 1) },
	{ "I3C11",  I3C_I2C_MUX_OFFSET, (1 << 3) },
	{ "I3C12",  I3C_I2C_MUX_OFFSET, (1 << 0) },
	{ "I3C13",  I3C_I2C_MUX_OFFSET, (1 << 1) },
	{ "I3C20",  I3C_I2C_MUX_OFFSET, (1 << 0) },
	{ "I3C21",  I3C_I2C_MUX_OFFSET, (1 << 1) },
	{ "I3C22",  I3C_I2C_MUX_OFFSET, (1 << 4) },
	{ "I3C26",  I3C_I2C_MUX_OFFSET, (1 << 2) },
	{ "I3C27",  I3C_I2C_MUX_OFFSET, (1 << 3) },
	{ "I3C28",  I3C_I2C_MUX_OFFSET, (1 << 4) },
	{ "I3C29",  I3C_I2C_MUX_OFFSET, (1 << 5) },
	{ "I3C30",  I3C_I2C_MUX_OFFSET, (1 << 6) },
	{ "I3C31",  I3C_I2C_MUX_OFFSET, (1 << 7) },
	{ "I3C32",  I3C_I2C_MUX_OFFSET, (1 << 8) },
    { "TPM0",   SPI_TPM_MUX_OFFSET, (1 << 2) },
    { "TPM1",   SPI_TPM_MUX_OFFSET, (1 << 2) },
};

static int axiado_pinctrl_get_function_count(struct udevice *dev)
{
    printf("PINCTRL: get_(functions/groups)_count\n");
	return ARRAY_SIZE(axiado_groups);
}

static const char *axiado_pinctrl_get_function_name(struct udevice *dev,
						  unsigned selector)
{
    printf("PINCTRL: get_(function/group)_name %u\n", selector);
	return axiado_groups[selector].group_name;
}

static int axiado_pinctrl_set(struct udevice *dev, unsigned selector,
				     unsigned func_selector)
{
	const struct axiado_group_config *config;

	if (selector >= ARRAY_SIZE(axiado_groups))
		return -EINVAL;

    config = &axiado_groups[selector];

    printf("function = %s\n", config->group_name);

	return 0;
}

static int axiado_pinctrl_set_state(struct udevice *dev,
				    struct udevice *config)
{
    struct axiado_pinctrl_priv *priv = dev_get_priv(dev);
	ofnode node;
	const char *function;
	u32 pins[32];
    u32 value;
	int pin_count;
    int size;
	int selector;
	int ret;
	int i;

    node = dev_ofnode(config);
    function = ofnode_read_string(node, "function");
    if (!function) {
        pr_err("pinctrl: missing function property in %s\n",
                ofnode_get_name(node));
        return -EINVAL;
    }

    size = ofnode_read_size(node, "axiado,pins");
    if (size < 0)
        return size;

    if (size % sizeof(u32))
        return -EINVAL;

    pin_count = size / sizeof(u32);

    if (pin_count > ARRAY_SIZE(pins))
        return -E2BIG;

    ret =  ofnode_read_u32_array(node, "axiado,pins", pins, pin_count);
    if (ret)
        return ret;

    for (selector = 0;
	     selector < ARRAY_SIZE(axiado_groups);
	     selector++) {
		if (!strcmp(function,
			    axiado_groups[selector].group_name)) {
            /* value = readl(priv->base + axiado_groups[selector].offset); */
            /* value |= axiado_groups[selector].ctrl_bit_mask; */
            /* writel(value, priv->base + axiado_groups[selector].offset); */           
            writel(axiado_groups[selector].ctrl_bit_mask, priv->base + axiado_groups[selector].offset);           
            printf("%s: function=%s, selector=%d\n", dev->name, function, selector);
			break;
        }
	}

    if (selector >= ARRAY_SIZE(axiado_groups)) {
		printf("pinctrl: unsupported function %s trigger bypass only\n",
		       function);
	}

    printf("Request bypass mode for pin: ");
    for (i = 0; i < pin_count; i++) {
        gpio_request(pins[i], "bypass");
        printf("%d ", pins[i]);
    }
    printf("\n");

    mdelay(5);

	return 0;
}

static struct pinctrl_ops axiado_pinctrl_ops = {
    .set_state = axiado_pinctrl_set_state,
    .get_functions_count = axiado_pinctrl_get_function_count,
    .get_function_name = axiado_pinctrl_get_function_name,
    .pinmux_set = axiado_pinctrl_set,
};

static const struct udevice_id axiado_pinctrl_ids[] = {
    { .compatible = "axiado,ax3000-pinctrl" },
    { }
};

U_BOOT_DRIVER(axiado_pinctrl) = {
    .name = "axiado_pinctrl",
    .id = UCLASS_PINCTRL,
    .of_match = axiado_pinctrl_ids,
    .priv_auto	= sizeof(struct axiado_pinctrl_priv),
    .ops = &axiado_pinctrl_ops,
    .probe = axiado_pinctrl_probe,
};

