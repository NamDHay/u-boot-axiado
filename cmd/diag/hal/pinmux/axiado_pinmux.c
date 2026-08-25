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
#include "ax_pinmux.h"

#define UNUSED_PINMUX (-1)
#define I3C_I2C_MUX_OFFSET 0x1B4
#define SPI_TPM_MUX_OFFSET    0x120

struct axiado_group_config {
    char *group_name;
    unsigned offset;
    u32 ctrl_bit_mask;
    u8 pin_cnt;
    u8 pin[7];
};

static const struct axiado_group_config axiado_groups[] = {
    /* PWM Group */
    { "PWM0",  UNUSED_PINMUX, 0x0, 0x1, { 6 } },
    { "PWM1",  UNUSED_PINMUX, 0x0, 0x1, { 8 } },
    { "PWM2",  UNUSED_PINMUX, 0x0, 0x1, { 38 } },
    { "PWM3",  UNUSED_PINMUX, 0x0, 0x1, { 40 } },
    { "PWM4",  UNUSED_PINMUX, 0x0, 0x1, { 70 } },
    { "PWM5",  UNUSED_PINMUX, 0x0, 0x1, { 72 } },
    { "PWM6",  UNUSED_PINMUX, 0x0, 0x1, { 102 } },
    { "PWM7",  UNUSED_PINMUX, 0x0, 0x1, { 104 } },
    { "PWM8",  UNUSED_PINMUX, 0x0, 0x1, { 134 } },
    { "PWM9",  UNUSED_PINMUX, 0x0, 0x1, { 136 } },
    { "PWM10", UNUSED_PINMUX, 0x0, 0x1, { 166 } },
    { "PWM11", UNUSED_PINMUX, 0x0, 0x1, { 168 } },
    { "PWM12", UNUSED_PINMUX, 0x0, 0x1, { 198 } },
    { "PWM13", UNUSED_PINMUX, 0x0, 0x1, { 200 } },
    { "PWM14", UNUSED_PINMUX, 0x0, 0x1, { 230 } },
    { "PWM15", UNUSED_PINMUX, 0x0, 0x1, { 232 } },

    /* I2C Group */
    { "I2C0",  UNUSED_PINMUX, 0x0, 0x2, { 0, 1 } },
    { "I2C1",  UNUSED_PINMUX, 0x0, 0x2, { 3, 4 } },
    { "I2C2",  UNUSED_PINMUX, 0x0, 0x2, { 32, 33 } },
    { "I2C3",  UNUSED_PINMUX, 0x0, 0x2, { 35, 36 } },
    { "I2C4",  UNUSED_PINMUX, 0x0, 0x2, { 64, 65 } },
    { "I2C5",  UNUSED_PINMUX, 0x0, 0x2, { 67, 68 } },
    { "I2C6",  UNUSED_PINMUX, 0x0, 0x2, { 96, 97 } },
    { "I2C7",  UNUSED_PINMUX, 0x0, 0x2, { 99, 100 } },
    { "I2C8",  UNUSED_PINMUX, 0x0, 0x2, { 128, 129 } },
    { "I2C9",  UNUSED_PINMUX, 0x0, 0x2, { 131, 132 } },
    { "I2C10", UNUSED_PINMUX, 0x0, 0x2, { 160, 161 } },
    { "I2C11", UNUSED_PINMUX, 0x0, 0x2, { 163, 164 } },
    { "I2C12", UNUSED_PINMUX, 0x0, 0x2, { 192, 193 } },
    { "I2C13", UNUSED_PINMUX, 0x0, 0x2, { 195, 196 } },
    { "I2C14", UNUSED_PINMUX, 0x0, 0x2, { 224, 225 } },
    { "I2C15", UNUSED_PINMUX, 0x0, 0x2, { 227, 228 } },
    { "I2C17", UNUSED_PINMUX, 0x0, 0x2, { 25, 26 } },
    { "I2C18", UNUSED_PINMUX, 0x0, 0x2, { 28, 29 } },
    { "I2C19", UNUSED_PINMUX, 0x0, 0x2, { 56, 57 } },
    { "I2C20", UNUSED_PINMUX, 0x0, 0x2, { 58, 59 } },
    { "I2C21", UNUSED_PINMUX, 0x0, 0x2, { 61, 62 } },
    { "I2C22", UNUSED_PINMUX, 0x0, 0x2, { 123, 124 } },
    { "I2C23", UNUSED_PINMUX, 0x0, 0x2, { 208, 209 } },
    { "I2C24", UNUSED_PINMUX, 0x0, 0x2, { 217, 218 } },
    { "I2C25", UNUSED_PINMUX, 0x0, 0x2, { 220, 221 } },
    { "I2C26", UNUSED_PINMUX, 0x0, 0x2, { 234, 235 } },
    { "I2C27", UNUSED_PINMUX, 0x0, 0x2, { 237, 238 } },
    { "I2C28", UNUSED_PINMUX, 0x0, 0x2, { 240, 241 } },
    { "I2C29", UNUSED_PINMUX, 0x0, 0x2, { 243, 244 } },
    { "I2C30", UNUSED_PINMUX, 0x0, 0x2, { 246, 247 } },
    { "I2C31", UNUSED_PINMUX, 0x0, 0x2, { 249, 250 } },
    { "I2C32", UNUSED_PINMUX, 0x0, 0x2, { 252, 253 } },

    /* UART Group */
    { "UART0",  UNUSED_PINMUX, 0x0, 0x2, { 20, 21 } },
    { "UART1",  UNUSED_PINMUX, 0x0, 0x2, { 52, 53 } },
    { "UART2",  UNUSED_PINMUX, 0x0, 0x2, { 84, 85 } },
    { "UART4",  UNUSED_PINMUX, 0x0, 0x2, { 94, 95 } },
    { "UART5",  UNUSED_PINMUX, 0x0, 0x2, { 202, 203 } },
    { "UART6",  UNUSED_PINMUX, 0x0, 0x2, { 204, 205 } },
    { "UART7",  UNUSED_PINMUX, 0x0, 0x2, { 206, 207 } },
    { "UART8",  UNUSED_PINMUX, 0x0, 0x2, { 54, 55 } },

    /* I3C Group */
    { "I3C4",   I3C_I2C_MUX_OFFSET, (1 << 0), 0x3, { 64, 65, 66 } },
    { "I3C5",   I3C_I2C_MUX_OFFSET, (1 << 1), 0x3, { 67, 68, 69 } },
    { "I3C6",   I3C_I2C_MUX_OFFSET, (1 << 2), 0x3, { 96, 97, 98 } },
    { "I3C7",   I3C_I2C_MUX_OFFSET, (1 << 3), 0x3, { 99, 100, 101 } },
    { "I3C8",   I3C_I2C_MUX_OFFSET, (1 << 0), 0x3, { 128, 129, 130 } },
    { "I3C9",   I3C_I2C_MUX_OFFSET, (1 << 1), 0x3, { 131, 132, 133 } },
    { "I3C11",  I3C_I2C_MUX_OFFSET, (1 << 3), 0x3, { 163, 164, 165 } },
    { "I3C12",  I3C_I2C_MUX_OFFSET, (1 << 0), 0x3, { 192, 193, 194 } },
    { "I3C13",  I3C_I2C_MUX_OFFSET, (1 << 1), 0x3, { 195, 196, 197 } },
    { "I3C20",  I3C_I2C_MUX_OFFSET, (1 << 0), 0x3, { 58, 59, 60 } },
    { "I3C21",  I3C_I2C_MUX_OFFSET, (1 << 1), 0x3, { 61, 62, 63 } },
    { "I3C22",  I3C_I2C_MUX_OFFSET, (1 << 4), 0x3, { 123, 124, 125 } },
    { "I3C26",  I3C_I2C_MUX_OFFSET, (1 << 2), 0x3, { 234, 235, 236 } },
    { "I3C27",  I3C_I2C_MUX_OFFSET, (1 << 3), 0x3, { 237, 238, 239 } },
    { "I3C28",  I3C_I2C_MUX_OFFSET, (1 << 4), 0x3, { 240, 241, 242 } },
    { "I3C29",  I3C_I2C_MUX_OFFSET, (1 << 5), 0x3, { 243, 244, 245 } },
    { "I3C30",  I3C_I2C_MUX_OFFSET, (1 << 6), 0x3, { 246, 247, 248 } },
    { "I3C31",  I3C_I2C_MUX_OFFSET, (1 << 7), 0x3, { 249, 250, 251 } },
    { "I3C32",  I3C_I2C_MUX_OFFSET, (1 << 8), 0x3, { 252, 253, 254 } },

    /* SPI Group */
    { "SPI0",   UNUSED_PINMUX, 0x0, 0x7, { 10, 11, 12, 13, 15, 16, 17 } },
    { "SPI1",   UNUSED_PINMUX, 0x0, 0x7, { 42, 43, 44, 45, 47, 48, 49 } },
    { "SPI2",   UNUSED_PINMUX, 0x0, 0x7, { 142, 143, 144, 145, 148, 149 } },
    { "SPI3",   UNUSED_PINMUX, 0x0, 0x7, { 106, 107, 108, 109, 111, 112, 113 } },
    { "SPI5",   UNUSED_PINMUX, 0x0, 0x7, { 60, 61, 62, 63, 57, 58, 59 } },
    { "SPI6",   UNUSED_PINMUX, 0x0, 0x7, { 239, 240, 241, 242, 235, 236, 237 } },
    { "TPM0",   SPI_TPM_MUX_OFFSET, (1 << 2), 0x7, { 42, 43, 44, 45, 47, 48, 49 } },
    { "TPM1",   SPI_TPM_MUX_OFFSET, (1 << 2), 0x7, { 239, 240, 241, 242, 235, 236, 237 } },
};

static void __iomem *axiado_pinmux_get_base(int selector) {
    u8 first_pin = axiado_groups[selector].pin[0];
    if (first_pin < 64)
        return (void __iomem *)AX3000_CSR_BASE_ADRS_IOCTL_0;
    else if (first_pin < 128)
        return (void __iomem *)AX3000_CSR_BASE_ADRS_IOCTL_1;
    else if (first_pin < 192)
        return (void __iomem *)AX3000_CSR_BASE_ADRS_IOCTL_2;
    else
        return (void __iomem *)AX3000_CSR_BASE_ADRS_IOCTL_3;
}

static int axiado_pinmux_set_state(const char* function) {
    int i;
    int selector;
    int pin_count;
    void __iomem *base;

    for (selector = 0;
            selector < ARRAY_SIZE(axiado_groups);
            selector++) {
        if (!strcmp(function,
                    axiado_groups[selector].group_name)) {
            break;
        }
    }
    printf("%s: function=%s\n", __func__, function);

    if (selector >= ARRAY_SIZE(axiado_groups))
        return 0;

    pin_count = axiado_groups[selector].pin_cnt;
    printf("%s: Request bypass mode for %d pin: ", __func__, pin_count);
    for (i = 0; i < pin_count; i++) {
        ax_gpio_request(axiado_groups[selector].pin[i], "bypass");
        printf("%d ", axiado_groups[selector].pin[i]);
    }
    printf("\n");

    if (axiado_groups[selector].offset == UNUSED_PINMUX)
        return 0;
    base = axiado_pinmux_get_base(selector);
    writel(axiado_groups[selector].ctrl_bit_mask, base + axiado_groups[selector].offset);           

    return 0;
}

struct ax_pinmux_ops pinmux = {
    .set_state = axiado_pinmux_set_state,
};
