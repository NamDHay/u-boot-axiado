// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <asm/io.h>
#include <stdio.h>
#include <linux/delay.h>

#include "ax_diag.h"

#include "ax_i3c.h"
#include "ax_pinmux.h"

#include "cdns_i3c.h"

#define I3C_STATUS_TIMEOUT 10000
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

	for (timeout = 0; timeout < I3C_STATUS_TIMEOUT; timeout++) {
		int_status = readl(base + MST_STATUS0);
		if (int_status & mask)
			break;
		udelay(1);
	}

	/* Clear interrupt status flags */
    int_status = readl(base + MST_STATUS0);

	return int_status & mask;
}

static inline void cdns_i3c_writel_fifo(void __iomem *addr,
                                        const void *buf,
                                        int nbytes)
{
    const u8 *data = buf;
    u32 tmp;
    int words;

    words = nbytes / sizeof(u32);

    while (words--) {
        memcpy(&tmp, data, sizeof(tmp));
        writel(tmp, addr);
        data += sizeof(tmp);
    }

    if (nbytes & 3) {
        tmp = 0;
        memcpy(&tmp, data, nbytes & 3);
        writel(tmp, addr);
    }
}

static inline void cdns_i3c_readl_fifo(const void __iomem *addr,
                                       void *buf,
                                       int nbytes)
{
    u8 *data = buf;
    u32 tmp;
    int words;

    words = nbytes / sizeof(u32);

    while (words--) {
        tmp = readl(addr);
        memcpy(data, &tmp, sizeof(tmp));
        data += sizeof(tmp);
    }

    if (nbytes & 3) {
        tmp = readl(addr);
        memcpy(data, &tmp, nbytes & 3);
    }
}

static void cdns_i3c_flush_queue(uint bus)
{
    void __iomem *base = (void __iomem *)i3c_base[bus];
	clrbits_le32(base + CTRL, CTRL_DEV_EN); /* disable controller */
    writel(FLUSH_CMD_RESP |
            FLUSH_RX_FIFO |
            FLUSH_TX_FIFO |
            FLUSH_CMD_FIFO,
            base + FLUSH_CTRL);
	readl(base + CMDR);
	setbits_le32(base + CTRL, CTRL_DEV_EN); /* enable controller */
}

static int cdns_i3c_cmdr_check_status(u8 bus)
{
    void __iomem *base = (void __iomem *)i3c_base[bus];
    u32 cmdr, cmd_id, cmd_err, xfer_byte;
    u32 mst_stt_val;

    mst_stt_val = readl(base + MST_STATUS0);
    while (!(mst_stt_val & MST_STATUS0_CMDR_EMP)) {
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
            return -EFAULT;
        }
        mst_stt_val = readl(base + MST_STATUS0);
    }
    return 0;
}

static int cdns_i3c_submit_cmd(u8 bus,
                               u32 cmd0,
                               u32 cmd1)
{
    void __iomem *base;
    int ret;

    base = (void __iomem *)i3c_base[bus];

    writel(cmd1, base + CMD1_FIFO);
    writel(cmd0, base + CMD0_FIFO);

    setbits_le32(base + CTRL, CTRL_MCS);

    mdelay(50);
    ret = cdns_i3c_wait(bus, MST_STATUS0_IDLE);
    if (!(ret & (MST_STATUS0_IDLE))) {
        return -ETIMEDOUT;
    }

    return cdns_i3c_cmdr_check_status(bus);
}

static int cdns_i3c_init_clock(uint bus,
                               uint32_t i2c_freq,
                               uint32_t i3c_freq)
{
    void __iomem *base;
    uint32_t sysclk_rate = SYS_CLK;
    uint32_t pres;
    uint32_t prescl0;
    uint32_t prescl1;
    uint32_t pres_step;
    uint32_t ncycles;
    uint32_t actual_i2c_freq;
    uint32_t actual_i3c_freq;

    if (bus >= CONFIG_MAX_I2C)
        return -EINVAL;

    if (!i2c_freq || !i3c_freq)
        return -EINVAL;

    base = (void __iomem *)i3c_base[bus];

    /* Disable controller before changing clock configuration. */
    clrbits_le32(base + CTRL, CTRL_DEV_EN);

    /*
     * Calculate I3C prescaler
     * SCL = SYS_CLK / ((pres + 1) * 4)
     */
    pres = DIV_ROUND_UP(sysclk_rate, i3c_freq * 4) - 1;
    if (pres > PRESCL_CTRL0_I3C_MAX)
        return -ERANGE;

    actual_i3c_freq =
        sysclk_rate / ((pres + 1) * 4);

    prescl0 = PRESCL_CTRL0_I3C(pres);

    /*
     * Calculate I2C prescaler
     * SCL = SYS_CLK / ((pres + 1) * 5)
     */
    pres = (sysclk_rate / (i2c_freq * 5)) - 1;

    if (pres > PRESCL_CTRL0_I2C_MAX)
        return -ERANGE;

    actual_i2c_freq =
        sysclk_rate / ((pres + 1) * 5);

    prescl0 |= PRESCL_CTRL0_I2C(pres);

    /*
     * Program PRESCL_CTRL0.
     */
    writel(prescl0, base + PRESCL_CTRL0);

    /*
     * Calculate I3C Open-Drain LOW timing
     * pres_step = duration of one prescaler clock step in ns
     * pres_step = 1e9 / (actual_i3c_freq * 4)
     * ncycles = ceil(TLOW_OD_MIN / pres_step) - 2
     * ---------------------------------------------------------
     */
    pres_step = 1000000000U /
                (actual_i3c_freq * 4);

    ncycles = DIV_ROUND_UP(I3C_BUS_TLOW_OD_MIN_NS,
                           pres_step) - 2;

    if (ncycles < 0)
        ncycles = 0;

    prescl1 = PRESCL_CTRL1_OD_LOW(ncycles);

    writel(prescl1, base + PRESCL_CTRL1);

    /* Enable controller again. */
    setbits_le32(base + CTRL, CTRL_DEV_EN);

    return 0;
}

static int cdns_i3c_bus_init(u8 bus)
{
    int ret;
    char name[5];
    void __iomem *base = (void __iomem *)i3c_base[bus];
    sprintf(name, "I3C%d", bus);

    /* Request bypass mode and Pin mux */
    ax_pinmux_set_state(name);
    
	if (readl(base + DEV_ID) != DEV_ID_I3C_MASTER) {
        printf("%s:%p %x\n", name, base, readl(base + DEV_ID));
		return -EINVAL;
	}

    clrbits_le32(base + CTRL, CTRL_DEV_EN);

	/* init base config */
	/* Set MST_IDR and SLV_IDR */
	writel(MST_IDR_SET, base + MST_IDR);
	writel(SLV_IDR_SET, base + SLV_IDR);

	/*
	 * QUIRK: The AX3000-i3c controller may generate spurious IBI
	 * interrupts when standard I2C devices (which do not
	 * support IBI) are connected to the bus.
	 *
	 * Handling these unexpected IBIs can lead to a driver crash.
	 * To avoid this, disable IBIR handling entirely for this
	 * controller variant.
	 */
	writel(IBIR_THR(1), base + CMD_IBI_THR_CTRL);
    writel(MST_INT_IBIR_THR, base + MST_IER);

    writel(DEVS_CTRL_DEV_CLR_ALL, base + DEVS_CTRL);

    /* Enable MST_IER */
    writel(MST_IER_ENABLE, base + MST_IER);

    ret = cdns_i3c_init_clock(bus, I2C_FREQ, I3C_FREQ);
    if (ret) {
        printf("cdns_i3c_init_clock failed\n");
        return ret;
    }

    writel(FLUSH_CMD_RESP |
            FLUSH_RX_FIFO |
            FLUSH_TX_FIFO |
            FLUSH_CMD_FIFO,
            base + FLUSH_CTRL);
	readl(base + CMDR);

    setbits_le32(base + CTRL, CTRL_MCS_EN | I3C_BUS_MODE_MIXED_SLOW);

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

static int cdns_i3c_attach_i2c_dev(u8 bus, u8 slv_addr, u8 slot)
{
    void __iomem *base = (void __iomem *)i3c_base[bus];

    clrbits_le32(base + CTRL, CTRL_DEV_EN); /* dis controller before edit RR */

    writel((u32)prepare_dev_addr(slv_addr), base + DEV_ID_RR0(slot));

    writel(readl(base + DEVS_CTRL) |
           DEVS_CTRL_DEV_ACTIVE(slot), 
           base + DEVS_CTRL);

    setbits_le32(base + CTRL, CTRL_DEV_EN); /* enable controller */
    return 0;
}

static int cdns_i3c_detach_i2c_dev(u8 bus, u8 slot)
{
    void __iomem *base = (void __iomem *)i3c_base[bus];

    clrbits_le32(base + CTRL, CTRL_DEV_EN); /* dis controller before edit RR */

    writel(readl(base + DEVS_CTRL) |
           DEVS_CTRL_DEV_CLR(slot), 
           base + DEVS_CTRL);

    setbits_le32(base + CTRL, CTRL_DEV_EN); /* enable controller */
    return 0;
}

static int cdns_i3c_read(u8 bus, uint slv_addr, uint reg, uint len, u8 *buf)
{
    void __iomem *base;
    u32 cmd0, cmd1;
    int ret;

    if (bus >= CONFIG_MAX_I2C || !buf || !len)
        return -EINVAL;

    base = (void __iomem *)i3c_base[bus];

    ret = cdns_i3c_wait(bus, MST_STATUS0_IDLE);
    if (!(ret & (MST_STATUS0_IDLE))) {
        return -ETIMEDOUT;
    }
    cdns_i3c_flush_queue(bus);

    /* Do we need this ? */
    setbits_le32(base + CTRL, CTRL_MCS_EN | I3C_BUS_MODE_MIXED_SLOW);

    cmd0 = CMD0_FIFO_DEV_ADDR(slv_addr) | CMD0_FIFO_PL_LEN(len) | CMD0_FIFO_RNW;
    cmd1 = CMD1_FIFO_CMDID(0xCB) | reg;

    ret = cdns_i3c_submit_cmd(bus, cmd0, cmd1);
    if (ret)
        goto out;

    cdns_i3c_readl_fifo(base + RX_FIFO, buf, len);

out:
    cdns_i3c_flush_queue(bus);

    return ret;
}

static int cdns_i3c_write(u8 bus, uint slv_addr, uint reg, uint len, u8 *buf) 
{  
    void __iomem *base;
    u32 cmd0, cmd1;
    int ret;

    if (bus >= CONFIG_MAX_I2C || !buf || !len)
        return -EINVAL;

    base = (void __iomem *)i3c_base[bus];

    ret = cdns_i3c_wait(bus, MST_STATUS0_IDLE);
    if (!(ret & (MST_STATUS0_IDLE))) {
        return -ETIMEDOUT;
    }
    cdns_i3c_flush_queue(bus);

    /* Do we need this ? */
    setbits_le32(base + CTRL, CTRL_MCS_EN | I3C_BUS_MODE_MIXED_SLOW);

    cdns_i3c_writel_fifo(base + TX_FIFO, buf, len);

    cmd0 = CMD0_FIFO_DEV_ADDR(slv_addr) | CMD0_FIFO_PL_LEN(len);  
    cmd1 = CMD1_FIFO_CMDID(0xCD) | reg;

    ret = cdns_i3c_submit_cmd(bus, cmd0, cmd1);

    cdns_i3c_flush_queue(bus);

    return ret;
}


struct ax_i3c_ops i3c = {
    .bus_init = cdns_i3c_bus_init,
    .attach_dev = cdns_i3c_attach_i2c_dev,
    .detach_dev = cdns_i3c_detach_i2c_dev,
    .read = cdns_i3c_read,
    .write = cdns_i3c_write,
};

