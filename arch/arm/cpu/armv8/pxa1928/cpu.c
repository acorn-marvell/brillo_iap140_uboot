/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <emmd_rsv.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/pxa1928.h>

void pxa_cpu_configure(void)
{
	/* Configure to 64 bit */
	writel(0x8F000000, PXA1928_CPU_BASE + 0x88);
	writel(0x8F000000, PXA1928_CPU_BASE + 0xE4);
	writel(0x8F000000, PXA1928_CPU_BASE + 0xE8);
}

#ifdef CONFIG_PXA_AMP_SUPPORT

#define PXA1928_PMUA_CORE_RSTCTRL_BASE	(PXA1928_APMU_BASE + 0x2A0)
#define CPU_RELEASE 0x33

void pxa_cpu_reset(u64 cpu, u64 addr)
{
	if (!cpu || cpu >= CONFIG_NR_CPUS) {
		printf("Wrong CPU number. Should be 1 to %d\n",
		       CONFIG_NR_CPUS - 1);
		return;
	}

	invoke_fn_smc(0x85000002, cpu, addr, 0);
	writel(CPU_RELEASE, PXA1928_PMUA_CORE_RSTCTRL_BASE + cpu * 4);
}
#endif

#ifdef CONFIG_MMP_DISP
#define CIU_FABRIC_CKGT_CTRL0		(PXA1928_CPU_BASE + 0x64)
#define CIU_FABRIC_CLKT_CTRL0_X2H_CKGT_DISABLE   0x1
void powerup_display_controller(struct pxa1928apmu_registers *apmu)
{
	u32 val, count = 0;
	int is_b0 = cpu_is_pxa1928_b0();

	/* 1. Enalbe AXI clock and set frequency 312M */
	val = readl(&apmu->disp_clkctrl2)
		& (~DISP_CLKCTRL2_ACLK_CLKSRC_SEL_MASK);
	val |= (DISP_CLKCTRL2_ACLK_CLKSRC_SEL | DISP_CLKCTRL2_ACLK_DIV);
	if (is_b0)
		val |= DISP_CLKCTRL2_ACLK_EN;
	writel(val, &apmu->disp_clkctrl2);

	/*
	 * Enable ESC clock and set frequency 52M
	 * Enable display1 clock and set frequency 416M
	 * Enable display2 clock and set frequency 416M
	 * Enable VMDA Clock and set frequency 312M
	 */
	val = readl(&apmu->disp_clkctrl) & (~DISP_CLKCTRL_ESC_CLKSRC_MASK)
	      & (~DISP_CLKCTRL_CLK1_CLKSRC_MASK)
	      &	(~DISP_CLKCTRL_CLK2_CLKSRC_MASK)
	      & (~DISP_CLKCTRL_VDMA_CLKSRC_MASK);
	val |= (DISP_CLKCTRL_VDMA_CLKSRC_SEL | DISP_CLKCTRL_VDMA_DIV |
		DISP_CLKCTRL_CLK2_CLKSRC_SEL | DISP_CLKCTRL_CLK2_DIV |
		DISP_CLKCTRL_CLK1_CLKSRC_SEL | DISP_CLKCTRL_CLK1_DIV |
		DISP_CLKCTRL_ESC_CLKSRC_SEL | DISP_CLKCTRL_ESC_CLK_EN);
	if (is_b0) {
		val &= ~DISP_CLKCTRL_CLK2_EN;
		val |= DISP_CLKCTRL_CLK1_EN | DISP_CLKCTRL_VDMA_EN;
	}
	writel(val, &apmu->disp_clkctrl);

	/* 2. Disable fabric dynamic clock gating */
	val = readl(CIU_FABRIC_CKGT_CTRL0);
	val |= CIU_FABRIC_CLKT_CTRL0_X2H_CKGT_DISABLE;
	writel(val, CIU_FABRIC_CKGT_CTRL0);

	/* 3. Enable HWMODE to power up the island and clear up interrupt */
	val = readl(&apmu->isld_lcd_pwrctrl);
	val |= ISLD_LCD_PWR_HWMODE_EN;
	val &= ~ISLD_LCD_PWR_INT_MASK;
	writel(val, &apmu->isld_lcd_pwrctrl);

	/* 4. Power up the island */
	val |= ISLD_LCD_PWR_UP;
	writel(val, &apmu->isld_lcd_pwrctrl);

	/* 5. Wait for Island to be powered up */
	while (!(readl(&apmu->isld_lcd_pwrctrl) & ISLD_LCD_PWR_INT_STATUS)) {
		count++;
		if (count > 1000) {
			printf("Timeout for polling disp active interrupt\n");
			break;
		}
	}

	/* 6. Clear the interrupt and set the interrupt mask */
	val = readl(&apmu->isld_lcd_pwrctrl);
	val |= (ISLD_LCD_PWR_INT_CLR | ISLD_LCD_PWR_INT_MASK);
	writel(val, &apmu->isld_lcd_pwrctrl);

	/* 7. Enable Dummy Clocks to SRAMS */
	val = readl(&apmu->isld_lcd_ctrl);
	val |= ISLD_LCD_CTRL_CMEM_DMMYCLK_EN;
	writel(val, &apmu->isld_lcd_ctrl);

	/* 8. Wait for 500 ns, fix me */
	count = 0;
	while (count < 5000)
		count++;

	/* 9. Disable Dummy Clocks to SRAMS */
	val &= ~ISLD_LCD_CTRL_CMEM_DMMYCLK_EN;
	writel(val, &apmu->isld_lcd_ctrl);

	/* 10. Release Power on Reset ACLK & VMDA Clock Domain */
	val = readl(&apmu->disp_rstctrl);
	val |= (DISP_RSTCTRL_VDMA_PORSTN | DISP_RSTCTRL_ACLK_PORSTN);
	writel(val, &apmu->disp_rstctrl);

	/* 11. Wait for 500 ns, fix me */
	count = 0;
	while (count < 5000)
		count++;

	/* 12. Release Reset ESC & VMDA & HCLK Clock Domain */
	val = readl(&apmu->disp_rstctrl);
	val |= (DISP_RSTCTRL_ESC_CLK_RSTN | DISP_RSTCTRL_VDMA_CLK_RSTN
		| DISP_RSTCTRL_HCLK_RSTN);
	writel(val, &apmu->disp_rstctrl);

	/* 13. Wait for 300 ns, fix me */
	count = 0;
	while (count < 3000)
		count++;

	/* 14. Release Reset ACLK Clock Domain */
	val = readl(&apmu->disp_rstctrl);
	val |= DISP_RSTCTRL_ACLK_RSTN;
	writel(val, &apmu->disp_rstctrl);

	/* 15. Wait for 300 ns, fix me */
	count = 0;
	while (count < 3000)
		count++;

	/* 16. Enable fabric dynamic clock gating */
	val = readl(CIU_FABRIC_CKGT_CTRL0);
	val &= ~CIU_FABRIC_CLKT_CTRL0_X2H_CKGT_DISABLE;
	writel(val, CIU_FABRIC_CKGT_CTRL0);
}
#endif

#define UARTCLK14745KHZ	(APBC_APBCLK | APBC_FNCLK | APBC_FNCLKSEL(1))

/* Get SoC Access to Generic Timer */
int timer_init(void)
{
	u32 tmp;

	/* Enable WDTR2*/
	tmp  = readl(PXA1928_MPMU_BASE + MPMU_CPRR);
	tmp = tmp | MPMU_APRR_WDTR;
	writel(tmp, PXA1928_MPMU_BASE + MPMU_CPRR);

	/* Initialize Counter to zero */
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	writel(0x0, PXA1928_TMR2_BASE + GEN_TMR_LD1);

	/* Program Generic Timer Clk Frequency */
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	tmp = readl(PXA1928_TMR2_BASE + GEN_TMR_CFG);
	tmp |= (3 << 4); /* 3.25MHz/32KHz Counter auto switch enabled */
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	writel(tmp, PXA1928_TMR2_BASE + GEN_TMR_CFG);

	/* Start the Generic Timer Counter */
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	tmp = readl(PXA1928_TMR2_BASE + GEN_TMR_CFG);
	tmp |= 0x3;
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	writel(tmp, PXA1928_TMR2_BASE + GEN_TMR_CFG);

	return 0;
}

int arch_cpu_init(void)
{
	__attribute__((unused))	struct pxa1928apmu_registers *apmu =
		(struct pxa1928apmu_registers *)PXA1928_APMU_BASE;
	struct pxa1928mpmu_registers *mpmu =
		(struct pxa1928mpmu_registers *)PXA1928_MPMU_BASE;
	struct pxa1928apbc_registers *apbc =
		(struct pxa1928apbc_registers *)PXA1928_APBC_BASE;
	u32 val;

	/* Turn on APB, PLL1, PLL2 clock */
	writel(0x3FFFF, &apmu->gbl_clkctrl);

	/* Turn on APB2 clock, select APB2 clock 26MHz */
	writel(0x12, &apmu->apb2_clkctrl);

	/* Turn on MPMU register clock */
	writel(APBC_APBCLK, &apbc->mpmu);

	/*
	 * FIXME: This is a secure register so system may hang in global secure
	 * mode. This register should control timer2/timer3 clock while
	 * apbc->timers control timer1 clock.
	 */
	/* Turn on MPMU1 Timer register clock */
	writel(0, &apbc->mpmu1);

	/* Turn on clock gating (PMUM_CGR_PJ) */
	/*writel(0xFFFFFFFF, &mpmu->acgr);*/
	val = readl(&mpmu->cgr_pj);
	val |= (0x1<<19) | (0x1<<17) | (0x7<<13) | (0x1<<9) | (0x1F<<1);
	writel(val, &mpmu->cgr_pj);

	/* Turn on AIB clock */
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->aib);

	/* Turn on uart1 clock */
	writel(UARTCLK14745KHZ, &apbc->uart1);

	/* Turn on uart3 clock */
	writel(UARTCLK14745KHZ, &apbc->uart3);

	/* Turn on GPIO clock */
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->gpio);

#ifdef CONFIG_I2C_MV
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi1);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi2);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi3);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi4);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi5);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi6);
#endif

#ifdef CONFIG_MMP_DISP
	powerup_display_controller(apmu);
#endif

#ifdef CONFIG_MV_SDHCI
	/* Enable SD1 clock */
	writel(APMU_PERIPH_CLK_EN | APMU_AXI_CLK_EN | APMU_PERIPH_RESET |
		APMU_AXI_RESET | (6 << 10), &apmu->sd1); /* PLL1(624Mhz)/6 */

	/* Enable SD3 clock */
	writel(APMU_PERIPH_CLK_EN | APMU_AXI_CLK_EN | APMU_PERIPH_RESET |
		APMU_AXI_RESET, &apmu->sd3);
#endif

#ifdef CONFIG_USB_GADGET_MV
	/* Enable usb clock */
	writel(APMU_AXI_CLK_EN | APMU_AXI_RESET , &apmu->usb);
#endif
	return 0;
}

#ifdef CONFIG_I2C_MV
void i2c_clk_enable(void)
{
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("SoC:   PXA1928 (CA53 Core)\n");

	return 0;
}
#endif

enum reset_reason get_reset_reason(void)
{
	int rsr_pj, tmr2_wsr;
	enum reset_reason rr = rr_reset;

#if defined(CONFIG_EMMD_DUMP) || defined(CONFIG_RAMDUMP)
	unsigned int *address;
	emmd_page = (struct emmd_page *)CRASH_BASE_ADDR;
#endif
	__attribute__((unused))	struct pxa1928mpmu_registers *mpmu =
		(struct pxa1928mpmu_registers *)PXA1928_MPMU_BASE;

	rsr_pj = readl(&mpmu->rsr_pj);
	tmr2_wsr = readl(TMR2_WSR);

#if defined(CONFIG_EMMD_DUMP) || defined(CONFIG_RAMDUMP)
	/* Save Marvell Reset Status to reserved memory for ram dump */
	address = emmd_page->reset_status;
	*address++ = rsr_pj;
	*address = tmr2_wsr;
#endif
	if (rsr_pj & PMUM_RSR_PJ_TSR)
		rr = rr_thermal;
	else if ((rsr_pj & PMUM_RSR_PJ_WDTR) && (tmr2_wsr & TMR2_WSR_WTS))
		rr = rr_wdt;
	else if (rsr_pj & PMUM_RSR_PJ_POR)
		rr = rr_poweron;
	return rr;
}

#ifdef CONFIG_DISPLAY_REBOOTINFO
int print_rebootinfo(void)
{
	switch (get_reset_reason()) {
	case rr_thermal:
		printf("Reset by thermal sensor last time\n");
		break;
	case rr_wdt:
		printf("Reset by watchdog event last time\n");
		break;
	case rr_poweron:
		printf("Reset by power event last time\n");
		break;
	default:
		printf("Reset by press reset key\n");
	}
	return 0;
}
#endif

u32 smp_config(void)
{
	return 1;
}
