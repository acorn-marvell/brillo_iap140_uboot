/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Lei Wen <leiwen@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef _USB_MV_PHY_H
#define _USB_MV_PHY_H

/*
 * USB DEVICE PHY(UTMI) Registers for pxa988/1088/1l88
 */
#define MV_USB2_PHY_PLL0		0x4
#define MV_USB2_PHY_PLL1		0x8
#define MV_USB2_PHY_TX0			0x10
#define MV_USB2_PHY_TX1			0x14
#define MV_USB2_PHY_TX2			0x18
#define MV_USB2_PHY_RX0			0x20
#define MV_USB2_PHY_RX1			0x24
#define MV_USB2_PHY_RX2			0x28
#define MV_USB2_PHY_ANA0		0x30
#define MV_USB2_PHY_ANA1		0x34
#define MV_USB2_PHY_DIG0		0x3c
#define MV_USB2_PHY_DIG1		0x40
#define MV_USB2_PHY_DIG2		0x44
#define MV_USB2_PHY_T0			0x4c
#define MV_USB2_PHY_T1			0x50
#define MV_USB2_PHY_CHARGE0		0x58
#define MV_USB2_PHY_OTG			0x5C
#define MV_USB2_PHY_PHY_MON		0x60
#define MV_USB2_PHY_CTRL		0x104

/* default values are got from spec */
#define MV_USB2_PHY_PLL0_DEFAULT	0x5A78
#define MV_USB2_PHY_PLL1_DEFAULT	0x0231
#define MV_USB2_PHY_TX0_DEFAULT		0x0488
#define MV_USB2_PHY_ANA1_DEFAULT	0x1680
#define MV_USB2_PHY_OTG_DEFAULT		0x0
#define MV_USB2_PHY_CTRL_DEFAULT	0x00801000
#define MV_USB2_PHY_CTRL_OTG_DEFAULT	0x0398F000

#define MV_USB2_PHY_PLL0_PLLVDD18(x)			(((x) & 0x3) << 14)
#define MV_USB2_PHY_PLL0_REFDIV(x)			(((x) & 0x1f) << 9)
#define MV_USB2_PHY_PLL0_FBDIV(x)			(((x) & 0x1ff) << 0)

#define MV_USB2_PHY_PLL1_PLL_READY			(0x1 << 15)
#define MV_USB2_PHY_PLL1_PLL_CONTROL_BY_PIN		(0x1 << 14)
#define MV_USB2_PHY_PLL1_PU_PLL				(0x1 << 13)
#define MV_USB2_PHY_PLL1_PLL_LOCK_BYPASS		(0x1 << 12)
#define MV_USB2_PHY_PLL1_DLL_RESET			(0x1 << 11)
#define MV_USB2_PHY_PLL1_ICP(x)				(((x) & 0x7) << 8)
#define MV_USB2_PHY_PLL1_KVCO_EXT			(0x1 << 7)
#define MV_USB2_PHY_PLL1_KVCO(x)			(((x) & 0x7) << 4)
#define MV_USB2_PHY_PLL1_CLK_BLK_EN			(0x1 << 3)
#define MV_USB2_PHY_PLL1_VCOCAL_START			(0x1 << 2)
#define MV_USB2_PHY_PLL1_PLLCAL12(x)			(((x) & 0x3) << 0)

#define MV_USB2_PHY_TX0_TXDATA_BLK_EN			(0x1 << 14)
#define MV_USB2_PHY_TX0_RCAL_START			(0x1 << 13)
#define MV_USB2_PHY_TX0_EXT_HS_RCAL_EN			(0x1 << 12)
#define MV_USB2_PHY_TX0_EXT_FS_RCAL_EN			(0x1 << 11)
#define MV_USB2_PHY_TX0_IMPCAL_VTH(x)			(((x) & 0x7) << 8)
#define MV_USB2_PHY_TX0_EXT_HS_RCAL(x)			(((x) & 0xf) << 4)
#define MV_USB2_PHY_TX0_EXT_FS_RCAL(x)			(((x) & 0xf) << 0)

#define MV_USB2_PHY_TX1_TXVDD15(x)			(((x) & 0x3) << 10)
#define MV_USB2_PHY_TX1_TXVDD12(x)			(((x) & 0x3) << 8)
#define MV_USB2_PHY_TX1_LOWVDD_EN			(0x1 << 7)
#define MV_USB2_PHY_TX1_AMP(x)				(((x) & 0x7) << 4)
#define MV_USB2_PHY_TX1_CK60_PHSEL(x)			(((x) & 0xf) << 0)

#define MV_USB2_PHY_TX2_DRV_SLEWRATE(x)			(((x) & 0x3) << 10)
#define MV_USB2_PHY_TX2_IMP_CAL_DLY(x)			(((x) & 0x3) << 8)
#define MV_USB2_PHY_TX2_FSDRV_EN(x)			(((x) & 0xf) << 4)
#define MV_USB2_PHY_TX2_HSDEV_EN(x)			(((x) & 0xf) << 0)

#define MV_USB2_PHY_RX0_PHASE_FREEZE_DLY		(0x1 << 15)
#define MV_USB2_PHY_RX0_USQ_LENGTH			(0x1 << 14)
#define MV_USB2_PHY_RX0_ACQ_LENGTH(x)			(((x) & 0x3) << 12)
#define MV_USB2_PHY_RX0_SQ_LENGTH(x)			(((x) & 0x3) << 10)
#define MV_USB2_PHY_RX0_DISCON_THRESH(x)		(((x) & 0x3) << 8)
#define MV_USB2_PHY_RX0_SQ_THRESH(x)			(((x) & 0xf) << 4)
#define MV_USB2_PHY_RX0_LPF_COEF(x)			(((x) & 0x3) << 2)
#define MV_USB2_PHY_RX0_INTPI(x)			(((x) & 0x3) << 0)

#define MV_USB2_PHY_RX1_EARLY_VOS_ON_EN			(0x1 << 13)
#define MV_USB2_PHY_RX1_RXDATA_BLOCK_EN			(0x1 << 12)
#define MV_USB2_PHY_RX1_EDGE_DET_EN			(0x1 << 11)
#define MV_USB2_PHY_RX1_CAP_SEL(x)			(((x) & 0x7) << 8)
#define MV_USB2_PHY_RX1_RXDATA_BLOCK_LENGTH(x)		(((x) & 0x3) << 6)
#define MV_USB2_PHY_RX1_EDGE_DET_SEL(x)			(((x) & 0x3) << 4)
#define MV_USB2_PHY_RX1_CDR_COEF_SEL			(0x1 << 3)
#define MV_USB2_PHY_RX1_CDR_FASTLOCK_EN			(0x1 << 2)
#define MV_USB2_PHY_RX1_S2TO3_DLY_SEL(x)		(((x) & 0x3) << 0)

#define MV_USB2_PHY_RX2_USQ_FILTER			(0x1 << 8)
#define MV_USB2_PHY_RX2_SQ_CM_SEL			(0x1 << 7)
#define MV_USB2_PHY_RX2_SAMPLER_CTRL			(0x1 << 6)
#define MV_USB2_PHY_RX2_SQ_BUFFER_EN			(0x1 << 5)
#define MV_USB2_PHY_RX2_SQ_ALWAYS_ON			(0x1 << 4)
#define MV_USB2_PHY_RX2_RXVDD18(x)			(((x) & 0x3) << 2)
#define MV_USB2_PHY_RX2_RXVDD12(x)			(((x) & 0x3) << 0)

#define MV_USB2_PHY_ANA0_BG_VSEL(x)			(((x) & 0x3) << 8)
#define MV_USB2_PHY_ANA0_DIG_SEL(x)			(((x) & 0x3) << 6)
#define MV_USB2_PHY_ANA0_TOPVDD18(x)			(((x) & 0x3) << 4)
#define MV_USB2_PHY_ANA0_VDD_USB2_DIG_TOP_SEL		(0x1 << 3)
#define MV_USB2_PHY_ANA0_IPTAT_SEL(x)			(((x) & 0x7) << 0)

#define MV_USB2_PHY_ANA1_PU_ANA				(0x1 << 14)
#define MV_USB2_PHY_ANA1_ANA_CONTROL_BY_PIN		(0x1 << 13)
#define MV_USB2_PHY_ANA1_SEL_LPFR			(0x1 << 12)
#define MV_USB2_PHY_ANA1_V2I_EXT			(0x1 << 11)
#define MV_USB2_PHY_ANA1_V2I(x)				(((x) & 0x7) << 8)
#define MV_USB2_PHY_ANA1_R_ROTATE_SEL			(0x1 << 7)
#define MV_USB2_PHY_ANA1_STRESS_TEST_MODE		(0x1 << 6)
#define MV_USB2_PHY_ANA1_TESTMON_ANA(x)			(((x) & 0x3f) << 0)

#define MV_USB2_PHY_DIG0_FIFO_UF			(0x1 << 15)
#define MV_USB2_PHY_DIG0_FIFO_OV			(0x1 << 14)
#define MV_USB2_PHY_DIG0_FS_EOP_MODE			(0x1 << 13)
#define MV_USB2_PHY_DIG0_HOST_DISCON_SEL1		(0x1 << 12)
#define MV_USB2_PHY_DIG0_HOST_DISCON_SEL0		(0x1 << 11)
#define MV_USB2_PHY_DIG0_FORCE_END_EN			(0x1 << 10)
#define MV_USB2_PHY_DIG0_SYNCDET_WINDOW_EN		(0x1 << 8)
#define MV_USB2_PHY_DIG0_CLK_SUSPEND_EN			(0x1 << 7)
#define MV_USB2_PHY_DIG0_HS_DRIBBLE_EN			(0x1 << 6)
#define MV_USB2_PHY_DIG0_SYNC_NUM(x)			(((x) & 0x3) << 4)
#define MV_USB2_PHY_DIG0_FIFO_FILL_NUM(x)		(((x) & 0xf) << 0)

#define MV_USB2_PHY_DIG1_FS_RX_ERROR_MODE2		(0x1 << 15)
#define MV_USB2_PHY_DIG1_FS_RX_ERROR_MODE1		(0x1 << 14)
#define MV_USB2_PHY_DIG1_FS_RX_ERROR_MODE		(0x1 << 13)
#define MV_USB2_PHY_DIG1_CLK_OUT_SEL			(0x1 << 12)
#define MV_USB2_PHY_DIG1_EXT_TX_CLK_SEL			(0x1 << 11)
#define MV_USB2_PHY_DIG1_ARC_DPDM_MODE			(0x1 << 10)
#define MV_USB2_PHY_DIG1_DP_PULLDOWN			(0x1 << 9)
#define MV_USB2_PHY_DIG1_DM_PULLDOWN			(0x1 << 8)
#define MV_USB2_PHY_DIG1_SYNC_IGNORE_SQ			(0x1 << 7)
#define MV_USB2_PHY_DIG1_SQ_RST_RX			(0x1 << 6)
#define MV_USB2_PHY_DIG1_MON_SEL(x)			(((x) & 0x3f) << 0)

#define MV_USB2_PHY_DIG2_PAD_STRENGTH(x)		(((x) & 0x1f) << 8)
#define MV_USB2_PHY_DIG2_LONG_EOP			(0x1 << 5)
#define MV_USB2_PHY_DIG2_NOVBUS_DPDM00			(0x1 << 4)
#define MV_USB2_PHY_DIG2_ALIGN_FS_OUTEN			(0x1 << 2)
#define MV_USB2_PHY_DIG2_HS_HDL_SYNC			(0x1 << 1)
#define MV_USB2_PHY_DIG2_FS_HDL_OPMD			(0x1 << 0)

#define MV_USB2_PHY_CHARGE0_ENABLE_SWITCH		(0x1 << 3)
#define MV_USB2_PHY_CHARGE0_PU_CHRG_DTC			(0x1 << 2)
#define MV_USB2_PHY_CHARGE0_TESTMON_CHRGDTC(x)		(((x) & 0x3) << 0)

#define MV_USB2_PHY_OTG_TESTMODE(x)			(((x) & 0x7) << 0)
#define MV_USB2_PHY_OTG_CONTROL_BY_PIN			(0x1 << 4)
#define MV_USB2_PHY_OTG_PU				(0x1 << 3)

#define MV_USB2_PHY_CTRL_PU_PLL				(0x1 << 1)
#define MV_USB2_PHY_CTRL_PU				(0x1 << 0)

struct usb_file {
#define SEL_LPFR_MASK		(0x3 << 28)
#define SEL_LPFR(x)		((x << 28) & SEL_LPFR_MASK)
#define FB_DIV_MASK		(0x1ff << 16)
#define FB_DIV(x)		((x << 16) & FB_DIV_MASK)
#define REFDIV_MASK		0x7f
#define REFDIV(x)		(x & REFDIV_MASK)
#define ICP_MASK		(0xf << 8)
#define ICP(x)			((x << 8) & ICP_MASK)
#define PLL_READY_MASK		(0x1 << 31)
	u32 pll_reg0;
#define PLL_PU_MASK		(0x1 << 0)
#define PLL_PU(x)		((x << 0) & PLL_PU_MASK)
#define PU_BY_MASK		(0x1 << 1)
#define PU_BY(x)		((x << 1) & PU_BY_MASK)
	u32 pll_reg1;
#define PLLCAL_DONE_MASK	(0x1 << 31)
#define PLLCAL_DONE(x)		((x << 31) & PLLCAL_DONE_MASK)
#define IMPCAL_DONE_MASK	(0x1 << 23)
#define IMPCAL_DONE(x)		((x << 23) & IMPCAL_DONE_MASK)
#define IMPCAL_START		(0x1 << 13)
#define PLLCAL_START		(0x1 << 22)
	u32 cal_reg;
#define TX_PU_BY_MASK		(0x1 << 25)
#define TX_PU_BY(x)		((x << 25) & TX_PU_BY_MASK)
#define ANA_PU_MASK		(0x1 << 24)
#define ANA_PU(x)		((x << 24) & ANA_PU_MASK)
#define AMP_MASK		(0x7 << 20)
#define AMP(x)			((x << 20) & AMP_MASK)
	u32 tx_reg0;
	u32 tx_reg1;
#define SQ_THRESH_MASK		0xf
#define SQ_THRESH(x)		(x & SQ_THRESH_MASK)
	u32 rx_reg0;
#define SQCAL_DONE_MASK		(0x1 << 31)
#define SQCAL_DONE(x)		((x << 31) & SQCAL_DONE_MASK)
#define SQCAL_START			(0x1 << 4)
	u32 rx_reg1;
#define BITSTAFFING_ERR_MASK	(0x1 << 31)
#define SYNC_ERR_MASK		(0x1 << 30)
#define SQ_FILT_MASK		(0x7 << 16)
#define SQ_FILT(x)		((x << 16) & SQ_FILT_MASK)
#define SQ_BLK_MASK		(0x7 << 12)
#define SQ_BLK(x)		((x << 12) & SQ_BLK_MASK)
#define SYNC_NUM_MASK		(0x3 << 0)
#define SYNC_NUM(x)		((x << 0) & SYNC_NUM_MASK)
	u32 dig_reg0;
	u32 dig_reg1;
	u32 test_reg0;
	u32 test_reg1;
	u32 mon_reg;
	u32 res;

#define OTG_CTRL_BY_MASK	(0x1 << 5)
#define OTG_CTRL_BY(x)		((x << 5) & OTG_CTRL_BY_MASK)
#define PU_OTG_MASK		(0x1 << 4)
#define PU_OTG(x)		((x << 4) & PU_OTG_MASK)
	u32 otg_reg;
	u32 chrg_reg;
	u32 pad[34];
	u32 usb_ctrl0;
	u32 usb_ctrl1;
	u32 pad1[2];
	u32 usb_ctrl2;
	u32 usb_ctrl3;
};

int usb_lowlevel_init(int index, enum usb_init_type init, void **controller);
#endif /* _USB_MV_PHY_H */
