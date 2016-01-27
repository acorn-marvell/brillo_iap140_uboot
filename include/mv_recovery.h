/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	  GPL-2.0+
 */

#ifndef __RECOVERY_H__
#define __RECOVERY_H__
#define SKU_CPU_MAX_PREFER	0
#define SKU_DDR_MAX_PREFER	1
#define SKU_VPU_MAX_PREFER	2
#define SKU_GC3D_MAX_PREFER	3
#define SKU_GC2D_MAX_PREFER     4
struct misc_end {
	char command[32];
	u32 idx;
	u32 ddr_max;
	u32 vpu_max;
	u32 gc3d_max;
	u32 gc2d_max;
	u32	cpu_arch;
	char reserved[456];
};
struct recovery_reg_funcs {
	unsigned char (*recovery_reg_read)(void);
	unsigned char (*recovery_reg_write)(unsigned char value);
};
extern struct recovery_reg_funcs *p_recovery_reg_funcs;
int recovery(int primary_valid, int recovery_valid, int magic_key,
		struct recovery_reg_funcs *funcs);
u32 get_sku_max_freq(void);
u32 get_sku_max_setting(unsigned int type);
u32 root_detection(void);
#endif
