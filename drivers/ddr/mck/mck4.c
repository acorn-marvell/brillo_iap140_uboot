/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <pxa_amp.h>
#include <asm/io.h>
#include <asm/arch/pxa988.h>
#include <asm/arch/cpu.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Pantheon DRAM controller supports upto 8 banks
 * for chip select 0 and 1
 */

/*
 * DDR Memory Control Registers
 * Refer Datasheet 4.4
 */
struct pxa988ddr_map_registers {
	u32	cs;	/* Memory Address Map Register -CS */
};

struct pxa988ddr_registers {
	u8	pad[0x10];
	struct pxa988ddr_map_registers mmap[2];
};

/*
 * pxa988_sdram_base - reads SDRAM Base Address Register
 */
u32 pxa988_sdram_base(int chip_sel, u32 base)
{
	struct pxa988ddr_registers *ddr_regs;
	u32 cs_valid = 0;
	u32 result = 0;

	ddr_regs = (struct pxa988ddr_registers *)base;
	cs_valid = 0x01 & readl(&ddr_regs->mmap[chip_sel].cs);
	if (!cs_valid)
		return 0;

	result = readl(&ddr_regs->mmap[chip_sel].cs) & 0xFF800000;
	return result;
}

/*
 * pxa988_sdram_size - reads SDRAM size
 */
static u32 pxa988_sdram_size(int chip_sel, u32 base)
{
	struct pxa988ddr_registers *ddr_regs;
	u32 cs_valid = 0;
	u32 result = 0;

	ddr_regs = (struct pxa988ddr_registers *)base;
	cs_valid = 0x01 & readl(&ddr_regs->mmap[chip_sel].cs);
	if (!cs_valid)
		return 0;

	result = readl(&ddr_regs->mmap[chip_sel].cs);
	result = (result >> 16) & 0xF;
	if (result < 0x7) {
		printf("Unknown DRAM Size\n");
		return -1;
	} else {
		return (0x8 << (result - 0x7)) * 1024 * 1024;
	}
}

#ifndef CONFIG_SYS_BOARD_DRAM_INIT
int dram_init(void)
{
	u32 base = PXA988_DRAM_BASE;
	int i, mcb_conf0;
	ulong dram_start, dram_size;
	unsigned int reloc_end;

	gd->ram_size = 0;
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		dram_start = pxa988_sdram_base(i, base);
		dram_size = pxa988_sdram_size(i, base);

		if (gd->bd) {
			gd->bd->bi_dram[i].start = dram_start;
			gd->bd->bi_dram[i].size = dram_size;
		}
		/*
		 * It is assumed that all memory banks are consecutive
		 * and without gaps.
		 * If the gap is found, ram_size will be reported for
		 * consecutive memory only
		 */
		if (dram_start != gd->ram_size)
			break;

		gd->ram_size += dram_size;
	}

	/*
	 * limit the uboot ddr usage between CONFIG_SYS_TEXT_BASE and
	 * CONFIG_SYS_RELOC_END.
	 */
	reloc_end = pxa_amp_reloc_end();
	if (gd->ram_size < reloc_end)
		hang();
	else
		gd->ram_size = reloc_end;

	for (; i < CONFIG_NR_DRAM_BANKS; i++) {
		/*
		 * If above loop terminated prematurely, we need to set
		 * remaining banks' start address & size as 0. Otherwise other
		 * u-boot functions and Linux kernel gets wrong values which
		 * could result in crash
		 */
		if (gd->bd) {
			gd->bd->bi_dram[i].start = 0;
			gd->bd->bi_dram[i].size = 0;
		}
	}

	/*
	 * Set MSA as critical priority for DDR access.
	 * And set LCD as high priority for DDR access, which is helpful to fix
	 * the flicker issue.
	 * Set CP as high priority for LTE, which is helpful for performance.
	 */
	if (cpu_is_pxa1L88())
		writel(0xF7402008, base + 0x140);
	else
		writel(0xF7402000, base + 0x140);
	mcb_conf0 = readl(MC_QOS_CTRL);
	writel(mcb_conf0 | 0x3, MC_QOS_CTRL);

	return 0;
}

/*
 * If this function is not defined here,
 * board.c alters dram bank zero configuration defined above.
 */
void dram_init_banksize(void)
{
	dram_init();

	if (gd->bd->bi_dram[0].size < CONFIG_TZ_HYPERVISOR_SIZE) {
		printf("Cannot meet requirement for TrustZone!\n");
	} else {
		gd->bd->bi_dram[0].start += CONFIG_TZ_HYPERVISOR_SIZE;
		gd->bd->bi_dram[0].size  -= CONFIG_TZ_HYPERVISOR_SIZE;
		gd->ram_size -= CONFIG_TZ_HYPERVISOR_SIZE;
	}
}
#endif /* CONFIG_SYS_BOARD_DRAM_INIT */
