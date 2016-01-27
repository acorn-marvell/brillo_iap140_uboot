/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _PXA988_H
#define _PXA988_H

/* Common APB clock register bit definitions */
#define APBC_APBCLK		(1<<0)  /* APB Bus Clock Enable */
#define APBC_FNCLK		(1<<1)  /* Functional Clock Enable */
#define APBC_RST		(1<<2)  /* Reset Generation */

/* Common APBCP clock register bit definitions */
#define APBCP_APBCCLK		(1<<0) /* APBC Bus Clock Enable */
#define APBCP_FNCLK		(1<<1) /* Functional Clock Enable */
#define APBCP_RST		(1<<2) /* Reset Generation */
#define CLK_32MHZ		(0<<3) /* 32Mhz */
#define CLK_52MHZ		(1<<3) /* 52Mhz */
#define CLK_62MHZ		(2<<3) /* 62.4Mhz */

/* Functional Clock Selection Mask */
#define APBC_FNCLKSEL(x)        (((x) & 0x7) << 4)

/* Common APMU clock register bit definitions */
#define APMU_AXI_RESET		(1<<0)	/* AXI BUS reset */
#define APMU_PERIPH_RESET	(1<<1)	/* Peripheral reset */
#define APMU_AXI_CLK_EN		(1<<3)	/* AXI clock enable */
#define APMU_PERIPH_CLK_EN	(1<<4)	/* Peripheral clock enable */

/* APMU clock register bit definitions for SD host */
#define SDH_CLK_FC_REQ			(1<<11)
#define SDH_CLK_SEL_FIELD		(1<<6)
#define SDH_CLK_SEL_416MHZ		(0<<6)
#define SDH_CLK_SEL_624MHZ		(1<<6)
#define SDH_CLK_DIV_FIELD		(0x7<<8)
#define SDH_CLK_DIV(n)			((n & 0x7) << 8)

/* APMU clock register bit definitions for DSI phy */
#define DSI_PHYSLOW_PRER	(0x1A << 6)
#define DSI_ESC_SEL		(0x0)
#define DSI_PHYESC_SELDIV	\
	(DSI_PHYSLOW_PRER | DSI_ESC_SEL)
#define DSI_PHYESC_SELDIV_MSK	((0x1f << 6) | 0x3)
#define DSI_ESC_SEL_MSK	(0x3)
#define DSI_PHY_CLK_EN	((1 << 2) | (1 << 5))
#define DSI_PHY_CLK_RST	((1 << 3) | (1 << 4))
#define DSI_ESC_CLK_EN	(1<<2)
#define DSI_ESC_CLK_RST	(1<<3)

/* APMU clock register bit definitions for LCD/DSI */
#define LCD_CI_ISP_ACLK_REQ		(1 << 22)
#define LCD_CI_ISP_ACLK_DIV(n)		((n & 0x7) << 19)
#define LCD_CI_ISP_ACLK_DIV_MSK		(0x7 << 19)
#define LCD_CI_ISP_ACLK_SEL(n)		((n & 0x3) << 17)
#define LCD_CI_ISP_ACLK_SEL_MSK		(0x3 << 17)

#define LCD_CI_ISP_ACLK_EN		(1 << 3)
#define LCD_CI_ISP_ACLK_RST		(1 << 16)

#define LCD_CI_HCLK_RST		(1 << 2)

#ifdef CONFIG_PXA1U88
#define LCD_CI_HCLK_EN		(1 << 4)
#define LCD_FCLK_EN(n)		(1 << (4 + n))
#define LCD_FCLK_RST		(1 << 1 | 1 << 0)
#define LCD_DEF_FCLK1_SEL(val)	((val & 0x3) << 9)
#define LCD_DEF_FCLK2_SEL(val)	((val & 0x3) << 11)
#define LCD_DEF_FCLK3_SEL(val)	((val & 0x1) << 13)
#define LCD_DEF_FCLK4_SEL(val)	((val & 0x1) << 14)
#else
#define LCD_CI_HCLK_EN		(1 << 5)
#define LCD_FCLK_EN		(1 << 4)
#define LCD_FCLK_RST		(1 << 1 | 1 << 0)
#define LCD_DEF_FCLK_SEL	(1 << 6)
#define LCD_FCLK_SEL_MASK	(1 << 6)
#endif

/* Register Base Addresses */
#define PXA988_DRAM_BASE	0xC0100000
#define PXA988_KPC_BASE		0xD4012000
#define PXA988_TMR1_BASE	0xD4014000
#define PXA988_APBC_BASE	0xD4015000
#define PXA988_UART0_BASE	0xD4036000
#define PXA988_UART1_BASE	0xD4017000
#define PXA988_MPMU_BASE	0xD4050000
#define PXA988_APMU_BASE	0xD4282800
#define PXA988_CIU_BASE		0xD4282C00
#define PXA988_MFPR_BASE	0xD401E000
#define PXA988_GPIO_BASE	0xD4019000
#define PXA988_APBCP_BASE	0xD403B000
#define PXA988_SQU_BASE		0xD42A0000

#define BOOT_ROM_VER		0xFFE00028

/* Register offset */
#define PXA988_APBC_KEYPAD (PXA988_APBC_BASE+0x50)

#define KPC_PC (PXA988_KPC_BASE+0x00)
#define KPC_DK (PXA988_KPC_BASE+0x08)
#define KPC_AS (PXA988_KPC_BASE+0x20)
#define KPC_ASMKP0 (PXA988_KPC_BASE+0x28)
#define KPC_ASMKP1 (PXA988_KPC_BASE+0x30)
#define KPC_ASMKP2 (PXA988_KPC_BASE+0x38)

#define APMU_DEBUG	(PXA988_APMU_BASE+0x88)
#define APMU_GC		(PXA988_APMU_BASE+0xcc)
#define APMU_GC_2D	(PXA988_APMU_BASE+0xf4)
#define APMU_PWR_BLK_TMR_REG    (PXA988_APMU_BASE+0xdc)
#define APMU_PWR_CTRL_REG       (PXA988_APMU_BASE+0xd8)
#define APMU_PWR_STATUS_REG     (PXA988_APMU_BASE+0xf0)
#define APMU_VPU_CLK_RES_CTRL	(PXA988_APMU_BASE+0xa4)

/* SQU debug register for 1u88 */
#define FABTIMEOUT_HELD_WSTATUS	(PXA988_SQU_BASE + 0x78)
#define FABTIMEOUT_HELD_RSTATUS	(PXA988_SQU_BASE + 0x80)
#define DVC_HELD_STATUS		(PXA988_SQU_BASE + 0xB0)
#define FCDONE_HELD_STATUS	(PXA988_SQU_BASE + 0xB8)
#define PMULPM_HELD_STATUS	(PXA988_SQU_BASE + 0xC0)
#define CORELPM_HELD_STATUS	(PXA988_SQU_BASE + 0xC8)
/* PLL status is not supported on pxa1x88, just a dummy register here */
#define PLL_HELD_STATUS		(PXA988_SQU_BASE + 0xD0)
/* APSR to check WDT reset */
#define MPMU_APSR		(PXA988_MPMU_BASE + 0x1028)

/* MCK QoS control */
#define MC_QOS_CTRL		(PXA988_CIU_BASE + 0x1C)

#define MASK_LCD_BLANK_CHECK	(1 << 27)

#define GC_AUTO_PWR_ON		(0x1 << 0)
#define GC2D_AUTO_PWR_ON	(0x1 << 6)

#define GC_CLK_EN	\
	((0x1 << 3) | (0x1 << 4) | (0x1 << 5))

#define GC_ACLK_RST	(0x1 << 0)
#define GC_FCLK_RST	(0x1 << 1)
#define GC_HCLK_RST	(0x1 << 2)
#define GC_CLK_RST	\
	(GC_ACLK_RST | GC_FCLK_RST | GC_HCLK_RST)
#define GC_SHADER_CLK_RST	(0x1 << 24)

#define GC_ISOB		(0x1 << 8)
#define GC_PWRON1	(0x1 << 9)
#define GC_PWRON2	(0x1 << 10)
#define GC_HWMODE	(0x1 << 11)

#define GC_FCLK_SEL_MASK	(0x3 << 6)
#define GC_ACLK_SEL_MASK	(0x3 << 20)
#define GC_FCLK_DIV_MASK	(0x7 << 12)
#define GC_ACLK_DIV_MASK	(0x7 << 17)
#define GC_FCLK_REQ		(0x1 << 15)
#define GC_ACLK_REQ		(0x1 << 16)

#define GC_CLK_SEL_WIDTH	(2)
#define GC_CLK_DIV_WIDTH	(3)
#define GC_FCLK_SEL_SHIFT	(6)
#define GC_ACLK_SEL_SHIFT	(20)
#define GC_FCLK_DIV_SHIFT	(12)
#define GC_ACLK_DIV_SHIFT	(17)

#define VPU_HW_MODE	(0x1 << 19)
#define VPU_AUTO_PWR_ON	(0x1 << 2)
#define VPU_PWR_STAT	(0x1 << 2)


#endif /* _PXA988_H */
