/*
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Gang Wu <gwu3@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef PXA1928_FREQ
#define PXA1928_FREQ

#ifdef CONFIG_PXA1928_DFC
extern void pxa1928_fc_init(int ddr_mode);
#endif /* CONFIG_PXA1928_DFC */

#endif
