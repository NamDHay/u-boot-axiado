// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy namhuyngn03@gmail.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <linux/delay.h>

#include "ax3000_base_adrs.h"
#include "ax3000_slo_base_adrs.h"

#include "ax_i3c.h"
#include "ax_pinmux.h"

#include "cdns_i3c.h"

#define SYS_CLK 200000000 // 200Mhz

#define I3C_FREQ 2000000
#define I2C_FREQ 100000

#define CONFIG_MAX_I2C 33
static const ulong i3c_base[CONFIG_MAX_I2C] = {
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

static u32 cdns_i3c_wait(uint bus, u32 mask)
{
    void __iomem *base = (void __iomem *)i3c_base[bus];
	int timeout, int_status;

	for (timeout = 0; timeout < 100; timeout++) {
		int_status = readl(base + MST_STATUS0);
		if (int_status & mask)
			break;
		udelay(100);
	}

	/* Clear interrupt status flags */
    int_status = readl(base + MST_STATUS0);

	return int_status & mask;
}

static void cdns_i3c_flush_queue(uint bus)
{
    void __iomem *base = (void __iomem *)i3c_base[bus];
	clrbits_le32(base + CTRL, CTRL_DEV_EN); /* disable controller */
	writel(FLUSH_SLV_DDR_RX_FIFO | FLUSH_SLV_DDR_TX_FIFO |
            FLUSH_RX_FIFO | FLUSH_TX_FIFO |
            FLUSH_CMD_FIFO, base + FLUSH_CTRL);
	readl(base + CMDR);
	setbits_le32(base + CTRL, CTRL_DEV_EN); /* enable controller */
}

static void cdns_i3c_init_clock(uint bus, uint32_t i2c_freq, uint32_t i3c_freq)
{
    void __iomem *base = (void __iomem *)i3c_base[bus];
	uint32_t prscl0_v = 0;
	uint32_t pre_i2c = 0;
	uint32_t pre_i3c = 0;

	pre_i2c = SYS_CLK / (i2c_freq * 5) - 1;
	pre_i3c = SYS_CLK / (i3c_freq * 4) - 1;
	prscl0_v = (pre_i2c << 16) | (pre_i3c);

	clrbits_le32(base + CTRL, CTRL_DEV_EN); /* dis controller before init clock */

	writel(prscl0_v, base + PRESCL_CTRL0);
	writel(0x00001509, base + PRESCL_CTRL1);
	setbits_le32(base + CTRL, CTRL_DEV_EN); /* enable controller */
}

static int cdns_i3c_bus_init(u8 bus)
{
    char name[5];
    void __iomem *base = (void __iomem *)i3c_base[bus];
    sprintf(name, "I3C%d", bus);

    /* Request bypass mode and Pin mux */
    ax_pinmux_set_state(name);
    
	if (readl(base + DEV_ID) != DEV_ID_I3C_MASTER) {
        printf("%s:%p %x\n", name, base, readl(base + DEV_ID));
		return -EINVAL;
	}

    /* Set MST_IDR and SLV_IDR */
	writel(MST_IDR_SET, base + MST_IDR);
	writel(SLV_IDR_SET, base + SLV_IDR);

    /* Set CMD_IBI_THR_CTRL (set 24th bit to 1) */
	writel(CMD_IBI_THR_CTRL_SET, base + CMD_IBI_THR_CTRL);

    /* Enable MST_IER */
	writel(MST_IER_ENABLE, base + MST_IER);

    cdns_i3c_init_clock(bus, I2C_FREQ, I3C_FREQ);
    cdns_i3c_flush_queue(bus);

    /* Enable master with MCS */
    setbits_le32(base + CTRL, CTRL_MCS_EN | CTRL_MCS);
    setbits_le32(base + CTRL, CTRL_DEV_EN);

    return 0;
}

static u8 prepare_dev_addr(uint8_t slave_address_7bit)
{
	/* Ensure the input is a 7-bit value */
	uint8_t bits_7_to_1 = slave_address_7bit & 0x7F;

	/* XOR all bits from 7 to 1 */
	uint8_t parity = 0;
	for (int i = 0; i < 7; i++) {
		parity ^= (bits_7_to_1 >> i) & 0x1;
	}

	/* Invert the parity (XOR -> XNOR) to
	 * calculate bit 0 */
	uint8_t bit_0 = ~parity & 0x1;

	/* Append the parity bit as bit 0 to the
	 * original 7-bit address */
	uint8_t address_8bit = (bits_7_to_1 << 1) | bit_0;

	return address_8bit;
}

static int cdns_i3c_attach_dev(u8 bus, u8 slv_addr, u8 slot)
{
    void __iomem *base = (void __iomem *)i3c_base[bus];
    u32 rr0_val, dev_ctrl;

    clrbits_le32(base + CTRL, CTRL_DEV_EN); /* dis controller before edit RR */

    rr0_val = (u32)prepare_dev_addr(slv_addr);
    writel(rr0_val, base + DEV_ID_RR0(slot));

    dev_ctrl = readl(base + DEVS_CTRL);
    dev_ctrl |= (1 << slot);
    writel(dev_ctrl, base + DEVS_CTRL);

	setbits_le32(base + CTRL, CTRL_DEV_EN); /* enable controller */
    return 0;
}

static int cdns_i3c_cmdr_check_status(u8 bus)
{
    void __iomem *base = (void __iomem *)i3c_base[bus];

    u32 mst_stt_val = readl(base + MST_STATUS0);
    u32 cmdr, cmd_id, cmd_err, xfer_byte;
    if (!(mst_stt_val & MST_STATUS0_CMDR_EMP)) {
        cmdr =  readl(base + CMDR);
        cmd_id = CMDR_CMDID(cmdr);
        cmd_err = CMDR_ERROR(cmdr);
        xfer_byte = CMDR_XFER_BYTES(cmdr);

        if (cmd_err) {
			pr_err("====> CMD %X has "
				   "ERROR - ERROR "
				   "value: %X\n",
				   cmd_id, cmd_err);
			pr_err("Bytes transfer: "
				   "%d\n",
				   xfer_byte);
            return -EAGAIN;
        }
    }
    return 0;
}

static int cdns_i3c_read(u8 bus, uint slv_addr, uint reg, uint len, u8 *buf) 
{  
    void __iomem *base = (void __iomem *)i3c_base[bus];
    u32 cmd0, cmd1;
    u32 data_idx = 0;
    int ret, i;

    cdns_i3c_flush_queue(bus);

    /* Enable master with MCS */
    setbits_le32(base + CTRL, CTRL_MCS_EN | CTRL_MCS);
    setbits_le32(base + CTRL, CTRL_DEV_EN);

    readl(base + CMDR);

    cmd0 = CMD0_FIFO_DEV_ADDR(slv_addr) | CMD0_FIFO_PL_LEN(len) | CMD0_FIFO_RNW;  
    cmd1 = CMD1_FIFO_CMDID(0xCB) | reg;

    writel(cmd1, base + CMD1_FIFO);
    writel(cmd0, base + CMD0_FIFO);

    /* Trigger command process */
    setbits_le32(base + CTRL, CTRL_MCS | CTRL_DEV_EN);

    mdelay(10);
    ret = cdns_i3c_wait(bus, MST_STATUS0_IDLE);
	if (!(ret & (MST_STATUS0_IDLE))) {
		return -ETIMEDOUT;
    }

    ret = cdns_i3c_cmdr_check_status(bus);
    if (ret < 0)
        return ret;

    while (data_idx < len) {
        u32 val = readl(base + RX_FIFO);

        for (i = 0; i < 4 && data_idx < len; i++) {
            buf[data_idx] = (u8)(val >> (i * 8));
            data_idx++;
        }
    }

    cdns_i3c_flush_queue(bus);

    return 0;
}

static int cdns_i3c_write(u8 bus, uint slv_addr, uint reg, uint len, u8 *buf) 
{  
    void __iomem *base = (void __iomem *)i3c_base[bus];
    u16 data_idx = 0;
    u32 fifo_val = 0;
    u32 cmd0, cmd1;
    int ret, i;

    cdns_i3c_flush_queue(bus);

    /* Enable master with MCS */
    setbits_le32(base + CTRL, CTRL_MCS_EN | CTRL_MCS);
    setbits_le32(base + CTRL, CTRL_DEV_EN);

    readl(base + CMDR);

    while (data_idx < len) {
        fifo_val = 0;
        for (i = 0; i < 4 && data_idx < len; i++) {
            fifo_val = (fifo_val << 8) | buf[data_idx];
            data_idx++;
        }
        writel(fifo_val, base + TX_FIFO);
    }

    cmd0 = CMD0_FIFO_DEV_ADDR(slv_addr) | CMD0_FIFO_PL_LEN(len);  
    cmd1 = CMD1_FIFO_CMDID(0xCD) | reg;

    writel(cmd1, base + CMD1_FIFO);
    writel(cmd0, base + CMD0_FIFO);

    /* Trigger command process */
    setbits_le32(base + CTRL, CTRL_MCS | CTRL_DEV_EN);

    mdelay(10);
    ret = cdns_i3c_wait(bus, MST_STATUS0_IDLE);
	if (!(ret & (MST_STATUS0_IDLE))) {
		return -ETIMEDOUT;
    }

    ret = cdns_i3c_cmdr_check_status(bus);
    if (ret < 0)
        return ret;

    cdns_i3c_flush_queue(bus);

    return 0;
}

struct ax_i3c_ops i3c = {
    .bus_init = cdns_i3c_bus_init,
    /* .do_daa */
    .attach_dev = cdns_i3c_attach_dev,
    .read = cdns_i3c_read,
    .write = cdns_i3c_write,
};
