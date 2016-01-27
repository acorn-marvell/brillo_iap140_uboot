/*
 *  linux/arch/arm/mach-pxa/ramdump_defs.h
 *
 *  Support for the Marvell PXA RAMDUMP error handling capability.
 *
 *  Author:     Anton Eidelman (anton.eidelman@marvell.com)
 *  Created:    May 20, 2010
 *  Copyright:  (C) Copyright 2006 Marvell International Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef ARCH_ARM_MACH_PXA_RAMDUMP_DEFS_H
#define ARCH_ARM_MACH_PXA_RAMDUMP_DEFS_H
/************************************************************************/
/*				RAMDUMP definitions			*/
/************************************************************************/
/* RDC header is at fixed address adjacent to the top of DDR space */
#define RDC_SIGNATURE   0x41434452 /* ascii(RDCA), little endian */
#define RDC_OFFSET	0x002FFC00 /* from CP area start */
#define ISRAM_SIZE	0x40000
#define ISRAM_PA	0x5c000000

#define DDR0_BASE 0x00000000
#define DDR1_BASE 0x40000000
#define CP_AREA_SIZE 0x01000000 /* fixed top 16MB of bank 0 */

/* Don't use CP_ADDRESS, as it's different between architectures */
#define CP_ADDRESS(ddr0_size) (DDR0_BASE+(ddr0_size)-CP_AREA_SIZE)
#define RDC_ADDRESS(ddr0_size) (CP_ADDRESS(ddr0_size)+RDC_OFFSET)

#define RDC_ADDRESS_CPA(cp_addr) (cp_addr + RDC_OFFSET)

/* ISRAM dump is located before RDC header. More objects can be added here.
	Not included into the struct rdc_area as the size of isram and
	other objects might not be known at compile time.
	Macro's below may use function calls or variable references.*/
#define RDC_HEADROOM (ISRAM_SIZE)
#define RDC_START(header) ((void *)(((unsigned)header)-RDC_HEADROOM))
#define RDC_HEAD(start) ((struct rdc_area *)(((unsigned)start)+RDC_HEADROOM))
#define RDC_ISRAM_START(header) ((void *)(((unsigned)header)-ISRAM_SIZE))

/* Data Item objects in RDC */
#define MAX_RDI_NAME	8
struct rdc_dataitem {
	unsigned char	size;	/* total bytes to the next item */
	unsigned char	type;	/* one of enum rdc_di_type */
	unsigned short	attrbits;/* bit per body element, lsb first */
	char		name[MAX_RDI_NAME];/* 8-char ascii item name */
	union {
		unsigned w[1];	/* contents: not aligned to 64-bit */
	} body;
};

/* RAMFILE object descriptor */
#define RAMFILE_PHYCONT 1 /* physical memory is continuous (kmalloc) */
struct ramfile_desc {
	unsigned next; /* next object (pa) or NULL */
	unsigned payload_size; /* bytes, excluding this header */
	unsigned flags;
	unsigned reserved[5];
};

/* RDC: the location is fixed at RDC_ADDRESS. Size is 1KB. */
struct rdc_area {
	struct rdc_header {
		unsigned signature;
		unsigned kernel_build_id;
		unsigned error_id;
		unsigned reserved[5];
		unsigned ramdump_data_addr; /* physical addr of ramdump_data */
		unsigned isram_pa; /* physical addr of ISRAM dump */
		unsigned isram_size; /* size of ISRAM dump */
		unsigned isram_crc32; /* verify contents survived flush/reset */
		unsigned ramfile_addr; /* physical addr of the first or NULL */
		/* mipsram is here so it can be extracted w/o symbol table */
		unsigned mipsram_pa; /* physical addr of mipsram buffer */
		unsigned mipsram_size; /* size of mipsram buffer */
		unsigned ddr_bank0_size; /* for future use in RDP */
		unsigned pgd; /* init_mm.pgd  pa: translate vmalloc addresses */
		unsigned kallsyms; /* VA of struct kallsyms_record */
		unsigned kallsyms_hi; /* upper 32 bit of the above */
		unsigned kernel_build_id_hi; /* upper 32 bit of the above */
		unsigned reserved1[12]; /* Up to offset 0x80 */
	} header;
	union { /*upto 1KB*/
		unsigned char space[0x400-sizeof(struct rdc_header)];
		struct rdc_dataitem rdi[1];
	} body;
};

/* use this for debug and memory consistency checking.
   Note: CRC32 no 2-s complement option is used */
#define RAMDUMP_ISRAM_CRC_CHECK

/* Error_id values:
[0xffff..0]  is used for ARM die() err codes, see fault.c/trapc.c */
#define RAMDUMP_ERR_EEH_CP 0x80000000
#define RAMDUMP_ERR_EEH_AP 0x80000100
#define RAMDUMP_ERR_NONE   0x8FFFFFFF


/* Functions: */
int ramdump_attach_ramfile(struct ramfile_desc *desc);

#define RAMDUMP_PHASE_1_1 /* allow KO's to work with older kernel */
#endif
