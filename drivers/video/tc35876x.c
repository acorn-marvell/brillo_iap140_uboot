/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/types.h>
#include <i2c.h>
#include <asm/gpio.h>
#include "tc35876x.h"
#include <mmp_disp.h>

#define tc_read16(reg, pval)	i2c_read(0xf, (unsigned int)reg,\
		2, (void *)pval, 2)
#define tc_read32(reg, pval)	i2c_read(0xf, (unsigned int)reg,\
		2, (void *)pval, 4)
#define tc_write32(reg, val)	do { u32 _val; _val = val; \
			i2c_write(0xf, reg, 2, (u8 *)&_val, 4); \
			} while (0)

#define TC358765_CHIPID_REG	0x0580
#define TC358765_CHIPID		0x6500
static int dsi_dump_tc358765(void)
{
	u32 val;

	tc_read32(PPI_TX_RX_TA, &val);
	printf("tc35876x - PPI_TX_RX_TA = 0x%x\n", val);
	tc_read32(PPI_LPTXTIMECNT, &val);
	printf("tc35876x - PPI_LPTXTIMECNT = 0x%x\n", val);
	tc_read32(PPI_D0S_CLRSIPOCOUNT, &val);
	printf("tc35876x - PPI_D0S_CLRSIPOCOUNT = 0x%x\n", val);
	tc_read32(PPI_D1S_CLRSIPOCOUNT, &val);
	printf("tc35876x - PPI_D1S_CLRSIPOCOUNT = 0x%x\n", val);

	tc_read32(PPI_D2S_CLRSIPOCOUNT, &val);
	printf("tc35876x - PPI_D2S_CLRSIPOCOUNT = 0x%x\n", val);
	tc_read32(PPI_D3S_CLRSIPOCOUNT, &val);
	printf("tc35876x - PPI_D3S_CLRSIPOCOUNT = 0x%x\n", val);

	tc_read32(PPI_LANEENABLE, &val);
	printf("tc35876x - PPI_LANEENABLE = 0x%x\n", val);
	tc_read32(DSI_LANEENABLE, &val);
	printf("tc35876x - DSI_LANEENABLE = 0x%x\n", val);
	tc_read32(PPI_STARTPPI, &val);
	printf("tc35876x - PPI_STARTPPI = 0x%x\n", val);
	tc_read32(DSI_STARTDSI, &val);
	printf("tc35876x - DSI_STARTDSI = 0x%x\n", val);

	tc_read32(VPCTRL, &val);
	printf("tc35876x - VPCTRL = 0x%x\n", val);
	tc_read32(HTIM1, &val);
	printf("tc35876x - HTIM1 = 0x%x\n", val);
	tc_read32(HTIM2, &val);
	printf("tc35876x - HTIM2 = 0x%x\n", val);
	tc_read32(VTIM1, &val);
	printf("tc35876x - VTIM1 = 0x%x\n", val);
	tc_read32(VTIM2, &val);
	printf("tc35876x - VTIM2 = 0x%x\n", val);
	tc_read32(VFUEN, &val);
	printf("tc35876x - VFUEN = 0x%x\n", val);
	tc_read32(LVCFG, &val);
	printf("tc35876x - LVCFG = 0x%x\n", val);

	tc_read32(DSI_INTSTAUS, &val);
	printf("!! - DSI_INTSTAUS= 0x%x BEFORE\n", val);
	tc_write32(DSI_INTCLR, 0xFFFFFFFF);
	tc_read32(DSI_INTSTAUS, &val);
	printf("!! - DSI_INTSTAUS= 0x%x AFTER\n", val);

	tc_read32(DSI_LANESTATUS0, &val);
	printf("tc35876x - DSI_LANESTATUS0= 0x%x\n", val);
	tc_read32(DSIERRCNT, &val);
	printf("tc35876x - DSIERRCNT= 0x%x\n", val);
	tc_read32(DSIERRCNT, &val);
	printf("tc35876x - DSIERRCNT= 0x%x AGAIN\n", val);
	tc_read32(SYSSTAT, &val);
	printf("tc35876x - SYSSTAT= 0x%x\n", val);

	tc_read32(LVMX0003, &val);
	printf(" - LVMX0003,= 0x%x\n", val);
	tc_read32(LVMX0407, &val);
	printf(" - LVMX0407,= 0x%x\n", val);
	tc_read32(LVMX0811, &val);
	printf(" - LVMX0811,= 0x%x\n", val);
	tc_read32(LVMX1215, &val);
	printf(" - LVMX1215,= 0x%x\n", val);
	tc_read32(LVMX1619, &val);
	printf(" - LVMX1619,= 0x%x\n", val);
	tc_read32(LVMX2023, &val);
	printf(" - LVMX2023,= 0x%x\n", val);
	tc_read32(LVMX2427, &val);
	printf(" - LVMX2427,= 0x%x\n", val);

	return 0;
}

int dsi_set_tc358765(struct mmp_disp_info *fbi)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;
	unsigned int cur_bus = i2c_get_bus_num();
	u16 chip_id = 0;
	int status;

	/* Switch to TWSIx bus */
	i2c_set_bus_num(mi->twsi_id);
	status = tc_read16(TC358765_CHIPID_REG, &chip_id);
	if ((status < 0) || (chip_id != TC358765_CHIPID)) {
		printf("tc358765 chip ID is 0x%x\n", chip_id);
		i2c_set_bus_num(cur_bus);
		return -1;
	}

	/* REG 0x13C,DAT 0x000C000F */
	tc_write32(PPI_TX_RX_TA, 0x00040004);
	/* REG 0x114,DAT 0x0000000A */
	tc_write32(PPI_LPTXTIMECNT, 0x00000004);

	/* get middle value of mim-max value
	 * 0-0x13 for 2lanes-rgb888, 0-0x26 for 4lanes-rgb888
	 * 0-0x21 for 2lanes-rgb565, 0-0x25 for 4lanes-rgb565
	 */
	if (di->lanes == 4)
		status = 0x13;
	else if (di->bpp == 24)
		status = 0xa;
	else
		status = 0x11;
	/* REG 0x164,DAT 0x00000005 */
	tc_write32(PPI_D0S_CLRSIPOCOUNT, status);
	/* REG 0x168,DAT 0x00000005 */
	tc_write32(PPI_D1S_CLRSIPOCOUNT, status);
	if (di->lanes == 4) {
		/* REG 0x16C,DAT 0x00000005 */
		tc_write32(PPI_D2S_CLRSIPOCOUNT, status);
		/* REG 0x170,DAT 0x00000005 */
		tc_write32(PPI_D3S_CLRSIPOCOUNT, status);
	}

	/* REG 0x134,DAT 0x00000007 */
	tc_write32(PPI_LANEENABLE, (di->lanes == 4) ? 0x1f : 0x7);
	/* REG 0x210,DAT 0x00000007 */
	tc_write32(DSI_LANEENABLE, (di->lanes == 4) ? 0x1f : 0x7);

	/* REG 0x104,DAT 0x00000001 */
	tc_write32(PPI_STARTPPI, 0x0000001);
	/* REG 0x204,DAT 0x00000001 */
	tc_write32(DSI_STARTDSI, 0x0000001);

	/* REG 0x450,DAT 0x00012020, VSDELAY = 8 pixels */
	if (strcmp(mi->id, "GFX Layer T7") == 0)
		tc_write32(VPCTRL, 0x00800020 |
			(di->bpp == 24 ? (1 << 8) : (0 << 8)));
	else
		tc_write32(VPCTRL, 0x00800020);

	/* REG 0x454,DAT 0x00200008*/
	tc_write32(HTIM1, 0x00200008);

	/* REG 0x45C,DAT 0x00040004*/
	tc_write32(VTIM1, 0x00040004);

	/*
	 * We need to change the default bit mapping
	 * from LSB on 4th channel, to MSB.
	 */
	if (strcmp(mi->id, "GFX Layer T7") == 0) {
		tc_write32(LVMX0003, 0x03020100);
		tc_write32(LVMX0407, 0x08050704);
		tc_write32(LVMX0811, 0x0f0e0a09);
		tc_write32(LVMX1215, 0x100d0c0b);
		tc_write32(LVMX1619, 0x12111716);
		tc_write32(LVMX2023, 0x1b151413);
		tc_write32(LVMX2427, 0x061a1918);
	}

	/* REG 0x49C,DAT 0x00000201 */
	tc_write32(LVCFG, 0x00000001);

	dsi_dump_tc358765();
	i2c_set_bus_num(cur_bus);

	return 0;
}

int tc358765_reset(void)
{
	gpio_set_value(LCD_RST_GPIO, 0);
	udelay(100000);

	gpio_set_value(LCD_RST_GPIO, 1);
	udelay(100000);

	return 0;
}
