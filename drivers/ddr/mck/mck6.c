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

#ifdef CONFIG_PXA1978
#define MCK6_ADDR_BASE	0xF6F00000
#endif

#define MCK6_CH0_MMAP0	(MCK6_ADDR_BASE + 0x200)
#define MCK6_CH0_MMAP1	(MCK6_ADDR_BASE + 0x208)
#define MCK6_CH0_MMAP2  (MCK6_ADDR_BASE + 0x210)
#define MCK6_CH0_MMAP3  (MCK6_ADDR_BASE + 0x218)
/* _HIGH registers follow immediately, i.e. +4 */
#define MCK6_PC_CNT0	(MCK6_ADDR_BASE + 0x110)
#define MCK6_PC_CNT1	(MCK6_ADDR_BASE + 0x114)
#define MCK6_PC_CNT2	(MCK6_ADDR_BASE + 0x118)
#define MCK6_PC_CNT3	(MCK6_ADDR_BASE + 0x11C)
#define MCK6_PC_CNT4	(MCK6_ADDR_BASE + 0x120)
#define MCK6_PC_CNT5	(MCK6_ADDR_BASE + 0x124)
#define MCK6_PC_CNT6	(MCK6_ADDR_BASE + 0x128)
#define MCK6_PC_CNT7	(MCK6_ADDR_BASE + 0x12C)
#define MCK6_PC_CFG	(MCK6_ADDR_BASE + 0x100)
#define MCK6_PC_CFG1	(MCK6_ADDR_BASE + 0x104)
#define MCK6_PC_CTRL	(MCK6_ADDR_BASE + 0x10C)
#define MCK6_MAX_CNT	8

static const u32 mmap_addr[] = {
	MCK6_CH0_MMAP0,
	MCK6_CH0_MMAP1,
	MCK6_CH0_MMAP2,
	MCK6_CH0_MMAP3
};
#define MCK6_MAX_CS (sizeof(mmap_addr)/sizeof(mmap_addr[0]))

/* Note: the code below does not support non-interleaved CH1 */
int MCK6_sdram_base(int cs, ulong *base)
{
	u32 mmap;

	if (cs > MCK6_MAX_CS)
		return -1;

	mmap = readl(mmap_addr[cs]);
	if (!(0x1 & mmap))
		return -1;

	if (readl(mmap_addr[cs] + 4) != 0) {
		printf("Ignoring CS%d configured for LPAE\n", cs);
		return -1;
	}
	*base = 0xFF800000 & mmap;
	return 0;
}

int MCK6_sdram_size(int cs, ulong *size)
{
	u32 mmap, _size;

	if (cs > MCK6_MAX_CS)
		return -1;

	mmap = readl(mmap_addr[cs]);
	if (!(0x1 & mmap))
		return -1;

	_size = (mmap >> 16) & 0x1F;
	if (_size < 0x4) {
		*size = (384 << _size) * 1024 * 1024;
	} else if (_size < 7) {
		printf("Unknown dram size at CS%d!\n", cs);
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
		if (MCK6_sdram_base(i, &start))
			continue;
		if (start != gd->ram_size) {
			printf("error: unspported not-continous ram\n");
			hang();
		}
		if (MCK6_sdram_size(i, &size)) {
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
		if (!MCK6_sdram_base(i, &gd->bd->bi_dram[b].start))
			MCK6_sdram_size(i, &gd->bd->bi_dram[b++].size);
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

#ifdef CONFIG_TZ_HYPERVISOR
	if (gd->bd->bi_dram[0].size < CONFIG_TZ_HYPERVISOR_SIZE) {
		printf("Cannot meet requirement for TrustZone!\n");
	} else {
		gd->bd->bi_dram[0].start += CONFIG_TZ_HYPERVISOR_SIZE;
		gd->bd->bi_dram[0].size  -= CONFIG_TZ_HYPERVISOR_SIZE;
		gd->ram_size -= CONFIG_TZ_HYPERVISOR_SIZE;
	}
#endif
}
#endif /* CONFIG_SYS_BOARD_DRAM_INIT */

void MCK6_perf_cnt_clear(uint32_t cntmask)
{
	int i = 0;

	for (i = 0; i < MCK6_MAX_CNT; i++)
		if (cntmask & (1 << i))
			writel(0x0, (unsigned long)(MCK6_PC_CNT0 + i * 4));
}

/* by default, we config the perf counter to monitor busy-ratio/data-ratio */
void MCK6_perf_cnt_init(void)
{
	writel(0x10, MCK6_PC_CTRL);
	writel(0x9a9880, MCK6_PC_CFG);
}

void MCK6_perf_cnt_stop(void)
{
	writel(0x0, MCK6_PC_CFG);
}

void MCK6_perf_cnt_get(uint32_t *total, uint32_t *busy, uint32_t *data)
{
	*total = readl(MCK6_PC_CNT0);
	*busy = readl(MCK6_PC_CNT1);
	*data = readl(MCK6_PC_CNT2);
}
