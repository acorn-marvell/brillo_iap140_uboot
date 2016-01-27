/*
 * squ debug feature from helan2
 *
 * Copyright (C) 2013 MARVELL Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <emmd_rsv.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>

/* squ debug feature added from helan2 */

/* reserve the last 256byte in emmd descriptor for squ register dump*/
struct debug_held_status {
	u32 fabws;
	u32 fabrs;
	u32 dvcs;
	u32 fcdones;
	u32 pmulpms;
	u32 corelpms;
	u32 plls;
	u32 rsv[54];
};

int save_squdbg_regval2ddr(void)
{
	struct debug_held_status *dumpaddr;

	dumpaddr = (struct debug_held_status *)(emmd_page->held_status);
	dumpaddr->fabws = readl(FABTIMEOUT_HELD_WSTATUS);
	dumpaddr->fabrs = readl(FABTIMEOUT_HELD_RSTATUS);
	dumpaddr->dvcs = readl(DVC_HELD_STATUS);
	dumpaddr->fcdones = readl(FCDONE_HELD_STATUS);
	dumpaddr->pmulpms = readl(PMULPM_HELD_STATUS);
	dumpaddr->corelpms = readl(CORELPM_HELD_STATUS);
	if (cpu_is_pxa1908())
		dumpaddr->plls = readl(PLL_HELD_STATUS);
	return 0;
}

int show_squdbg_regval(void)
{
	u32 val;

	printf("*************************************\n");
	printf("Fabric/LPM/DFC/DVC held status dump:\n");

	val = readl(FABTIMEOUT_HELD_WSTATUS);
	if (val & 0x1)
		printf("AXI timeout occurred when write addr 0x%x!!\n",
			val & 0xfffffffc);

	val = readl(FABTIMEOUT_HELD_RSTATUS);
	if (val & 0x1)
		printf("AXI timeout occurred when read addr 0x%x!!\n",
			val & 0xfffffffc);

	printf("DVC[%x]\n", readl(DVC_HELD_STATUS));
	printf("FCDONE[%x]\n", readl(FCDONE_HELD_STATUS));
	printf("PMULPM[%x]\n", readl(PMULPM_HELD_STATUS));
	printf("CORELPM[%x]\n", readl(CORELPM_HELD_STATUS));
	if (cpu_is_pxa1908())
		printf("PLL[%x]\n", readl(PLL_HELD_STATUS));
	printf("*************************************\n");

	return 0;
}
