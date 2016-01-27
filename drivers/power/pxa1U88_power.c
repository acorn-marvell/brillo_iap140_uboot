/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Xiaoguang Chen <chenxg@marvell.com>
 * Written-by: Lu Cao <lucao@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/arch/features.h>
#include <power/pxa_common.h>

#define APMU_BASE		0xD4282800
#define APMU_REG(x)		(APMU_BASE + x)
#define APMU_VPU		APMU_REG(0xa4)
#define APMU_ISPDXO		APMU_REG(0x038)
#define APMU_ISP_CLK_RES_CTRL	APMU_REG(0x38)
#define APMU_CCIC_CLK_RES_CTRL	APMU_REG(0x50)
#define APMU_LCD		APMU_REG(0x4c)
#define APMU_DSI		APMU_REG(0x44)
#define DSI_PHY_BASE		0xD420B800
#define DSI_REG(x)		(DSI_PHY_BASE + x)
#define DSI_CFG0		DSI_REG(0x0)

#define GC_ACLK_EN		(0x1 << 3)
#define GC_FCLK_EN		(0x1 << 4)
#define GC_HCLK_EN		(0x1 << 5)
#define GC_ACLK_REQ		(0x1 << 16)
#define GC_FCLK_REQ		(0x1 << 15)
#define GC_SHADERCLK_EN		(0x1 << 25)

#define VPU_ACLK_EN		(0x1 << 3)
#define VPU_FCLK_EN		(0x1 << 4)
#define VPU_AHBCLK_EN		(0x1 << 5)
#define VPU_FCLK_REQ		(0x1 << 20)
#define VPU_ACLK_REQ		(0x1 << 21)

#define ISP_RP_MC_TRIG		(1 << 30)
#define ISP_MCU_CLK_EN		(1 << 29)
#define ISP_MCU_CLK_RST		(1 << 27)
#define ISP_CI_ACLK_EN		(1 << 17)
#define ISP_CI_ACLK_RST		(1 << 16)
#define ISP_HW_MODE		(1 << 15)
#define ISP_CLK_EN		(1 << 1)
#define ISP_CLK_RST		(1 << 0)

#define ISP_CCIC_AHB_RST	(1 << 22)
#define ISP_CCIC_AHBCLK_EN	(1 << 21)

#define LCD_CI_ISP_ACLK_REQ	(1 << 22)
#define LCD_CI_ISP_ACLK_EN	(1 << 3)
#define LCD_CI_ISP_HCLK_EN	(1 << 5)
#define LCD_CI_ISP_ACLK_RST	(1 << 16)
#define LCD_CI_ISP_HCLK_RST	(1 << 2)

#define ISP_DXO_CLK_EN		\
	((1 << 1) | (1 << 9) | (1 << 11))
#define ISP_DXO_CLK_RST		\
	((1 << 0) | (1 << 8) | (1 << 10))
#define ISP_DXO_CLK_REQ		(1 << 7)

static struct periph_parents gc3d_fclk_parents[] = {
	{
		.name = "pll1_832",
		.rate = 832000000,
		.hw_sel = 0x0,
	},
	{
		.name = "pll1_624",
		.rate = 624000000,
		.hw_sel = 0x1,
	},
};

static struct periph_parents periph_parents[] = {
	{
		.name = "pll1_416",
		.rate = 416000000,
		.hw_sel = 0x0,
	},
	{
		.name = "pll1_624",
		.rate = 624000000,
		.hw_sel = 0x1,
	},
};

static struct periph_clk_tbl gc3d_clk_tbl[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 156000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_832",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 624000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
};

static struct peri_reg_info gc_aclk_reg = {
	.reg_addr = (void *)APMU_GC_2D,
	.src_sel_shift = 20,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 17,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 16,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static struct peri_reg_info gc_fclk_reg = {
	.reg_addr = (void *)APMU_GC,
	.src_sel_shift = 6,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 12,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 15,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static int do_set3dgcrate(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	struct periph_clk_tbl *tbl = gc3d_clk_tbl;
	unsigned int tbl_size = ARRAY_SIZE(gc3d_clk_tbl), i;
	struct periph_parents *fparents_tbl = gc3d_fclk_parents;
	unsigned int fp_size = ARRAY_SIZE(gc3d_fclk_parents);
	struct periph_parents *aparents_tbl = periph_parents;
	unsigned int ap_size = ARRAY_SIZE(periph_parents);
	ulong freq;
	int res = -1;

	if (argc != 2) {
		printf("usage: set 3dgc frequency xxxx(unit Mhz)\n");
		printf("Supported 3dgc frequency:\n");
		printf("gc fclk:\t gc aclk:\n");
		for (i = 0; i < tbl_size; i++) {
			printf("%lu\t%lu\n", tbl[i].fclk_rate,
			       tbl[i].aclk_rate);
		}
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = peri_setrate(freq, tbl, tbl_size, &gc_aclk_reg,
			&gc_fclk_reg, aparents_tbl, ap_size,
			fparents_tbl, fp_size);

	if (res == 0)
		printf("3D GC freq change was successful\n");
	else
		printf("3D GC freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	set3dgcrate, 2, 1, do_set3dgcrate,
	"Setting 3dgc rate",
	"set3dgcrate xxx(Mhz)"
);

static void enable_3dgc(int on)
{
	if (on) {
		CLK_SET_BITS(APMU_GC_2D, GC_ACLK_EN, 0);
		CLK_SET_BITS(APMU_GC, (GC_FCLK_EN | GC_HCLK_EN), 0);
	} else {
		CLK_SET_BITS(APMU_GC_2D, 0, GC_ACLK_EN);
		CLK_SET_BITS(APMU_GC, 0, (GC_FCLK_EN | GC_HCLK_EN));
	}
}

static int do_enable_3dgc(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	ulong on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable 3dgc clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	if (res == 0)
		enable_3dgc(on);

	return 0;
}

U_BOOT_CMD(
	enable_3dgc, 2, 1, do_enable_3dgc,
	"enable 3dgc clk",
	"enable_3dgc 1/0"
);

struct periph_parents gcsh_parents[] = {
	{
		.name = "pll1_832",
		.rate = 832000000,
		.hw_sel = 0x0,
	},
	{
		.name = "pll1_624",
		.rate = 624000000,
		.hw_sel = 0x1,
	},
};

static struct peri_reg_info gc_shader_reg = {
	.reg_addr = (void *)APMU_GC,
	.src_sel_shift = 26,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 28,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 31,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static struct periph_clk_tbl gc_shader_clk_tbl_1u88[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 624000000,
		.fclk_parent = "pll1_624",
	},
};

static struct periph_clk_tbl gc_shader_clk_tbl[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_832",
	},
	{
		.fclk_rate = 624000000,
		.fclk_parent = "pll1_624",
	},
};

static int do_setgcshrate(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	struct periph_clk_tbl *tbl = gc_shader_clk_tbl;
	unsigned int tbl_size = ARRAY_SIZE(gc_shader_clk_tbl), i;
	struct periph_parents *parents_tbl = gcsh_parents;
	unsigned int size = ARRAY_SIZE(gcsh_parents);
	ulong freq;
	int res = -1;

	if (cpu_is_pxa1U88()) {
		tbl = gc_shader_clk_tbl_1u88;
		tbl_size = ARRAY_SIZE(gc_shader_clk_tbl_1u88);
		parents_tbl = periph_parents;
		size = ARRAY_SIZE(periph_parents);
	}

	if (argc != 2) {
		printf("usage: set gc shader frequency xxxx(unit Mhz)\n");
		printf("Supported gc shader frequency:\n");
		printf("gc shader clk:\n");
		for (i = 0; i < tbl_size; i++)
			printf("%lu\n", tbl[i].fclk_rate);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = peri_set_clk(freq, tbl, tbl_size, &gc_shader_reg,
				   parents_tbl, size);

	if (res == 0)
		printf("GC Shader freq change was successful\n");
	else
		printf("GC Shader freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setgcshrate, 2, 1, do_setgcshrate,
	"Setting gc shader rate",
	"setgcshrate xxx(Mhz)"
);

static void enable_gcsh(int on)
{
	if (on) {
		CLK_SET_BITS(APMU_GC, GC_SHADERCLK_EN, 0);
	} else {
		CLK_SET_BITS(APMU_GC, 0, GC_SHADERCLK_EN);
	}
}

static int do_enable_gcsh(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	ulong on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable 3dgc clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	if (res == 0)
		enable_gcsh(on);

	return 0;
}

U_BOOT_CMD(
	enable_gc_sh, 2, 1, do_enable_gcsh,
	"enable gc shader clk",
	"enable_gc_sh 1/0"
);

static struct peri_reg_info gc2d_aclk_reg = {
	.reg_addr = (void *)APMU_GC_2D,
	.src_sel_shift = 20,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 17,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 16,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static struct peri_reg_info gc2d_fclk_reg = {
	.reg_addr = (void *)APMU_GC_2D,
	.src_sel_shift = 6,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 12,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 15,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static struct periph_clk_tbl gc2d_clk_tbl[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 156000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 624000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
};

static int do_set2dgcrate(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	struct periph_clk_tbl *tbl = gc2d_clk_tbl;
	unsigned int tbl_size = ARRAY_SIZE(gc2d_clk_tbl), i;
	struct periph_parents *parents_tbl = periph_parents;
	unsigned int size = ARRAY_SIZE(periph_parents);
	ulong freq;
	int res = -1;

	if (argc != 2) {
		printf("usage: set 2dgc frequency xxxx(unit Mhz)\n");
		printf("Supported 2dgc frequency:\n");
		printf("gc fclk:\t gc aclk:\n");
		for (i = 0; i < tbl_size; i++)
			printf("%lu\t%lu\n", tbl[i].fclk_rate,
			       tbl[i].aclk_rate);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = peri_setrate(freq, tbl, tbl_size, &gc2d_aclk_reg,
			&gc2d_fclk_reg, parents_tbl, size, parents_tbl, size);

	if (res == 0)
		printf("2D GC freq change was successful\n");
	else
		printf("2D GC freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	set2dgcrate, 2, 1, do_set2dgcrate,
	"Setting 2dgc rate",
	"set2dgcrate xxx(Mhz)"
);

static void enable_2dgc(int on)
{
	if (on) {
		CLK_SET_BITS(APMU_GC_2D, GC_ACLK_EN, 0);
		CLK_SET_BITS(APMU_GC_2D, (GC_FCLK_EN | GC_HCLK_EN), 0);
	} else {
		CLK_SET_BITS(APMU_GC_2D, 0, GC_ACLK_EN);
		CLK_SET_BITS(APMU_GC_2D, 0, (GC_FCLK_EN | GC_HCLK_EN));
	}
}

static int do_enable_2dgc(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	ulong on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable 2dgc clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	if (res == 0)
		enable_2dgc(on);

	return 0;
}

U_BOOT_CMD(
	enable_2dgc, 2, 1, do_enable_2dgc,
	"enable 2dgc clk",
	"enable_2dgc 1/0"
);

static struct periph_clk_tbl vpu_clk_tbl[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 156000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
};

static struct peri_reg_info vpu_aclk_reg = {
	.reg_addr = (void *)APMU_VPU,
	.src_sel_shift = 11,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 13,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 21,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static struct peri_reg_info vpu_fclk_reg = {
	.reg_addr = (void *)APMU_VPU,
	.src_sel_shift = 6,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 8,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 20,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static int do_setvpurate(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	struct periph_clk_tbl *tbl = vpu_clk_tbl;
	unsigned int tbl_size = ARRAY_SIZE(vpu_clk_tbl), i;
	struct periph_parents *parents_tbl = periph_parents;
	unsigned int size = ARRAY_SIZE(periph_parents);
	ulong freq;
	int res = -1;

	if (argc != 2) {
		printf("usage: set vpu frequency xxxx(unit Mhz)\n");
		printf("Supported vpu frequency:\n");
		printf("vpu fclk:\t vpu aclk:\n");
		for (i = 0; i < tbl_size; i++)
			printf("%lu\t%lu\n", tbl[i].fclk_rate,
			       tbl[i].aclk_rate);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = peri_setrate(freq, tbl, tbl_size, &vpu_aclk_reg,
			&vpu_fclk_reg, parents_tbl, size, parents_tbl, size);

	if (res == 0)
		printf("VPU freq change was successful\n");
	else
		printf("VPU freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setvpurate, 2, 1, do_setvpurate,
	"Setting vpu rate",
	"setvpurate xxx(Mhz)"
);

static void enable_vpu(int on)
{
	if (on) {
		CLK_SET_BITS(APMU_VPU, VPU_ACLK_EN, 0);
		CLK_SET_BITS(APMU_VPU, (VPU_FCLK_EN | VPU_AHBCLK_EN), 0);
	} else {
		CLK_SET_BITS(APMU_VPU, 0, VPU_ACLK_EN);
		CLK_SET_BITS(APMU_VPU, 0, (VPU_FCLK_EN | VPU_AHBCLK_EN));
	}
}

static int do_enable_vpu(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	ulong on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable vpu clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	enable_vpu(on);

	return res;
}

U_BOOT_CMD(
	enable_vpu, 2, 1, do_enable_vpu,
	"enable vpu clk",
	"enable_vpu 1/0"
);

static struct periph_clk_tbl isp_clk_tbl[] = {
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
};

static struct peri_reg_info isp_aclk_reg = {
	.reg_addr = (void *)APMU_ISP_CLK_RES_CTRL,
	.src_sel_shift = 21,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 18,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 23,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static struct peri_reg_info isp_fclk_reg = {
	.reg_addr = (void *)APMU_ISP_CLK_RES_CTRL,
	.src_sel_shift = 2,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 4,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 7,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static int do_setisprate(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	struct periph_clk_tbl *tbl = isp_clk_tbl;
	unsigned int tbl_size = ARRAY_SIZE(isp_clk_tbl), i;
	struct periph_parents *parents_tbl = periph_parents;
	unsigned int size = ARRAY_SIZE(periph_parents);
	ulong freq;
	int res = -1;

	if (argc != 2) {
		printf("usage: set isp frequency xxxx(unit Mhz)\n");
		printf("Supported isp frequency:\n");
		printf("isp fclk:\t isp aclk:\n");
		for (i = 0; i < tbl_size; i++)
			printf("%lu\t%lu\n", tbl[i].fclk_rate,
			       tbl[i].aclk_rate);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = peri_setrate(freq, tbl, tbl_size, &isp_aclk_reg,
			&isp_fclk_reg, parents_tbl, size, parents_tbl, size);

	if (res == 0)
		printf("isp freq change was successful\n");
	else
		printf("isp freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setisprate, 2, 1, do_setisprate,
	"Setting isp rate",
	"setisprate xxx(Mhz)"
);

static void enable_isp(int on)
{
	if (on) {
		/* enable isp mcu clk */
		CLK_SET_BITS(APMU_ISP_CLK_RES_CTRL,
			     ISP_MCU_CLK_RST | ISP_MCU_CLK_EN, 0);

		/* enable isp axi clk */
		CLK_SET_BITS(APMU_ISP_CLK_RES_CTRL,
			     ISP_CI_ACLK_EN | ISP_CI_ACLK_RST, 0);

		/* enable isp fun clk */
		CLK_SET_BITS(APMU_ISP_CLK_RES_CTRL,
			     ISP_CLK_EN | ISP_CLK_RST, 0);

		/* enable isp&ccic AHB clk */
		CLK_SET_BITS(APMU_CCIC_CLK_RES_CTRL,
			     ISP_CCIC_AHB_RST | ISP_CCIC_AHBCLK_EN, 0);
	} else {
		/* disable isp mcu clk */
		CLK_SET_BITS(APMU_ISP_CLK_RES_CTRL, 0, ISP_MCU_CLK_EN);

		/* disable isp axi clk */
		CLK_SET_BITS(APMU_ISP_CLK_RES_CTRL, 0, ISP_CI_ACLK_EN);

		/* disable isp fun clk */
		CLK_SET_BITS(APMU_ISP_CLK_RES_CTRL, 0, ISP_CLK_EN);

		/* disable isp&ccic AHB clk */
		CLK_SET_BITS(APMU_CCIC_CLK_RES_CTRL, 0, ISP_CCIC_AHBCLK_EN);
	}
}

static int do_enable_isp(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	ulong on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable isp clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	enable_isp(on);

	return res;
}

U_BOOT_CMD(
	enable_isp, 2, 1, do_enable_isp,
	"enable isp clk",
	"enable_isp 1/0"
);

static struct periph_clk_tbl lcd_clk_tbl[] = {
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 624000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_416",
	},
};

static struct peri_reg_info lcd_reg = {
	.reg_addr = (void *)APMU_LCD,
	.src_sel_shift = 6,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 10,
	.div_mask = MASK(REG_WIDTH_3BIT),
};

static struct periph_parents lcd_parents[] = {
	{
		.name = "pll1_416",
		.rate = 416000000,
		.hw_sel = 0x1,
	},
	{
		.name = "pll1_624",
		.rate = 624000000,
		.hw_sel = 0x0,
	},
};

static struct peri_reg_info lcd_ccic_isp_aclk_reg = {
	.reg_addr = (void *)APMU_LCD,
	.src_sel_shift = 17,
	.src_sel_mask = MASK(REG_WIDTH_2BIT),
	.div_shift = 19,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 22,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
};

static int do_setlcdrate(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	struct periph_clk_tbl *tbl = lcd_clk_tbl;
	unsigned int tbl_size = ARRAY_SIZE(lcd_clk_tbl), i;
	struct periph_parents *aparents_tbl = periph_parents;
	unsigned int aparent_size = ARRAY_SIZE(periph_parents);
	struct periph_parents *fparents_tbl = lcd_parents;
	unsigned int fparent_size = ARRAY_SIZE(lcd_parents);
	ulong freq;
	int res = -1;

	if (argc != 2) {
		printf("usage: set lcd frequency xxxx(unit Mhz)\n");
		printf("Supported lcd frequency:\n");
		printf("lcd fclk:\t lcd aclk:\n");
		for (i = 0; i < tbl_size; i++) {
			printf("%lu\t%lu\n", tbl[i].fclk_rate,
				tbl[i].aclk_rate);
		}
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = peri_setrate(freq, tbl, tbl_size, &lcd_ccic_isp_aclk_reg,
			&lcd_reg, aparents_tbl, aparent_size,
			fparents_tbl, fparent_size);

	if (res == 0)
		printf("lcd freq change was successful\n");
	else
		printf("lcd freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setlcdrate, 2, 1, do_setlcdrate,
	"Setting lcd rate",
	"setlcdrate xxx(Mhz)"
);

#define DSI_PHY_CLK_EN	((1 << 2) | (1 << 5))
#define DSI_PHY_CLK_RST	((1 << 3) | (1 << 4))

#define LCD_PST_CKEN		(1 << 9)
#define LCD_PST_OUTDIS		(1 << 8)
#define LCD_CLK_EN		(1 << 4)
#define LCD_CLK_RST		(1 << 1 | 1 << 0)
void enable_lcd(int on)
{
	if (on) {
		/* axi clk enable  */
		CLK_SET_BITS(APMU_LCD,
			     (LCD_CI_ISP_ACLK_RST | LCD_CI_ISP_HCLK_RST |
			     LCD_CI_ISP_ACLK_EN | LCD_CI_ISP_HCLK_EN), 0);
		/* dsi phy clk enable  */
		CLK_SET_BITS(APMU_DSI, DSI_PHY_CLK_EN, 0);
		CLK_SET_BITS(APMU_DSI, DSI_PHY_CLK_RST, 0);
		/* lcd fclk enable  */
		CLK_SET_BITS(APMU_LCD, (LCD_CLK_EN | LCD_CLK_RST), 0);
	} else {
		/* axi clk disable  */
		CLK_SET_BITS(APMU_LCD, 0, (LCD_CI_ISP_ACLK_EN |
			     LCD_CI_ISP_HCLK_EN));
		/* dsi phy clk disable  */
		CLK_SET_BITS(APMU_DSI, 0, DSI_PHY_CLK_EN);
		/* lcd fclk disable  */
		CLK_SET_BITS(APMU_LCD, 0, LCD_CLK_EN);
	}
}

static int do_enable_lcd(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	ulong on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable isp clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	enable_lcd(on);

	return res;
}

U_BOOT_CMD(
	enable_lcd, 2, 1, do_enable_lcd,
	"enable lcd clk",
	"enable_lcd 1/0"
);

#define DSI_PHY_OFF 0xFF830000
static int pxa988_lcd_power_control(int on)
{
	unsigned int val;
	/*  HW mode power on/off*/
	if (on)
		printf("lcd pwr on do nothing\n");
	else {
		val = __raw_readl(DSI_CFG0);
		val = DSI_PHY_OFF;
		__raw_writel(val, DSI_CFG0);
	}
	return 0;
}

static int do_lcdpwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;
	if (argc != 2) {
		printf("usage: lcd_pwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		pxa988_lcd_power_control(onoff);

	return res;
}

U_BOOT_CMD(
	lcd_pwr, 6, 1, do_lcdpwr,
	"LCD power on/off",
	""
);
