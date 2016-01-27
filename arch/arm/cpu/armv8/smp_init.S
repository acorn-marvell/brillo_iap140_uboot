/*
 * (C) Copyright 2013
 * Marvell Semiconductors Ltd. <www.marvell.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <asm/macro.h>
#include <linux/linkage.h>

.ltorg

.globl lowlevel_init
        .type   lowlevel_init, %function
lowlevel_init:
	mov	x19, x30	/* Save LR */
	switch_el x0, 3f, 0f, 0f
3:	bl      gic_init
	b	0f
0:
#ifdef CONFIG_SMP
	mrs     x0, mpidr_el1
	tst     x0, #0xffff
	b.eq    master_cpu

slave_cpu:
	b	retn

master_cpu:
	/* Init the mailbox to zero */
	ldr     x1, =SECONDARY_CPU_MAILBOX
	str	xzr, [x1]

	bl	pxa_cpu_configure

retn:
#endif
	mov	x30, x19	/* Restore LR */
	ret

.ltorg

.globl invoke_fn_smc
	.type	invoke_fn_smc, %function
invoke_fn_smc:
	smc	#0
	ret

.ltorg

.globl smp_hw_cpuid
	.type	smp_hw_cpuid, %function
smp_hw_cpuid:
	mrs     x0, mpidr_el1
	and     x1, x0, #0xff		/* cpu id */
	and     x0, x0, #0xff00		/* cluster id */
	add     x0, x1, x0, LSR #6	/* linear id */
	ret

.ltorg

.globl read_cpuid_id
	.type	read_cpuid_id, %function
read_cpuid_id:
	mrs     x0, midr_el1
	ret

.ltorg

.globl loop_delay
	.type loop_delay, %function
loop_delay:
	subs	x0, x0, #1
	bhi	loop_delay
	ret

.ltorg

.globl save_boot_params
/* CONFIG_OBM_PARRAM_MAGIC is the address which
 * holds the magic number indicating if it is a warm
 * reset. When cold reset, obm loads 1M block which
 * overpass the limit of uboot image thereafter clear
 * magic numer
 */
save_boot_params:
	ldr	x0, =CONFIG_OBM_PARAM_MAGIC
	ldr	w1, =0x4f424d50
	ldr	w2, [x0]
	cmp	w1, w2
	b.eq	warm_reset
	str	w1, [x0]

#ifdef CONFIG_OBM_PARAM_ADDR
	ldr     x0, =CONFIG_OBM_PARAM_ADDR
	str	w11, [x0]
#endif
	ldr	x0, =CONFIG_WARM_RESET
	mov	w3, #0
	str	w3, [x0]
	/* by default, set only core0 core status as active */
	ldr	x0, =CONFIG_CORE_BUSY_ADDR
	mov	w3, #0xfffffffe
	str	w3, [x0]
	ret

warm_reset:
	ldr	x0, =CONFIG_WARM_RESET
	mov	w3, #1
	str	w3, [x0]
	ret

/* by default, do nothing */
WEAK(pxa_cpu_configure)
	ret
ENDPROC(pxa_cpu_configure)
