/*
 * (C) Copyright 2014
 * Marvell Inc, <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _HELANX_PLLL_H_
#define _HELANX_PLLL_H_

#define MHZ	(1000000)

enum pll {
	PLL_START = 0,
	PLL1, /* not configurable */
	PLL2 = 2,
	PLL3 = 3,
	PLL4 = 4,
	PLL_MAX,
};

enum ssc_mode {
	CENTER_SPREAD = 0x0,
	DOWN_SPREAD = 0x1,
};

struct pll_rate_cfg {
	unsigned int pll_vco;	/* vco rate Mhz */
	unsigned int pll;	/* pll se rate(pll) Mhz */
	unsigned int pllp;	/* pll diff rate(pll2) Mhz */
};

struct pll_ssc_cfg {
	unsigned int ssc_enflg;	/* if SSC enabled? */
	enum ssc_mode ssc_mode;	/* ssc_mode */
	int base;
	unsigned int amplitude;
	int desired_mod_freq;
};

struct pll_cfg {
	struct pll_rate_cfg pll_rate;
	struct pll_ssc_cfg pll_ssc;
};


#define PLAT_PLL_RATE(_id, _name)			\
	(cur_platform_opt->pll_cfg[_id].pll_rate._name) \

extern u32 get_pll_freq(enum pll pll, u32 *pll_freq, u32 *pllp_freq);
extern void set_pll_freq(enum pll pll, u32 pllvco_freq,
		u32 pll_freq, u32 pllp_freq);

extern void turn_off_pll(enum pll pll);
extern void turn_on_pll(enum pll pll, struct pll_cfg pll_cfg);

extern int smc_get_fuse_info(u64 function_id, void *arg);
struct fuse_info {
	u32 arg0;
	u32 arg1;
	u32 arg2;
	u32 arg3;
	u32 arg4;
	u32 arg5;
	u32 arg6;
};

#endif
