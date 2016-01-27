/*
 * Copyright 2014 Broadcom Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fb_mmc.h>
#include <part.h>
#include <aboot.h>
#include <sparse_format.h>
#include <mmc.h>

/* The 64 defined bytes plus the '\0' */
#define RESPONSE_LEN	(64 + 1)

static char *response_str;

void fastboot_fail(const char *s)
{
	strncpy(response_str, "FAIL", 4);
	strncat(response_str, s, RESPONSE_LEN - 4 - 1);
}

void fastboot_okay(const char *s)
{
	strncpy(response_str, "OKAY", 4);
	strncat(response_str, s, RESPONSE_LEN - 4 - 1);
}

static void write_raw_image(block_dev_desc_t *dev_desc, disk_partition_t *info,
		const char *part_name, void *buffer,
		unsigned int download_bytes)
{
	lbaint_t blkcnt;
	lbaint_t blks;

	/* determine number of blocks to write */
	blkcnt = ((download_bytes + (info->blksz - 1)) & ~(info->blksz - 1));
	blkcnt = blkcnt / info->blksz;

	if (blkcnt > info->size) {
		error("too large for partition: '%s'\n", part_name);
		fastboot_fail("too large for partition");
		return;
	}

	puts("Flashing Raw Image\n");

	blks = dev_desc->block_write(dev_desc->dev, info->start, blkcnt,
				     buffer);
	if (blks != blkcnt) {
		error("failed writing to device %d\n", dev_desc->dev);
		fastboot_fail("failed writing to device");
		return;
	}

	printf("........ wrote " LBAFU " bytes to '%s'\n", blkcnt * info->blksz,
	       part_name);
	fastboot_okay("");
}

static void erase_image(block_dev_desc_t *dev_desc, disk_partition_t *info,
			const char *part_name)
{
	lbaint_t blks;

	puts("Erasing Image\n");

	blks = dev_desc->block_erase(dev_desc->dev, info->start, info->size);
	if (blks != info->size) {
		error("failed erasing partition %s to device %d\n",
		      part_name, dev_desc->dev);
		fastboot_fail("failed erase device");
		return;
	}

	printf("........ erased " LBAFU " block with '%s'\n",
	       info->size, part_name);
	fastboot_okay("");
}

void fb_mmc_flash_write(const char *cmd, void *download_buffer,
			unsigned int download_bytes, char *response)
{
	int ret;
	block_dev_desc_t *dev_desc;
	disk_partition_t info;

	/* initialize the response buffer */
	response_str = response;

	legacy_mbr *mbr;
	gpt_header *gpt_h;
	gpt_entry *gpt_e;

	int expected;

	dev_desc = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		error("invalid mmc device\n");
		fastboot_fail("invalid mmc device");
		return;
	}

#if 0
	char cmd_buf[64] = {0};

	ulong mmc_part;
	ulong mmc_flash_start;

	if (!strncmp("to:", cmd, strlen("to:"))) {
		strsep(&cmd, ":");
		if (!cmd) {
			error("missing variable\n");
			fastboot_fail("missing var");
			return;
		}
		mmc_part = simple_strtoul(cmd, NULL, 10);
		if(mmc_part > 16){
			error("Part # is too large\n");
			fastboot_fail("Part # is too large");
			return;
		}
		
		strsep(&cmd, ":");
		if (!cmd) {
			error("missing variable\n");
			fastboot_fail("missing var");
			return;
		}
		mmc_flash_start = simple_strtoul(cmd, NULL, 16);
		
		if(mmc_flash_start != PAD_TO_BLOCKSIZE(mmc_flash_start, dev_desc)){
			error("Offset must start from block size boudry\n");
			fastboot_fail("Offset must start from block size boudry");
			return;
		}

		sprintf(cmd_buf, "mmc dev %d %d", CONFIG_FASTBOOT_FLASH_MMC_DEV, mmc_part);
		run_command(cmd_buf, 0);

		if(dev_desc->lba != 0){
			if (dev_desc->block_write(dev_desc->dev, mmc_flash_start/dev_desc->blksz, BLOCK_CNT(download_bytes, dev_desc), download_buffer) != BLOCK_CNT(download_bytes, dev_desc)){
				error("flash data failed:\n");
				fastboot_fail("flash data failed:");
			}else{
				fastboot_okay("");
			}
		}else{
			error("Invalid mmc part\n");
			fastboot_fail("Invalid mmc part");
		}

		/* switch back to main part */
		sprintf(cmd_buf, "mmc dev %d 0", CONFIG_FASTBOOT_FLASH_MMC_DEV);
		run_command(cmd_buf, 0);
	}else 
#endif
	if (!strncmp("partition", cmd, strlen("partition"))) {
		/* do sanity check of the download data */
		expected = sizeof(legacy_mbr)+PAD_TO_BLOCKSIZE(sizeof(gpt_header), dev_desc)+PAD_TO_BLOCKSIZE(GPT_ENTRY_NUMBERS
					       * sizeof(gpt_entry), dev_desc);

		if(expected != download_bytes){
			error("wrong size for download data, expected: %d\n", expected);
			fastboot_fail("wrong size for download data");
			return;
		}

		mbr = download_buffer;
		gpt_h = (void *)mbr + sizeof(legacy_mbr);
		gpt_e = (void *)gpt_h+PAD_TO_BLOCKSIZE(sizeof(gpt_header), dev_desc);

		/* Check the MBR signature */
		if (le16_to_cpu(mbr->signature) != MSDOS_MBR_SIGNATURE){
			error("MBR signature is wrong:"
				"0x%X != 0x%X\n",
				le16_to_cpu(mbr->signature),
				MSDOS_MBR_SIGNATURE);
			fastboot_fail("wrong data");
			return;
		}

		/* Check the GPT header signature */
		if (le64_to_cpu(gpt_h->signature) != GPT_HEADER_SIGNATURE) {
			error("GUID Partition Table Header signature is wrong:"
				"0x%llX != 0x%llX\n",
				le64_to_cpu(gpt_h->signature),
				GPT_HEADER_SIGNATURE);
			fastboot_fail("wrong data");
			return;
		}
	
		const int pte_blk_cnt = BLOCK_CNT((gpt_h->num_partition_entries
					   * sizeof(gpt_entry)), dev_desc);
		u32 calc_crc32;
		u64 val;

		printf("max lba: %x\n", (u32) dev_desc->lba);

		/* Write the Legacy MBR */
		if (dev_desc->block_write(dev_desc->dev, 0, 1, mbr) != 1)
			goto err;

		/* Write the First GPT to the block right after the Legacy MBR */
		if (dev_desc->block_write(dev_desc->dev, 1, 1, gpt_h) != 1)
			goto err;

		if (dev_desc->block_write(dev_desc->dev, 2, pte_blk_cnt, gpt_e)
		    != pte_blk_cnt)
			goto err;

		/* recalculate the values for the Second GPT Header */
		val = le64_to_cpu(gpt_h->my_lba);
		gpt_h->my_lba = gpt_h->alternate_lba;
		gpt_h->alternate_lba = cpu_to_le64(val);
		gpt_h->header_crc32 = 0;

		calc_crc32 = crc32(0, (const unsigned char *)gpt_h,
				      le32_to_cpu(gpt_h->header_size));
		gpt_h->header_crc32 = cpu_to_le32(calc_crc32);

		if (dev_desc->block_write(dev_desc->dev,
					  le32_to_cpu(gpt_h->last_usable_lba + 1),
					  pte_blk_cnt, gpt_e) != pte_blk_cnt)
			goto err;

		if (dev_desc->block_write(dev_desc->dev,
					  le32_to_cpu(gpt_h->my_lba), 1, gpt_h) != 1)
			goto err;

		printf("GPT successfully written to block device!\n");
		fastboot_okay("");
		return;

	 err:
		error("flash partition data failed: '%s'\n", cmd);
		fastboot_fail("cannot flash partition");
		return;

	}else{

		ret = get_partition_info_efi_by_name(dev_desc, cmd, &info);
		if (ret) {
			error("cannot find partition: '%s'\n", cmd);
			fastboot_fail("cannot find partition");
			return;
		}

		if (is_sparse_image(download_buffer))
			write_sparse_image(dev_desc, &info, cmd, download_buffer,
					   download_bytes);
		else
			write_raw_image(dev_desc, &info, cmd, download_buffer,
					download_bytes);
	}
}

void fb_mmc_erase(const char *cmd, char *response)
{
	int ret;
	block_dev_desc_t *dev_desc;
	disk_partition_t info;

	/* initialize the response buffer */
	response_str = response;

	dev_desc = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		error("invalid mmc device\n");
		fastboot_fail("invalid mmc device");
		return;
	}

	ret = get_partition_info_efi_by_name(dev_desc, cmd, &info);
	if (ret) {
		error("cannot find partition: '%s'\n", cmd);
		fastboot_fail("cannot find partition");
		return;
	}

	erase_image(dev_desc, &info, cmd);
}

void print_partion_info(void)
{
	block_dev_desc_t *dev_desc;
	disk_partition_t info;
	int pnum = 0;
	int i;
	unsigned long long start;
	unsigned long long size;

	dev_desc = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		error("invalid mmc device\n");
		fastboot_fail("invalid mmc device");
		return;
	}
	pnum = get_partition_num(dev_desc);
	for (i = 1; i <= pnum; i++) {
		if (get_partition_info(dev_desc, i, &info))
			break;
		start = (unsigned long long)info.start * info.blksz;
		size = (unsigned long long)info.size * info.blksz;
		printf("part %3d::%12s\t start 0x%08llx, size 0x%08llx\n",
		       i, info.name, start, size);
	}
}
