/*
 * mv_bvb.h Marvell Brillo Verified Boot 
 *
 * (C) Copyright 2016
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Zhengxi Hu <zhxihu@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#ifndef _MV_BVB_H
#define _MV_BVB_H

#include "bvb/bvb_verify.h"

#define MV_BVB_PWR_WP_EN	1
#define MV_BVB_PWR_WP_DIS	0

#ifdef CONFIG_MV_BVB_DEBUG
#define bvb_debug(fmt, args...)		printf(fmt, ##args)
#else
#define bvb_debug(fmt, args...)
#endif

void mv_bvb_load_header(struct BvbBootImageHeader *bhdr,
		unsigned long base_emmc_addr);
void mv_bvb_load_image(unsigned long load_addr, unsigned long base_emmc_addrm,
		size_t image_size);

void mv_bvb_key_dump(uint8_t *out_key_data, size_t out_key_length);

int mv_bvb_power_on_wp_set(int mmc_dev);

BvbVerifyResult mv_bvb_verify(const uint8_t *data, size_t length,
		const uint8_t **out_key_data, size_t *out_key_length);

static inline unsigned int mv_bvb_image_size(struct BvbBootImageHeader *bhdr)
{
	if (bhdr)
		return (BVB_BOOT_IMAGE_HEADER_SIZE +
				bhdr->authentication_data_block_size +
				bhdr->auxilary_data_block_size +
				bhdr->payload_data_block_size);

	return 0;
}

#endif


