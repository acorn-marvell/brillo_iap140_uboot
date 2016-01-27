/*
 * FILENAME:	mv_cp.c
 * PURPOSE:	cp related operation
 *
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Wei Shu <weishu@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <mmc.h>
#include "mrd_flag.h"
#include "mv_cp.h"
#include "tim.h"
#include "obm2osl.h"

/* #define CP_TIME_DEBUG */

#define CPIMG_ALIGNMENT (1 << 16)	/* 64K */

#ifndef min
#define min(x, y) ((x) > (y) ? (y) : (x))
#endif
#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef roundup
/* y must be the multiple of 2 */
#define roundup(x, y) ((x + y - 1) & ~(y - 1))
#endif

#ifndef rounddown
/* y must be the multiple of 2 */
#define rounddown(x, y) (x & ~(y - 1))
#endif

static unsigned int mrd_flag_size;

static int read_image(unsigned int flash_entryaddr, unsigned int img_size,
		      unsigned char *buf, unsigned int offset,
		      unsigned int size)
{
	char cmd[128];
	unsigned char *tbuf = NULL;
	unsigned int aligned_size = 0;

	if (offset > img_size)
		return -1;

	if (offset + size > img_size)
		size = img_size - offset;

	aligned_size = roundup(offset + size, 512) - rounddown(offset, 512);

	tbuf = malloc(aligned_size);
	if (!tbuf) {
		printf("%s: malloc error\n", __func__);
		return -1;
	}

	memset(tbuf, 0, aligned_size);
	sprintf(cmd, "%s; mmc read %p %x %x", CONFIG_MMC_BOOT_DEV, tbuf,
		flash_entryaddr + offset / 512, aligned_size / 512);
	run_command(cmd, 0);

	memcpy(buf, tbuf + (offset & (512 - 1)), size);

	free(tbuf);

	return size;
}

static void update_lwg_prop(unsigned int value, unsigned int *property)
{
	*property &= ~(0xF << 4);
	*property |= (value & 0xF) << 4;
}

static void update_ltg_prop(unsigned int value, unsigned int *property)
{
	*property &= ~0xF;
	*property |= value & 0xF;
}

static int __get_cp_ddr_range(struct TIM *tim, unsigned int tim_version,
			      unsigned int imgid, unsigned int *start,
			      unsigned int *end, unsigned int *exist_cp_prop)
{
	unsigned int flash_entryaddr = 0;
	unsigned int img_size = 0;
	struct cp_load_table_head hdr;

	if (get_image_flash_entryaddr(tim, tim_version,
				      imgid, &flash_entryaddr,
				      &img_size) >= 0) {
		memset(&hdr, 0, sizeof(hdr));
		if (read_image(flash_entryaddr, img_size,
			       (unsigned char *)&hdr,
			       OFFSET_IN_ARBEL_IMAGE,
			       sizeof(hdr)) == sizeof(hdr)) {
			/* check the signature */
			if (strcmp(hdr.Signature, LOAD_TABLE_SIGNATURE) == 0) {
				unsigned int cp = 0;
				unsigned int msa = 0;
				unsigned int rf = 0;
				unsigned int istart = 0;
				unsigned int iend = 0;

				cp = rounddown(hdr.imageBegin, CPIMG_ALIGNMENT);
				msa = rounddown(hdr.dp.MSALoadAddrStart,
						CPIMG_ALIGNMENT);
				rf = rounddown(hdr.dp.RFBinLoadAddrStart,
					       CPIMG_ALIGNMENT);
				if (!rf)
					rf = msa;
				istart = min(min(cp, msa), rf);

				cp = roundup(hdr.imageEnd, CPIMG_ALIGNMENT);
				msa = roundup(hdr.dp.MSALoadAddrEnd,
					      CPIMG_ALIGNMENT);
				rf = roundup(hdr.dp.RFBinLoadAddrEnd,
					     CPIMG_ALIGNMENT);
				if (!rf)
					rf = msa;
				iend = max(max(cp, msa), rf);

				printf
				    ("%s: image(0x%08x) start: 0x%08x, end: 0x%08x\n",
				     __func__, imgid, istart, iend);

				if (start)
					*start = istart;

				if (end)
					*end = iend;

				/* set the property */
				if (exist_cp_prop) {
					unsigned int cp_partition = 0;

					if (imgid == ARBIID)
						cp_partition = 1;
					else if (imgid == ARB2)
						cp_partition = 2;

					if (hdr.dp.NetworkModeIndication == NETWORK_MODE_TD) {
						update_ltg_prop(cp_partition, exist_cp_prop);
					} else if (hdr.dp.NetworkModeIndication
									== NETWORK_MODE_WB) {
						update_lwg_prop(cp_partition, exist_cp_prop);
					} else {
						printf("%s: Image: %d unknow Network Mode Type\n",
						       __func__, imgid);
					}
				}

				return 0;
			} else {
				printf("%s: cp signature error\n", __func__);
			}
		} else {
			printf("%s: read image(0x%08x) error\n",
			       __func__, imgid);
		}
	} else {
		printf("%s: get image(0x%08x) entry address error\n",
		       __func__, imgid);
	}

	return -1;
}

int get_cp_ddr_range(struct OBM2OSL *obm2osl,
		     unsigned int *start, unsigned int *size)
{
	return get_cp_ddr_range2(obm2osl, NULL, start, size);
}

int get_cp_ddr_range2(struct OBM2OSL *obm2osl, unsigned int *exist_cp_prop,
		     unsigned int *start, unsigned int *size)
{
	struct TIM tim;
	unsigned int start1 = 0xffffffff, end1 = 0;
	unsigned int start2 = 0xffffffff, end2 = 0;
	unsigned int istart, iend;
#ifdef CP_TIME_DEBUG
	uint32_t tmsec1 = 0;
	uint32_t tmsec2 = 0;
#endif

#ifdef CP_TIME_DEBUG
	tmsec1 = get_timer(0);
	printf("[%08x] %s: enter\n", tmsec1, __func__);
#endif

	if (set_tim_pointers((unsigned char *)(unsigned long)
			     obm2osl->cp.tim_ddr_address, &tim) < 0) {
		printf("%s: get tim pointer error\n", __func__);
		return -1;
	}

	__get_cp_ddr_range(&tim, obm2osl->cp.tim_version,
			   ARBIID, &start1, &end1, exist_cp_prop);
	__get_cp_ddr_range(&tim, obm2osl->cp.tim_version,
			   ARB2, &start2, &end2, exist_cp_prop);

	istart = min(start1, start2);
	iend = max(end1, end2);

	if (start)
		*start = istart;

	if (size) {
		if (istart > iend)
			*size = 0;
		else
			*size = iend - istart;
	}
#ifdef CP_TIME_DEBUG
	tmsec2 = get_timer(0);
	printf("[%08x] %s: leave\n", tmsec2, __func__);
	printf("%s: elapsed time: %d msec\n", __func__, tmsec2 - tmsec1);
#endif

	return 0;
}

static enum MFLAG_ERR_CODES mrd_flag_read(unsigned char *mrd_flag_buffer,
		      unsigned int tag, char **value, unsigned int *flag_size)
{
	unsigned int  len;
	unsigned char pad_size;
	int file_found = 0;
	unsigned char *ptr, *ptr_end;
	struct MRD_FLAG_ENTRY  *pmrd_flag_entry;

	ptr = mrd_flag_buffer + sizeof(struct MRD_FLAG_HEADER);
	ptr_end = mrd_flag_buffer + mrd_flag_size;

	do {
		pmrd_flag_entry = (struct MRD_FLAG_ENTRY *)ptr;
		len = pmrd_flag_entry->length;
		if (pmrd_flag_entry->tag == TAG_EOF)
			break;

		if (pmrd_flag_entry->tag == tag) {
			if (value) {
				*value = (char *)calloc(1, len + 1);
				memcpy(*value, pmrd_flag_entry->value, len);
			}
			if (flag_size)
				*flag_size = len;
			file_found = 1;
			break;
		}
		pad_size = (sizeof(int) - (len & 0x3)) & 0x3;
		ptr += sizeof(struct MRD_FLAG_ENTRY) + len + pad_size;
	} while (ptr < ptr_end);

	if (file_found)
		return MF_NO_ERROR;
	else
		return MF_FILE_NOT_FOUND_ERROR;
}

static int get_mep2_range(unsigned int *start, unsigned int *size)
{
	block_dev_desc_t *mmc_dev;
	struct mmc *mmc;
	int part, pnum;
	disk_partition_t part_info;
	mmc = find_mmc_device(2);
	if (!mmc) {
		printf("no mmc device at slot 2\n");
		return -1;
	}
	mmc_init(mmc);
	mmc_dev = mmc_get_dev(2);
	if (mmc_dev == NULL || mmc_dev->type == DEV_TYPE_UNKNOWN)
		return -1;

	pnum = get_partition_num(mmc_dev);
	for (part = 1; part <= pnum; part++) {
		if (get_partition_info(mmc_dev, part, &part_info) == 0)
			if (!strcmp((const char *)part_info.name, MEP2_PART_NAME))
				break;
	}

	if (part == pnum + 1)
		return -1;
	if (start)
		*start = part_info.start;
	if (size)
		*size = part_info.size * part_info.blksz;

	return 0;
}

int get_powermode(void)
{
	int mode = POWER_MODE_NORMAL;
	unsigned char *mrdflag_buf;
	struct MRD_FLAG_HEADER *pmrd_flag_hdr;
	char *value = NULL;
	unsigned int mep2_start = 0;
	unsigned int mep2_size = 0;
	unsigned int size = 0;

	if (get_mep2_range(&mep2_start, &mep2_size) < 0)
		return mode;

	mrd_flag_size = mep2_size > MRD_FLAG_MEP2_OFFSET ? mep2_size - MRD_FLAG_MEP2_OFFSET : 0;
	if (mrd_flag_size == 0)
		return mode;

	mrdflag_buf = malloc(mrd_flag_size);
	if (mrdflag_buf == NULL) {
		printf("%s: malloc mrdflag buf error\n", __func__);
		return mode;
	}
	if (read_image(mep2_start, mep2_size, mrdflag_buf,
				   MRD_FLAG_MEP2_OFFSET, mrd_flag_size) < 0) {
		free(mrdflag_buf);
		return mode;
	}

	pmrd_flag_hdr = (struct MRD_FLAG_HEADER *)mrdflag_buf;

	if (pmrd_flag_hdr->magic == VALID_FLAG_HEADER) {
		if (mrd_flag_read(mrdflag_buf, TAG_POWER_MODE, &value, &size) == MF_NO_ERROR) {
			if (size == 1) {
				switch (value[0]) {
				case DIAG_OVER_USB_TAG:
					mode = POWER_MODE_DIAG_OVER_USB;
					break;
				case DIAG_OVER_UART_TAG:
					mode = POWER_MODE_DIAG_OVER_UART;
					break;
				default:
					mode = POWER_MODE_NORMAL;
					break;
				}
			}
		}
	}
	free(mrdflag_buf);
	free(value);

	return mode;
}
