// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>

#include "ax3000_base_adrs.h"
#include "ax3000_slo_base_adrs.h"

#include "ax_gpio.h"

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
#define GET_BANK(pin) \
	(pin / CONFIG_MAX_PIN_PER_BANK) /**< get bank number with pin number */
#define GPIO_BIT(pin) \
	(pin %              \
	 CONFIG_MAX_PIN_PER_BANK) /**< pin number in a bank range 0 - 31 */

#define CONFIG_MAX_GPIO_BANK 8
#define CONFIG_MAX_GPIO_PIN 256

static const ulong gpio_chip[CONFIG_MAX_GPIO_BANK] = {
	AX3000_CSR_BASE_ADRS_GPIO_0, AX3000_CSR_BASE_ADRS_GPIO_1,
	AX3000_CSR_BASE_ADRS_GPIO_2, AX3000_CSR_BASE_ADRS_GPIO_3,
	AX3000_CSR_BASE_ADRS_GPIO_4, AX3000_CSR_BASE_ADRS_GPIO_5,
	AX3000_CSR_BASE_ADRS_GPIO_6, AX3000_CSR_BASE_ADRS_GPIO_7
};

static int cdns_gpio_request(unsigned pin, const char *label) 
{	
    unsigned int bank = GET_BANK(pin);
    void __iomem *regs = (void __iomem *)gpio_chip[bank];
    u32 bypass = readl(regs + CDNS_GPR_BYPASS_MODE_REG);

    if (!strcmp(label, "gpio")) {
        bypass &= ~GPIO_BIT(pin);
    } else if (!strcmp(label, "bypass")) {
        bypass |= GPIO_BIT(pin);
    }

    writel(bypass, regs + CDNS_GPR_BYPASS_MODE_REG);

    return 0;
}

static int cdns_gpio_free(unsigned pin) 
{
    unsigned int bank = GET_BANK(pin);
    void __iomem *regs = (void __iomem *)gpio_chip[bank];
    u32 bypass = readl(regs + CDNS_GPR_BYPASS_MODE_REG);

    bypass |= GPIO_BIT(pin);

    writel(bypass, regs + CDNS_GPR_BYPASS_MODE_REG);

    return 0;
}

static int cdns_gpio_set_direction(unsigned pin, unsigned dir) 
{
    unsigned int bank = GET_BANK(pin);
    void __iomem *regs = (void __iomem *)gpio_chip[bank];
    u32 val;
    u32 bypass;

    bypass = readl(regs + CDNS_GPR_BYPASS_MODE_REG);
    bypass &= ~GPIO_BIT(pin);
    writel(bypass, regs + CDNS_GPR_BYPASS_MODE_REG);

    val = readl(regs + CDNS_GPR_DIRECTION_MODE_REG);
    if (dir)
        val |= GPIO_BIT(pin);
    else
        val &= ~GPIO_BIT(pin);
    writel(val, regs + CDNS_GPR_DIRECTION_MODE_REG);
    
    val = readl(regs + CDNS_GPR_OUTPUT_ENABLE_REG);
    if (dir)
        val &= ~GPIO_BIT(pin);
    else
        val |= GPIO_BIT(pin);
    writel(val, regs + CDNS_GPR_OUTPUT_ENABLE_REG);

    return 0;
}

static int cdns_gpio_set_value(unsigned pin, unsigned value) 
{
    unsigned int bank = GET_BANK(pin);
    void __iomem *regs = (void __iomem *)gpio_chip[bank];
    u32 data = readl(regs + CDNS_GPR_OUTPUT_VALUE_REG);

	if (value)
		data |= GPIO_BIT(pin);
	else
		data &= ~GPIO_BIT(pin);

	writel(data, regs + CDNS_GPR_OUTPUT_VALUE_REG);
    return 0;
}

static int cdns_gpio_get_value(unsigned pin) 
{
    unsigned int bank = GET_BANK(pin);
    void __iomem *regs = (void __iomem *)gpio_chip[bank];
	return !!(readl(regs + CDNS_GPR_INPUT_VALUE_REG) & GPIO_BIT(pin));
}

struct ax_gpio_ops gpio = {
    .request = cdns_gpio_request,
    .set_direction = cdns_gpio_set_direction,
    .set_value = cdns_gpio_set_value,
    .get_value = cdns_gpio_get_value,
    .free = cdns_gpio_free,
};
