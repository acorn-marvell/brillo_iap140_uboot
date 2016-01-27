/*
 * (C) Copyright 2013
 * Marvell Semiconductor <www.marvell.com>
 * Leo Song <liangs@marvell.com>
 * Yi Zhang <yizhang@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/config.h>
#include <usb.h>
#include <linux/usb/mv-phy.h>

#ifdef CONFIG_MRVL_USB_PHY_28LP
static void dump_phy_regs(u32 base)
{
	int i;
	u32 tmp32;
	for (i = 0; i < 0x3f; i++) {
		tmp32 = readl((uintptr_t)base + i * 4);
		printf("[0x%x] = 0x%x\t", (base + i * 4), tmp32);
		if (i % 5 == 0)
			printf("\n");
	}
}

static int wait_for_phy_ready(struct usb_file *file)
{
	u32 tmp, done_mask;
	int count = 0;
	/*
	 * polling calibration done?
	 * 0xd420_7018[31] = 0x1;
	 * 0xd420_7008[31] = 0x1;
	 * 0xd420_7008[23] = 0x1;
	 */

	/* IMP Calibrate */
	writel(readl(&file->cal_reg) |
		IMPCAL_START, &file->cal_reg);
	/* Wait For Calibrate PHY */
	udelay(400);
	/* Make sure PHY Calibration is ready */
	do {
		tmp = readl(&file->cal_reg);
		done_mask = IMPCAL_DONE_MASK;
		tmp &= done_mask;
		if (tmp == IMPCAL_DONE(1))
			break;
		udelay(1000);
	} while ((count++) && (count < 100));
        if (count >= 100)
                printf("USB PHY Calibrate not done after 100mS.");
        writel(readl(&file->cal_reg) &
                (~IMPCAL_START), &file->cal_reg);

        /* PLL Calibrate */
	writel(readl(&file->cal_reg) |
		PLLCAL_START, &file->cal_reg);
	udelay(400);
	count = 0;
	do {
		tmp = readl(&file->cal_reg);
		done_mask = PLLCAL_DONE_MASK;
		tmp &= done_mask;
		if (tmp == PLLCAL_DONE(1))
			break;
		udelay(1000);
	} while ((count++) && (count < 100));
	if (count >= 100)
		printf("USB PHY Calibrate not done after 100mS.");
	writel(readl(&file->cal_reg) &
		(~PLLCAL_START), &file->cal_reg);

	/* SQ Calibrate */
	writel(readl(&file->rx_reg1) |
		SQCAL_START, &file->rx_reg1);
	/* Wait For Calibrate PHY */
	udelay(400);
	/* Make sure PHY Calibration is ready */
	count = 0;
	do {
		tmp = readl(&file->rx_reg1);
		done_mask = SQCAL_DONE_MASK;
		tmp &= done_mask;
		if (tmp == SQCAL_DONE(1))
			break;
		udelay(1000);
	} while ((count++) && (count < 100));
	if (count >= 100)
		printf("USB PHY Calibrate not done after 100mS.");
	writel(readl(&file->rx_reg1) &
		(~SQCAL_START), &file->rx_reg1);

	/*
	 * phy is ready?
	 * 0xd420_7000[31] = 0x1;
	 */
	count = 100;
	while (((readl(&file->pll_reg0) & PLL_READY_MASK) == 0) && count--)
		udelay(1000);
	if (count <= 0) {
		printf("%s %d: wait for ready timeout, UTMI_PLL 0x%x\n",
		       __func__, __LINE__, readl(&file->pll_reg1));
		return -1;
	}
	return 0;
}

static int mrvl_usb_phy_28nm_init(u32 base)
{
	struct usb_file *file = (struct usb_file *)(uintptr_t)base;
	u32 tmp32;
	int ret;

	/*
	 * pll control 0:
	 * 0xd420_7000[6:0] = 0xd, b'000_1101	  --->  REFDIV
	 * 0xd420_7000[24:16] = 0xf0, b'1111_0000 --->  FBDIV
	 * 0xd420_7000[11:8] = 0x3, b'0011        --->  ICP
	 * 0xd420_7000[29:28] = 0x1,  b'01	  --->  SEL_LPFR
	 */
	tmp32 = readl(&file->pll_reg0);
	tmp32 &= ~(REFDIV_MASK | FB_DIV_MASK | ICP_MASK | SEL_LPFR_MASK);
	tmp32 |= REFDIV(0xd) | FB_DIV(0xf0) | ICP(0x3) | SEL_LPFR(0x1);
	writel(tmp32, &file->pll_reg0); /*
	 * pll control 1:
	 * 0xd420_7004[1:0] = 0x3, b'11	   ---> [PU_PLL_BY_REG:PU_PLL]
	 */
	tmp32 = readl(&file->pll_reg1);
	tmp32 &= ~(PLL_PU_MASK | PU_BY_MASK);
	tmp32 |= PLL_PU(0x1) | PU_BY(0x1);
	writel(tmp32, &file->pll_reg1);
	/*
	 * tx reg 0:
	 * 0xd420_700c[22:20] = 0x3, b'11  ---> AMP
	 */
	tmp32 = readl(&file->tx_reg0);
	tmp32 &= ~(AMP_MASK);
	tmp32 |= AMP(0x3);
	writel(tmp32, &file->tx_reg0);
	/*
	 * rx reg 0:
	 * 0xd420_7018[3:0] = 0xa, b'1010 ---> SQ_THRESH
	 */
	tmp32 = readl(&file->rx_reg0);
	tmp32 &= ~(SQ_THRESH_MASK);
	tmp32 |= SQ_THRESH(0xa);
	writel(tmp32, &file->rx_reg0);
	/*
	 * dig reg 0:
	 * 0xd420_701c[31] = 0, b'0 ---> BITSTAFFING_ERROR
	 * 0xd420_701c[30] = 0, b'0 ---> LOSS_OF_SYNC_ERROR
	 * 0xd420_701c[18:16] = 0x7, b'111 ---> SQ_FILT
	 * 0xd420_701c[14:12] = 0x4, b'100 ---> SQ_BLK
	 * 0xd420_701c[1:0] = 0x2, b'10 ---> SYNC_NUM
	 */
	tmp32 = readl(&file->dig_reg0);
	tmp32 &= ~(BITSTAFFING_ERR_MASK | SYNC_ERR_MASK | SQ_FILT_MASK | SQ_BLK_MASK | SYNC_NUM_MASK);
	tmp32 |= (SQ_FILT(0x0) | SQ_BLK(0x0) | SYNC_NUM(0x1));
	writel(tmp32, &file->dig_reg0);

	/*
	 * otg reg:
	 * 0xd420_7034[5:4] = 0x1,  b'01   ---> [OTG_CONTROL_BY_PIN:PU_OTG]
	 */
	tmp32 = readl(&file->otg_reg);
	tmp32 &= ~(OTG_CTRL_BY_MASK | PU_OTG_MASK);
	tmp32 |= OTG_CTRL_BY(0x0) | PU_OTG(0x1);
	writel(tmp32, &file->otg_reg);
	/*
	 * tx reg 0:
	 * 0xd420_700c[25:24] = 0x3, b'11  --->  [PU_ANA:PU_BY_REG]
	 */
	tmp32 = readl(&file->tx_reg0);
	tmp32 &= ~(ANA_PU_MASK | TX_PU_BY_MASK);
	tmp32 |= ANA_PU(0x1) | TX_PU_BY(0x1);
	writel(tmp32, &file->tx_reg0);

	udelay(400);
	ret = wait_for_phy_ready(file);
	if (ret < 0) {
		printf("initialize usb phy failed, dump usb registers:\n");
		dump_phy_regs(base);
	}

	printf("usb phy inited 0x%x!\n", readl(&file->usb_ctrl0));
	return 0;
}
#endif


#ifdef CONFIG_MRVL_USB_PHY_40LP
static int mrvl_usb_phy_40nm_init(u32 base)
{
	u16 tmp16;
	u32 tmp32;

	/* Program 0xd4207004[8:0]= 0xF0 */
	/* Program 0xd4207004[13:9]=0xD */
	writew((MV_USB2_PHY_PLL0_DEFAULT
		& (~MV_USB2_PHY_PLL0_FBDIV(~0))
		& (~MV_USB2_PHY_PLL0_REFDIV(~0)))
		| MV_USB2_PHY_PLL0_FBDIV(0xF0)
		| MV_USB2_PHY_PLL0_REFDIV(0xD), base + MV_USB2_PHY_PLL0);

	/* Program 0xd4207008[11:8]=0x1 */
	/* Program 0xd4207008[14:13]=0x1 */
	writew((MV_USB2_PHY_PLL1_DEFAULT
		& (~MV_USB2_PHY_PLL1_ICP(~0))
		& (~MV_USB2_PHY_PLL1_PLL_CONTROL_BY_PIN))
		| MV_USB2_PHY_PLL1_ICP(0x1)
		| MV_USB2_PHY_PLL1_PU_PLL, base + MV_USB2_PHY_PLL1);

	/* Program 0xd4207034[14]=0x1 */
	writew(MV_USB2_PHY_ANA1_DEFAULT
		| MV_USB2_PHY_ANA1_PU_ANA, base + MV_USB2_PHY_ANA1);

	/* Program 0xd420705c[3]=0x1 */
	writew(MV_USB2_PHY_OTG_DEFAULT
		| MV_USB2_PHY_OTG_PU, base + MV_USB2_PHY_OTG);

	/* Program 0xD4207104[1] = 0x1 */
	/* Program 0xD4207104[0] = 0x1 */
	tmp32 = readl(base + MV_USB2_PHY_CTRL);
	writel(tmp32 | MV_USB2_PHY_CTRL_PU_PLL
		     | MV_USB2_PHY_CTRL_PU, base + MV_USB2_PHY_CTRL);

	/* Wait for 200us */
	udelay(200);

	/* Program 0xd4207008[2]=0x1 */
	tmp16 = readw(base + MV_USB2_PHY_PLL1);
	writew(tmp16
		| MV_USB2_PHY_PLL1_VCOCAL_START, base + MV_USB2_PHY_PLL1);

	/* Wait for 400us */
	udelay(400);

	/* Polling 0xd4207008[15]=0x1 */
	while ((readw(base + MV_USB2_PHY_PLL1)
		& MV_USB2_PHY_PLL1_PLL_READY) == 0)
		printf("polling usb phy\n");

	/* Program 0xd4207010[13]=0x1 */
	writew(MV_USB2_PHY_TX0_DEFAULT
		| MV_USB2_PHY_TX0_RCAL_START, base + MV_USB2_PHY_TX0);

	/* Wait for 40us */
	udelay(40);

	/* Program 0xd4207010[13]=0x0 */
	tmp16 = readw(base + MV_USB2_PHY_TX0);
	writew(tmp16
		& (~MV_USB2_PHY_TX0_RCAL_START), base + MV_USB2_PHY_TX0);

	/* Wait for 400us */
	udelay(400);

	printf("usb phy inited %x!\n", readl(base + MV_USB2_PHY_CTRL));

	return 0;
}
#endif

int usb_lowlevel_init(int index, enum usb_init_type init, void **controller)
{
#ifdef CONFIG_MRVL_USB_PHY_28LP
	mrvl_usb_phy_28nm_init(CONFIG_USB_PHY_BASE);
#else /* CONFIG_MRVL_USB_PHY_40LP */
	mrvl_usb_phy_40nm_init(CONFIG_USB_PHY_BASE);
#endif
	return 0;
}
