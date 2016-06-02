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
#ifdef CONFIG_FASTBOOT_BVB
#include <bvb_mmc.h>
#endif

/* The 64 defined bytes plus the '\0' */
#define RESPONSE_LEN	(64 + 1)

static char *response_str;

void fastboot_fail(const char *s)
{
	strncpy(response_str, "FAIL\0", 5);
	strncat(response_str, s, RESPONSE_LEN - 4 - 1);
}

void fastboot_okay(const char *s)
{
	strncpy(response_str, "OKAY\0", 5);
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

#ifdef CONFIG_FASTBOOT_BVB
void fb_bvb_lock_device(char *response)
{
	int ret;

	/* initialize the response buffer */
	response_str = response;
	ret = bvb_lock_device();
	if (ret == BVB_ERR_WRITE_SAME)
	{
		printf("already locked\n");
		fastboot_fail("already locked");
	} else if (ret) {
		error("lock device failed\n");
		fastboot_fail("lock device failed");
	} else {
		printf("lock device succeed\n");
		fastboot_okay("");
	}
}

void fb_bvb_unlock_device(char *response)
{
	int ret;

	/* initialize the response buffer */
	response_str = response;
	ret = bvb_unlock_device();
	if (ret == BVB_ERR_WRITE_SAME)
	{
		printf("already unlocked\n");
		fastboot_fail("already unlocked");
	} else if (ret) {
		error("unlock device failed\n");
		fastboot_fail("unlock device failed");
	} else {
		printf("unlock device succeed\n");
		fastboot_okay("");
	}
}
#endif

void fb_mmc_flash_write(const char *cmd, void *download_buffer,
			unsigned int download_bytes, char *response)
{
	int ret;
        int expected;
        int pte_blk_cnt;

	block_dev_desc_t *dev_desc;
	disk_partition_t info;

	/* initialize the response buffer */
	response_str = response;

	legacy_mbr *mbr;
	gpt_header *primary_gpt_h;
        gpt_entry *second_gpt_e;

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
                /*do sanity check of the downloader data */
                expected = sizeof(legacy_mbr) + 2 * ((PAD_TO_BLOCKSIZE(sizeof(gpt_header), dev_desc) + PAD_TO_BLOCKSIZE(GPT_ENTRY_NUMBERS
                                               * sizeof(gpt_entry), dev_desc)));
                
		if(expected != download_bytes){
			error("wrong size for download data, expected: %d\n", expected);
			fastboot_fail("wrong size for download data");
			return;
		}

		mbr = download_buffer;
		primary_gpt_h = (void *)mbr + sizeof(legacy_mbr);
                pte_blk_cnt = BLOCK_CNT((primary_gpt_h->num_partition_entries * sizeof(gpt_entry)), dev_desc);
                second_gpt_e = primary_gpt_h + PAD_TO_BLOCKSIZE(sizeof(gpt_header), dev_desc)
                                             + PAD_TO_BLOCKSIZE(GPT_ENTRY_NUMBERS * sizeof(gpt_entry), dev_desc);

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
		if (le64_to_cpu(primary_gpt_h->signature) != GPT_HEADER_SIGNATURE) {
			error("GUID Partition Table Header signature is wrong:"
				"0x%llX != 0x%llX\n",
				le64_to_cpu(primary_gpt_h->signature),
				GPT_HEADER_SIGNATURE);
			fastboot_fail("wrong data");
			return;
		}

		/* Write the Legacy MBR */
		if (dev_desc->block_write(dev_desc->dev, 0, 1, mbr) != 1){
                        printf("Write mbr failed!\n");
                        goto err;
                }

		/* Write the First GPT to the block right after the Legacy MBR */
		if (dev_desc->block_write(dev_desc->dev, 1, pte_blk_cnt + 1, primary_gpt_h) != pte_blk_cnt + 1){
                        printf("Write primary gpt failed!\n");
                        goto err;
                }
               
                /*Write the Second GPT at the end of the block*/
                lbaint_t second_gpt_offset = le32_to_cpu(primary_gpt_h->last_usable_lba + 1);
                if(dev_desc->block_write(dev_desc->dev, second_gpt_offset,
                                         pte_blk_cnt + 1, second_gpt_e) != pte_blk_cnt + 1){
                       printf("write second gpt  failed!\n");
                       goto err;
                }
   
#if 0
		/* do sanity check of the download data */
		expected = sizeof(legacy_mbr)+ 1 * (PAD_TO_BLOCKSIZE(sizeof(gpt_header), dev_desc)+PAD_TO_BLOCKSIZE(GPT_ENTRY_NUMBERS
					       * sizeof(gpt_entry), dev_desc));
		/*if(expected != download_bytes){
			error("wrong size for download data, expected: %d\n", expected);
			fastboot_fail("wrong size for download data");
			return;
		}*/
                printf("legacy_mbr is %d, gpt_header is %d, gpt_entry is %d\n",sizeof(legacy_mbr),PAD_TO_BLOCKSIZE(sizeof(gpt_header), 
                                    dev_desc),PAD_TO_BLOCKSIZE(GPT_ENTRY_NUMBERS * sizeof(gpt_entry), dev_desc));
                 
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

                printf("last_usable_lba is %d, my_lba is %d, pte_blk_cnt is %d\n",le32_to_cpu(gpt_h->last_usable_lba + 1),le32_to_cpu(gpt_h->my_lba),
                        pte_blk_cnt);
		/* Write the Legacy MBR */
		if (dev_desc->block_write(dev_desc->dev, 0, 1, mbr) != 1){
			printf("Write mbr failed!\n");
			goto err;
                }
                printf("last_usable_lba is %d, my_lba is %d, pte_blk_cnt is %d\n",le32_to_cpu(gpt_h->last_usable_lba + 1),le32_to_cpu(gpt_h->my_lba),
                        pte_blk_cnt);

		/* Write the First GPT to the block right after the Legacy MBR */
		if (dev_desc->block_write(dev_desc->dev, 1, 1, gpt_h) != 1){
                        printf("Write gpt header failed!\n");
			goto err;
                }
                printf("last_usable_lba is %d, my_lba is %d, pte_blk_cnt is %d\n",le32_to_cpu(gpt_h->last_usable_lba + 1),le32_to_cpu(gpt_h->my_lba),
                        pte_blk_cnt);

		if (dev_desc->block_write(dev_desc->dev, 2, pte_blk_cnt, gpt_e)
		    != pte_blk_cnt){
                        printf("Write gpt_e failed!\n");
			goto err;
                 }
                printf("last_usable_lba is %d, my_lba is %d, pte_blk_cnt is %d\n",le32_to_cpu(gpt_h->last_usable_lba + 1),le32_to_cpu(gpt_h->my_lba),
                        pte_blk_cnt);

		/* recalculate the values for the Second GPT Header */
                printf("last_usable_lba is %d, my_lba is %d, pte_blk_cnt is %d\n",le32_to_cpu(gpt_h->last_usable_lba + 1),le32_to_cpu(gpt_h->my_lba),
                        pte_blk_cnt);
		val = le64_to_cpu(gpt_h->my_lba);
		gpt_h->my_lba = gpt_h->alternate_lba;
		gpt_h->alternate_lba = cpu_to_le64(val);
		gpt_h->header_crc32 = 0;

		calc_crc32 = crc32(0, (const unsigned char *)gpt_h,
				      le32_to_cpu(gpt_h->header_size));
		gpt_h->header_crc32 = cpu_to_le32(calc_crc32);

                printf("last_usable_lba is %d, my_lba is %d, pte_blk_cnt is %d\n",le32_to_cpu(gpt_h->last_usable_lba + 1),le32_to_cpu(gpt_h->my_lba),
                        pte_blk_cnt);
 
		if (dev_desc->block_write(dev_desc->dev,
					  le32_to_cpu(gpt_h->last_usable_lba + 1),
					  pte_blk_cnt, gpt_e) != pte_blk_cnt){
			printf("Write second gpt_e failed!\n");
                        goto err;
                }

		if (dev_desc->block_write(dev_desc->dev,
					  le32_to_cpu(gpt_h->my_lba), 1, gpt_h) != 1){
                        printf("Write second gpt_h failed!\n");
			goto err;
                }
#endif
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
#ifdef CONFIG_FASTBOOT_BVB
			if (!strncmp(cmd, "bvb_devkey", strlen("bvb_devkey"))) {
				ret = bvb_write_key(download_buffer,download_bytes);
				if (ret == BVB_ERR_WRITE_SAME) {
					printf("same bvb_devkey in mmc, no need to flash\n");
					fastboot_fail("same bvb_devkey on board");
				} else if (ret) {
					error("bvb_devkey flash failed\n");
					fastboot_fail("bvb_devkey write failed");
				} else {
					printf("bvb_devkey flashed\n");
					fastboot_okay("");
				}
				return;
			}
#endif
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
#ifdef CONFIG_FASTBOOT_BVB
		if (!strncmp(cmd, "bvb_devkey", strlen("bvb_devkey"))) {
			ret = bvb_erase_key();
			if (ret) {
				error("bvb_devkey erase failed\n");
				fastboot_fail("erase key failed");
			} else {
				printf("bvb_devkey erased\n");
				fastboot_okay("");
			}
			return;
		}
#endif
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
