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
 */

#define TZLC_FLAG (0xE0000000)

.globl __tzlc_send_req
@ int32_t __tzlc_send_req(tzlc_req_param *param);
__tzlc_send_req:
	stmdb sp!, {r1-r12, lr}
	ldmia r0, {r1-r6}
	mov r7, r0
	ldr r0, =TZLC_FLAG
	.arch_extension sec
	smc #0
	cmp r8, #0
	stmneia r7, {r1-r6}
	ldmia sp!, {r1-r12, pc}
