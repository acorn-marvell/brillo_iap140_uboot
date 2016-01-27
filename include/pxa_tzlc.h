/*
 * Copyright (c) [2009-2013] Marvell International Ltd. and its affiliates.
 * All rights reserved.
 * This software file (the "File") is owned and distributed by Marvell
 * International Ltd. and/or its affiliates ("Marvell") under the following
 * licensing terms.
 * If you received this File from Marvell, you may opt to use, redistribute
 * and/or modify this File in accordance with the terms and conditions of
 * the General Public License Version 2, June 1991 (the "GPL License"), a
 * copy of which is available along with the File in the license.txt file
 * or by writing to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307 or on the worldwide web at
 * http://www.gnu.org/licenses/gpl.txt. THE FILE IS DISTRIBUTED AS-IS,
 * WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
 * DISCLAIMED. The GPL License provides additional details about this
 * warranty disclaimer.
 *
 *  @file           pxa_tzlc.h
 *  @author         Michael Zhao
 *  @date           03/08/2012 (dd/mm/yy)
 *  @brief          The header file declares the tzlc functions' interfaces.
 *
 */


#ifndef __PXA_TZLC_H__
#define __PXA_TZLC_H__

#include <common.h>

#define TEE_FUSE_CMD_SHIFT	(0)
#define TEE_FUSE_SUB_CMD_SHIFT	(16)
#define TZLC_CMD_MASK		((1 << TEE_FUSE_SUB_CMD_SHIFT) - 1)
#define PLATFORM_SPEC_TZLC_FLAG	(0x80000000)
typedef void *tzlc_handle;

#define TZLC_CMD_SET_WARM_RESET_ENTRY		(2) /* set warm reset entry */
#define TZLC_CMD_TW_CPU_PM_ENTER		(5)
#define TZLC_CMD_READ_MANUFACTURING_BITS	(6)
#define TZLC_CMD_TRIGER_SGI			(7)
#define TZLC_CMD_READ_OEM_UNIQUE_ID		(8)
#define TZLC_CMD_END				(9)
#define PLATFORM_SPEC_TZLC_CMD_END		(0x80000001)

/* integrated cmd, eg: flush cache */
typedef struct _tzlc_cmd_desc {
	int32_t op;
	uint32_t args[4];
} tzlc_cmd_desc;

extern tzlc_handle pxa_tzlc_create_handle(void);
extern void pxa_tzlc_destroy_handle(tzlc_handle handle);
extern int32_t pxa_tzlc_cmd_op(tzlc_handle handle, tzlc_cmd_desc *cmd);

#endif
