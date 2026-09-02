// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <linux/delay.h>

#include "ax_diag.h"

#include "ax_pinmux.h"
#include "ax_i2c.h"

#define CONFIG_MAX_I2C 33
static const ulong i2c_base[CONFIG_MAX_I2C] = {
    AX3000_CSR_BASE_ADRS_I2C_0,
    AX3000_CSR_BASE_ADRS_I2C_1,
    AX3000_CSR_BASE_ADRS_I2C_2,
    AX3000_CSR_BASE_ADRS_I2C_3,
    AX3000_CSR_BASE_ADRS_I2C_4,
    AX3000_CSR_BASE_ADRS_I2C_5,
    AX3000_CSR_BASE_ADRS_I2C_6,
    AX3000_CSR_BASE_ADRS_I2C_7,
    AX3000_CSR_BASE_ADRS_I2C_8,
    AX3000_CSR_BASE_ADRS_I2C_9,
    AX3000_CSR_BASE_ADRS_I2C_10,
    AX3000_CSR_BASE_ADRS_I2C_11,
    AX3000_CSR_BASE_ADRS_I2C_12,
    AX3000_CSR_BASE_ADRS_I2C_13,
    AX3000_CSR_BASE_ADRS_I2C_14,
    AX3000_CSR_BASE_ADRS_I2C_15,
    AX3000_CSR_BASE_ADRS_I2C,
    AX3000_CSR_BASE_ADRS_I2C_17,
    AX3000_CSR_BASE_ADRS_I2C_18,
    AX3000_CSR_BASE_ADRS_I2C_19,
    AX3000_CSR_BASE_ADRS_I2C_20,
    AX3000_CSR_BASE_ADRS_I2C_21,
    AX3000_CSR_BASE_ADRS_I2C_22,
    AX3000_CSR_BASE_ADRS_I2C_23,
    AX3000_CSR_BASE_ADRS_I2C_24,
    AX3000_CSR_BASE_ADRS_I2C_25,
    AX3000_CSR_BASE_ADRS_I2C_26,
    AX3000_CSR_BASE_ADRS_I2C_27,
    AX3000_CSR_BASE_ADRS_I2C_28,
    AX3000_CSR_BASE_ADRS_I2C_29,
    AX3000_CSR_BASE_ADRS_I2C_30,
    AX3000_CSR_BASE_ADRS_I2C_31,
    AX3000_CSR_BASE_ADRS_I2C_32,
};

#define CDNS_I2C_CR_OFFSET      0X00
#define CDNS_I2C_SR_OFFSET      0x04
#define CDNS_I2C_AR_OFFSET      0x08
#define CDNS_I2C_DR_OFFSET      0x0C
#define CDNS_I2C_ISR_OFFSET     0x10
#define CDNS_I2C_TSR_OFFSET     0x14
#define CDNS_I2C_SMPR_OFFSET    0x18
#define CDNS_I2C_TOR_OFFSET     0x1C
#define CDNS_I2C_IMR_OFFSET     0x20
#define CDNS_I2C_IER_OFFSET     0x24
#define CDNS_I2C_IDR_OFFSET     0x28

#define I2C_SPEED_NORM_RATE 1000000
#define I2C_SPEED_FAST_RATE 4000000

/* Control register fields */
#define CDNS_I2C_CONTROL_RW		0x00000001
#define CDNS_I2C_CONTROL_MS		0x00000002
#define CDNS_I2C_CONTROL_NEA		0x00000004
#define CDNS_I2C_CONTROL_ACKEN		0x00000008
#define CDNS_I2C_CONTROL_HOLD		0x00000010
#define CDNS_I2C_CONTROL_SLVMON		0x00000020
#define CDNS_I2C_CONTROL_CLR_FIFO	0x00000040
#define CDNS_I2C_CONTROL_DIV_B_SHIFT	8
#define CDNS_I2C_CONTROL_DIV_B_MASK	0x00003F00
#define CDNS_I2C_CONTROL_DIV_A_SHIFT	14
#define CDNS_I2C_CONTROL_DIV_A_MASK	0x0000C000

/* Status register values */
#define CDNS_I2C_STATUS_RXDV	0x00000020
#define CDNS_I2C_STATUS_TXDV	0x00000040
#define CDNS_I2C_STATUS_RXOVF	0x00000080
#define CDNS_I2C_STATUS_BA	0x00000100

/* Interrupt register fields */
#define CDNS_I2C_INTERRUPT_COMP		0x00000001
#define CDNS_I2C_INTERRUPT_DATA		0x00000002
#define CDNS_I2C_INTERRUPT_NACK		0x00000004
#define CDNS_I2C_INTERRUPT_TO		0x00000008
#define CDNS_I2C_INTERRUPT_SLVRDY	0x00000010
#define CDNS_I2C_INTERRUPT_RXOVF	0x00000020
#define CDNS_I2C_INTERRUPT_TXOVF	0x00000040
#define CDNS_I2C_INTERRUPT_RXUNF	0x00000080
#define CDNS_I2C_INTERRUPT_ARBLOST	0x00000200

#define CDNS_I2C_INTERRUPTS_MASK	(CDNS_I2C_INTERRUPT_COMP | \
        CDNS_I2C_INTERRUPT_DATA | \
        CDNS_I2C_INTERRUPT_NACK | \
        CDNS_I2C_INTERRUPT_TO | \
        CDNS_I2C_INTERRUPT_SLVRDY | \
        CDNS_I2C_INTERRUPT_RXOVF | \
        CDNS_I2C_INTERRUPT_TXOVF | \
        CDNS_I2C_INTERRUPT_RXUNF | \
        CDNS_I2C_INTERRUPT_ARBLOST)

#define CDNS_I2C_FIFO_DEPTH_DEFAULT	16
#define CDNS_I2C_TRANSFER_SIZE_MAX	255 /* Controller transfer limit */
#define CDNS_I2C_TRANSFER_SIZE		(CDNS_I2C_TRANSFER_SIZE_MAX - 3)

#define CDNS_I2C_BROKEN_HOLD_BIT	BIT(0)

#define CDNS_I2C_ARB_LOST_MAX_RETRIES	10

#define CDNS_I2C_RXDV_TIMEOUT_MS	1000

#define CONFIG_I2C_CLK 100000000

#define CONFIG_I2C_MAX_DEPTH    32

#define CDNS_I2C_DIVA_MAX	4
#define CDNS_I2C_DIVB_MAX	64

/* Wait for an interrupt */
static u32 cdns_i2c_wait(uint bus, u32 mask)
{
    void __iomem *base = (void __iomem *)i2c_base[bus];
	int timeout, int_status;

	for (timeout = 0; timeout < 100; timeout++) {
		int_status = readl(base + CDNS_I2C_ISR_OFFSET);
		if (int_status & mask)
			break;
		udelay(100);
	}

	/* Clear interrupt status flags */
	writel(int_status & mask, base + CDNS_I2C_ISR_OFFSET);

	return int_status & mask;
}

static int cdns_i2c_calc_divs(unsigned long *f, unsigned long input_clk,
        unsigned int *a, unsigned int *b)
{
    unsigned long fscl = *f, best_fscl = *f, actual_fscl, temp;
    unsigned int div_a, div_b, calc_div_a = 0, calc_div_b = 0;
    unsigned int last_error, current_error;

    /* calculate (divisor_a+1) x (divisor_b+1) */
    temp = input_clk / (22 * fscl);

    /*
     * If the calculated value is negative or 0CDNS_I2C_DIVA_MAX,
     * the fscl input is out of range. Return error.
     */
    if (!temp || (temp > (CDNS_I2C_DIVA_MAX * CDNS_I2C_DIVB_MAX)))
        return -EINVAL;

    last_error = -1;
    for (div_a = 0; div_a < CDNS_I2C_DIVA_MAX; div_a++) {
        div_b = DIV_ROUND_UP(input_clk, (22 * fscl * (div_a + 1) - 1));

        if ((div_b < 1) || (div_b > CDNS_I2C_DIVB_MAX))
            continue;
        div_b--;

        actual_fscl = input_clk / (22 * (div_a + 1) * (div_b + 1));

        if (actual_fscl > fscl)
            continue;

        current_error = ((actual_fscl > fscl) ? (actual_fscl - fscl) :
                (fscl - actual_fscl));

        if (last_error > current_error) {
            calc_div_a = div_a;
            calc_div_b = div_b;
            best_fscl = actual_fscl;
            last_error = current_error;
        }
    }

    *a = calc_div_a;
    *b = calc_div_b;
    *f = best_fscl;

    return 0;
}

static int cdns_i2c_set_bus_speed(uint bus, unsigned int speed)
{
    void __iomem *base = (void __iomem *)i2c_base[bus];
    u32 div_a = 0, div_b = 0;
    unsigned long speed_p = speed;
    int ret = 0;
    char name[5];

    if (bus == 16)
        sprintf(name, "I2C");
    else
        sprintf(name, "I2C%d", bus);

    ax_pinmux_set_state(name);

    if (speed > I2C_SPEED_FAST_RATE) {
        printf("%s, failed to set clock speed to %u\n", __func__,
                speed);
        return -EINVAL;
    }

    ret = cdns_i2c_calc_divs(&speed_p, CONFIG_I2C_CLK, &div_a, &div_b);
    if (ret)
        return ret;

    printf("%s: I2C_%d:%p \ndiv_a: %d, div_b: %d, input freq: %d, speed: %d/%ld\n",
            __func__, bus, base, div_a, div_b, CONFIG_I2C_CLK, speed, speed_p);

    writel((div_b << CDNS_I2C_CONTROL_DIV_B_SHIFT) |
            (div_a << CDNS_I2C_CONTROL_DIV_A_SHIFT) , base + CDNS_I2C_CR_OFFSET);

    /* Enable master mode, ack, and 7-bit addressing */
    setbits_le32(base + CDNS_I2C_CR_OFFSET, CDNS_I2C_CONTROL_MS |
            CDNS_I2C_CONTROL_ACKEN | CDNS_I2C_CONTROL_NEA);

    writel(CDNS_I2C_INTERRUPTS_MASK, base + CDNS_I2C_IDR_OFFSET);
    writel(CDNS_I2C_INTERRUPTS_MASK, base + CDNS_I2C_ISR_OFFSET);

    return 0;
}

static int cdns_i2c_write(uint bus, uint addr, uint reg, uint len, u8 *buf) {  
    void __iomem *base = (void __iomem *)i2c_base[bus];
	u32 ret;
    uint i;

    /* Set the controller in Master transmit mode and clear FIFO */
	clrbits_32(base + CDNS_I2C_CR_OFFSET, CDNS_I2C_CONTROL_RW);
	setbits_32(base + CDNS_I2C_CR_OFFSET, CDNS_I2C_CONTROL_CLR_FIFO);

    /* Set target write address */
	writel(reg, base + CDNS_I2C_DR_OFFSET);

    for (i = 0; i < len; i++) {
        writel(buf[i], base + CDNS_I2C_DR_OFFSET);
    }

    /* Trigger write only after loading data */
    writel(addr, base + CDNS_I2C_AR_OFFSET);

	ret = cdns_i2c_wait(bus, CDNS_I2C_INTERRUPT_COMP);
	if (!(ret & (CDNS_I2C_INTERRUPT_COMP))) {
		return -ETIMEDOUT;
    }

    return 0;
}

static int cdns_i2c_read(uint bus, uint addr, uint reg, uint len, u8 *buf) {  
    void __iomem *base = (void __iomem *)i2c_base[bus];
	u32 ret;
    uint i;

    /* Set the controller in Master receive mode and clear FIFO */
	clrbits_le32(base + CDNS_I2C_CR_OFFSET, CDNS_I2C_CONTROL_RW);
	setbits_le32(base + CDNS_I2C_CR_OFFSET, CDNS_I2C_CONTROL_CLR_FIFO);
    setbits_le32(base + CDNS_I2C_CR_OFFSET, CDNS_I2C_CONTROL_HOLD);

    /* Set target read address */
	writel(reg, base + CDNS_I2C_DR_OFFSET);

    writel(addr, base + CDNS_I2C_AR_OFFSET);

	ret = cdns_i2c_wait(bus, CDNS_I2C_INTERRUPT_COMP);
	if (!(ret & (CDNS_I2C_INTERRUPT_COMP))) {
		return -ETIMEDOUT;
    }

	setbits_le32(base + CDNS_I2C_CR_OFFSET, CDNS_I2C_CONTROL_CLR_FIFO);
	setbits_le32(base + CDNS_I2C_CR_OFFSET, CDNS_I2C_CONTROL_RW);
    writel(len, base + CDNS_I2C_TSR_OFFSET);
    clrbits_le32(base + CDNS_I2C_CR_OFFSET, CDNS_I2C_CONTROL_HOLD);

    writel(addr, base + CDNS_I2C_AR_OFFSET);

	ret = cdns_i2c_wait(bus, CDNS_I2C_INTERRUPT_COMP);
	if (!(ret & (CDNS_I2C_INTERRUPT_COMP))) {
		return -ETIMEDOUT;
    }

    for (i = 0; i < len; i++) {
        buf[i] = readl(base + CDNS_I2C_DR_OFFSET);
    }

    return 0;
}

struct ax_i2c_ops i2c = {
    .set_bus_speed = cdns_i2c_set_bus_speed,
    .read = cdns_i2c_read,
    .write = cdns_i2c_write,
};
