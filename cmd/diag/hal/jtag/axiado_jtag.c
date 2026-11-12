// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2026
 * Nguyen Nam Huy hnnguyen@axiado.com
 */

#include <dm.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/bitfield.h>

#include "ax_diag.h"

#include "ax_jtag.h"
#include "ax_pinmux.h"
#include "axiado_jtag.h"

#define AXIADO_HOST_JTAG_APB_CLK    125000000

/* DSUB_JTAG block (offsets relative to the mapped reg base). */
#define REG_JTAG_CSW		0x0d00
#define REG_JTAG_PSEL		0x0d04
#define REG_JTAG_PSTA		0x0d08
#define REG_JTAG_BFIFO1		0x0d10
#define REG_JTAG_BFIFO2		0x0d14
#define REG_JTAG_BFIFO3		0x0d18
#define REG_JTAG_BFIFO4		0x0d1c
#define REG_JTAG_IDR		0x0dfc

/* CSW fields */
#define CSW_SRST_OUT		BIT(0)
#define CSW_TRST_OUT		BIT(1)
#define CSW_PORTCONNECTED	BIT(3)
#define CSW_SERACTV		BIT(31)

/* LOCAL.HOST_JTAG fields */
#define AXIADO_HOST_JTAG_ENABLE	BIT(0)
#define AXIADO_HOST_JTAG_TCK_SEL_MASK	GENMASK(5, 4)

/*
 * BFIFO shift command opcode.
 *
 * Performs simultaneous TDI write and TDO capture while asserting
 * TMS during the final shift clock to exit Shift-DR/Shift-IR.
 */
#define SHIFT_OPCODE_TDI_TDO	0x8cU

/* Expected CSS600 JTAG-AP identification register value. */
#define JTAG_AP_IDR_EXPECTED	0x24760020U

/*
 * Maximum transfer size supported by the driver.
 *
 * The Linux JTAG framework passes transfer buffers through userspace,
 * and the driver limits a single transfer to 256 bits.
 */
#define AXIADO_MAX_XFER_BITS	256U
#define AXIADO_MAX_XFER_BYTES   DIV_ROUND_UP(AXIADO_MAX_XFER_BITS, 8)

/*
 * Driver private data.
 *
 * One instance is allocated for each controller by jtag_alloc() and
 * stores the MMIO base, device pointer, and synchronization primitive.
 */
struct axiado_jtag {
	void __iomem *base;
	void __iomem *host_jtag;
};
static struct axiado_jtag *ajtag;

/**
 * axiado_wait_idle() - Wait for the JTAG serial engine to become idle.
 * @p: AX3005 JTAG private driver data.
 *
 * Polls the SERACTV bit in the Control/Status Register (CSW) until the
 * hardware completes the current serial operation. All JTAG shift and
 * TMS commands must wait for the previous command to finish before
 * programming the next BFIFO transaction.
 *
 * Return: 0 on success or -ETIMEDOUT if the serial engine remains busy.
 */
static int axiado_wait_idle(struct axiado_jtag *p)
{
	u32 v;
	int ret;

	/* printf("wait_idle: waiting for SERACTV clear\n"); */

	ret = readl_poll_timeout(p->base + REG_JTAG_CSW, v,
				 !(v & CSW_SERACTV), 200000);

	/* if (ret) */
	/* 	printf("%s: timeout CSW=0x%08x\n", __func__, v); */
	/* else */
	/* 	printf("%s: done CSW=0x%08x\n", __func__, v); */

	return ret;
}

/**
 * axiado_tms() - Execute a JTAG TAP state transition.
 * @p: AX3005 JTAG private driver data.
 * @pattern: TMS bit sequence (LSB shifted first).
 * @numbits: Number of TMS clocks (1-5).
 *
 * Builds a hardware TMS command and writes it into BFIFO1. The controller
 * clocks the requested TMS sequence to move the TAP controller between
 * JTAG states without shifting IR or DR data.
 *
 * Return: 0 on success or a negative error code.
 */
static int axiado_tms(struct axiado_jtag *p, u8 pattern, u8 numbits)
{
	u32 val = (1U << numbits) | (pattern & 0x1fU);

	/* printf("%s: pattern=0x%x bits=%u cmd=0x%08x\n", */
	/* 	__func__, pattern, numbits, val); */

	writel(val, p->base + REG_JTAG_BFIFO1);

	return axiado_wait_idle(p);
}

/**
 * axiado_fifo_io() - Transfer one FIFO data block.
 * @p: AX3005 JTAG private driver data.
 * @data_in: Data to transmit on TDI.
 * @numbits: Number of bits to shift (1-32).
 * @data_out: Optional buffer receiving sampled TDO bits.
 *
 * Writes one data block into the hardware write FIFO and immediately reads
 * the corresponding receive FIFO entry. The FIFO register width is selected
 * automatically according to the transfer size.
 *
 * The receive FIFO is always accessed using its native register width.
 * For transfers that are not byte-aligned (e.g. 17-31 bits), the unused
 * upper bits are masked off before returning the sampled TDO data.
 */
static void axiado_fifo_io(struct axiado_jtag *p,
			   u32 data_in,
			   u8 numbits,
			   u32 *data_out)
{
	void __iomem *fiforeg;
	u32 dout = 0;

	if (numbits > 24)
		fiforeg = p->base + REG_JTAG_BFIFO4;
	else if (numbits > 16)
		fiforeg = p->base + REG_JTAG_BFIFO3;
	else if (numbits > 8)
		fiforeg = p->base + REG_JTAG_BFIFO2;
	else
		fiforeg = p->base + REG_JTAG_BFIFO1;

	/*
	 * Write TDI data to the transmit FIFO.
	 * Reading the same FIFO register returns the corresponding
	 * sampled TDO data.
	 */
	writel(data_in, fiforeg);

	if (numbits > 16) {
		/*
		 * Read the full 32-bit FIFO entry.
		 *
		 * For 17-24 bit transfers, using readl() avoids incorrect
		 * byte reconstruction (e.g. readw() << 8 | readb()) on
		 * little-endian systems. Mask off any unused upper bits.
		 */
		dout = readl(fiforeg);

		if (numbits < 32)
			dout &= GENMASK(numbits - 1, 0);

	} else if (numbits > 8) {
		dout = readw(fiforeg);

		if (numbits < 16)
			dout &= GENMASK(numbits - 1, 0);

	} else {
		dout = readb(fiforeg);

		if (numbits < 8)
			dout &= GENMASK(numbits - 1, 0);
	}

	if (data_out)
		*data_out = dout;
}

/**
 * axiado_shift() - Shift arbitrary data through the JTAG scan chain.
 * @p: AX3005 JTAG private driver data.
 * @tx_buf: Buffer containing TDI data.
 * @numbits: Number of bits to shift.
 * @rx_buf: Optional receive buffer for TDO data.
 *
 * Programs the hardware shift command and transfers the data in chunks of
 * up to 32 bits using the BFIFO interface. This function is the common
 * backend used for both IR and DR scan operations.
 *
 * Return: 0 on success or a negative error code.
 */
static int axiado_shift(struct axiado_jtag *p,
			const u8 *tx_buf,
			u32 numbits,
			u8 *rx_buf)
{
	u8 opcode;
	u8 length;
	u32 remaining_bits;
	u32 byte_off = 0;
	u32 fifo_in;
	u32 fifo_out;
	u32 chunk_bits;
	u32 chunk_bytes;

	if (!numbits || numbits > AXIADO_MAX_XFER_BITS)
		return -EINVAL;

	/*
	 * Packet header
	 *
	 * opcode:
	 *   bit[7:5] = 100 (TDI_TDO)
	 *   bit[3]   = 1 (TMS on last bit)
	 *   bit[2]   = 1 (capture TDO)
	 */

	opcode = SHIFT_OPCODE_TDI_TDO;

	length = numbits - 1;

	writel(((u32)length << 8) | opcode,
	       p->base + REG_JTAG_BFIFO2);

	remaining_bits = numbits;

	while (remaining_bits) {

		chunk_bits = min(remaining_bits, 32U);
		chunk_bytes = DIV_ROUND_UP(chunk_bits, 8);

		fifo_in = 0;

		memcpy(&fifo_in, tx_buf + byte_off, chunk_bytes);

		if (chunk_bits < 32)
			fifo_in &= GENMASK(chunk_bits - 1, 0);

		axiado_fifo_io(p,
			       fifo_in,
			       chunk_bits,
			       &fifo_out);

		if (rx_buf)
			memcpy(rx_buf + byte_off,
			       &fifo_out,
			       chunk_bytes);

		byte_off += chunk_bytes;
		remaining_bits -= chunk_bits;
	}

	/* printf("shift complete (%u bits)\n", */
	/* 	numbits); */

	return 0;
}

/**
 * axiado_shift_ir() - Shift an instruction into the Instruction Register.
 * @p: AX3005 JTAG private driver data.
 * @tx: Instruction bits.
 * @bits: Instruction length in bits.
 *
 * Moves the TAP controller from Run-Test/Idle to Shift-IR, shifts the
 * instruction bits, and returns the TAP to Run-Test/Idle.
 *
 * Return: 0 on success or a negative error code.
 */
static int axiado_shift_ir(struct axiado_jtag *p,
			   const u8 *tx,
			   u32 bits)
{
	int ret;

	ret = axiado_tms(p, 0x3, 4);   /* RTI->ShiftIR */
	if (ret)
		return ret;

	ret = axiado_shift(p, tx, bits, NULL);
	if (ret)
		return ret;

	return axiado_tms(p, 0x1, 2);  /* ExitIR->UpdateIR->RTI */
}

/**
 * axiado_shift_dr() - Shift data through the Data Register.
 * @p: AX3005 JTAG private driver data.
 * @tx: Data to transmit.
 * @rx: Optional receive buffer.
 * @bits: Number of bits to shift.
 *
 * Moves the TAP controller into Shift-DR, performs the requested scan,
 * captures any returned TDO bits, and restores the TAP to Run-Test/Idle.
 *
 * Return: 0 on success or a negative error code.
 */
static int axiado_shift_dr(struct axiado_jtag *p,
			   const u8 *tx,
			   u8 *rx,
			   u32 bits)
{
	int ret;

	ret = axiado_tms(p, 0x1, 3);
	if (ret)
		return ret;

	ret = axiado_shift(p, tx, bits, rx);
	if (ret)
		return ret;

	return axiado_tms(p, 0x1, 2);
}

/**
 * axiado_trst_pulse() - Generate a TRST pulse.
 * @p: AX3005 JTAG private driver data.
 * @assert_ms: Time to hold TRST asserted.
 * @release_ms: Delay after releasing TRST.
 *
 * Drives the JTAG TRST output to reset the external TAP controller before
 * beginning communication.
 */
static void axiado_trst_pulse(struct axiado_jtag *p,
			      unsigned int assert_ms,
			      unsigned int release_ms)
{
	u32 v;

	v = readl(p->base + REG_JTAG_CSW);

	/* printf("%s: assert=%ums release=%ums\n", */
	/* 	__func__, assert_ms, release_ms); */

	writel(v | CSW_TRST_OUT,  p->base + REG_JTAG_CSW);
	mdelay(assert_ms);
	writel(v & ~CSW_TRST_OUT, p->base + REG_JTAG_CSW);
	mdelay(assert_ms);

	/* printf("TRST pulse done\n"); */
}

/**
 * axiado_hw_deinit() - Deinitialize the AX3005 JTAG controller.
 * @p: AX3005 JTAG private driver data.
 *
 * Disables the host JTAG interface and releases the peripheral clock
 * previously enabled by axiado_hw_init(). This function is called when
 * the JTAG controller is disabled or the driver is removed to place the
 * hardware into an inactive state.
 */
static int axiado_jtag_hw_deinit(struct axiado_jtag *p)
{
	u32 v;

    v = readl(p->host_jtag);
    writel(v & ~AXIADO_HOST_JTAG_ENABLE, p->host_jtag);
    printf("Host jtag [0x%08x -> 0x%08x]\n", v, readl(p->host_jtag));

	printf("%s done\n", __func__);

	return 0;
}

/**
 * axiado_hw_init() - Initialize the AX3005 JTAG controller.
 * @p: AX3005 JTAG private driver data.
 *
 * Enables the JTAG host interface, selects JTAG Port 0, waits for the
 * controller to become idle, resets the external TAP using TRST, and
 * places the TAP into the Run-Test/Idle state.
 *
 * Return: 0 on success or a negative error code.
 */
static int axiado_jtag_hw_init(struct axiado_jtag *p)
{
    int ret;
    u32 v;

    /* Enable Host JTAG */
    v = readl(p->host_jtag);
    writel(v | AXIADO_HOST_JTAG_ENABLE, p->host_jtag);
    printf("Host jtag [0x%08x -> 0x%08x]\n", v, readl(p->host_jtag));

    /* Select JTAG Port 0 */
	writel(0x1, p->base + REG_JTAG_PSEL);

    ret = axiado_wait_idle(p);
	if (ret)
        return ret;

    /* Reset external TAP */
	axiado_trst_pulse(p, 50, 50);

	/* Test-Logic-Reset */
	ret = axiado_tms(p, 0x1f, 5);
	if (ret)
        return ret;

	/* Run-Test/Idle */
	ret = axiado_tms(p, 0x0, 1);
	if (ret)
        return ret;

	printf("%s done\n", __func__);

    return 0;
}

/* ------------------------------------------------------------------ */
/* JTAG framework ops                                                  */
/* ------------------------------------------------------------------ */

/**
 * axiado_jtag_xfer() - Execute a JTAG scan transaction.
 * @port: Linux JTAG device.
 * @xfer: Transfer descriptor.
 * @data: Shared transmit/receive buffer.
 *
 * Implements the Linux JTAG framework transfer callback. Depending on the
 * transfer type, the function performs either an IR or DR scan while
 * optionally capturing returned TDO data.
 *
 * Return: 0 on success or a negative error code.
 */
static int axiado_jtag_xfer(u8 port,
			    struct jtag_xfer *xfer,
			    u8 *data)
{
	struct axiado_jtag *p = ajtag;
	u8 dout[AXIADO_MAX_XFER_BYTES] = {0};
	u32 bytes;
	int ret;

    (void)port;

	if (!xfer->length || xfer->length > AXIADO_MAX_XFER_BITS)
		return -EINVAL;

	bytes = DIV_ROUND_UP(xfer->length, 8);

	if (xfer->type == JTAG_SIR_XFER)
		ret = axiado_shift_ir(p, data, xfer->length);
	else
		ret = axiado_shift_dr(p,
				      data,
				      dout,
				      xfer->length);

	if (!ret && (xfer->direction & JTAG_READ_XFER))
		memcpy(data, dout, bytes);

	return ret;
}

/**
 * axiado_jtag_enable() - Enable the AX3005 JTAG controller.
 * @port: Linux JTAG device.
 *
 * Initializes the hardware and prepares the JTAG controller for scan
 * operations.
 *
 * Return: 0 on success or a negative error code.
 */
static int axiado_jtag_enable(u8 port)
{
	struct axiado_jtag *p;
    u32 idr;
    int ret;

    (void)port;

    /* Allocate once; a repeated enable re-initialises the same context. */
    if (!ajtag) {
        ajtag = (struct axiado_jtag *)malloc(sizeof(struct axiado_jtag));
        if (!ajtag)
            return -ENOMEM;
    }
    p = ajtag;

    p->base = (void __iomem *)AX3000_CSR_BASE_ADRS_DSUB_JTAG;
    p->host_jtag = (void __iomem *)AX3000_CSR_BASE_ADRS_DSUB_JTAG + 0x1c14;

    idr = readl(p->base + REG_JTAG_IDR);
    if (idr != JTAG_AP_IDR_EXPECTED)
        printf("Unexpected JTAG-AP IDR 0x%08x (expected 0x%08x)\n",
                idr, JTAG_AP_IDR_EXPECTED);
    else
        printf("JTAG-AP IDR 0x%08x\n", idr);

    ret = axiado_jtag_hw_init(p);
    if (ret)
        return ret;
    printf("Jtag enabled\n");

    return 0;
}

/**
 * axiado_jtag_disable() - Disable the AX3005 JTAG controller.
 * @jtag: Linux JTAG device.
 *
 * Disables the JTAG host interface to stop further scan operations.
 *
 * Return: Always 0.
 */
static int axiado_jtag_disable(u8 port)
{
	struct axiado_jtag *p = ajtag;

    (void)port;

    if (!p)
        return 0;

    axiado_jtag_hw_deinit(p);
    free(p);
    ajtag = NULL;
    printf("Jtag disabled\n");
    return 0;
}

/**
 * axiado_jtag_status_set() - Update the TAP controller state.
 * @port: Linux JTAG device.
 * @tap: Requested TAP state.
 *
 * Supports forcing the TAP controller into Test-Logic-Reset by issuing the
 * required TMS sequence.
 *
 * Return: 0 on success or a negative error code.
 */
static int axiado_jtag_status_set(u8 port,
				  struct jtag_tap_state *tap)
{
	struct axiado_jtag *p = ajtag;
	int ret = 0;

	if (tap->reset == JTAG_FORCE_RESET) {
		ret = axiado_tms(p, 0x1f, 5);   /* -> TLR */
		if (!ret)
			ret = axiado_tms(p, 0x0, 1); /* TLR -> RTI */
	}
	return ret;
}

/**
 * axiado_jtag_status_get() - Report the current TAP state.
 * @port: Linux JTAG device.
 * @state: Returned TAP state.
 *
 * Returns the driver's current view of the TAP state. The controller does
 * not track TAP state in hardware, therefore Run-Test/Idle is reported.
 *
 * Return: Always 0.
 */
static int axiado_jtag_status_get(u8 port, u32 *state)
{
    (void)port;
	*state = JTAG_STATE_IDLE;
	return 0;
}

/**
 * axiado_jtag_mode_set() - Configure the JTAG operating mode.
 * @port: Linux JTAG device.
 * @mode: Requested operating mode.
 *
 * The AX3005 controller currently supports only JTAG mode. This callback
 * is provided for compatibility with the Linux JTAG framework.
 *
 * Return: Always 0.
 */
static int axiado_jtag_mode_set(u8 port, struct jtag_mode *mode)
{
    (void)port;
	return 0;
}

/**
 * axiado_freq_to_sel() - Convert a requested TCK frequency into hardware
 *                        divider selection.
 * @freq: Desired TCK frequency in Hz.
 *
 * Return: Hardware divider selector value.
 */
static unsigned int axiado_freq_to_sel(unsigned long apb_clk, u32 freq)
{
	if (!freq || freq < apb_clk / 6)
		return 3;

	if (freq < apb_clk / 4)
		return 2;

	if (freq < apb_clk / 2)
		return 1;

	return 0;
}

/**
 * axiado_jtag_freq_set() - Configure the JTAG clock frequency.
 * @port: Linux JTAG device.
 * @freq: Requested TCK frequency in Hz.
 *
 * Programs the hardware clock divider to generate the closest supported
 * JTAG clock frequency.
 *
 * Return: Always 0.
 */
static int axiado_jtag_freq_set(u8 port, u32 freq)
{
	struct axiado_jtag *p = ajtag;
	unsigned long apb_clk;
	unsigned int sel;
	u32 v;

    (void)port;

	if (!freq)
		return -EINVAL;

	apb_clk = AXIADO_HOST_JTAG_APB_CLK;
	if (!apb_clk)
		return -EOPNOTSUPP;

	sel = axiado_freq_to_sel(apb_clk, freq);
	v = readl(p->host_jtag);
	v &= ~AXIADO_HOST_JTAG_TCK_SEL_MASK;
	v |= FIELD_PREP(AXIADO_HOST_JTAG_TCK_SEL_MASK, sel);
	writel(v, p->host_jtag);
	printf("%s: APB=%lu Hz requested=%u Hz selected=%u HOST_JTAG=0x%08x\n",
		__func__, apb_clk, freq, sel, readl(p->host_jtag));
	return 0;
}

/**
 * axiado_jtag_freq_get() - Return the configured JTAG clock frequency.
 * @jtag: Linux JTAG device.
 * @freq: Returned frequency in Hz.
 *
 * Calculates the effective TCK frequency from the programmed divider.
 *
 * Return: Always 0.
 */
static int axiado_jtag_freq_get(u8 port, u32 *freq)
{
	struct axiado_jtag *p = ajtag;
	unsigned long apb_clk;
	unsigned int sel;
	u32 v;

	if (!freq)
		return -EINVAL;

	apb_clk = AXIADO_HOST_JTAG_APB_CLK;
	if (!apb_clk)
		return -EOPNOTSUPP;

	v = readl(p->host_jtag);
	sel = FIELD_GET(AXIADO_HOST_JTAG_TCK_SEL_MASK, v);

	*freq = apb_clk / (2U * (sel + 1U));

	printf("%s: HOST_JTAG=0x%08x APB=%lu Hz divider=%u TCK=%u Hz\n",
		__func__, v, apb_clk, sel, *freq);

	return 0;
}

/**
 * axiado_jtag_trst_set() - Control the TRST output signal.
 * @jtag: Linux JTAG device.
 * @active: Non-zero to assert TRST, zero to release it.
 *
 * Drives the dedicated TRST output pin connected to the external TAP.
 *
 * Return: Always 0.
 */
static int axiado_jtag_trst_set(u8 port, u32 active)
{
	struct axiado_jtag *p = ajtag;
	u32 v;

	v = readl(p->base + REG_JTAG_CSW);
	if (active)
		v |= CSW_TRST_OUT;
	else
		v &= ~CSW_TRST_OUT;
	writel(v, p->base + REG_JTAG_CSW);
	return 0;
}

/**
 * axiado_jtag_bitbang() - Execute single-bit JTAG clock operations.
 * @jtag: Linux JTAG device.
 * @bitbang: Bitbang request.
 * @data: Array describing TMS/TDI values for each clock.
 *
 * Executes one JTAG clock per array entry using the controller's TMS
 * command interface. Intended for debugging and framework compatibility.
 *
 * Return: 0 on success or a negative error code.
 */
static int axiado_jtag_bitbang(u8 port,
			       struct bitbang_packet *bitbang,
			       struct tck_bitbang *data)
{
	struct axiado_jtag *p = ajtag;
	u32 val;
	int i;
	int ret = 0;

	for (i = 0; i < bitbang->length; i++) {
		/*
		 * TMS packet (BFIFO1):
		 *   bit[7]   = 0  -> TMS command
		 *   bit[6]   = TDI value
		 *   bit[5:0] = one-hot length | TMS bit
		 */
		val = (1 << 1);
		if (data[i].tms)
			val |= 0x1;
		if (data[i].tdi)
			val |= BIT(6);

		/* printf("%s: bitbang[%d]: TMS=%d TDI=%d val=0x%08x\n", */
		/* 	__func__, i, data[i].tms, data[i].tdi, val); */

		writel(val, p->base + REG_JTAG_BFIFO1);

		ret = axiado_wait_idle(p);
		if (ret) {
			/* printf("bitbang wait idle timeout\n"); */
			goto out;
		}

		data[i].tdo = 0;
	}

out:
	return ret;
}
struct ax_jtag_ops jtag = {
	.enable		= axiado_jtag_enable,
	.disable	= axiado_jtag_disable,
	.xfer		= axiado_jtag_xfer,
	.status_set	= axiado_jtag_status_set,
	.status_get	= axiado_jtag_status_get,
	.freq_set	= axiado_jtag_freq_set,
	.freq_get	= axiado_jtag_freq_get,
	.mode_set	= axiado_jtag_mode_set,
	.trst_set	= axiado_jtag_trst_set,
	.bitbang	= axiado_jtag_bitbang,
};

