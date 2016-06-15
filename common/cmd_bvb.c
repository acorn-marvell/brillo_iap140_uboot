/*
 * (C) Copyright 2016
 * Ethan Xia, xiasb@marvell.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <mmc.h>

#include "bvb_mmc.h"
#include "fb_flashing.h"

typedef enum {
	BVB_TYPE_DEVICE_STATE = 0,
	BVB_TYPE_ROLLBACK_INDEX,
	BVB_TYPE_DEVEL_KEY,
} bvb_blob_type;

const char * const blob_types[] = {
	"DEVICE_STATE",
	"ROLLBACK_INDEX",
	"DEVEL_KEY"
};

static int do_bvb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	u32 rb_index;
	char buf[64] = {0};
	int ret = CMD_RET_SUCCESS;

	if (argc == 2 && !strcmp(argv[1], "list")) {
		int i = 0;
		printf("Available bvb blob types:\n");
		for (i = 0; i < sizeof(blob_types) / sizeof(blob_types[0]); ++i)
			printf(" - %d: %s\n", i, blob_types[i]);
	} else if (argc == 3 && !strcmp(argv[1], "read")) {
		if (!strcmp(argv[2], "index")) {
			if (bvb_read_rollback_index(&rb_index) != BVB_OK) {
				printf("no NVRAM_minimum_rollback_index found\n");
				ret =  CMD_RET_FAILURE;
			} else
				printf("NVRAM_minimum_rollback_index: 0x%08x\n", rb_index);
		} else if (!strcmp(argv[2], "state")) {
			get_device_state(buf);
			printf("device_state: %s\n", buf);
		} else
			ret = CMD_RET_USAGE;
	} else if (argc == 4 && strcmp(argv[1], "write") == 0) {
		if (!strcmp(argv[2], "index")) {
			rb_index = (u32)simple_strtoul(argv[3], NULL, 16);
			if (bvb_write_rollback_index(rb_index) != BVB_OK)
				ret = CMD_RET_FAILURE;
		} else if (!strcmp(argv[2], "state")) {
			if (!strcmp(argv[3], "locked"))
				lock_device();
			else if (!strcmp(argv[3], "unlocked"))
				unlock_device();
			else
				ret = CMD_RET_USAGE;
		} else
			ret = CMD_RET_USAGE;
	} else
		ret = CMD_RET_USAGE;

	return ret;
}

U_BOOT_CMD(
	bvb, 4, 1, do_bvb,
	"bvb blob read/write in write protected eMMC",
	"list - list bvb blob types\n"
	"bvb read index - read NVRAM_minimum_rollback_index\n"
	"bvb write index <index> - NVRAM_minimum_rollback_index\n"
	"bvb read state - read device state(locked or unlocked)\n"
	"bvb write state <state> - write device state(locked or unlocked)\n"
);
