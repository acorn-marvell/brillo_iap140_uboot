/*
 * hai_dev.h
 *
 * Hardware Abstraction Interface descriptor definition for block device.
 * interface.
 *
 *   Copyright (C) 2008 MARVELL Corporation (antone@marvell.com)
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _HAI_DEV_H
#define _HAI_DEV_H

#include <common.h> 
struct tdev_t {
	block_dev_desc_t 	*dev_desc;
	unsigned long		part_offset;
	unsigned		sector_size;
};


#endif
