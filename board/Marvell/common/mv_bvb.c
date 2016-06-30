/*
 * mv_bvb.c Marvell Brillo Verified Boot 
 *
 * (C) Copyright 2016
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Zhengxi Hu <zhxihu@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <errno.h>
#include <common.h>
#include <mmc.h>
#include <linux/linux_string.h>
#include "mv_bvb.h"

#define DM_PARAM_HEADR		"dm=\""
#define DM_PARAM_SEP		'"'
#define PARTUUID_FOR_REPLACE	"$(ANDROID_SYSTEM_PARTUUID)"

void mv_bvb_key_dump(uint8_t *out_key_data, size_t out_key_length)
{
	int i;
	printf("BVB: Key Dump:\n");

	for (i =0; i < out_key_length; i++) {
		if ((i % 16) == 0)
			printf("\n");

		printf("0x%x ", *out_key_data++);
	}

	printf("\n");
}

static void header_dump(struct BvbBootImageHeader *bhdr)
{
	printf("\n===== BvBHeader Dump ===== \n");
	printf("BvBHeader: magic %.4s\n", bhdr->magic);
	printf("BvBHeader: version %d:%d\n",
			bhdr->header_version_major, bhdr->header_version_minor);
	printf("BvBHeader: authentication_data_block_size %d\n",
			bhdr->authentication_data_block_size);
	printf("BvBHeader: auxilary_data_block_size %d\n",
			bhdr->auxilary_data_block_size);
	printf("BvBHeader: auxilary_data_block_size %d\n",
			bhdr->payload_data_block_size);
	printf("BvBHeader: algorithm_type %d\n",
			bhdr->algorithm_type);
	printf("BvBHeader: rollback_index %d\n",
			bhdr->rollback_index);
	printf("BvBHeader: kernel_offset 0x%x, kernel_size %d\n",
			bhdr->kernel_offset, bhdr->kernel_size);
	printf("BvBHeader: initrd_offset 0x%x, initrd_size %d\n",
			bhdr->initrd_offset, bhdr->initrd_size);
}

void mv_bvb_load_header(struct BvbBootImageHeader *bhdr,
		unsigned long base_emmc_addr)
{
	char cmd[128];
	struct BvbBootImageHeader h;
	int blkcnt = BVB_BOOT_IMAGE_HEADER_SIZE / 512;

	sprintf(cmd, "%s; mmc read %p %lx %x", CONFIG_MMC_BOOT_DEV,
			&h, base_emmc_addr/512, blkcnt);
	run_command(cmd, 0);

	bvb_boot_image_header_to_host_byte_order(
				(const BvbBootImageHeader *)&h, bhdr);

#ifdef CONFIG_MV_BVB_DEBUG
	header_dump(bhdr);
#endif

	return;
}

void mv_bvb_load_image(unsigned long load_addr, unsigned long base_emmc_addr,
						size_t image_size)
{
	char cmd[128];

	block_dev_desc_t *dev_desc = mmc_get_dev(CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("BVB: %s: invalid MMC device\n", __func__);
		return;
	}

	sprintf(cmd, "%s; mmc read %p %lx %x", CONFIG_MMC_BOOT_DEV,
			load_addr, base_emmc_addr/512, BLOCK_CNT(image_size, dev_desc));
	run_command(cmd, 0);

	return;
}

int mv_bvb_power_on_wp_set(int mmc_dev)
{
	struct mmc *mmc;
	int err;

	mmc = find_mmc_device(mmc_dev);
	if (!mmc) {
		printf("BVB: No mmc device at %x\n", mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		printf("BVB: Unable to initialize mmc\n");
		return -1;
	}

	err = mmc_boot_power_on_write_protect_set(mmc);
	if (err) {
		printf("BVB: Error %d applying write protect\n", err);
		return -1;
	} else {
		/* printf("BOOT partition Power-on write protect enabled\n"); */
		return 0;
	}

	return 0;
}

int mv_bvb_power_on_wp_get(int mmc_dev)
{
	struct mmc *mmc;
	int vaule;

	mmc = find_mmc_device(mmc_dev);
	if (!mmc) {
		printf("BVB: No mmc device at %x\n", mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		printf("BVB: Unable to initialize mmc\n");
		return -1;
	}

	vaule = mmc_boot_power_on_write_protect_get(mmc);
	if (vaule) {
		printf("BVB: BOOT partition Power-on write protect enabled\n");
		return MV_BVB_PWR_WP_EN;
	} else {
		printf("BVB: BOOT partition Power-on write protect disabled\n");
		return MV_BVB_PWR_WP_DIS;
	}

	return MV_BVB_PWR_WP_DIS;
}

BvbVerifyResult mv_bvb_verify(const uint8_t *data, size_t length,
		const uint8_t **out_key_data, size_t *out_key_length)
{
	return bvb_verify_boot_image(data, length, out_key_data, out_key_length);
}

static char *str_replace(char *target, const char *search, const char *replace) {
	char buf[2048] = { 0 };
	const char *t = target;
	char *pos = &buf[0];
	const char *p;
	size_t s_len = strlen(search);
	size_t r_len = strlen(replace);
	size_t t_len = strlen(target);

	if (t_len > sizeof(buf) || t_len - s_len + r_len > sizeof(buf)) {
		printf("BVB: str_replace: Too long target string for replacing.\n");
		return NULL;
	}

	while (1) {
		p = strstr(t, search);
		if (p == NULL) {
			strcpy(pos, t);
			break;
		}

		/* copy substring before search pattern */
		memcpy(pos, t, p - t);
		pos += p - t;

		/* copy replace string */
		memcpy(pos, replace, r_len);
		pos += r_len;

		/* move target to unparsed substring */
		t = p + s_len;
	}

	strcpy(target, buf);

	return target;
}

static int get_bootable_slot_part_info(size_t bvb_boot_slot,
		disk_partition_t *part_info)
{
	block_dev_desc_t *mmc_dev;
	struct mmc *mmc;
	int part, pnum;

	mmc = find_mmc_device(2);
	if (!mmc) {
		printf("BVB: get_bootable_slot_part_info: no mmc device\n");
		return -1;
	}
	mmc_init(mmc);
	mmc_dev = mmc_get_dev(2);
	if (mmc_dev == NULL || mmc_dev->type == DEV_TYPE_UNKNOWN)
		return -1;

	pnum = get_partition_num(mmc_dev);
	for (part = 1; part <= pnum; part++) {
		if (get_partition_info(mmc_dev, part, part_info) == 0)
			if (!strcmp((const char *)part_info->name,
				bootctrl_get_boot_slot_system(bvb_boot_slot)))
				break;
	}

	if (part == pnum + 1) {
		printf("BVB: get_bootable_slot_part_info: Can not find slot %s\n",
				bootctrl_get_boot_slot_system(bvb_boot_slot));
		return -1;
	}

	return 0;
}

int mv_bvb_append_dm_param(size_t bvb_boot_slot, uint8_t *bvb_cmdline,
		uint8_t *kernel_cmdline, size_t max_cmdline_size)
{
	char *dm_param_start;
	char *dm_param_end;
	disk_partition_t part_info;

	if (!bvb_cmdline || !kernel_cmdline)
		return -1;

	if (bvb_boot_slot > 1) {
		printf("BVB: append_dm_param: Error boot slot %d\n", bvb_boot_slot);
		return -1;
	}

	dm_param_start = skip_spaces(strdup(kernel_cmdline));
	/* There is only one DM parameter with one pair of double quotes */
	dm_param_end = strrchr(dm_param_start, DM_PARAM_SEP);

	if (!dm_param_end) {
		printf("BVB: append_dm_param: Parse parameter ending failed\n");
		free(dm_param_start);
		return -1;
	}
	*(dm_param_end + 1) = '\0';

	if ((strlen(dm_param_start) +
			strlen(bvb_cmdline) + 2) > max_cmdline_size) {
		printf("BVB: append_dm_param: Too long cmdline for appending\n");
		free(dm_param_start);
		return -1;
	}

	/* Copy the "DM=" parameter substring */
	strcat(bvb_cmdline, " ");
	strlcpy(bvb_cmdline + strlen(bvb_cmdline), dm_param_start,
			max_cmdline_size);

	/* Append the UUID parameter */
	memset(&part_info, 0, sizeof(disk_partition_t));
	if (get_bootable_slot_part_info(bvb_boot_slot, &part_info)) {
		printf("BVB: append_dm_param: get bootable slot UUID error\n");
	} else {
		if (strlen(part_info.uuid) > 0)
			str_replace(bvb_cmdline, PARTUUID_FOR_REPLACE, part_info.uuid);
	}

	free(dm_param_start);
	return 0;
}
