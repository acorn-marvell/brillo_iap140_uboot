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
#include "mv_bvb.h"


void mv_bvb_key_dump(uint8_t *out_key_data, size_t out_key_length)
{
	int i;
	printf("Key Dump:\n");

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
		printf("%s: invalid MMC device\n", __func__);
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
		printf("No mmc device at %x\n", mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		printf("Unable to initialize mmc\n");
		return -1;
	}

	err = mmc_boot_power_on_write_protect_set(mmc);
	if (err) {
		printf("Error %d applying write protect\n", err);
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
		printf("No mmc device at %x\n", mmc_dev);
		return -1;
	}

	if (mmc_init(mmc)) {
		printf("Unable to initialize mmc\n");
		return -1;
	}

	vaule = mmc_boot_power_on_write_protect_get(mmc);
	if (vaule) {
		printf("BOOT partition Power-on write protect enabled\n");
		return MV_BVB_PWR_WP_EN;
	} else {
		printf("BOOT partition Power-on write protect disabled\n");
		return MV_BVB_PWR_WP_DIS;
	}

	return MV_BVB_PWR_WP_DIS;
}

BvbVerifyResult mv_bvb_verify(const uint8_t *data, size_t length,
		const uint8_t **out_key_data, size_t *out_key_length)
{
	return bvb_verify_boot_image(data, length, out_key_data, out_key_length);
}
