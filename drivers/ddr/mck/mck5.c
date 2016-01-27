/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/cpu.h>
#include <pxa_amp.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_PXA1U88
#define MCK5_ADDR_BASE	0xC0100000
#else
#define MCK5_ADDR_BASE	0xD0000000
#endif

#ifdef CONFIG_PXA1928
#define CIU_MCK5_PRI   0xD4282CEC
#endif

/* PXA1928 only use CH0. So ignore CH1 here */
#define MCK5_CH0_MMAP0	(MCK5_ADDR_BASE + 0x200)
#define MCK5_CH0_MMAP1	(MCK5_ADDR_BASE + 0x204)
#define MCK5_PC_CNT0	(MCK5_ADDR_BASE + 0x110)
#define MCK5_PC_CNT1	(MCK5_ADDR_BASE + 0x114)
#define MCK5_PC_CNT2	(MCK5_ADDR_BASE + 0x118)
#define MCK5_PC_CNT3	(MCK5_ADDR_BASE + 0x11C)
#define MCK5_PC_CNT4	(MCK5_ADDR_BASE + 0x120)
#define MCK5_PC_CNT5	(MCK5_ADDR_BASE + 0x124)
#define MCK5_PC_CNT6	(MCK5_ADDR_BASE + 0x128)
#define MCK5_PC_CNT7	(MCK5_ADDR_BASE + 0x12C)
#define MCK5_PC_CFG	(MCK5_ADDR_BASE + 0x100)
#define MCK5_PC_CTRL	(MCK5_ADDR_BASE + 0x10C)
#define MCK5_MAX_CNT	8

int MCK5_sdram_base(int cs, ulong *base)
{
	u32 mmap;

	switch (cs) {
	case 0:
		mmap = readl(MCK5_CH0_MMAP0);
		break;
	case 1:
		mmap = readl(MCK5_CH0_MMAP1);
		break;
	default:
		return -1;
	}

	if (!(0x1 & mmap))
		return -1;
	else
		*base = 0xFF800000 & mmap;
	return 0;
}

int MCK5_sdram_size(int cs, ulong *size)
{
	u32 mmap, _size;

	switch (cs) {
	case 0:
		mmap = readl(MCK5_CH0_MMAP0);
		break;
	case 1:
		mmap = readl(MCK5_CH0_MMAP1);
		break;
	default:
		return -1;
	}

	if (!(0x1 & mmap))
		return -1;

	_size = (mmap >> 16) & 0x1F;
	if (_size < 0x7) {
		printf("Unknown dram size!\n");
		return -1;
	} else {
		*size = (0x8 << (_size - 0x7)) * 1024 * 1024;
	}
	return 0;
}

#ifndef CONFIG_SYS_BOARD_DRAM_INIT
/* dram init when bd not avaliable */
int dram_init(void)
{
	int i;
	unsigned int reloc_end;
	ulong start, size;

	gd->ram_size = 0;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (MCK5_sdram_base(i, &start))
			continue;
		if (start != gd->ram_size) {
			printf("error: unspported not-continous ram\n");
			hang();
		}
		if (MCK5_sdram_size(i, &size)) {
			printf("Read dram size error for chip %d.\n", i);
			hang();
		}
		gd->ram_size += size;
	}

	/*
	 * limit the uboot ddr usage between CONFIG_SYS_TEXT_BASE and
	 * CONFIG_SYS_RELOC_END, which reside in ion range
	 * and won't impact emmddump.
	 */
	reloc_end = pxa_amp_reloc_end();
	if (gd->ram_size < reloc_end)
		hang();
	else
		gd->ram_size = reloc_end;

	/*
	 * Set MSA as critical priority for DDR access to resolve CP assert.
	 * And set LCD as high priority for DDR access, which is helpful to fix
	 * the flicker issue.
	 * Set CP as high priority for LTE, which is helpful for performance.
	 */

#ifdef CONFIG_PXA1928
	writel(0xfb002000, MCK5_ADDR_BASE + 0x84);
	writel(0x00118400,  CIU_MCK5_PRI);
#else
	if (cpu_is_pxa1908() || cpu_is_pxa1936()) {
		writel(0xf7002008, MCK5_ADDR_BASE + 0x84);
		writel(readl(MC_QOS_CTRL) | 0x3, MC_QOS_CTRL);
	} else if (cpu_is_pxa1956()) {
		/* per DE suggestions, only enable high for P7 at initial */
		writel(0xff008000, MCK5_ADDR_BASE + 0x84);
	}

#endif

	return 0;
}

/*
 * If this function is not defined here,
 * board.c alters dram bank zero configuration defined above.
 * read from mck register again and skip error as already
 * checked before
 */
void dram_init_banksize(void)
{
	int i, b = 0;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (!MCK5_sdram_base(i, &gd->bd->bi_dram[b].start))
			MCK5_sdram_size(i, &gd->bd->bi_dram[b++].size);
	}

	/*
	 * If there are banks that are not valid, we need to set
	 * their start address and size to 0. Otherwise other
	 * u-boot functions and Linux kernel gets wrong values which
	 * could result in crash.
	 */
	for (; b < CONFIG_NR_DRAM_BANKS; b++) {
		gd->bd->bi_dram[b].start = 0;
		gd->bd->bi_dram[b].size = 0;
	}

	if (gd->bd->bi_dram[0].size < CONFIG_TZ_HYPERVISOR_SIZE) {
		printf("Cannot meet requirement for TrustZone!\n");
	} else {
		gd->bd->bi_dram[0].start += CONFIG_TZ_HYPERVISOR_SIZE;
		gd->bd->bi_dram[0].size  -= CONFIG_TZ_HYPERVISOR_SIZE;
		gd->ram_size -= CONFIG_TZ_HYPERVISOR_SIZE;
	}
}
#endif /* CONFIG_SYS_BOARD_DRAM_INIT */

void MCK5_perf_cnt_clear(uint32_t cntmask)
{
	int i = 0;

	for (i = 0; i < MCK5_MAX_CNT; i++)
		if (cntmask & (1 << i))
			writel(0x0, (unsigned long)(MCK5_PC_CNT0 + i * 4));
}

/* by default, we config the perf counter to monitor busy-ratio/data-ratio */
void MCK5_perf_cnt_init(void)
{
	writel(0x10, MCK5_PC_CTRL);
	writel(0x9a9880, MCK5_PC_CFG);
}

void MCK5_perf_cnt_stop(void)
{
	writel(0x0, MCK5_PC_CFG);
}

void MCK5_perf_cnt_get(uint32_t *total, uint32_t *busy, uint32_t *data)
{
	*total = readl(MCK5_PC_CNT0);
	*busy = readl(MCK5_PC_CNT1);
	*data = readl(MCK5_PC_CNT2);
}
