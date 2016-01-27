/*
 * FILENAME:	tim.c
 * PURPOSE:	parsing TIM/NTIM
 *
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Wei Shu <weishu@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <stddef.h>
#include <common.h>
#include "tim.h"
#include "obm2osl.h"

int set_tim_pointers(unsigned char *start_addr, struct TIM *tim)
{
	if (!start_addr) {
		printf("%s: start addr is null\n", __func__);
		return -1;
	}

	memset(tim, 0, sizeof(*tim));
	tim->ctim = (struct CTIM *)start_addr;

	if ((tim->ctim->version_bind.identifier != TIMIDENTIFIER) ||
	    (tim->ctim->version_bind.version < TIM_3_2_00)) {
		/* incorrect tim */
		tim->ctim = NULL;
		return -1;
	}

	tim->img = (void *)(start_addr + sizeof(struct CTIM));

	/* keep key, reserved, tbtim_ds NULL here */
	return 0;
}

void *return_imgptr(struct TIM *tim, unsigned int tim_version,
		    unsigned int img_num)
{
	void *img = NULL;

	/* only support v3.5.0 and v3.4.0 */
	if (tim_version == DTIM_TIM_VER_3_5_0)
		img = (void *)&((struct IMAGE_INFO_3_5_0 *)tim->img)[img_num];
	else if (tim_version == DTIM_TIM_VER_3_4_0)
		img = (void *)&((struct IMAGE_INFO_3_4_0 *)tim->img)[img_num];

	return img;
}

void *find_image_intim(struct TIM *tim, unsigned int tim_version,
		       unsigned int imageid)
{
	void *img = NULL;
	unsigned int i;

	if (tim) {
		for (i = 0; i < tim->ctim->num_images; i++) {
			img = return_imgptr(tim, tim_version, i);
			if (tim_version == DTIM_TIM_VER_3_5_0) {
				if (((struct IMAGE_INFO_3_5_0 *)img)->imageid ==
				    imageid)
					return img;
			} else if (tim_version == DTIM_TIM_VER_3_4_0) {
				if (((struct IMAGE_INFO_3_4_0 *)img)->imageid ==
				    imageid)
					return img;
			}
		}
	}

	return NULL;
}

int get_image_flash_entryaddr(struct TIM *tim, unsigned int tim_version,
			      unsigned int imageid, unsigned int *addr,
			      unsigned int *size)
{
	void *img = find_image_intim(tim, tim_version, imageid);
	int ret = -1;

	if (img) {
		if (tim_version == DTIM_TIM_VER_3_5_0) {
			struct IMAGE_INFO_3_5_0 *pimg = img;

			if (addr)
				*addr = pimg->flash_entryaddr;
			if (size)
				*size = pimg->image_size;

			ret = 0;
		} else if (tim_version == DTIM_TIM_VER_3_4_0) {
			struct IMAGE_INFO_3_5_0 *pimg = img;

			if (addr)
				*addr = pimg->flash_entryaddr;
			if (size)
				*size = pimg->image_size;

			ret = 0;
		}
	}

	return ret;
}
