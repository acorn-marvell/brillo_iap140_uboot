/*
 * Copyright 2011, Marvell Semiconductor Inc.
 * Lei Wen <leiwen@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>

#include <command.h>
#include <environment.h>
#include <sparse_format.h>
#include <linux/stddef.h>

#define SPARSE_HEADER_MAJOR_VER 1
#define SECTOR_SIZE	512

int unsparse(block_dev_desc_t *dev, unsigned long from, u64 to, u64 sz)
{
	sparse_header_t *header = (sparse_header_t *)from;
	u32 i;
	u64 outlen = 0;

	if ((u64)(header->total_blks * header->blk_sz) > sz) {
		printf("sparse: section size %llu MB limit: exceeded\n",
		       (unsigned long long)sz / (1024*1024));
		return 1;
	}

	if (header->magic != SPARSE_HEADER_MAGIC) {
		printf("sparse: bad magic\n");
		return 1;
	}

	if ((header->major_version != SPARSE_HEADER_MAJOR_VER) ||
	    (header->file_hdr_sz != sizeof(sparse_header_t)) ||
	    (header->chunk_hdr_sz != sizeof(chunk_header_t))) {
		printf("sparse: incompatible format\n");
		return 1;
	}
	/* todo: ensure image will fit */

	/* Skip the header now */
	from += header->file_hdr_sz;

	for (i = 0; i < header->total_chunks; i++) {
		u64 len = 0;
		int r;
		chunk_header_t *chunk = (void *)from;

		printf(".");

		/* move to next chunk */
		from += sizeof(chunk_header_t);

		switch (chunk->chunk_type) {
		case CHUNK_TYPE_RAW:
			len = chunk->chunk_sz * header->blk_sz;

			if (chunk->total_sz != (len + sizeof(chunk_header_t))) {
				printf("sparse: bad chunk size for chunk %d, type Raw\n",
				       i);
				return 1;
			}

			outlen += len;
			if (outlen > sz) {
				printf("sparse: section size %llu MB limit: exceeded\n",
				       (unsigned long long)sz / (1024*1024));
				return 1;
			}
#ifdef DEBUG
			printf("sparse: RAW blk=%u bsz=%u: write(sector=%lu,len=%llu)\n",
			       chunk->chunk_sz, header->blk_sz,
			       from, (unsigned long long)len);
#endif
			r = dev->block_write(dev->dev, to / SECTOR_SIZE,
				len / SECTOR_SIZE, (const void *)from);
			if (r <= 0) {
				printf("sparse: mmc write failed\n");
				return 1;
			}

			to += len;
			from += len;
			break;

		case CHUNK_TYPE_DONT_CARE:
			if (chunk->total_sz != sizeof(chunk_header_t)) {
				printf("sparse: bogus DONT CARE chunk\n");
				return 1;
			}
			len = chunk->chunk_sz * header->blk_sz;
#ifdef DEBUG
			printf("sparse: DONT_CARE blk=%u bsz=%u: skip(sector=%llu,len=%llu)\n",
			       chunk->chunk_sz, header->blk_sz,
			       (unsigned long long)to, (unsigned long long)len);
#endif

			outlen += len;
			if (outlen > sz) {
				printf("sparse: section size %llu MB limit: exceeded\n",
				       (unsigned long long)sz/(1024*1024));
				return 1;
			}
			to += len;
			break;

		default:
			printf("sparse: unknown chunk ID %04x\n", chunk->chunk_type);
			return 1;
		}
	}

	printf("\nsparse: out-length-0x%llu MB\n",
	       (unsigned long long)outlen/(1024*1024));
	return 0;
}

static int do_unsparse(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	unsigned long addr;
	unsigned long long size, offset;
	block_dev_desc_t *dev_desc = NULL;
	int dev = 0;
	char *ep;

	if (argc < 5)
		return CMD_RET_USAGE;

	dev = (int)simple_strtoul(argv[2], &ep, 16);
	dev_desc = get_dev(argv[1], dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid boot device **\n");
		return 1;
	}
	addr = simple_strtoul(argv[3], NULL, 16);
	offset = simple_strtoull(argv[4], NULL, 16);
	size = simple_strtoull(argv[5], NULL, 16);

	return unsparse(dev_desc, addr, offset, size);
}

U_BOOT_CMD(
	unsparse,       6,      0,      do_unsparse,
	"write sparsed file into block device",
	"<interface> <dev> <ram addr> <block offset> <size>\n"
	"    - unsparse sparsed image from the address 'addr' in RAM\n"
	"      to 'dev' on 'interface'"
);
