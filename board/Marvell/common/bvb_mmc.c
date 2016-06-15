/*
 * bvb_mmc.c Brillo verified boot support
 *
 * (C) Copyright 2016
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Yonghui Wan <yhwan@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <mmc.h>
#include <part.h>
#include <bvb_mmc.h>

#define BVB_MMC_OFFSET 0x200000

/* device state blob */
#define BVB_MMC_DEVICE_STATE_OFFSET     BVB_MMC_OFFSET
#define BVB_MMC_DEVICE_STATE_MAXSIZE    0x200
#define BVB_MMC_DEVICE_STATE_MAGIC      0x19080001

/* nvram rollback_index blob */
#define BVB_MMC_ROLLBACK_INDEX_OFFSET   \
	(BVB_MMC_DEVICE_STATE_OFFSET + BVB_MMC_DEVICE_STATE_MAXSIZE)
#define BVB_MMC_ROLLBACK_INDEX_MAXSIZE  0x200
#define BVB_MMC_ROLLBACK_INDEX_MAGIC    (BVB_MMC_DEVICE_STATE_MAGIC + 1)

/* devkey blob */
#define BVB_MMC_DEVKEY_OFFSET           \
	(BVB_MMC_ROLLBACK_INDEX_OFFSET + BVB_MMC_ROLLBACK_INDEX_MAXSIZE)
#define BVB_MMC_DEVKEY_MAXSIZE          0x1000
#define BVB_MMC_DEVKEY_MAGIC            (BVB_MMC_ROLLBACK_INDEX_MAGIC + 1)

#define BVB_STR_LOCKED                  "locked"
#define BVB_STR_UNLOCKED                "unlocked"


static void bvb_blob_desc_init(mmc_blob_desc_t *desc, u_int offset,
		u_int max_size, u_int magic)
{
	mmc_blob_desc_init(desc, MMC_DEV, MMC_BOOT0, offset,
						max_size, magic, BLOB_TYPE_HEADER);
}

int bvb_write_rollback_index(u_int index)
{
	mmc_blob_desc_t desc;
	mmc_blob_data_t *bdata;
	int ret = BVB_OK;

	bvb_blob_desc_init(&desc, BVB_MMC_ROLLBACK_INDEX_OFFSET,
			BVB_MMC_ROLLBACK_INDEX_MAXSIZE, BVB_MMC_ROLLBACK_INDEX_MAGIC);
	bdata = mmc_blob_data_new((u_char*)&index, sizeof(index));
	if (!bdata) {
		ret = BVB_ERR;
		goto out;
	}

	ret = mmc_blob_oper(&desc, bdata, MMC_BLOB_WRITE);
	mmc_blob_data_delete(bdata);

out:
	return ret;
}

int bvb_read_rollback_index(u_int *index)
{
	mmc_blob_desc_t desc;
	mmc_blob_data_t *bdata;
	int ret = BVB_OK;

	bvb_blob_desc_init(&desc, BVB_MMC_ROLLBACK_INDEX_OFFSET,
			BVB_MMC_ROLLBACK_INDEX_MAXSIZE, BVB_MMC_ROLLBACK_INDEX_MAGIC);
	bdata = mmc_blob_data_new(NULL, desc.max_size);
	if (!bdata) {
		ret = BVB_ERR;
		goto out;
	}

	ret = mmc_blob_oper(&desc, bdata, MMC_BLOB_READ);
	if (ret) {
		ret = BVB_ERR;
		goto err_out;
	}

	*index = *((u_int *)bdata->data);

err_out:
	mmc_blob_data_delete(bdata);

out:
	return ret;
}

static int bvb_write_device_state(int state, int check_state)
{
	mmc_blob_desc_t desc;
	mmc_blob_data_t *bdata;
	char buf[16] = {0};
	int ret;

	if (check_state && (bvb_check_device_lock() == state)) {
		ret = BVB_ERR_WRITE_SAME;
		goto out;
	}

	if (state) {
		strncpy(buf, BVB_STR_LOCKED, strlen(BVB_STR_LOCKED));
	} else {
		strncpy(buf, BVB_STR_UNLOCKED, strlen(BVB_STR_UNLOCKED));
	}

	bvb_blob_desc_init(&desc, BVB_MMC_DEVICE_STATE_OFFSET,
			BVB_MMC_DEVICE_STATE_MAXSIZE, BVB_MMC_DEVICE_STATE_MAGIC);
	bdata = mmc_blob_data_new((u_char*)buf, strlen(buf)+1);
	if (!bdata) {
		ret = BVB_ERR;
		goto out;
	}

	ret = mmc_blob_oper(&desc, bdata, MMC_BLOB_WRITE);
	mmc_blob_data_delete(bdata);

out:
	return ret;
}

int bvb_check_device_lock()
{
	mmc_blob_desc_t desc;
	mmc_blob_data_t *bdata;
	char buf[16] = {0};
	int ret;

	bvb_blob_desc_init(&desc, BVB_MMC_DEVICE_STATE_OFFSET,
			BVB_MMC_DEVICE_STATE_MAXSIZE, BVB_MMC_DEVICE_STATE_MAGIC);
	bdata = mmc_blob_data_new(NULL, desc.max_size);
	if (!bdata) {
		ret = BVB_ERR;
		goto out;
	}

	ret = mmc_blob_oper(&desc, bdata, MMC_BLOB_READ);
	if (ret) {
		goto err_out;
	}

	if (!strcmp(BVB_STR_LOCKED, (char *)bdata->data)) {
		ret = BVB_LOCKED;
	} else if (!strcmp(BVB_STR_UNLOCKED, (char *)bdata->data)) {
		ret = BVB_UNLOCKED;
	} else {
		printf("Invalid data in device state blob, Try to set locked state.\n");
		ret = bvb_write_device_state(BVB_LOCKED, BVB_CHECK_STATE_NO);
		if (ret) {
			ret = BVB_ERR;
			goto err_out;
		} else {
			printf("Device state has been set to locked \n");
			ret = BVB_LOCKED;
		}
	}

err_out:
	mmc_blob_data_delete(bdata);

out:
	return ret;
}

int bvb_lock_device()
{
	return bvb_write_device_state(BVB_LOCKED, BVB_CHECK_STATE_YES);
}

int bvb_unlock_device()
{
	return bvb_write_device_state(BVB_UNLOCKED, BVB_CHECK_STATE_YES);
}

int bvb_write_key(u_char *buf, u_int len)
{
	mmc_blob_desc_t desc;
	mmc_blob_data_t *key_to_write;
	mmc_blob_data_t *key_in_mmc;
	int ret;

	bvb_blob_desc_init(&desc, BVB_MMC_DEVKEY_OFFSET,
			BVB_MMC_DEVKEY_MAXSIZE, BVB_MMC_DEVKEY_MAGIC);
	key_to_write = mmc_blob_data_new(buf, len);
	if (!key_to_write) {
		ret = BVB_ERR;
		goto out;
	}

	key_in_mmc = bvb_make_key_from_mmc();
	if (!bvb_compare_key(key_in_mmc, key_to_write)) {
		ret = BVB_ERR_WRITE_SAME;
		goto err_out;
	}

#ifdef CONFIG_MV_BVB
	if (mv_bvb_power_on_wp_get(desc.mmc_dev)) {
		ret = BVB_ERR;
		goto err_out;
	}
#endif

	ret = mmc_blob_oper(&desc, key_to_write, MMC_BLOB_WRITE);

err_out:
	mmc_blob_data_delete(key_in_mmc);
	mmc_blob_data_delete(key_to_write);

out:
	return ret;
}

int bvb_erase_key(void)
{
	mmc_blob_desc_t desc;
	int ret;

	bvb_blob_desc_init(&desc, BVB_MMC_DEVKEY_OFFSET,
			BVB_MMC_DEVKEY_MAXSIZE, BVB_MMC_DEVKEY_MAGIC);
	ret = mmc_blob_oper(&desc, NULL, MMC_BLOB_ERASE);

	return ret;
}

int bvb_read_key_to_buf(u_char *buf)
{
	mmc_blob_desc_t desc;

	bvb_blob_desc_init(&desc, BVB_MMC_DEVKEY_OFFSET,
			BVB_MMC_DEVKEY_MAXSIZE, BVB_MMC_DEVKEY_MAGIC);
	return mmc_blob_oper(&desc, buf, MMC_BLOB_READ);
}

mmc_blob_data_t* bvb_make_key_from_mmc()
{
	mmc_blob_desc_t desc;
	mmc_blob_data_t *bdata;
	int ret;

	bvb_blob_desc_init(&desc, BVB_MMC_DEVKEY_OFFSET,
				BVB_MMC_DEVKEY_MAXSIZE, BVB_MMC_DEVKEY_MAGIC);
	bdata = mmc_blob_data_new(NULL, desc.max_size);
	if (!bdata) {
		goto out;
	}

	ret = mmc_blob_oper(&desc, bdata, MMC_BLOB_READ);
	if (ret) {
		printf("No valid bvb_devkey found in MMC\n");
		goto err_out;
	}

out:
	return bdata;

err_out:
	mmc_blob_data_delete(bdata);
	return NULL;
}

int bvb_compare_key(const mmc_blob_data_t *key1,
		const mmc_blob_data_t *key2)
{
	return mmc_blob_cmp(key1, key2, 0);
}

int bvb_devkey_match(u_char *buf, u_int len)
{
	mmc_blob_data_t *devkey, *embeded_key;
	int ret = -1;

	if (!buf || !len) {
		printf("Invalid buffer arguments\n");
		goto err;
	}
	devkey = bvb_make_key_from_mmc();
	if (!devkey) {
		goto err;
	}
	embeded_key = mmc_blob_data_new(buf, len);
	if (!embeded_key) {
		goto err_dev;
	}

	ret = bvb_compare_key(devkey, embeded_key);

	mmc_blob_data_delete(embeded_key);
err_dev:
	mmc_blob_data_delete(devkey);
err:
	return ret;
}
