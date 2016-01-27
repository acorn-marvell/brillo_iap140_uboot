/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _PXA1978_H
#define _PXA1978_H

/* Common APB clock register bit definitions */
#define APBC_APBCLK		(1<<9)  /* APB Bus Clock Enable */
#define APBC_FNCLK		(1<<8)  /* Functional Clock Enable */
#define APBC_RST		(1<<0)  /* Reset Generation */

/* Functional Clock Selection Mask */
#define APBC_FNCLKSEL(x)        (((x) & 0x1f) << 16)

/* ACCU common clock select options */
#define ACCUCLK_SYSPLL1_DIV2	2
#define ACCUCLK_SYSPLL1_DIV2_5	3
#define ACCUCLK_SYSPLL1_DIV3	4
#define ACCUCLK_SYSPLL1_DIV3_5	5
#define ACCUCLK_SYSPLL1_DIV4	6
#define ACCUCLK_SYSPLL1_DIV4_5	7
#define ACCUCLK_SYSPLL1_DIV5	8
#define ACCUCLK_SYSPLL1_DIV6	0xa
#define ACCUCLK_SYSPLL1_DIV7	0xc
#define ACCUCLK_SYSPLL1_DIV8	0xd
#define ACCUCLK_SYSPLL1_DIV10	0xf
#define ACCUCLK_SYSPLL1_DIV12	0x11
#define ACCUCLK_SYSPLL1_DIV16	0x13
#define ACCUCLK_SYSPLL1_DIV20	0x15
#define ACCUCLK_SYSPLL1_DIV24	0x16
#define ACCUCLK_SYSPLL1_DIV32	0x17
#define ACCUCLK_VCXO		0x19
#define ACCUCLK_VCXO_DIV2	0x1a
#define ACCUCLK_VCXO_DIV4	0x1b
#define ACCUCLK_VCXO_DIV8	0x1c
#define ACCUCLK_SYSPLL2		0x1d
#define ACCUCLK_SYSPLL3		0x1e
#define ACCUCLK_EXTR_PLL	0x1f

/* Register Base Addresses */
#define PXA1978_DRAM_BASE	0xF6F00000
#define PXA1978_TMR1_BASE	0xF7222000
#define PXA1978_UART1_BASE	0xF7218000
#define PXA1978_UART2_BASE	0xF7217000
#define PXA1978_UART3_BASE	0xF7212000
#define PXA1978_MPMU_BASE	0xF6109000
#define PXA1978_MCCU_BASE	0xF610A000
#define PXA1978_APMU_BASE	0xF7220000
#define PXA1978_ACCU_BASE	0xF721F000
#define PXA1978_MFPR_BASE	0xF6120000 /* PCU */
#define PXA1978_GPIO_BASE	0xF6100000
#define PXA1978_SQU_BASE	0xF71F0000 /* SQU Config */
#define PXA1978_CL_GP_BASE	0xF610C000
#define PXA1978_APPS_GP_BASE	0xF7301C00

#define BOOT_ROM_VER		0xFFE00028

/* Register offset */
#define PXA1978_PRODUCT_ID (PXA1978_CL_GP_BASE+0xc)
#define PXA1978_APBC_KEYPAD (PXA1978_APBC_BASE+0x50)

#define KPC_PC (PXA1978_KPC_BASE+0x00)
#define KPC_DK (PXA1978_KPC_BASE+0x08)
#define KPC_AS (PXA1978_KPC_BASE+0x20)
#define KPC_ASMKP0 (PXA1978_KPC_BASE+0x28)
#define KPC_ASMKP1 (PXA1978_KPC_BASE+0x30)
#define KPC_ASMKP2 (PXA1978_KPC_BASE+0x38)

#endif /* _PXA1978_H */
