#ifndef __MACH_FEATURES_H
#define __MACH_FEATURES_H

#include <asm/arch/cpu.h>

static inline int has_feat_legacy_apmu_core_status(void)
{
	return 0;
}

static inline int has_feat_d1p_hipwr(void)
{
	return 0;
}

static inline int has_feat_debugreg_in_d1p(void)
{
	return 0;
}

static inline int has_feat_gnss(void)
{
	return 0;
}

static inline int has_feat_ccic2(void)
{
	return 0;
}

static inline int has_feat_timer2(void)
{
	return 0;
}

/* separate pll2 interface */
static inline int has_feat_sepa_pll2interf(void)
{
	return cpu_is_pxa1U88();
}

#endif
