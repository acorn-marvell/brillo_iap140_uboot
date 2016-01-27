/*
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Xiang Wang <wangx@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MV_SDHCI_H
#define __MV_SDHCI_H

/* Functions */
int mv_sdh_init(unsigned long regbase, u32 max_clk, u32 min_clk, u32 quirks);

#endif /* __MV_SDHCI_H */
