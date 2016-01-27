/*
 * (C) Copyright 2015
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Tim Wang <wangtt@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_HELAN4_DKB_H
#define __CONFIG_HELAN4_DKB_H

#include "helan3_board_common.h"

/*
 * Version number information
 */
#define CONFIG_IDENT_STRING	"\nMarvell PXA1956 DKB"

#define CONFIG_MACH_HELAN4_DKB		1	/* Machine type */

#ifndef CONFIG_MMP_FPGA
#define CONFIG_POWER_88PM860		1
#define CONFIG_POWER_88PM830		1
#define CONFIG_POWER_88PM880		1
#endif

#endif	/* __CONFIG_HELAN4_DKB_H */
