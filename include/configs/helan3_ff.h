/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_HELAN3_FF_H
#define __CONFIG_HELAN3_FF_H

#include "helan3_board_common.h"

/*
 * Version number information
 */
#define CONFIG_IDENT_STRING	"\nMarvell PXA1936 FF"

#define CONFIG_MACH_HELAN3_FF		1	/* Machine type */

#define CONFIG_POWER_88PM880		1
#define CONFIG_POWER_88PM88X_USE_SW_BD	1
#define CONFIG_POWER_88PM88X_GPADC_NO	1
#define CONFIG_POWER_OFF_CHARGE		1

/* LCD definitions */
#define LCD_PWM_GPIO32		32

/* Backlight definitions */
#define CONFIG_LEDS_LM3532
#define LCD_BACKLIGHT_EN	32
#define LM3532_I2C_NUM		0


#endif	/* __CONFIG_HELAN3_FF_H */
