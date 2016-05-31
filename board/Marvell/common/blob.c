/*
 * blob.c - TLV blob read/write
 *
 * (C) Copyright 2016
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Ethan Xia <xiasb@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <mmc.h>
#include "blob.h"

#define MMC_DEV_BLOB 2
#define MMC_BLOCK_SIZE 512
#define MMC_PART_USER 0
#define MMC_PART_BOOT0 1

static blob_data *blob_data_new(blob_type type, unsigned char *buf, unsigned int len)
{
	if (type < BLOB_TYPE_NULL || type > BLOB_TYPE_END) {
		printf("%s: unknown blob type\n", __func__);
		return NULL;
	}

	if (!buf || len <= 0) {
		printf("%s: invalid blob len\n", __func__);
		return NULL;
	}

	blob_data *blob = (blob_data *)malloc(sizeof(blob_data) + len);
	if (!blob) {
		printf("%s: malloc fail\n", __func__);
		return NULL;
	}

	memcpy(blob->data, buf, len);
	blob->type = type;
	blob->length = len;

	return blob;
}

static void blob_data_delete(blob_data *bd)
{
	if (!bd) {
		free(bd);
	}

	return;
}

int blob_write(unsigned int offset, blob_type type, unsigned char *buf, unsigned int len)
{
	blob_data *blob = NULL;
	blob = blob_data_new(type, buf, len);
	if (!blob) {
		printf("%s: blob_data_new failed\n", __func__);
		return BLOB_FAIL;
	}

	/* use eMMC BOOT0 to store blob */
	block_dev_desc_t *dev_desc = mmc_get_dev(MMC_DEV_BLOB);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("%s: invalid	MMC device\n", __func__);
		blob_data_delete(blob);
		return BLOB_FAIL;
	}

	mmc_switch_part(MMC_DEV_BLOB, MMC_PART_BOOT0);

	unsigned int total_size = sizeof(blob_data) + len;
	dev_desc->block_write(dev_desc->dev, offset / dev_desc->blksz,
		BLOCK_CNT(total_size, dev_desc), (char *)blob);

	mmc_switch_part(MMC_DEV_BLOB, MMC_PART_USER);

	blob_data_delete(blob);

	return BLOB_OK;
}

int blob_read(unsigned int offset, unsigned char **buf, unsigned int *len)
{
	char tmpbuf[MMC_BLOCK_SIZE] = {0};
	unsigned int total_size = 0;
	block_dev_desc_t *dev_desc = mmc_get_dev(MMC_DEV_BLOB);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("%s: invalid	MMC device\n", __func__);
		return BLOB_FAIL;
	}

	mmc_switch_part(MMC_DEV_BLOB, MMC_PART_BOOT0);

	dev_desc->block_read(dev_desc->dev, offset / dev_desc->blksz,
		1, tmpbuf);
	blob_data *bd = (blob_data *)tmpbuf;
	if (bd->type > BLOB_TYPE_END || bd->type < BLOB_TYPE_NULL) {
		printf("%s: invalid blob type\n", __func__);
		return BLOB_FAIL;
	}

	total_size = sizeof(blob_data) + bd->length;
	dev_desc->block_read(dev_desc->dev, offset / dev_desc->blksz,
		BLOCK_CNT(total_size, dev_desc), *buf);
	*len = total_size;

	mmc_switch_part(MMC_DEV_BLOB, MMC_PART_USER);

	return BLOB_OK;
}
