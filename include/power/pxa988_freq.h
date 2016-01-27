/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Jane Li <jiel@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef PXA988_FREQ
#define PXA988_FREQ

#ifdef CONFIG_TZ_HYPERVISOR
extern void read_fuse_info(void);
#else
int get_cpu_version(void);
#endif

#ifdef CONFIG_PXA988_POWER
extern void set_plat_max_corefreq(u32 max_freq);
extern int setop(int opnum);
extern void pxa988_fc_init(int ddr_mode);
extern unsigned int get_max_cpurate(void);
#elif defined CONFIG_HELAN2_POWER
extern u32 helan2_fc_init(int ddr_mode);
extern int setop(unsigned int pclk, unsigned int dclk, unsigned int aclk);
#elif defined CONFIG_HELAN3_POWER
extern u32 helan3_fc_init(int ddr_mode);
extern int setop(unsigned int c0pclk, unsigned int c1pclk, unsigned int dclk, unsigned int aclk);
#else
void set_plat_max_corefreq(u32 max_freq) {}
int setop(int opnum) {return 0; }
void pxa988_fc_init(int ddr_mode) {}
extern unsigned int get_max_cpurate(void) {return 0; }
#endif /* CONFIG_PXA988_POWER */

#if defined(CONFIG_HELAN2_POWER) || defined(CONFIG_HELAN3_POWER)
#define DDR_MAX_FREQ	624000
#define	APB_SPARE1_REG	0xd4090100
enum pll {
	PLL_START = 0,
	PLL1, /* not configurable */
	PLL2 = 2,
	PLL3 = 3,
	PLL4 = 4,
	PLL_MAX,
};
extern void set_pll_freq(enum pll pll, u32 pllvco_freq, u32 pll_freq, u32 pllp_freq);
#endif
#endif
