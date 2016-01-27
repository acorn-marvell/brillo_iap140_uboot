/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _PXA1928_H
#define _PXA1928_H

/* Common APB clock register bit definitions */
#define APBC_APBCLK		(1<<0)  /* APB Bus Clock Enable */
#define APBC_FNCLK		(1<<1)  /* Functional Clock Enable */
#define APBC_RST		(1<<2)  /* Reset Generation */
/* Functional Clock Selection Mask */
#define APBC_FNCLKSEL(x)        (((x) & 0x7) << 4)

/* Common APMU clock register bit definitions */
#define APMU_PERIPH_CLK_EN	(1<<4)	/* Peripheral clock enable */
#define APMU_AXI_CLK_EN		(1<<3)	/* AXI clock enable */
#define APMU_PERIPH_RESET	(1<<1)	/* Peripheral reset */
#define APMU_AXI_RESET		(1<<0)	/* AXI BUS reset */

/* APMU clock register bit definitions for LCD/DSI */
#define DISPLAY1_AXI_RST	(1<<0)
#define DISPLAY1_RST		(1<<1)
#define DSI_PHY_SLOW_RST	(1<<2)
#define DISPLAY1_AXICLK_EN	(1<<3)
#define DISPLAY1_CLK_EN		(1<<4)
#define DSI_PHY_SLOW_CLK_EN	(1<<5)
#define DISPLAY1_CLK_SEL_MASK	(3<<6)
#define DISPLAY1_DEF_CLK_SEL	(3<<6)
#define DISPLAY1_CLK_DIV_MASK	(0xf<<8)
#define DISPLAY1_CLK_DIV(n)	((n & 0xf) << 8)
#define DSI_ESCCLK_EN		(1<<12)
#define DSI_PHYSLOW_DIV_MASK	(0x1f<<15)
#define DSI_PHYSLOW_DIV(n)	((n & 0x1f) << 15)
#define PLL1_CLKOUTP_SEL	(1<<21)
#define DSI_ESC_CLK_SEL_MASK	(0x3<<22)
#define DSI_ESC_DEF_CLK_SEL	(0x0<<22)
#define	DISPLAY2_VDMA_AXICLK_EN	(0x1<<3)
#define	DISPLAY2_VDMA_AXI_RST	(0x1<<0)

/* APMU DMA CLK RESET CONTROLLER Register 0xd4282864 */
#define MDMA_AXICLK_EN		(0x1 << 9)
#define MDMA_AXI_RSTN		(0x1 << 8)
#define DMA_AXICLK_EN		(0x1 << 3)
#define DMA_AXI_RSTN		(0x1 << 0)

/* APMU DISP CONTROLLER RESET Control Register 0xd4282980 */
#define DISP_RSTCTRL_ESC_CLK_RSTN	(0x1 << 5)
#define DISP_RSTCTRL_VDMA_CLK_RSTN	(0x1 << 4)
#define DISP_RSTCTRL_HCLK_RSTN		(0x1 << 3)
#define DISP_RSTCTRL_VDMA_PORSTN	(0x1 << 2)
#define DISP_RSTCTRL_ACLK_PORSTN	(0x1 << 1)
#define DISP_RSTCTRL_ACLK_RSTN		(0x1 << 0)

/* APMU DISP CLOCK CONTROLLER Regsiters 0xd4282984 */
#define DISP_CLKCTRL_VDMA_CLKSRC_MASK	(0x7 << 28)
#define DISP_CLKCTRL_VDMA_CLKSRC_SEL	(0x0 << 28)
#define DISP_CLKCTRL_VDMA_DIV		(0x1 << 24)
#define DISP_CLKCTRL_VDMA_EN		(0x1 << 27)
#define DISP_CLKCTRL_CLK2_CLKSRC_MASK	(0x7 << 20)
#define DISP_CLKCTRL_CLK2_CLKSRC_SEL	(0x1 << 20)
#define DISP_CLKCTRL_CLK2_DIV		(0x1 << 16)
#define DISP_CLKCTRL_CLK2_EN		(0x1 << 23)
#define DISP_CLKCTRL_CLK1_CLKSRC_MASK	(0x7 << 12)
#define DISP_CLKCTRL_CLK1_CLKSRC_SEL	(0x1 << 12)
#define DISP_CLKCTRL_CLK1_DIV		(0x1 << 8)
#define DISP_CLKCTRL_CLK1_EN		(0x1 << 15)
#define DISP_CLKCTRL_ESC_CLKSRC_MASK	(0x3 << 5)
#define DISP_CLKCTRL_ESC_CLKSRC_SEL	(0x0 << 5)
#define DISP_CLKCTRL_ESC_CLK_EN		(0x1 << 4)

/* APMU DISP CLOCK CONTROLLER Regsiters 0xd4282988 */
#define DISP_CLKCTRL2_ACLK_CLKSRC_SEL		(0x0 << 4)
#define DISP_CLKCTRL2_ACLK_CLKSRC_SEL_MASK	(0x7 << 4)
#define DISP_CLKCTRL2_ACLK_DIV			(0x1 << 0)
#define DISP_CLKCTRL2_ACLK_EN			(0x1 << 7)

/* APMU ISLD LCD CONTROLLER Register 0xd42829ac */
#define ISLD_LCD_CTRL_RF1P_RTC		(0x2 << 30)
#define ISLD_LCD_CTRL_RF1P_WTC		(0x2 << 28)
#define ISLD_LCD_CTRL_RF2P_RTC		(0x2 << 26)
#define ISLD_LCD_CTRL_RF2P_WTC		(0x1 << 24)
#define ISLD_LCD_CTRL_SR1P_RTC		(0x2 << 22)
#define ISLD_LCD_CTRL_SR1P_WTC		(0x2 << 20)
#define ISLD_LCD_CTRL_SR2P_RTC		(0x2 << 18)
#define ISLD_LCD_CTRL_SR2P_WTC		(0x2 << 16)
#define	ISLD_LCD_CTRL_CMEM_DMMYCLK_EN	(0x1 << 4)
#define ISLD_LCD_CTRL_MEM_FWALLBAR	(0x1 << 3)

/* APMU ISLD LCD PWRCTRL Register 0xd4282a04 */
#define ISLD_LCD_PWR_INT_STATUS		(0x1 << 8)
#define ISLD_LCD_PWR_INT_MASK		(0x1 << 7)
#define ISLD_LCD_PWR_INT_CLR		(0x1 << 6)
#define ISLD_LCD_PWR_REDUN_STATUS	(0x1 << 5)
#define	ISLD_LCD_PWR_STATUS		(0x1 << 4)
#define ISLD_LCD_PWR_UP			(0x1 << 1)
#define ISLD_LCD_PWR_HWMODE_EN		(0x1 << 0)

/*  Marvell CPU Reset Status Register 0xd4051028*/
#define	PMUM_RSR_PJ_TSR		(0x1 << 4)
#define	PMUM_RSR_PJ_WDTR	(0x1 << 2)
#define	PMUM_RSR_PJ_POR		(0x1 << 0)

/* Timer2 Watchdog STatus Register */
#define TMR2_WSR			0xD4080070
#define TMR2_WSR_WTS			(0x1 << 0)

/* Register Base Addresses */
#define PXA1928_APBC_BASE		0xD4015000
#define PXA1928_UART1_BASE		0xD4030000
#define PXA1928_UART3_BASE		0xD4018000
#define PXA1928_GPIO_BASE		0xD4019000
#define PXA1928_MFPR_BASE		0xD401E000
#define PXA1928_AIB_BASE		0xD401E800
#define PXA1928_MPMU_BASE		0xD4050000
#define PXA1928_TMR2_BASE		0xd4080000
#define PXA1928_APMU_BASE		0xD4282800
#define PXA1928_CPU_BASE		0xD4282C00

/* for chip type define*/
#define PXA_CHIP_TYPE_REG		0xD4292AB4
#define PXA1926_2L_DISCRETE		0x0
#define PXA1928_POP			0x1
#define PXA1928_4L			0x2
#define PXA1928_2L			0x3

/* for arch timer*/
#define MPMU_CPRR              0x0020
#define MPMU_WDTPCR1           0x0204
#define MPMU_APRR              0x1020

#define MPMU_APRR_WDTR         (1 << 4)
#define MPMU_APRR_CPR          (1 << 0)
#define MPMU_CPRR_DSPR         (1 << 2)
#define MPMU_CPRR_BBR          (1 << 3)

#define TMR_WFAR               (0x009c)
#define TMR_WSAR               (0x00A0)

#define GEN_TMR_CFG            (0x00B0)
#define GEN_TMR_LD1            (0x00B8)

/* GIC base */
#define GICD_BASE		0xd1e01000
#define GICC_BASE		0xd1e02000
/* 7 banks * 32 gpios/bank */
#define MV_MAX_GPIO		224
#endif /* _PXA1928_H */
