// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013
 * David Feng <fenghua@phytium.com.cn>
 *
 * (C) Copyright 2026
 * Nguyen Nam Huy <namhuyngn03@gmail.com>
 */

#include <dm.h>
#include <asm/esr.h>
#include <asm/global_data.h>
#include <asm/ptrace.h>
#include <irq_func.h>
#include <linux/compiler.h>
#include <efi_loader.h>
#include <semihosting.h>
#include <asm/io.h>
#include <asm/gic.h>
#include <asm/gic-v3.h>

DECLARE_GLOBAL_DATA_PTR;

#define MAX_IRQS    320

struct irq_action {
    interrupt_handler_t *handler;
    void *arg;
};

static struct irq_action irq_table[MAX_IRQS];

static inline u32 gic_read_iar(void)
{
    u64 val;

    asm volatile("mrs %0, ICC_IAR1_EL1" : "=r"(val));

    return (u32)val;
}

static inline void gic_write_eoir(u32 irq)
{
    asm volatile("msr ICC_EOIR1_EL1, %0"
                 :
                 : "r"((u64)irq));

    asm volatile("isb");
}

int interrupt_init(void)
{
	enable_interrupts();

	return 0;
}

void enable_interrupts(void)
{
	return;
}

int disable_interrupts(void)
{
	return 0;
}

static void show_efi_loaded_images(struct pt_regs *regs)
{
	efi_print_image_infos((void *)regs->elr);
}

static void dump_far(unsigned long esr)
{
	unsigned long el, far;

	switch ((esr >> 26) & 0b111111) {
	case 0x20:
	case 0x21:
	case 0x24:
	case 0x25:
	case 0x22:
	case 0x34:
	case 0x35:
		break;
	default:
		return;
	}

	asm("mrs	%0, CurrentEl": "=r" (el));

	switch (el >> 2) {
	case 1:
		asm("mrs	%0, FAR_EL1": "=r" (far));
		break;
	case 2:
		asm("mrs	%0, FAR_EL2": "=r" (far));
		break;
	default:
		/* don't print anything to make output pretty */
		return;
	}

	printf(", far 0x%lx", far);
}

static void dump_instr(struct pt_regs *regs)
{
	u32 *addr = (u32 *)(regs->elr & ~3UL);
	int i;

	printf("Code: ");
	for (i = -4; i < 1; i++)
		printf(i == 0 ? "(%08x) " : "%08x ", addr[i]);
	printf("\n");
}

void show_regs(struct pt_regs *regs)
{
	int i;

	if (gd->flags & GD_FLG_RELOC)
		printf("elr: %016lx lr : %016lx (reloc)\n",
		       regs->elr - gd->reloc_off,
		       regs->regs[30] - gd->reloc_off);
	printf("elr: %016lx lr : %016lx\n", regs->elr, regs->regs[30]);

	for (i = 0; i < 29; i += 2)
		printf("x%-2d: %016lx x%-2d: %016lx\n",
		       i, regs->regs[i], i+1, regs->regs[i+1]);
	printf("\n");
	dump_instr(regs);
}

/*
 * Try to "emulate" a semihosting call in the event that we don't have a
 * debugger attached.
 */
static bool smh_emulate_trap(struct pt_regs *regs)
{
	int size;

	if (ESR_ELx_EC(regs->esr) != ESR_ELx_EC_UNKNOWN)
		return false;

	if (regs->spsr & PSR_MODE32_BIT) {
		if (regs->spsr & PSR_AA32_T_BIT) {
			u16 *insn = (u16 *)ALIGN_DOWN(regs->elr, 2);

			if (*insn != SMH_T32_SVC && *insn != SMH_T32_HLT)
				return false;
			size = 2;
		} else {
			u32 *insn = (u32 *)ALIGN_DOWN(regs->elr, 4);

			if (*insn != SMH_A32_SVC && *insn != SMH_A32_HLT)
				return false;
			size = 4;
		}
	} else {
		u32 *insn = (u32 *)ALIGN_DOWN(regs->elr, 4);

		if (*insn != SMH_A64_HLT)
			return false;
		size = 4;
	}

	/* Avoid future semihosting calls */
	disable_semihosting();

	/* Just pretend the call failed */
	regs->regs[0] = -1;
	regs->elr += size;
	return true;
}

/*
 * do_bad_sync handles the impossible case in the Synchronous Abort vector.
 */
void do_bad_sync(struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf("Bad mode in \"Synchronous Abort\" handler, esr 0x%08lx\n",
	       pt_regs->esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_bad_irq handles the impossible case in the Irq vector.
 */
void do_bad_irq(struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf("Bad mode in \"Irq\" handler, esr 0x%08lx\n", pt_regs->esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_bad_fiq handles the impossible case in the Fiq vector.
 */
void do_bad_fiq(struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf("Bad mode in \"Fiq\" handler, esr 0x%08lx\n", pt_regs->esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_bad_error handles the impossible case in the Error vector.
 */
void do_bad_error(struct pt_regs *pt_regs)
{
	efi_restore_gd();
	printf("Bad mode in \"Error\" handler, esr 0x%08lx\n", pt_regs->esr);
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_sync handles the Synchronous Abort exception.
 */
void do_sync(struct pt_regs *pt_regs)
{
	if (CONFIG_IS_ENABLED(SEMIHOSTING_FALLBACK) &&
	    smh_emulate_trap(pt_regs))
		return;
	efi_restore_gd();
	printf("\"Synchronous Abort\" handler, esr 0x%08lx", pt_regs->esr);
	dump_far(pt_regs->esr);
	printf("\n");
	show_regs(pt_regs);
	show_efi_loaded_images(pt_regs);
	panic("Resetting CPU ...\n");
}

/*
 * do_irq handles the Irq exception.
 */
void do_irq(struct pt_regs *pt_regs)
{
    u32 iar;
    u32 irq;

    efi_restore_gd();

    iar = gic_read_iar();
    irq = iar & 0xffffff;
    gic_write_eoir(iar);

    if (irq < MAX_IRQS && irq_table[irq].handler)
        return irq_table[irq].handler(irq_table[irq].arg);
    else
        printf("Unhandled IRQ %u\n", irq);
}

/*
 * do_fiq handles the Fiq exception.
 */
void do_fiq(struct pt_regs *pt_regs)
{
    efi_restore_gd();
    printf("\"Fiq\" handler, esr 0x%08lx\n", pt_regs->esr);
    show_regs(pt_regs);
    show_efi_loaded_images(pt_regs);
    panic("Resetting CPU ...\n");
}

/*
 * do_error handles the Error exception.
 * Errors are more likely to be processor specific,
 * it is defined with weak attribute and can be redefined
 * in processor specific code.
 */
void __weak do_error(struct pt_regs *pt_regs)
{
    efi_restore_gd();
    printf("\"Error\" handler, esr 0x%08lx\n", pt_regs->esr);
    show_regs(pt_regs);
    show_efi_loaded_images(pt_regs);
    panic("Resetting CPU ...\n");
}

void irq_install_handler(int vec, interrupt_handler_t *handler, void *arg)
{
	struct udevice *dev;
	fdt_addr_t gicd;
    u32 reg, shift, val;
    int ret;

	if ((vec < 0) || (vec >= MAX_IRQS)) {
		return;
	}

	ret = uclass_get_device_by_driver(UCLASS_IRQ,
					  DM_DRIVER_GET(arm_gic_v3), &dev);
	if (ret) {
		pr_err("%s: failed to get %s irq device\n", __func__,
		       DM_DRIVER_GET(arm_gic_v3)->name);
		return;
	}

	gicd = dev_read_addr_index(dev, 0);
	if (gicd == FDT_ADDR_T_NONE) {
		pr_err("%s: failed to get GICD address\n", __func__);
		return;
	}

    reg = vec / 16;
    shift = (vec % 16) * 2;
    val = readl(gicd + GICD_ICFGR + reg * 4);
    val &= ~(0x3 << shift);
    writel(val, gicd + GICD_ICFGR + reg * 4);
    
    setbits_le32((void *)(gicd + GICD_IGROUPRn +
                 (vec / 32) * 4),
                 BIT(vec % 32));

    setbits_le32((void *)(gicd + GICD_ISENABLERn +
                 (vec / 32) * 4),
                 BIT(vec % 32));

    writeq(0x0,
       (void *)(gicd + GICD_IROUTERn +
       (vec - 32) * 8));

    irq_table[vec].handler = handler;
    irq_table[vec].arg = arg;
}

void irq_free_handler(int vec)
{
    struct udevice *dev;
    fdt_addr_t gicd;
    int ret;

    if ((vec < 0) || (vec >= MAX_IRQS)) {
        return;
    }

    ret = uclass_get_device_by_driver(UCLASS_IRQ,
            DM_DRIVER_GET(arm_gic_v3), &dev);
    if (ret) {
        pr_err("%s: failed to get %s irq device\n", __func__,
                DM_DRIVER_GET(arm_gic_v3)->name);
        return;
    }

    gicd = dev_read_addr_index(dev, 0);
    if (gicd == FDT_ADDR_T_NONE) {
        pr_err("%s: failed to get GICD address\n", __func__);
        return;
    }

    clrbits_le32((void *)(gicd + GICD_ISENABLERn +
                 (vec / 32) * 4),
                 BIT(vec % 32));

    irq_table[vec].handler = NULL;
    irq_table[vec].arg = NULL;
}
