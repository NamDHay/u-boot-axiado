// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2015 IBM Corp.
 * Joel Stanley <joel@jms.id.au>
 * Ryan Chen <ryan_chen@aspeedtech.com>
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
#include <dm/of_access.h>
#include <linux/bug.h>
#include <linux/sizes.h>

#define  CDNS_GPR_BYPASS_MODE_REG     0x00       // bypass function of block
#define  CDNS_GPR_DIRECTION_MODE_REG  0x04       // set pin to either I or O
#define  CDNS_GPR_OUTPUT_ENABLE_REG   0x08       // contains oe control value
#define  CDNS_GPR_OUTPUT_VALUE_REG    0x0C       // output value to be driven
#define  CDNS_GPR_INPUT_VALUE_REG     0x10       // gpio input value reg
#define  CDNS_GPR_INT_MASK_REG        0x14       // used to mask interrupts
#define  CDNS_GPR_INT_ENABLE_REG      0x18       // Used to Enable interrupts
#define  CDNS_GPR_INT_DISABLE_REG     0x1C       // used to Disbale interrupts
#define  CDNS_GPR_INT_STATUS_REG      0x20       // Interrupt status register
#define  CDNS_GPR_INT_TYPE_REG        0x24       // Interrupt type register
#define  CDNS_GPR_INT_VALUE_REG       0x28       // Interrupt value register
#define  CDNS_GPR_INT_ON_ANY_REG      0x2C       // Interrupt on_any_change

#define CONFIG_MAX_PIN_PER_BANK 32

#define GPIO_OFFSET(x)	((x) & 0x1f)
#define GPIO_BIT(x)	BIT(GPIO_OFFSET(x))

struct axiado_gpio_priv {
	void __iomem *regs;
};

static int axiado_gpio_request(struct udevice *dev, unsigned offset, const char *label)
{
	struct axiado_gpio_priv *gpio = dev_get_priv(dev);

    u32 bypass = readl(gpio->regs + CDNS_GPR_BYPASS_MODE_REG);

    if (!strcmp(label, "cmd_gpio")) {
        bypass &= ~GPIO_BIT(offset);
    } else if (!strcmp(label, "bypass")) {
        bypass |= GPIO_BIT(offset);
    }

    writel(bypass, gpio->regs + CDNS_GPR_BYPASS_MODE_REG);

    return 0;
}

static int axiado_gpio_rfree(struct udevice *dev, unsigned int offset)
{
	struct axiado_gpio_priv *gpio = dev_get_priv(dev);

    u32 bypass = readl(gpio->regs + CDNS_GPR_BYPASS_MODE_REG);

    bypass |= GPIO_BIT(offset);

    writel(bypass, gpio->regs + CDNS_GPR_BYPASS_MODE_REG);
    printf("Set bypass %d:%d\n", offset, bypass);

    return 0;
}

static int axiado_gpio_direction_input(struct udevice *dev, unsigned int offset) 
{
	struct axiado_gpio_priv *gpio = dev_get_priv(dev);

    u32 bypass = readl(gpio->regs + CDNS_GPR_BYPASS_MODE_REG);
    bypass &= ~GPIO_BIT(offset);
    writel(bypass, gpio->regs + CDNS_GPR_BYPASS_MODE_REG);

    u32 dir = readl(gpio->regs + CDNS_GPR_DIRECTION_MODE_REG);
    dir |= GPIO_BIT(offset);
    writel(dir, gpio->regs + CDNS_GPR_DIRECTION_MODE_REG);
    
    u32 en = readl(gpio->regs + CDNS_GPR_OUTPUT_ENABLE_REG);
    en &= ~GPIO_BIT(offset);
    writel(en, gpio->regs + CDNS_GPR_OUTPUT_ENABLE_REG);

    return 0;
}

static int axiado_gpio_direction_output(struct udevice *dev, unsigned int offset,
                    int value) 
{
	struct axiado_gpio_priv *gpio = dev_get_priv(dev);

    u32 bypass = readl(gpio->regs + CDNS_GPR_BYPASS_MODE_REG);
    bypass &= ~GPIO_BIT(offset);
    writel(bypass, gpio->regs + CDNS_GPR_BYPASS_MODE_REG);

    u32 dir = readl(gpio->regs + CDNS_GPR_DIRECTION_MODE_REG);
    dir &= ~GPIO_BIT(offset);
    writel(dir, gpio->regs + CDNS_GPR_DIRECTION_MODE_REG);

    u32 en = readl(gpio->regs + CDNS_GPR_OUTPUT_ENABLE_REG);
    en |= GPIO_BIT(offset);
    writel(en, gpio->regs + CDNS_GPR_OUTPUT_ENABLE_REG);

    u32 output = readl(gpio->regs + CDNS_GPR_OUTPUT_VALUE_REG);
	if (value)
		output |= GPIO_BIT(offset);
	else
		output &= ~GPIO_BIT(offset);

	writel(output, gpio->regs + CDNS_GPR_OUTPUT_VALUE_REG);

    return 0;
}

static int axiado_gpio_get_value(struct udevice *dev, unsigned int offset)
{
	struct axiado_gpio_priv *gpio = dev_get_priv(dev);

	return !!(readl(gpio->regs + CDNS_GPR_INPUT_VALUE_REG) & GPIO_BIT(offset));
}

static int
axiado_gpio_set_value(struct udevice *dev, unsigned int offset, int value)
{
	struct axiado_gpio_priv *gpio = dev_get_priv(dev);

    u32 data = readl(gpio->regs + CDNS_GPR_OUTPUT_VALUE_REG);

	if (value)
		data |= GPIO_BIT(offset);
	else
		data &= ~GPIO_BIT(offset);

	writel(data, gpio->regs + CDNS_GPR_OUTPUT_VALUE_REG);

	return 0;
}

static int tegra_gpio_set_flags(struct udevice *dev, unsigned int offset, ulong flags)
{
	if (flags & GPIOD_IS_AF) {
		return axiado_gpio_rfree(dev, offset);
	} else if (flags & GPIOD_IS_IN) {
		return axiado_gpio_direction_input(dev, offset);
	} else if (flags & GPIOD_IS_OUT) {
		bool value = flags & GPIOD_IS_OUT_ACTIVE;

		return axiado_gpio_direction_output(dev, offset, value);
	}
    return 0;
}

static struct dm_gpio_ops axiado_gpio_ops = {
    .direction_input = axiado_gpio_direction_input,
    .direction_output = axiado_gpio_direction_output,
	.get_value		= axiado_gpio_get_value,
	.set_value		= axiado_gpio_set_value,
	.set_flags		= tegra_gpio_set_flags,
    .request		= axiado_gpio_request,
	.rfree		    = axiado_gpio_rfree,
};

static int axiado_gpio_probe(struct udevice *dev)
{
    struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
    struct axiado_gpio_priv *priv = dev_get_priv(dev);

	uc_priv->bank_name = dev->name;
    uc_priv->gpio_count = CONFIG_MAX_PIN_PER_BANK;
	priv->regs = devfdt_get_addr_ptr(dev);

    return 0;
}

static const struct udevice_id axiado_gpio_ids[] = {
	{ .compatible = "axiado,ax3000-gpio",  },
	{ }
};

U_BOOT_DRIVER(gpio_axiado) = {
	.name   = "gpio-axiado",
	.id     = UCLASS_GPIO,
	.of_match = axiado_gpio_ids,
	.ops    = &axiado_gpio_ops,
	.probe  = axiado_gpio_probe,
	.priv_auto = sizeof(struct axiado_gpio_priv),
};

