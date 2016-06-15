/*
 * mmc_blob.c mmc blob operation
 *
 * (C) Copyright 2016
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Yonghui Wan <yhwan@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <common.h>
#include <mmc_blob.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>

/* #define MMC_BLOB_DEBUG */

#ifndef MMC_BLOB_DEBUG
#define pr_op(a,b,c,d)
#endif

void mmc_blob_desc_init(mmc_blob_desc_t *desc, u_char mmc_dev,
		u_char mmc_part, u_int offset, u_int max_size, u_int magic,
		mmc_blob_type type)
{
	desc->mmc_dev = mmc_dev;
	desc->mmc_part = mmc_part;
	desc->offset = offset;
	desc->max_size = max_size;
	desc->magic = magic;
	desc->type = type;
}

mmc_blob_data_t* mmc_blob_data_new(u_char *buf, u_int len)
{
	mmc_blob_data_t *blob_data;
	int toal_size;
	toal_size = sizeof(mmc_blob_data_t) + len;

	blob_data = (mmc_blob_data_t *)malloc(toal_size);
	if (!blob_data){
		return NULL;
	}

	blob_data->length = len;
	if (buf) {
		memcpy(blob_data->data, buf, len);
	}

	return blob_data;
}

void mmc_blob_data_delete(mmc_blob_data_t *blob_data)
{
	if (blob_data){
		free(blob_data);
	}
}

#ifdef MMC_BLOB_DEBUG
static void pr_op(u_int offset, u_int total_size,
		block_dev_desc_t *desc, char *str)
{

	lbaint_t blk, blkcnt,blks;
	blk = BLOCK_CNT(offset, desc);
	blkcnt = BLOCK_CNT(total_size, desc);

	printf("MMC Target: Byte [ %08x-%08x : %8d ]\n"
			"    Action: Byte [ %08x-%08x : %8d ] "
			"Block [ %08x-%08x : %8d ] ",
			offset, offset + total_size -1, total_size,
			blk*desc->blksz, (blk + blkcnt)*desc->blksz-1, blkcnt*desc->blksz,
			blk, blk + blkcnt-1, blkcnt);
	printf("%s\n------------------------------------------------"
			"-----------------------------------------------------\n", str);
}
#endif


int mmc_blob_oper(mmc_blob_desc_t *desc, mmc_blob_data_t *bdata,
		mmc_blob_op op)
{
	struct mmc *mmc;
	u_int total_size;
	u_char *buf;
	lbaint_t blk, blkcnt,blks;
	int ret = 0;

	mmc = find_mmc_device(desc->mmc_dev);
	if (!mmc) {
		goto err_out;
	}
	mmc_init(mmc);

	ret = mmc_switch_part(desc->mmc_dev, desc->mmc_part);
	if (ret) {
		goto err_out;
	}

	switch (op) {
		case MMC_BLOB_READ:
			if (BLOB_TYPE_HEADER == desc->type) {
				/* read header from mmc */
				total_size = sizeof(mmc_blob_data_t);
				buf = (u_char*)bdata;
				if (!desc->magic) {
					buf += sizeof(bdata->magic);
					total_size -= sizeof(bdata->magic);
				}

				blk = BLOCK_CNT(desc->offset, &(mmc->block_dev));
				blkcnt = BLOCK_CNT(total_size, &(mmc->block_dev));
				blks = mmc->block_dev.block_read(desc->mmc_dev, blk, blkcnt,
						buf);
				pr_op(desc->offset, total_size, &(mmc->block_dev),
						"Read(header)");
				if (blks != blkcnt) {
					printf("%s: Read failed " LBAFU "\n",__func__, blks);
					ret = -1;
					goto err_in_mmc;
				}

				if (desc->magic && desc->magic != bdata->magic) {
					printf("Invalid magic in blob reading\n");
					ret = -1;
					goto err_in_mmc;
				}

				/* read data from mmc according to the length in header */
				total_size = sizeof(mmc_blob_data_t) + bdata->length;
				buf = (u_char*)bdata;
				if (!desc->magic) {
					buf += sizeof(bdata->magic);
					total_size -= sizeof(bdata->magic);
				}
				if (total_size <= (blks * mmc->block_dev.blksz)) {
					pr_op(desc->offset, total_size, &(mmc->block_dev),
							"Read(ignore)");
					break;
				}

			} else {
				total_size = desc->max_size;
				bdata->length = total_size;
				buf = bdata->data;
			}

			if (total_size > desc->max_size) {
				pr_op(desc->offset, total_size, &(mmc->block_dev),
						"Read(cancel)");
				printf("read failed, length in header exceed blob size\n");
				ret = -1;
				goto err_in_mmc;
			}
			blk = BLOCK_CNT(desc->offset, &(mmc->block_dev));
			blkcnt = BLOCK_CNT(total_size, &(mmc->block_dev));
			blks = mmc->block_dev.block_read(desc->mmc_dev, blk, blkcnt, buf);
			pr_op(desc->offset, total_size, &(mmc->block_dev), "Read");

			if (blks != blkcnt) {
				printf("%s: Read failed " LBAFU "\n", __func__, blks);
				ret = -1;
				goto err_in_mmc;
			}
			break;


		case MMC_BLOB_WRITE:
			if (BLOB_TYPE_HEADER == desc->type) {
				total_size = sizeof(mmc_blob_data_t) + bdata->length;
				buf = (u_char*)bdata;
				if (!desc->magic) {
					buf += sizeof(bdata->magic);
					total_size -= sizeof(bdata->magic);
				}
			} else {
				total_size = bdata->length;
				buf = bdata->data;
			}

			if (total_size > desc->max_size) {
				pr_op(desc->offset, total_size, &(mmc->block_dev),
						"Write(cancel)");
				printf("write failed, data exceed blob size\n");
				ret = -1;
				goto err_in_mmc;
			}
			bdata->magic = desc->magic;
			blk = BLOCK_CNT(desc->offset, &(mmc->block_dev));
			blkcnt = BLOCK_CNT(total_size, &(mmc->block_dev));
			blks = mmc->block_dev.block_write(desc->mmc_dev, blk, blkcnt, buf);
			pr_op(desc->offset, total_size, &(mmc->block_dev), "Write");

			if (blks != blkcnt) {
				printf("%s: Write failed " LBAFU "\n", __func__, blks);
				ret = -1;
				goto err_in_mmc;
			}
			break;


		case MMC_BLOB_ERASE:
			total_size = desc->max_size;
			blk = BLOCK_CNT(desc->offset, &(mmc->block_dev));
			blkcnt = BLOCK_CNT(total_size, &(mmc->block_dev));
			pr_op(desc->offset, total_size, &(mmc->block_dev),
					"Erase(Write 0)");
			buf = malloc(mmc->block_dev.blksz);
			if (!buf) {
				printf("Erase failed, malloc failed \n");
				ret = -1;
				goto err_in_mmc;
			}
			memset(buf, 0, mmc->block_dev.blksz);
			while (blkcnt--) {
				blks = mmc->block_dev.block_write(desc->mmc_dev,
						blk++, 1, buf);
				if (blks != 1) {
					free(buf);
					printf("%s: Erase failed\n", __func__);
					ret = -1;
					goto err_in_mmc;
				}
			}
			free(buf);
			break;

		default:
			ret = -1;
			goto err_in_mmc;
	}

err_in_mmc:
	if (mmc_switch_part(desc->mmc_dev, 0)) {
		ret = -1;
	}

err_out:
	return ret;
}

int mmc_blob_cmp(const mmc_blob_data_t *bdata1,
		const mmc_blob_data_t *bdata2, int cmp_magic)
{
	if (!bdata1 || !bdata2) {
		return -1;
	}
	if (bdata1->length != bdata2->length) {
		return -1;
	}
	if (cmp_magic && (bdata1->magic != bdata2->magic)) {
		return -1;
	}

	return memcmp(bdata1->data, bdata2->data, bdata1->length);
}
