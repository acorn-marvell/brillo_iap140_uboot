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

#define PMR_BASE_ADDR   0xd1dfe000
#define CPU_PHYS_BASE   (PMR_BASE_ADDR + 0x100)

ca9_common_int:
	ldr     r1, =CPU_PHYS_BASE      @ CPU Int interfaces
	mov	r0, #0x1		@ Enable signalling of interrupts
	str	r0, [r1, #0x0]		@ CPU Interface Control (ICCICR)
	mov	r0, #0xF0		@ Enable SPIs
	str	r0, [r1, #0x4]		@ Interrupt Priority Mask (ICCPMR)

	mov	pc, lr			@ return

cpu_die:
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

/* PXA988 BOOTSTRAP - Begin */
#define USE_MP1
#define USE_MP2

bootstrap:
	@ Turn core interrupts off
	mrs	r0, cpsr
	orr	r0, r0, #0xc0	@ disable FIQ & IRQ
	msr	cpsr_c, r0

	@ Check core id
	mov	r2, #0
	mrc	p15, 0, r0, c0, c0, 5	@ read  MPIDR
	and	r0, r0, #0xf		@ Fetch CPUID

	cmp	r0, #1			@MP2
#ifndef USE_MP2
	ldreq	r2, =0xd4282918		@ PMU_CA9_CORE1_IDLE_CFG
#endif
	beq	core_stub

	cmp	r0, #0			@MP1
#ifndef USE_MP1
	ldreq	r2, =0xd4282914		@ PMU_CA9_CORE0_IDLE_CFG
#endif
	beq	core_stub

	b	bootstrap_endless_loop

core_stub:
	bl	ca9_common_int

	ands	r2, r2, r2
	@ core not define ?
	bne	cpu_die			@ never returns

	@ Use a variable in ddr
	adr	r1, sw_branch_addr	@ SW_BRANCH_ADDR

1:	mov	r3, #0x4000
2:	subs	r3, r3, #1
	bne	2b

	ldr	r0, [r1]		@ load sw branch address
	cmp	r0, #0x0		@ contains an address?
	movne	pc, r0			@ yes, then branch
	b	1b			@ otherwise, loop again

bootstrap_endless_loop:
1:	b	1b

	.globl sw_branch_addr
sw_branch_addr:
        .long   0


.ltorg

/* PXA988 - Begin */
.globl lowlevel_init
	.type   lowlevel_init, %function
lowlevel_init:
	@ Save link register in r3
	mov	r3, lr

	@ PMUM_ACGR enable all clocks
	ldr	r0, =0xd4051024
	mov	r1, #0xffffffff
	str	r1, [r0]
/*
        @ PMUA_GLB_CLK_CTRL enable all clocks
	ldr     r0, =0xd42828DC
	ldr     r1, =0x0003ffff
	str     r1, [r0]

	@ Ensure ICU_GBL_IRQ1_MSK is cleared
	ldr	r0, =0xd4282110
	mov	r1, #0x0
	str	r1, [r0]

	@ PMUA_CC3_PJ
	ldr	r0, =0xD4282988
	ldr	r1, [r0]
	orr	r1, r1, #(1<<3) @ PJ4_MP_TIMER_RST
	orr	r1, r1, #(1<<4) @ PJ4_MP_TIMER_CLK_EN
	str	r1, [r0]

	@ clear VINITHI for Core1 to let it reset from low vector
	ldr	r0, =0xd4282ce0
	ldr     r1, [r0]
	bic     r1, r1, #(1<<19) @ clear VINITHI
	str	r1, [r0]

	@ PMU_CC2_AP - release ca9 core 1
	ldr	r0, =0xd4282900
	ldr	r1, [r0]
	orr     r1, r1, #(1<<20) @ CPU1_CORE_RST
	orr     r1, r1, #(1<<22) @ CORE1_DBG_RST
	orr     r1, r1, #(1<<23) @ CORE1_WDOG_ RST
	str	r1, [r0]

	@ Check core where it run from
	mrc	p15, 0, r0, c0, c0, 5	@ Read CPUID register
	and	r0, r0, #0xf		@ Fetch CPUID
	cmp	r0, #0x0		@ core 1?
	bne	bootstrap		@ No, then jump to bootstrap code
*/
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
	mov	pc,  lr

warm_reset:
	ldr	r0,  =CONFIG_WARM_RESET
	mov	r3,  #1
	str	r3,  [r0, #0]
	mov	pc, lr
/*
.globl smp_config
       .type   smp_config, %function
smp_config:
	stmfd	sp!, {r1-r3}
	mov	r0, #0

	ldr	r1, =0xd4050020
	ldr	r3, [r1]
	ands	r2, r3, #1 << 1
#ifdef USE_MP1
	orreq	r0, #1
#endif
	ldr	r1, =0xd4282950
	ldr	r3, [r1]
	ands	r2, r3, #1 << 25
#ifdef USE_MP2
	orrne	r0, #2
#endif

	ldmfd	sp!, {r1-r3}
	mov	pc, lr
*/
