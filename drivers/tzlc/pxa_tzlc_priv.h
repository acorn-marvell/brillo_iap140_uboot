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
 *  @file           pxa_tzlc_priv.h
 *  @author         Jidong Sun
 *  @date           30/01/2013 (dd/mm/yy)
 *  @brief          Internal header file.
 *
 */

#ifndef __PXA_TZLC_PRIV_H__
#define __PXA_TZLC_PRIV_H__

#include <pxa_tzlc.h>

typedef enum _tzlc_req_type {
	TZLC_TYPE_INTEGRATED_CMD,
	TZLC_TYPE_END,
} tzlc_req_type;

typedef struct _tzlc_req_param {
	tzlc_req_type type;
	tzlc_cmd_desc cmd;
} tzlc_req_param;

extern int32_t __tzlc_send_req(tzlc_req_param *param);
#endif
