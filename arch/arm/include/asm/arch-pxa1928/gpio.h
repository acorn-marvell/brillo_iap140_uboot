/*
 * (C) Copyright 2013
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ASM_ARCH_GPIO_H
#define _ASM_ARCH_GPIO_H

#include <asm/types.h>
#include <asm/arch/pxa1928.h>
#include <mvgpio.h>

#define GPIO_TO_REG(gp)		(gp >> 5)
#define GPIO_TO_BIT(gp)		(1 << (gp & 0x1F))
#define GPIO_VAL(gp, val)	((val >> (gp & 0x1F)) & 0x01)

static inline void *get_gpio_base(int bank)
{
	/* GPIO modes: normal/secure
	* Current mode is normal
	*/
	const unsigned long offset[7] = {0, 4, 8, 0x100, 0x104, 0x108, 0x200};
	return (struct gpio_reg *)(PXA1928_GPIO_BASE + offset[bank]);
}

#endif /* _ASM_ARCH_GPIO_H */
