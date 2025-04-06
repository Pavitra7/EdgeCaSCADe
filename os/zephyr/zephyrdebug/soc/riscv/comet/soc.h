/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file configuration macros for riscv SOCs supporting the riscv
 *       privileged architecture specification
 */

#ifndef __SOC_COMMON_H_
#define __SOC_COMMON_H_

/* CLINT */
#define RISCV_MTIME_BASE             0x00000010
#define RISCV_MTIMECMP_BASE          0x00000100
/* PLIC */
#define RISCV_MSIP_BASE              0x00000050
/* IRQ numbers */
#define RISCV_MACHINE_SOFT_IRQ       3  /* Machine Software Interrupt */
#define RISCV_MACHINE_TIMER_IRQ      7  /* Machine Timer Interrupt */
#define RISCV_MACHINE_EXT_IRQ        11 /* Machine External Interrupt */

/* ECALL Exception numbers */
#define SOC_MCAUSE_ECALL_EXP         11 /* Machine ECALL instruction */
#define SOC_MCAUSE_USER_ECALL_EXP    8  /* User ECALL instruction */

/* SOC-specific MCAUSE bitfields */
#ifdef CONFIG_64BIT
/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK          (1 << 63)
/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK          0x7FFFFFFFFFFFFFFF
#else
/* Interrupt Mask */
#define SOC_MCAUSE_IRQ_MASK          (1 << 31)
/* Exception code Mask */
#define SOC_MCAUSE_EXP_MASK          0x7FFFFFFF
#endif

/* SOC-Specific EXIT ISR command */
#define SOC_ERET                     mret

#ifndef _ASMLANGUAGE

#if defined(CONFIG_RISCV_SOC_INTERRUPT_INIT)
void soc_interrupt_init(void);
#endif

#if defined(CONFIG_RISCV_HAS_PLIC)
void riscv_plic_irq_enable(uint32_t irq);
void riscv_plic_irq_disable(uint32_t irq);
int riscv_plic_irq_is_enabled(uint32_t irq);
void riscv_plic_set_priority(uint32_t irq, uint32_t priority);
int riscv_plic_get_irq(void);
#endif

#define LITEX_SUBREG_SIZE          0x1
#define LITEX_SUBREG_SIZE_BIT      (LITEX_SUBREG_SIZE * 8)

static inline unsigned char litex_read8(unsigned long addr)
{
	return sys_read8(addr);
}

static inline unsigned short litex_read16(unsigned long addr)
{
	return (sys_read8(addr) << 8)
		| sys_read8(addr + 0x4);
}

static inline unsigned int litex_read32(unsigned long addr)
{
	return (sys_read8(addr) << 24)
		| (sys_read8(addr + 0x4) << 16)
		| (sys_read8(addr + 0x8) << 8)
		| sys_read8(addr + 0xc);
}

static inline uint64_t litex_read64(unsigned long addr)
{
	return (((uint64_t)sys_read8(addr)) << 56)
		| ((uint64_t)sys_read8(addr + 0x4) << 48)
		| ((uint64_t)sys_read8(addr + 0x8) << 40)
		| ((uint64_t)sys_read8(addr + 0xc) << 32)
		| ((uint64_t)sys_read8(addr + 0x10) << 24)
		| ((uint64_t)sys_read8(addr + 0x14) << 16)
		| ((uint64_t)sys_read8(addr + 0x18) << 8)
		| (uint64_t)sys_read8(addr + 0x1c);
}

static inline void litex_write8(unsigned char value, unsigned long addr)
{
	sys_write8(value, addr);
}

static inline void litex_write16(unsigned short value, unsigned long addr)
{
	sys_write8(value >> 8, addr);
	sys_write8(value, addr + 0x4);
}

static inline void litex_write32(unsigned int value, unsigned long addr)
{
	sys_write8(value >> 24, addr);
	sys_write8(value >> 16, addr + 0x4);
	sys_write8(value >> 8, addr + 0x8);
	sys_write8(value, addr + 0xC);
}

static inline void litex_write(volatile uint32_t *reg, uint32_t reg_size, uint32_t val)
{
	uint32_t shifted_data, i;
	volatile uint32_t *reg_addr;

	for (i = 0; i < reg_size; ++i) {
		shifted_data = val >> ((reg_size - i - 1) *
					LITEX_SUBREG_SIZE_BIT);
		reg_addr = ((volatile uint32_t *) reg) + i;
		*(reg_addr) = shifted_data;
	}
}

static inline uint32_t litex_read(volatile uint32_t *reg, uint32_t reg_size)
{
	uint32_t shifted_data, i, result = 0;

	for (i = 0; i < reg_size; ++i) {
		shifted_data = *(reg + i) << ((reg_size - i - 1) *
						LITEX_SUBREG_SIZE_BIT);
		result |= shifted_data;
	}

	return result;
}

#endif /* !_ASMLANGUAGE */

#endif /* __SOC_COMMON_H_ */
