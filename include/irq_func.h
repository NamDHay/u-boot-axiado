/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Header file for interrupt functions
 *
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef __IRQ_FUNC_H
#define __IRQ_FUNC_H

struct pt_regs;
struct cmd_tbl;

#define IRQ_VEC_ID_MASK		0x7FFFFFFFU
#define IRQ_VEC_TRIGGER_BIT	BIT(31)

#define IRQ_TRIGGER_LEVEL	0
#define IRQ_TRIGGER_EDGE	1

#define IRQ_VEC(id, trigger) \
	((int)((id) | ((trigger) ? IRQ_VEC_TRIGGER_BIT : 0)))

#define IRQ_VEC_ID(vec) \
((uint32_t)(vec) & IRQ_VEC_ID_MASK)

#define IRQ_VEC_TRIGGER(vec) \
	(((uint32_t)(vec) >> 31) & 0x1)

typedef void (interrupt_handler_t)(void *arg);

int interrupt_init(void);
void timer_interrupt(struct pt_regs *regs);
void external_interrupt(struct pt_regs *regs);
void irq_install_handler(int vec, interrupt_handler_t *handler, void *arg);
void irq_free_handler(int vec);
void reset_timer(void);

void enable_interrupts(void);
int disable_interrupts(void);

/* Implemented in $(CPU)/interrupts.c */
int do_irqinfo(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[]);

#endif
