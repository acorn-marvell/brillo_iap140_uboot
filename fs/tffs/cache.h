/*
 * cache.h
 *
 * Include routines to caching the reading and writing the sectors.
 * head file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _TFFS_CACHE_H
#define _TFFS_CACHE_H

#include "comdef.h"
#include "hai.h"

#define CACHE_OK					(1)
#define ERR_CACHE_HARDWARE_FAIL		(-1)

#define SECBUF_EOF		(-1)

typedef struct _sec_buf{
	ubyte * secbuf;
	uint32 sec;
	ubyte is_dirty;
	int16 pre;
	int16 next;
}sec_buf_t;

typedef struct _tcache {
	tdev_handle_t hdev;
	sec_buf_t * secbufs;
	int16 use_cache;
	int16 seccnt;
	uint32 sector_size;
	int16 head;
	int16 tail;
}tcache_t;

tcache_t *
cache_init(
	IN	tdev_handle_t hdev,
	IN	uint32 seccnt,
	IN	uint32 sector_size
);

int32
cache_readsector(
    IN  tcache_t * pcache,
    IN  int32 addr,
    OUT ubyte * ptr
);

int32
cache_writesector(
    IN  tcache_t * pcache,
    IN  int32 addr,
    IN  ubyte * ptr
);

int32
cache_destroy(
	IN	tcache_t * pcache
);

#endif
