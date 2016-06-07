/*
 * fb_flashing.c - fastboot flashing lock/unlock command
 *
 * (C) Copyright 2016
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Ethan Xia <xiasb@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <common.h>
#include <part.h>
#include <mmc.h>
#include "blob.h"
#include <fb_flashing.h>

#define LOCK_MMC_OFFSET 0x200000
#define MMC_BLOCK_SIZE 512

#define LOCK_STATE "locked"
#define UNLOCK_STATE "unlocked"

#define MMC_DEV_USERDATA 2
#define MMC_PART_USERDATA "userdata"

static void clear_userdata()
{
	int ret;
	lbaint_t blks;
	block_dev_desc_t *dev_desc;
	disk_partition_t info;

	dev_desc = get_dev("mmc", MMC_DEV_USERDATA);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("%s: invalid mmc device\n", __func__);
		return;
	}

	ret = get_partition_info_efi_by_name(dev_desc, MMC_PART_USERDATA, &info);
	if (ret) {
		printf("%s: cannot find partition '%s'\n", __func__, MMC_PART_USERDATA);
		return;
	}

	blks = dev_desc->block_erase(dev_desc->dev, info.start, info.size);
	if (blks != info.size) {
		printf("%s: failed erasing partition %s\n", __func__, MMC_PART_USERDATA);
		return;
	}

	printf("%s: erased partition %s of " LBAFU " blocks\n", __func__,
		MMC_PART_USERDATA, blks);

	return;
}

static void assert_user_here()
{
	return;
}

/* All failed cases will LOCK the device */
device_state get_device_state(char *device_state)
{
	unsigned char loadaddr[MMC_BLOCK_SIZE] = {0};
	unsigned int total_len = 0;
	int ret = BLOB_OK;
	ret = blob_read(LOCK_MMC_OFFSET, loadaddr, &total_len);
	if (ret == BLOB_FAIL) {
		printf("%s: blob_read failed\n", __func__);
		return FB_FLASHING_DEVICE_LOCKED;
	}

	blob_data *bd = (blob_data *)loadaddr;
	if (!bd && bd->type != BLOB_TYPE_DEVICE_STATE) {
		printf("%s: blob type not matched\n", __func__);
		return FB_FLASHING_DEVICE_LOCKED;
	}

	unsigned int state_len = total_len - sizeof(blob_data);
	if (device_state && bd->data) {
		strncpy(device_state, bd->data, state_len);
	}

	if (bd->data && !strncmp(bd->data, UNLOCK_STATE, state_len)) {
		return FB_FLASHING_DEVICE_UNLOCKED;
	}

	return FB_FLASHING_DEVICE_LOCKED;
}

/**
 * do_lock_device - do lock the device
 *
 * @flag: Lock the device when flag==1 and unlock the device when flag==0.
 */
static flashing_state do_lock_device(int flag)
{
	int ret = BLOB_OK;
	flashing_state state_ok;
	flashing_state state_fail;

	if (flag != 0 && flag != 1)
		return FB_FLASHING_INTERNAL_ERROR;

	/* Set device state to locked/unlocked */
	char buf[64] = {0};
	if (flag == 1) {
		state_ok = FB_FLASHING_LOCK_DEVICE_OK;
		state_fail = FB_FLASHING_LOCK_DEVICE_FAILED;
		strncpy(buf, LOCK_STATE, strlen(LOCK_STATE));
	} else if (flag == 0) {
		state_ok = FB_FLASHING_UNLOCK_DEVICE_OK;
		state_fail = FB_FLASHING_UNLOCK_DEVICE_FAILED;
		strncpy(buf, UNLOCK_STATE, strlen(UNLOCK_STATE));
	}

	ret = blob_write(LOCK_MMC_OFFSET, BLOB_TYPE_DEVICE_STATE, buf,
		strlen(buf));
	if (ret == BLOB_FAIL) {
		printf("%s: blob_write failed when locking device\n", __func__);
		return state_fail;
	}

	if (flag == 0) {
		clear_userdata();
	}

	return state_ok;
}

flashing_state lock_device()
{
	device_state state;
	char buf[64] = {0};

	state = get_device_state(buf);
	if (state == FB_FLASHING_DEVICE_LOCKED && !strncmp(buf, LOCK_STATE, strlen(buf))) {
		printf("%s: already locked\n", __func__);
		return FB_FLASHING_DEVICE_ALREADY_LOCKED;
	} else {
		return do_lock_device(1);
	}
}

flashing_state unlock_device()
{
	/* Assert physical presence */
	assert_user_here();

	device_state state;
	char buf[64] = {0};

	state = get_device_state(buf);
	if (state == FB_FLASHING_DEVICE_UNLOCKED && !strncmp(buf, UNLOCK_STATE, strlen(buf))) {
		printf("%s: already unlocked\n", __func__);
		return FB_FLASHING_DEVICE_ALREADY_UNLOCKED;
	} else {
		return do_lock_device(0);
	}

	return FB_FLASHING_UNLOCK_DEVICE_OK;
}
