/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Xiaofan Tian <tianxf@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __PXA_AMP_H
#define __PXA_AMP_H

#ifdef CONFIG_PXA_AMP_SUPPORT
extern void pxa_amp_init(void);
extern unsigned int pxa_amp_reloc_end(void);
extern bool pxa_is_warm_reset(void);
extern bool pxa_amp_uart_enabled(void);
#else
static inline void pxa_amp_init(void) {}
static inline unsigned int pxa_amp_reloc_end(void) { return CONFIG_SYS_RELOC_END; }
static inline bool pxa_amp_uart_enabled(void) { return 1; }
static inline bool pxa_is_warm_reset(void) { return 0; }
#endif

#endif
