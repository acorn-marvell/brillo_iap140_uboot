/*
 *  Common function for power&freq support
 *
 * Copyright (C) 2013 Marvell International Ltd. All rights reserved.
 *
 * Author: Lucao <lucao@marvell.com>
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <power/pxa_common.h>

static void __clk_wait_fc_done(void *address, unsigned long fcreq_bit)
{
	/* fc done should be asserted in several clk cycles */
	unsigned int i = 50;
	udelay(2);
	while ((__raw_readl(address) & fcreq_bit) && i) {
		udelay(10);
		i--;
	}
	if (i == 0) {
		printf("CLK fc req failed! reg = 0x%x, fc_req = 0x%x\n",
		       (unsigned int)__raw_readl(address),
		       (unsigned int)fcreq_bit);
	}
}

unsigned int peri_get_parent_index(const char *parent,
	struct periph_parents *table, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		if (!strcmp(parent, table[i].name))
			break;
	}
	if (i == size) {
		printf("no supported source parent found\n");
		return 0;
	}
	return i;
}

void peri_set_mux_div(struct peri_reg_info *reg_info,
	unsigned long mux, unsigned long div)
{
	unsigned int regval;
	unsigned int muxmask, divmask;
	unsigned int muxval, divval, fcreqmsk = 0xFFFFFFFF;
	muxval = mux & reg_info->src_sel_mask;

	divval = div & reg_info->div_mask;
	muxval = muxval << reg_info->src_sel_shift;
	divval = divval << reg_info->div_shift;

	muxmask = reg_info->src_sel_mask << reg_info->src_sel_shift;
	divmask = reg_info->div_mask << reg_info->div_shift;

	regval = __raw_readl(reg_info->reg_addr);
	regval &= ~(muxmask | divmask);
	regval |= (muxval | divval);

	if (reg_info->fcreq_mask) {
		fcreqmsk = reg_info->fcreq_mask << reg_info->fcreq_shift;
		regval |= fcreqmsk;
	}
	__raw_writel(regval, reg_info->reg_addr);

	if (reg_info->fcreq_mask)
		__clk_wait_fc_done(reg_info->reg_addr, fcreqmsk);

	regval = __raw_readl(reg_info->reg_addr);
	printf("regval[%lx] = %x\n", (unsigned long)reg_info->reg_addr, regval);
}

int peri_set_clk(unsigned long freq, struct periph_clk_tbl *tbl,
	int tbl_size, struct peri_reg_info *clk_reg_info,
	struct periph_parents *periph_parents, int parent_size)
{
	unsigned long rate, mux, div;
	unsigned int parent, i;
	rate = freq * MHZ;

	for (i = 0; i < tbl_size; i++) {
		if (rate <= tbl[i].fclk_rate) {
			printf("i = %d\n", i);
			printf("set clk rate @ %lu\n", tbl[i].fclk_rate);
			break;
		}
	}
	if (i == tbl_size) {
		printf("no supported rate\n");
		return -1;
	}

	parent = peri_get_parent_index(tbl[i].fclk_parent, periph_parents,
					parent_size);
	mux = periph_parents[parent].hw_sel;
	div = (periph_parents[parent].rate / tbl[i].fclk_rate) - 1;
	peri_set_mux_div(clk_reg_info, mux, div);
	return 0;
}

int peri_setrate(unsigned long freq, struct periph_clk_tbl *tbl,
	int tbl_size, struct peri_reg_info *aclk_reg_info,
	struct peri_reg_info *fclk_reg_info,
	struct periph_parents *periph_aparents, int aparent_size,
	struct periph_parents *periph_fparents, int fparent_size)
{
	unsigned long rate, fmux, amux, fdiv, adiv;
	unsigned int fparent, aparent, i;
	rate = freq * MHZ;

	for (i = 0; i < tbl_size; i++) {
		if (rate <= tbl[i].fclk_rate) {
			printf("i = %d\n", i);
			printf("set fclk rate @ %lu aclk rate @ %lu\n",
			       tbl[i].fclk_rate, tbl[i].aclk_rate);
			break;
		}
	}
	if (i == tbl_size) {
		printf("no supported rate\n");
		return -1;
	}

	aparent = peri_get_parent_index(tbl[i].aclk_parent, periph_aparents,
					aparent_size);
	amux = periph_aparents[aparent].hw_sel;
	adiv = (periph_aparents[aparent].rate / tbl[i].aclk_rate) - 1;
	peri_set_mux_div(aclk_reg_info, amux, adiv);

	fparent = peri_get_parent_index(tbl[i].fclk_parent, periph_fparents,
					fparent_size);
	fmux = periph_fparents[fparent].hw_sel;
	fdiv = (periph_fparents[fparent].rate / tbl[i].fclk_rate) - 1;
	peri_set_mux_div(fclk_reg_info, fmux, fdiv);
	return 0;
}

/* power domain related */
enum pd_island {
	GC3D,
	GC2D,
	VPU,
	ISP,
	ISLAND_MAX
};

struct pd_common_data {
	u32 *reg_clk_res_ctrl;
	u32 bit_hw_mode;
	u32 bit_auto_pwr_on;
	u32 bit_pwr_stat;
};

struct pd_common_data pd_data[ISLAND_MAX] = {
	[GC3D] = {
		.reg_clk_res_ctrl	= (u32 *)0xd42828cc,
		.bit_hw_mode		= 11,
		.bit_auto_pwr_on	= 0,
		.bit_pwr_stat		= 0,
	},
	[GC2D] = {
		.reg_clk_res_ctrl	= (u32 *)0xd42828f4,
		.bit_hw_mode		= 11,
		.bit_auto_pwr_on	= 6,
		.bit_pwr_stat		= 6,
	},
	[VPU] = {
		.reg_clk_res_ctrl	= (u32 *)0xd42828a4,
		.bit_hw_mode		= 19,
		.bit_auto_pwr_on	= 2,
		.bit_pwr_stat		= 2,
	},
	[ISP] = {
		.reg_clk_res_ctrl	= (u32 *)0xd4282838,
		.bit_hw_mode		= 15,
		.bit_auto_pwr_on	= 4,
		.bit_pwr_stat		= 4,
	},
};


/* always use hw control */
static int island_poweron(enum pd_island island)
{
	u32 val;
	int loop = 5000;
	struct pd_common_data *data = &pd_data[island];

	/* set  HW on/off mode  */
	val = __raw_readl(data->reg_clk_res_ctrl);
	val |= (1 << data->bit_hw_mode);
	__raw_writel(val, data->reg_clk_res_ctrl);

	/* on1, on2, off timer */
	__raw_writel(0x20001fff, APMU_PWR_BLK_TMR_REG);

	/* auto power on */
	val = __raw_readl(APMU_PWR_CTRL_REG);
	val |= (1 << data->bit_auto_pwr_on);
	__raw_writel(val, APMU_PWR_CTRL_REG);

	/*
	 * power on takes 316us
	 */
	udelay(320);

	/* polling PWR_STAT bit */
	for (loop = 5000; loop > 0; loop--) {
		val = __raw_readl(APMU_PWR_STATUS_REG);
		if (val & (1 << data->bit_pwr_stat))
			break;
		udelay(1);
	}

	if (loop <= 0) {
		printf("island %d failed to power on!\n", island);
		return -1;
	}

	return 0;
}

/* always use hw control */
static int island_poweroff(enum pd_island island)
{
	u32 val;
	int loop = 5000;
	struct pd_common_data *data = &pd_data[island];

	/* auto power off */
	val = __raw_readl(APMU_PWR_CTRL_REG);
	val &= ~(1 << data->bit_auto_pwr_on);
	__raw_writel(val, APMU_PWR_CTRL_REG);

	/*
	 * power off takes 23us, add a pre-delay to reduce the
	 * number of polling
	 */
	udelay(20);

	/* polling PWR_STAT bit */
	for (loop = 5000; loop > 0; loop--) {
		val = __raw_readl(APMU_PWR_STATUS_REG);
		if (!(val & (1 << data->bit_pwr_stat)))
			break;
		udelay(1);
	}
	if (loop <= 0) {
		printf("island %d failed to power off!\n", island);
		return -1;
	}

	return 0;
}

static int island_pwr(enum pd_island island, u32 onoff)
{
	int ret = 0;
	if (onoff)
		ret = island_poweron(island);
	else
		ret = island_poweroff(island);
	return ret;
};

static int do_gcpwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: gcpwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		res = island_pwr(GC3D, onoff);

	return res;
}

U_BOOT_CMD(
	gc_pwr, 6, 1, do_gcpwr,
	"Gc power on/off",
	""
);

static int do_gc2dpwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: gc2d_pwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		res = island_pwr(GC2D, onoff);

	return res;
}

U_BOOT_CMD(
	gc2d_pwr, 6, 1, do_gc2dpwr,
	"Gc2D power on/off",
	""
);

static int do_vpupwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: vpu_pwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		res = island_pwr(VPU, onoff);

	return res;
}

U_BOOT_CMD(
	vpu_pwr, 6, 1, do_vpupwr,
	"VPU power on/off",
	""
);

static int do_isppwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: isp_pwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		res = island_pwr(ISP, onoff);

	return res;
}

U_BOOT_CMD(
	isp_pwr, 6, 1, do_isppwr,
	"ISP power on/off",
	""
);

/* GC internal scaling and clock gating feature support */
enum GC_COMP {
	GC3D_CTR,
	GC2D_CTR,
};

#define GC3D_BASE	(0xc0400000)
#define GC2D_BASE	(0xd420c000)
#define REG_GCPULSEEATER (GC3D_BASE + 0x10c)
#define AQHICLKCTR_OFF	(0x000)
#define POWERCTR_OFF	(0x100)

#define FSCALE_VALMASK	(0x1fc)
#define SHADSCALE_VALMASK	(0xfe)

u32 *gcctr_base[] = {
	(u32 *)GC3D_BASE, (u32 *)GC2D_BASE
};

static void gc_fs_on(enum GC_COMP comp)
{
	u32 val;
	/* switch to manual mode, only 3D has this extra step */
	if (GC3D_CTR == comp) {
		val = __raw_readl(REG_GCPULSEEATER);
		val &= ~(1 << 16);
		val |= (1 << 17);
		__raw_writel(val, REG_GCPULSEEATER);
		/* shader scaling to 1/64 */
		val |= (1 << 0);
		val &= ~(SHADSCALE_VALMASK);
		val |= (1 << 1);
		__raw_writel(val, REG_GCPULSEEATER);
		val &= ~(1 << 0);
		__raw_writel(val, REG_GCPULSEEATER);
	}

	val = __raw_readl(gcctr_base[comp] + AQHICLKCTR_OFF);
	val &= ~(FSCALE_VALMASK);
	/* set 1/64 scaling divider */
	val |= (1 << 2) | (1 << 9);
	__raw_writel(val, gcctr_base[comp] + AQHICLKCTR_OFF);
	/* make scaling work */
	val &= ~(1 << 9);
	__raw_writel(val, gcctr_base[comp] + AQHICLKCTR_OFF);
}

static void gc_fs_off(enum GC_COMP comp)
{
	u32 val;

	/* switch to auto mode, only 3D has this extra step */
	if (GC3D_CTR == comp) {
		val = __raw_readl(REG_GCPULSEEATER);
		val &= ~(1 << 16);
		val |= (1 << 17);
		__raw_writel(val, REG_GCPULSEEATER);
		/* shader scaling to 64/64 */
		val |= (1 << 0);
		val &= ~(SHADSCALE_VALMASK);
		val |= (0x40 << 1);
		__raw_writel(val, REG_GCPULSEEATER);
		val &= ~(1 << 0);
		__raw_writel(val, REG_GCPULSEEATER);
	}

	val = __raw_readl(gcctr_base[comp] + AQHICLKCTR_OFF);
	val &= ~(FSCALE_VALMASK);
	/* set 64/64 scaling divider */
	val |= (64 << 2) | (1 << 9);
	__raw_writel(val, gcctr_base[comp] + AQHICLKCTR_OFF);
	/* make scaling work */
	val &= ~(1 << 9);
	__raw_writel(val, gcctr_base[comp] + AQHICLKCTR_OFF);
}

static void gc_cg_ctrl(enum GC_COMP comp, u32 onoff)
{
	u32 val;

	val = __raw_readl(gcctr_base[comp] + POWERCTR_OFF);
	val &= ~(1 << 0);
	val |= (!!onoff);
	__raw_writel(val, gcctr_base[comp] + POWERCTR_OFF);
}

static int do_enable_3dgc_fs(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable_3dgc_fs 1/0\n");
		return res;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0) {
		if (onoff)
			gc_fs_on(GC3D_CTR);
		else
			gc_fs_off(GC3D_CTR);
	}
	return res;
}

U_BOOT_CMD(
	enable_3dgc_fs, 6, 1, do_enable_3dgc_fs,
	"Enable GC3D frequency scaling",
	""
);

static int do_enable_3dgc_cg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable_3dgc_cg 1/0\n");
		return res;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		gc_cg_ctrl(GC3D_CTR, onoff);
	return res;
}

U_BOOT_CMD(
	enable_3dgc_cg, 6, 1, do_enable_3dgc_cg,
	"Enable GC3D clock gating",
	""
);

static int do_enable_2dgc_fs(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable_2dgc_fs 1/0\n");
		return res;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0) {
		if (onoff)
			gc_fs_on(GC2D_CTR);
		else
			gc_fs_off(GC2D_CTR);
	}
	return res;
}

U_BOOT_CMD(
	enable_2dgc_fs, 6, 1, do_enable_2dgc_fs,
	"Enable GC2D frequency scaling",
	""
);

static int do_enable_2dgc_cg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable_2dgc_cg 1/0\n");
		return res;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		gc_cg_ctrl(GC2D_CTR, onoff);
	return res;
}

U_BOOT_CMD(
	enable_2dgc_cg, 6, 1, do_enable_2dgc_cg,
	"Enable GC2D clock gating",
	""
);
