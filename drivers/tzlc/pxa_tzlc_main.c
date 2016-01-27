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
 *  @file           pxa_tzlc_main.c
 *  @author         Jidong Sun
 *  @date           30/01/2013 (dd/mm/yy)
 *  @brief          tzlc functions' implementation.
 *
 */

#include <common.h>
#include "pxa_tzlc_priv.h"

tzlc_handle pxa_tzlc_create_handle(void)
{
	/* reserve for futher use */
	return (tzlc_handle)1;
}

void pxa_tzlc_destroy_handle(tzlc_handle handle)
{
	/* reserve for futher use */
	return;
}

int32_t pxa_tzlc_cmd_op(tzlc_handle handle, tzlc_cmd_desc *cmd)
{
	tzlc_req_param param;
	int32_t ret;

	if (NULL == cmd || NULL == handle)
		return -1;

	if ((cmd->op & TZLC_CMD_MASK) >=
		((cmd->op & PLATFORM_SPEC_TZLC_FLAG) ?
		PLATFORM_SPEC_TZLC_CMD_END : TZLC_CMD_END))
		return -1;

	param.type = TZLC_TYPE_INTEGRATED_CMD;
	param.cmd = *cmd;

	ret = __tzlc_send_req(&param);
	*cmd = param.cmd;

	return ret;
}
