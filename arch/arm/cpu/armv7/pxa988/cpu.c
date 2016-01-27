/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <serial.h>
#include <asm/arch/cpu.h>
#include <asm/arch/pxa988.h>

#define UARTCLK14745KHZ	(APBC_APBCLK | APBC_FNCLK | APBC_FNCLKSEL(1))
#define SET_MRVL_ID	(1<<8)
#define L2C_RAM_SEL	(1<<4)

int arch_cpu_init(void)
{
	u32 val;
	struct pxa988cpu_registers *cpuregs =
		(struct pxa988cpu_registers *)PXA988_CIU_BASE;

	struct pxa988apbc_registers *apbclkres =
		(struct pxa988apbc_registers *)PXA988_APBC_BASE;

#if defined(CONFIG_I2C_MV)
	struct pxa988apbcp_registers *apbcpclkres =
		(struct pxa988apbcp_registers *)PXA988_APBCP_BASE;
#endif

	struct pxa988mpmu_registers *mpmu =
		(struct pxa988mpmu_registers *)PXA988_MPMU_BASE;

#if defined(CONFIG_MMP_DISP) || defined(CONFIG_MV_SDHCI) || \
	defined(CONFIG_MV_UDC)
	struct pxa988apmu_registers *apmu =
		(struct pxa988apmu_registers *)PXA988_APMU_BASE;
#endif

	/* set SEL_MRVL_ID bit in PANTHEON_CPU_CONF register */
	val = readl(&cpuregs->cpu_conf);
	val = val | SET_MRVL_ID;
	writel(val, &cpuregs->cpu_conf);

	/* Turn on clock gating (PMUM_CCGR) */
	writel(0xFFFFFFFF, &mpmu->ccgr);

	/* Turn on clock gating (PMUM_ACGR) */
	writel(0xFFFFFFFF, &mpmu->acgr);

	/* Turn on uart1 (AP) clock */
	writel(UARTCLK14745KHZ, &apbclkres->uart1);

	/* Enable GPIO clock */
	writel(APBC_APBCLK, &apbclkres->gpio);

#ifdef CONFIG_I2C_MV
	/* Enable I2C clock */
	writel(APBC_RST | APBC_FNCLK | APBC_APBCLK, &apbclkres->twsi0);
	writel(APBC_FNCLK | APBC_APBCLK, &apbclkres->twsi0);

	writel(APBCP_RST | CLK_32MHZ | APBCP_FNCLK | APBCP_APBCCLK,
	       &apbcpclkres->twsi);
	writel(CLK_32MHZ | APBCP_FNCLK | APBCP_APBCCLK, &apbcpclkres->twsi);

	writel(APBC_RST | APBC_FNCLK | APBC_APBCLK, &apbclkres->twsi1);
	writel(APBC_FNCLK | APBC_APBCLK, &apbclkres->twsi1);

#ifdef CONFIG_PXA1U88
	writel(APBC_RST | APBC_FNCLK | APBC_APBCLK, &apbclkres->twsi2);
	writel(APBC_FNCLK | APBC_APBCLK, &apbclkres->twsi2);
#endif

#endif

#ifdef CONFIG_MMP_DISP
#ifdef CONFIG_PXA1U88
	/* Enable LCD_AXI/AHB/DSI/FCLK clock, 416MHz source FCLK as default */
	val = readl(&apmu->lcd_dsi);
	/* Enable lcd AXI clock control */
	val |= LCD_CI_ISP_ACLK_EN | LCD_CI_ISP_ACLK_RST;
	writel(val, &apmu->lcd_dsi);
	/* Set LCD AXI clock as 208MHZ freq */
	val &= ~(LCD_CI_ISP_ACLK_DIV_MSK | LCD_CI_ISP_ACLK_SEL_MSK);
	val |= LCD_CI_ISP_ACLK_REQ | LCD_CI_ISP_ACLK_DIV(1) |
		LCD_CI_ISP_ACLK_SEL(0);
	writel(val, &apmu->lcd_dsi);

	/* Enable LCD function clock1 */
	val |= LCD_CI_HCLK_EN | LCD_CI_HCLK_RST;
	val |= LCD_FCLK_EN(1) | LCD_FCLK_RST | LCD_DEF_FCLK1_SEL(1);
	writel(val, &apmu->lcd_dsi);

	val = readl(&apmu->dsi);
	/* Enable DSI ESC clock */
	val &= ~DSI_ESC_SEL_MSK;
	val |= DSI_ESC_SEL | DSI_ESC_CLK_EN | DSI_ESC_CLK_RST;
	writel(val, &apmu->dsi);
#else
	/* Enable LCD_AXI/AHB/DSI/FCLK clock, 416MHz source FCLK as default */
	val = readl(&apmu->lcd_dsi);
	/* Enable lcd AXI clock control */
	val |= LCD_CI_ISP_ACLK_EN | LCD_CI_ISP_ACLK_RST;
	writel(val, &apmu->lcd_dsi);
	/* Set LCD AXI clock as 208MHZ freq */
	val &= ~(LCD_CI_ISP_ACLK_DIV_MSK | LCD_CI_ISP_ACLK_SEL_MSK);
	val |= LCD_CI_ISP_ACLK_REQ | LCD_CI_ISP_ACLK_DIV(1) |
		LCD_CI_ISP_ACLK_SEL(0);
	writel(val, &apmu->lcd_dsi);

	/* Enable LCD function clock */
	val |= LCD_CI_HCLK_EN | LCD_CI_HCLK_RST;
	val |= LCD_FCLK_EN | LCD_FCLK_RST | LCD_DEF_FCLK_SEL;
	writel(val, &apmu->lcd_dsi);

	val = readl(&apmu->dsi);
	/* Enable DSI PHY/ESC clock */
	val &= ~DSI_PHYESC_SELDIV_MSK;
	val |= DSI_PHYESC_SELDIV | DSI_PHY_CLK_EN | DSI_PHY_CLK_RST;
	writel(val, &apmu->dsi);
#endif
#endif

#ifdef CONFIG_MV_SDHCI
	/* Enable mmc clock */
	u32 sdh_clk;

	/* 104MHz CLK default */
	sdh_clk = SDH_CLK_FC_REQ | SDH_CLK_SEL_416MHZ | SDH_CLK_DIV(3);
	writel(sdh_clk | APMU_PERIPH_CLK_EN | APMU_AXI_CLK_EN |
	       APMU_PERIPH_RESET | APMU_AXI_RESET, &apmu->sd0);
	writel(sdh_clk | APMU_PERIPH_CLK_EN | APMU_AXI_CLK_EN |
	       APMU_PERIPH_RESET, &apmu->sd2);
#endif

#ifdef CONFIG_MV_UDC
	/* Enable usb clock */
	writel(APMU_PERIPH_CLK_EN | APMU_AXI_CLK_EN | APMU_PERIPH_RESET |
	       APMU_AXI_RESET, &apmu->usb);
#endif

	icache_enable();

	if (cpu_is_pxa1088()) {
		val = readl(APMU_DEBUG);
		val |= (MASK_LCD_BLANK_CHECK);
		writel(val, APMU_DEBUG);
	}

	return 0;
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	u32 id;
	struct pxa988cpu_registers *cpuregs =
		(struct pxa988cpu_registers *)PXA988_CIU_BASE;

	id = readl(&cpuregs->chip_id);
	printf("SoC:   PXA988 88AP%X-%X\n", (id & 0xFFFF), (id >> 0x10));
	return 0;
}
#endif

#ifdef CONFIG_I2C_MV
void i2c_clk_enable(void)
{
}
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

#ifdef CONFIG_PXA_AMP_SUPPORT

#define PMU_CC2_AP	(PXA988_APMU_BASE + 0x100)

/* For PXA988 */
#define PXA988_CPU_CORE_RST(n)	(1 << ((n) * 4 + 16))
#define PXA988_CPU_DBG_RST(n)	(1 << ((n) * 4 + 18))
#define PXA988_CPU_WDOG_RST(n)	(1 << ((n) * 4 + 19))

/* For PXA1088/1L88/1U88 */
#define PXA1X88_CPU_POR_RST(n)  (1 << ((n) * 3 + 16))
#define PXA1X88_CPU_CORE_RST(n) (1 << ((n) * 3 + 17))
#define PXA1X88_CPU_DBG_RST(n)  (1 << ((n) * 3 + 18))

#define CPU_WARM_RESET_VECTOR	(PXA988_CIU_BASE + 0xD8)

void pxa_cpu_reset(int cpu, u32 addr)
{
	unsigned int tmp;

	if (!cpu || cpu >= CONFIG_NR_CPUS) {
		printf("Wrong CPU number. Should be 1 to %d\n",
		       CONFIG_NR_CPUS - 1);
		return;
	}

	writel(addr, CPU_WARM_RESET_VECTOR);

	tmp = readl(PMU_CC2_AP);
	if (cpu_is_pxa988())
		tmp &= ~(PXA988_CPU_CORE_RST(cpu) | PXA988_CPU_DBG_RST(cpu) |
				PXA988_CPU_WDOG_RST(cpu));
	else
		tmp &= ~(PXA1X88_CPU_CORE_RST(cpu) | PXA1X88_CPU_DBG_RST(cpu) |
				PXA1X88_CPU_POR_RST(cpu));
	writel(tmp, PMU_CC2_AP);
}
#endif

#if defined(CONFIG_ARM_ERRATA_802022)
#define WFI_BIT(n)		(5 + 3 * n)
#define CORE_N_WFI(n)		(1 << (WFI_BIT(n)))
#define AXI_PHYS_BASE	0xd4200000
#define APMU_PHYS_BASE	(AXI_PHYS_BASE + 0x82800)
#define APMU_REG(x)	(APMU_PHYS_BASE + (x))
#define PMU_CORE_STATUS	APMU_REG(0x0090)
#define CPU_POR_RST(n)	(1 << ((n) * 3 + 16))
#define CPU_CORE_RST(n)	(1 << ((n) * 3 + 17))
#define CPU_DBG_RST(n)	(1 << ((n) * 3 + 18))
void cpu_oor(void)
{
	u32 tmp, cpu, core_active_status, core_tmp_status;

	/* Wakeup all other cores */
	tmp = __raw_readl(PMU_CC2_AP);

	for (cpu = 1; cpu < CONFIG_NR_CPUS; cpu++)
		if ((tmp & CPU_CORE_RST(cpu)) == CPU_CORE_RST(cpu))
			tmp &= ~(CPU_CORE_RST(cpu) | CPU_DBG_RST(cpu) |
					CPU_POR_RST(cpu));
	__raw_writel(tmp, PMU_CC2_AP);

	/* Polling until all other cores are all in C12 */
	core_tmp_status = CORE_N_WFI(1) | CORE_N_WFI(2) | CORE_N_WFI(3);
	do {
		core_active_status = __raw_readl(PMU_CORE_STATUS);
	} while ((core_tmp_status & core_active_status) != core_tmp_status);

	/* Put all other cores into reset */
	for (cpu = 1; cpu < CONFIG_NR_CPUS; cpu++)
		tmp |= (CPU_CORE_RST(cpu) | CPU_DBG_RST(cpu) |
				CPU_POR_RST(cpu));
	__raw_writel(tmp, PMU_CC2_AP);
}
#endif
