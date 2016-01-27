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

.globl __tzlc_send_req
    /* int32_t __tzlc_send_req(tzlc_req_param *param); */
__tzlc_send_req:
    str x1, [sp]
    stp x2, x3, [sp, #8]
    stp x4, x5, [sp, #24]
    stp x6, x7, [sp, #40]
    stp x8, x9, [sp, #56]
    stp x10, x11, [sp, #72]
    stp x12, x13, [sp, #88]
    stp x14, x15, [sp, #104]
    stp x16, x17, [sp, #120]
    /* other registers stay unchanged */

    mov x18, x0 /* save x0 */
    ldp w1, w2, [x18]
    ldp w3, w4, [x18, #4]
    ldp w5, w6, [x18, #8]

    /* SMC64: CPU Service Calls */
    ldr x0, =0x00000000C1000000
    smc #0

	/* x1 is the flag to indicate if updating params */
    cmp x1, xzr
    beq _restore
    stp w1, w2, [x18]
    stp w3, w4, [x18, #4]
    stp w5, w6, [x18, #8]
_restore:
    ldp x16, x17, [sp, #120]
    ldp x14, x15, [sp, #104]
    ldp x12, x13, [sp, #88]
    ldp x10, x11, [sp, #72]
    ldp x8, x9, [sp, #56]
    ldp x6, x7, [sp, #40]
    ldp x4, x5, [sp, #24]
    ldp x2, x3, [sp, #8]
    ldr x1, [sp]

    ret
