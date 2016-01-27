/*
 * (C) Copyright 2012
 * Marvell Semiconductors Ltd. <www.marvell.com>
 * Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * NOTE: Stack isn't setup yet when these subroutines are called.
 * So we donot push the registers onto Stack during entry.
 */

#include <config.h>

#define GICD_BASE_ADDR   0xe0081000
#define GICC_BASE_ADDR   (GICD_BASE_ADDR + 0x1000)

ca9_common_int:
	ldr	r1, =GICD_BASE_ADDR
	mov	r0, #1
	str	r0, [r1]		@ Enable distributer
	ldr     r1, =GICC_BASE_ADDR     @ CPU interface
	str	r0, [r1]		@ CPU Interface Control (ICCICR)
	mov	r0, #0xF0		@ Enable SPIs
	str	r0, [r1, #0x4]		@ Interrupt Priority Mask (ICCPMR)

	mov	pc, lr			@ return

cpu_die:
#if (0)
	@ r0 = cpuid
	@ r2 = idle_cfg register address
	@ default C2 internal clock gate

	@ disable FIQ and IRQ
	mrs	r1, cpsr
	orr	r1, r1, #0xc0
	msr	cpsr_c, r1

	@ disable GIC CPU interface
	ldr	r4, =CPU_PHYS_BASE
	mov	r1, #0
	str	r1, [r4]

	@ set idle_cfg register
	ldr	r1, [r2]
	orr	r1, r1, #(1<<0)		@ external idle
	orr	r1, r1, #(1<<1)		@ core power down
	orr	r1, r1, #(1<<2)		@ sram/L1 power down
	str	r1, [r2]

	@mask gic global interrupt
1:	ldr	r1, [r2]
	orr	r1, r1, #(1<<3)		@ MASK_GIC_nIRQ_TO_CORE
	orr     r1, r1, #(1<<4)		@ MASK_GIC_nFIQ_TO_CORE
	str	r1, [r2]

	.word	0xf57ff04f		@ DSB
	.word	0xe320f003		@ WFI

	@clear to mask gic global interrupt
	ldr	r1, [r2]
	bic	r1, r1, #(1<<3)
	bic     r1, r1, #(1<<4)
	str	r1, [r2]

	@wake from c2, check whether has GIC pending
	ldr	r4, =CPU_PHYS_BASE
	ldr	r1, [r4, #0xc]
	str	r1, [r4, #0x10]

	b	1b

#endif

.ltorg

/* PXA1978 - Begin */
.globl lowlevel_init
	.type   lowlevel_init, %function
lowlevel_init:
	@ Save link register in r3
	mov	r3, lr

	@ Common init for ca9
	bl	ca9_common_int

	mov	pc, r3			@ return


.ltorg

.globl smp_hw_cpuid
       .type   smp_hw_cpuid, %function
smp_hw_cpuid:
	mrc	p15, 0, r0, c0, c0, 5	@ Read CPUID register
	and	r0, r0, #0xf		@ Fetch CPUID
	mov	pc, lr

.ltorg

.globl read_cpuid_id
       .type   read_cpuid_id, %function
read_cpuid_id:
	mrc	p15, 0, r0, c0, c0, 0	@ Read CPUID_ID register
	mov	pc, lr

.ltorg

.globl loop_delay
	.type loop_delay, %function
loop_delay:
	subs	r0, r0, #1
	bhi	loop_delay
	mov	pc, lr

.ltorg

.global save_boot_params
save_boot_params:
#if (0)
	ldr	r0,  =CONFIG_OBM_PARAM_MAGIC
	ldr	r1,  =0x4f424d50
	ldr	r2,  [r0]
	cmp	r1,  r2
	beq	warm_reset

	str	r1,  [r0]
	ldr	r0,  =CONFIG_OBM_PARAM_ADDR
	str	r11, [r0]
	ldr	r0,  =CONFIG_WARM_RESET
	mov	r3,  #0
	str	r3,  [r0]
#endif
	mov	pc,  lr

warm_reset:
	ldr	r0,  =CONFIG_WARM_RESET
	mov	r3,  #1
	str	r3,  [r0, #0]
	mov	pc, lr
