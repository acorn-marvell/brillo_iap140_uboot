#ifndef __MACH_FEATURES_H
#define __MACH_FEATURES_H

#include <asm/arch/cpu.h>

static inline int has_feat_legacy_apmu_core_status(void)
{
	return cpu_is_pxa988_z1() || cpu_is_pxa986_z1() ||
		cpu_is_pxa988_z2() || cpu_is_pxa986_z2() ||
		cpu_is_pxa988_z3() || cpu_is_pxa986_z3();
}

static inline int has_feat_d1p_hipwr(void)
{
	return cpu_is_pxa988_z1() || cpu_is_pxa988_z2() ||
		cpu_is_pxa986_z1() || cpu_is_pxa986_z2();
}

static inline int has_feat_debugreg_in_d1p(void)
{
	return cpu_is_pxa988_z1() || cpu_is_pxa988_z2() ||
		cpu_is_pxa988_z3() || cpu_is_pxa986_z1() ||
		cpu_is_pxa986_z2() || cpu_is_pxa986_z3();
}

/* A new feature to enable MCK4/AXI clock gating */
static inline int has_feat_mck4_axi_clock_gate(void)
{
	return !(cpu_is_pxa988_z1() || cpu_is_pxa986_z1() ||
			cpu_is_pxa988_z2() || cpu_is_pxa986_z2() ||
			cpu_is_pxa988_z3() || cpu_is_pxa986_z3());
}

/* default peri_clk = div_value *4, for Zx, peri_clk = div_value *2  */
static inline int has_feat_periclk_mult2(void)
{
	return cpu_is_pxa988_z1() || cpu_is_pxa986_z1() ||
			cpu_is_pxa988_z2() || cpu_is_pxa986_z2() ||
			cpu_is_pxa988_z3() || cpu_is_pxa986_z3();
}

/* When GC is enabled, GC2D is enabled. */
static inline int has_feat_gc2d_on_gc_on(void)
{
	return cpu_is_pxa1L88_a0();
}

static inline int has_feat_ssc(void)
{
	return cpu_is_pxa1L88_a0c();
}

static inline int has_feat_gcshader(void)
{
	return cpu_is_pxa1L88() || cpu_is_pxa1U88();
}

static inline int has_feat_gc2d(void)
{
	return cpu_is_pxa1088() || cpu_is_pxa1L88() ||
			cpu_is_pxa1U88();
}

static inline int has_feat_gc2d_separatepwr(void)
{
	return cpu_is_pxa1L88() || cpu_is_pxa1U88();
}

static inline int has_feat_gnss(void)
{
	return cpu_is_pxa1U88();
}

static inline int has_feat_ccic2(void)
{
	return cpu_is_pxa1L88() || cpu_is_pxa1U88();
}

static inline int has_feat_timer2(void)
{
	return cpu_is_pxa1088() || cpu_is_pxa1L88() ||
			cpu_is_pxa1U88();
}

static inline int has_feat_dvc_auto_dll_update(void)
{
	return cpu_is_pxa1L88() || cpu_is_pxa1U88();
}

/* separate pll2 interface */
static inline int has_feat_sepa_pll2interf(void)
{
	return cpu_is_pxa1U88();
}

#endif
