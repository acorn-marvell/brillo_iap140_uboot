/*
 * Copyright 2011, Marvell Semiconductor Inc.
 * Lei Wen <leiwen@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SPARSE_FORMAT_H_
#define _SPARSE_FORMAT_H_

typedef struct sparse_header {
	__le32	magic;		/* 0xed26ff3a */
	__le16	major_version;	/* (0x1) - reject images with higher major versions */
	__le16	minor_version;	/* (0x0) - allow images with higer minor versions */
	__le16	file_hdr_sz;	/* 28 bytes for first revision of the file format */
	__le16	chunk_hdr_sz;	/* 12 bytes for first revision of the file format */
	__le32	blk_sz;		/* block size in bytes, must be a multiple of 4 (4096) */
	__le32	total_blks;	/* total blocks in the non-sparse output image */
	__le32	total_chunks;	/* total chunks in the sparse input image */
	__le32	image_checksum; /* CRC32 checksum of the original data, counting "don't care" */
	/* as 0. Standard 802.3 polynomial, use a Public Domain */
	/* table implementation */
//	__le32	reserved1;
} sparse_header_t;

#define SPARSE_HEADER_MAGIC	0xed26ff3a

#define CHUNK_TYPE_RAW		0xCAC1
#define CHUNK_TYPE_FILL		0xCAC2
#define CHUNK_TYPE_DONT_CARE	0xCAC3
#define CHUNK_TYPE_CRC32        0xCAC4

typedef struct chunk_header {
	__le16	chunk_type;	/* 0xCAC1 -> raw; 0xCAC2 -> fill; 0xCAC3 -> don't care */
	__le16	reserved1;
//	__le32	reserved2;
	__le32	chunk_sz;	/* in blocks in output image */
	__le32	total_sz;	/* in bytes of chunk input file including chunk header and data */
} chunk_header_t;
#endif
