/*
 * recovery.c PXA9XX recovery support
 *
 * (C) Copyright 2013
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Xiaofan Tian <tianxf@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <common.h>
#include <mmc.h>
#include <mv_recovery.h>

#ifdef CONFIG_PXA1928
#define MMC_DEV_RECOVERY	0
#else
#define MMC_DEV_RECOVERY	2
#endif

/* block address in misc partition */
#define MISC_BLK_ADDR		0x110000
#define R_DTIM_BLK_ADDR		(MISC_BLK_ADDR + 0x0003)
#define R_UBOOT_BLK_ADDR	(MISC_BLK_ADDR + 0x0023)
#define R_RECOVERY_BLK_ADDR	(MISC_BLK_ADDR + 0x07FF)
#define MISC_END_BLK_ADDR	(MISC_BLK_ADDR + 0x4FFF)

#define RECOVERY_END_BLK_ADDR	(CONFIG_UBOOT_PA_START/512 - 0x1)

/* burn address in emmc */
#define R_UBOOT_BURN_ADDR	0x2000
#define R_UBOOT_BURN_SIZE	0x0800

#define R_RECOVERY_BURN_ADDR	0x2800
#define R_RECOVERY_BURN_SIZE	0x4800

#define R_DTIM_BURN_ADDR	0x1400
#define R_DTIM_BURN_SIZE	0x0400

#define BUFFER_MAX		512

static void recovery_flow_update(struct mmc *mmc, struct misc_end *me)
{
	char buffer[BUFFER_MAX * 512];
	unsigned long blk_addr[] = {R_UBOOT_BLK_ADDR, R_RECOVERY_BLK_ADDR, R_DTIM_BLK_ADDR};
	unsigned long burn_addr[] = {R_UBOOT_BURN_ADDR, R_RECOVERY_BURN_ADDR, R_DTIM_BURN_ADDR};
	unsigned long burn_size[] = {R_UBOOT_BURN_SIZE, R_RECOVERY_BURN_SIZE, R_DTIM_BURN_SIZE};
	int i, size, total;

	printf("=========================================\n");
	printf("==========Now update Recovery flow=======\n");

	for (i = 0; i < ARRAY_SIZE(blk_addr); i++) {
		if (me->idx & (1 << i)) {
			total = 0;
			while (total < burn_size[i]) {
				size = min(burn_size[i] - total, BUFFER_MAX);
				mmc->block_dev.block_read(MMC_DEV_RECOVERY,
					blk_addr[i] + total, size, buffer);
				mmc->block_dev.block_write(MMC_DEV_RECOVERY,
					burn_addr[i] + total, size, buffer);
				total += size;
			}
		}
	}

	printf("=========================================\n");

	/* clear flag */
	strcpy(me->command, "reserved");
	me->idx = 0;
	mmc->block_dev.block_write(MMC_DEV_RECOVERY, MISC_END_BLK_ADDR,
				   1, (char *)&me);

	return;
}

int recovery(int primary_valid, int recovery_valid, int magic_key,
		struct recovery_reg_funcs *funcs)
{
	static struct mmc *mmc;
	static struct misc_end me;
	uchar rtc_flag = 0x0;
	int boot_recovery, update_recovery;

	mmc = find_mmc_device(MMC_DEV_RECOVERY);
	mmc_init(mmc);
	mmc_switch_part(MMC_DEV_RECOVERY, 0);

	mmc->block_dev.block_read(MMC_DEV_RECOVERY, MISC_BLK_ADDR,
				  1, (char *)&me);
	boot_recovery = !strcmp(me.command, "boot-recovery");

	mmc->block_dev.block_read(MMC_DEV_RECOVERY, MISC_END_BLK_ADDR,
				  1, (char *)&me);
	update_recovery = !strcmp(me.command, "update_recovery");

	if (funcs && funcs->recovery_reg_read)
		rtc_flag = funcs->recovery_reg_read();

	if (!primary_valid && !recovery_valid) {
		printf("Error: no valid images\n");
		while (1)
			;
	} else if (!primary_valid && recovery_valid) {
		boot_recovery = 1;	/* Always boot recovery kernel */
		if (update_recovery) {
			/*
			 * Don't update recovery when primary is broken,
			 * it is dangrous
			 */
			strcpy(me.command, "reserved");
			me.idx = 0;
			mmc->block_dev.block_write(MMC_DEV_RECOVERY,
				MISC_END_BLK_ADDR, 1, (char *)&me);
			update_recovery = 0;
		}
	} else if (primary_valid && !recovery_valid) {
		if (boot_recovery) {
			/* recovery invalid, ignore boot_recovery flag */
			strcpy(me.command, "reserved");
			mmc->block_dev.block_write(MMC_DEV_RECOVERY,
				MISC_BLK_ADDR, 1, (char *)&me);
		}

		boot_recovery = 0;
	} else {
		if (update_recovery) {
			if (boot_recovery) {
				strcpy(me.command, "reserved");
				mmc->block_dev.block_write(MMC_DEV_RECOVERY,
					MISC_BLK_ADDR, 1, (char *)&me);
				boot_recovery = 0;
			}
		} else if ((rtc_flag & 0x01) || magic_key) {
			boot_recovery = 1;
		}
	}

	if ((rtc_flag & 0x01) && funcs && funcs->recovery_reg_write) {
		rtc_flag &= 0xFE;
		funcs->recovery_reg_write(rtc_flag);
	}

	if (update_recovery)
		recovery_flow_update(mmc, &me);

	return boot_recovery;
}

u32 get_sku_max_freq(void)
{
	static struct mmc *mmc;
	static struct misc_end me;
	u32 max_preferred_freq = 0;

	mmc = find_mmc_device(MMC_DEV_RECOVERY);
	mmc_init(mmc);
	mmc_switch_part(MMC_DEV_RECOVERY, 0);

	mmc->block_dev.block_read(MMC_DEV_RECOVERY, MISC_END_BLK_ADDR,
				  1, (char *)&me);
	if (!strcmp(me.command, "sku-setting")) {
		max_preferred_freq = me.idx;
		strcpy(me.command, "reserved");
		me.idx = 0;
		mmc->block_dev.block_write(MMC_DEV_RECOVERY,
			MISC_END_BLK_ADDR, 1, (char *)&me);
	}
	printf("max_preferred_freq %u!\n", max_preferred_freq);
	return max_preferred_freq;
}

u32 get_sku_max_setting(unsigned int type)
{
	static struct mmc *mmc;
	static struct misc_end me;
	u32 max_preferred_freq = 0;

	mmc = find_mmc_device(MMC_DEV_RECOVERY);
	mmc_init(mmc);
	mmc_switch_part(MMC_DEV_RECOVERY, 0);

	mmc->block_dev.block_read(MMC_DEV_RECOVERY, MISC_END_BLK_ADDR, 1, (char *)&me);
	if (!strcmp(me.command, "sku-setting")) {
		switch (type) {
		case SKU_CPU_MAX_PREFER:
			max_preferred_freq = me.idx;
			me.idx = 0;
			printf("cpu max preferred freq %u!\n", max_preferred_freq);
			break;

		case SKU_DDR_MAX_PREFER:
			max_preferred_freq = me.ddr_max;
			me.ddr_max = 0;
			printf("ddr max preferred freq %u!\n", max_preferred_freq);
			break;

		case SKU_GC3D_MAX_PREFER:
			max_preferred_freq = me.gc3d_max;
			me.gc3d_max = 0;
			printf("gc3d max preferred freq %u!\n", max_preferred_freq);
			break;

		case SKU_GC2D_MAX_PREFER:
			max_preferred_freq = me.gc2d_max;
			me.gc2d_max = 0;
			printf("gc2d max preferred freq %u!\n", max_preferred_freq);
			break;

		default:
			break;
		}
		mmc->block_dev.block_write(MMC_DEV_RECOVERY,
			MISC_END_BLK_ADDR, 1, (char *)&me);
	}
	return max_preferred_freq;
}

u32 root_detection(void)
{
	static struct mmc *mmc;
	static struct misc_end me;
	u32 root_mode = 0;

	mmc = find_mmc_device(MMC_DEV_RECOVERY);
	mmc_init(mmc);
	mmc_switch_part(MMC_DEV_RECOVERY, 0);

	mmc->block_dev.block_read(MMC_DEV_RECOVERY,
		RECOVERY_END_BLK_ADDR, 1, (char *)&me);

	if (!strncmp(me.command, "root", 4)) {
		if (!strcmp(me.command, "root=1"))
			root_mode = 1;
		else
			root_mode = 0;
	}

	return root_mode;
}

#ifdef CONFIG_ENV_RE_OFFSET
void set_cpu_arch(u32 cpu_arch)
{
	static struct mmc *mmc;
	static struct misc_end me;

	mmc = find_mmc_device(MMC_DEV_RECOVERY);
	mmc_init(mmc);
	mmc_switch_part(MMC_DEV_RECOVERY, 0);

	mmc->block_dev.block_read(MMC_DEV_RECOVERY,
		CONFIG_ENV_RE_OFFSET/512, 1, (char *)&me);

	if (cpu_arch) {
		me.cpu_arch = cpu_arch;
		mmc->block_dev.block_write(MMC_DEV_RECOVERY,
			CONFIG_ENV_RE_OFFSET/512, 1, (char *)&me);
	}
}

u32 get_cpu_arch(void)
{
	static struct mmc *mmc;
	static struct misc_end me;

	mmc = find_mmc_device(MMC_DEV_RECOVERY);
	mmc_init(mmc);
	mmc_switch_part(MMC_DEV_RECOVERY, 0);

	mmc->block_dev.block_read(MMC_DEV_RECOVERY,
		CONFIG_ENV_RE_OFFSET/512, 1, (char *)&me);

	return me.cpu_arch;
}
#endif
