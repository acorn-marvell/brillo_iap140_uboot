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
#include <asm/arch/pxa1908.h>

#ifdef CONFIG_PXA_AMP_SUPPORT

#define PMU_CC2_AP	(PXA1908_APMU_BASE + 0x100)

#define PXA1908_CPU_POR_RST(n)  (1 << ((n) * 3 + 16))
#define PXA1908_CPU_CORE_RST(n) (1 << ((n) * 3 + 17))
#define PXA1908_CPU_DBG_RST(n)  (1 << ((n) * 3 + 18))

void pxa_cpu_reset(u64 cpu, u64 addr)
{
	unsigned int tmp;

	if (!cpu || cpu >= CONFIG_NR_CPUS) {
		printf("Wrong CPU number. Should be 1 to %d\n",
		       CONFIG_NR_CPUS - 1);
		return;
	}

	/* setup the boot addr before release CPUx */
	invoke_fn_smc(0x85000002, cpu, addr, 0);

	tmp = readl(PMU_CC2_AP);
	tmp &= ~(PXA1908_CPU_CORE_RST(cpu) | PXA1908_CPU_DBG_RST(cpu) |
			PXA1908_CPU_POR_RST(cpu));
	writel(tmp, PMU_CC2_AP);
}
#endif

#define UARTCLK14745KHZ	(APBC_APBCLK | APBC_FNCLK | APBC_FNCLKSEL(1))
#define SET_MRVL_ID	(1<<8)
#define L2C_RAM_SEL	(1<<4)

int arch_cpu_init(void)
{
	u32 val;
	struct pxa1908cpu_registers *cpuregs =
		(struct pxa1908cpu_registers *)PXA1908_CIU_BASE;

	struct pxa1908apbc_registers *apbclkres =
		(struct pxa1908apbc_registers *)PXA1908_APBC_BASE;

#if defined(CONFIG_I2C_MV)
	struct pxa1908apbcp_registers *apbcpclkres =
		(struct pxa1908apbcp_registers *)PXA1908_APBCP_BASE;
#endif

	struct pxa1908mpmu_registers *mpmu =
		(struct pxa1908mpmu_registers *)PXA1908_MPMU_BASE;

#if defined(CONFIG_MMP_DISP) || defined(CONFIG_MV_SDHCI) || \
	defined(CONFIG_MV_UDC)
	struct pxa1908apmu_registers *apmu =
		(struct pxa1908apmu_registers *)PXA1908_APMU_BASE;
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
	/* Disable usb clock */
	writel(0, &apmu->usb);
	/* Enable usb clock */
	writel(APMU_AXI_CLK_EN | APMU_AXI_RESET, &apmu->usb);
#endif

	icache_enable();

	return 0;
}

/* Get SoC Access to Generic Timer */
int timer_init(void)
{
	u32 tmp;

	tmp = readl(APBC_COUNTER_CLK_SEL);
	/* Default is 26M/32768 = 0x319 */
	if ((tmp >> 16) != 0x319)
		return -1;

	/* bit0 = 1: Generic Counter Frequency control by hardware VCTCXO_EN */
	writel(tmp | CNT_FREQ_HW_CTRL, APBC_COUNTER_CLK_SEL);

	/* bit1 = 1: Halt on debug; bit0 = 1: Enable counter */
	writel(CNTCR_HDBG | CNTCR_EN, CNTCR_REG);

	return 0;
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	u32 id;
	struct pxa1908cpu_registers *cpuregs =
		(struct pxa1908cpu_registers *)PXA1908_CIU_BASE;

	id = readl(&cpuregs->chip_id);
	printf("SoC:   PXA1908 88AP%X-%X\n", (id & 0xFFFF), (id >> 0x10));
	return 0;
}
#endif

#ifdef CONFIG_I2C_MV
void i2c_clk_enable(void)
{
}
#endif
