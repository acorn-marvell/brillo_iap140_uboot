/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <mmp_disp.h>
#ifndef __MACH_TC35876x_H
#define __MACH_TC35876x_H

/* DSI PPI Layer Registers */
#define PPI_STARTPPI				0x0104
#define PPI_LPTXTIMECNT				0x0114
#define PPI_LANEENABLE				0x0134
#define PPI_TX_RX_TA		                0x013c
#define PPI_D0S_CLRSIPOCOUNT	                0x0164
#define PPI_D1S_CLRSIPOCOUNT	                0x0168
#define PPI_D2S_CLRSIPOCOUNT	                0x016c
#define PPI_D3S_CLRSIPOCOUNT	                0x0170
/* DSI Protocol Layer Register */
#define DSI_STARTDSI		                0x0204
#define DSI_LANEENABLE				0x0210
/* Video Path Register */
#define VPCTRL					0x0450
#define HTIM1					0x0454
#define HTIM2					0x0458
#define VTIM1					0x045C
#define VTIM2					0x0460
#define VFUEN					0x0464
/* LVDS Registers */
#define LVCFG					0x049c
/* DSI Protocol Layer Regsters */
#define DSI_LANESTATUS0				0x0214
#define DSI_INTSTAUS			        0x0220
#define DSI_INTCLR				0x0228
/* DSI General Registers */
#define DSIERRCNT				0x0300
/*LVDS Registers*/
#define LVMX0003				0x0480
#define LVMX0407				0x0484
#define LVMX0811				0x0488
#define LVMX1215				0x048c
#define LVMX1619				0x0490
#define LVMX2023				0x0494
#define LVMX2427				0x0498
/* System Registers */
#define SYSSTAT					0x0500

int dsi_set_tc358765(struct mmp_disp_info *fbi);
int tc358765_reset(void);
#endif
