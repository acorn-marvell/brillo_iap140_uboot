/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Zhoujie Wu<zjwu@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ASM_ARCH_GPIO_H
#define _ASM_ARCH_GPIO_H

#include <asm/types.h>
#include <asm/arch/pxa1908.h>
#include <mvgpio.h>

#define GPIO_TO_REG(gp)		(gp >> 5)
#define GPIO_TO_BIT(gp)		(1 << (gp % 32))
#define GPIO_VAL(gp, val)	((val >> (gp % 32)) & 0x01)
#define GPIO_VAL_2(gp, val)	((val >> (gp % 32)) & 0x08)

static inline void *get_gpio_base(int bank)
{
	const unsigned long offset[] = {0, 4, 8, 0x100};
	/* gpio register bank offset - refer Appendix A.36 */
	return (struct gpio_reg *)(PXA1908_GPIO_BASE + offset[bank]);
}

#endif /* _ASM_ARCH_GPIO_H */

