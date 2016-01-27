/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_HELAN3_ALOE_H
#define __CONFIG_HELAN3_ALOE_H

#include "helan3_board_common.h"

/*
 * Version number information
 */
#define CONFIG_IDENT_STRING	"\nMarvell PXA1936 DKB"

#define CONFIG_MACH_HELAN3_ALOE		1	/* Machine type */

#define CONFIG_POWER_OFF_CHARGE         1

#ifndef CONFIG_MMP_FPGA
#define CONFIG_POWER_88PM860		1
#define CONFIG_POWER_88PM830		1
#define CONFIG_POWER_88PM880		1
#define CONFIG_POWER_88PM88X_USE_SW_BD	1
#define CONFIG_POWER_88PM88X_GPADC_NO	1
#endif

#endif	/* __CONFIG_HELAN3_ALOE_H */
