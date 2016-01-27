/*
 * cache.c
 *
 * Include routines to caching the reading and writing the sectors.
 * we use the "Least Recently Used" algorithm as our cache algorithm
 * that discards the least recently used items. By building a static
 * double list in an array to track the least recently used item.
 * implimentation file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "pubstruct.h"
#include "debug.h"
#include "cache.h"

#ifndef DBG_CACHE
#undef DBG
#define DBG nulldbg
#endif

/* Private interface declaration. */

static BOOL
_find_in_list(
	IN	tcache_t * pcache,
	IN	uint32 sec,
	OUT	int16 * pret
);

static int16
_hash_function(
	IN	tcache_t * pcache,
	IN	uint32 sec
);

static void
_del_from_list(
	IN	tcache_t * pcache,
	IN	int16 sec_index
);

static void
_insert_into_list(
	IN	tcache_t * pcache,
	IN	int16 sec_index
);

static int16
_do_cache_miss(
	IN	tcache_t * pcache,
	IN	int16 secindex,
	IN	uint32 sec,
	IN	BOOL is_read /* Marvell fixed */
);

static void
_do_cache_hint(
	IN	tcache_t * pcache,
	IN	int16 secindex
);

/*----------------------------------------------------------------------------------------------------*/

tcache_t *
cache_init(
	IN	tdev_handle_t hdev,
	IN	uint32 seccnt,
	IN	uint32 sector_size)
{
	tcache_t * pcache;
	int16 seci;

	pcache = (tcache_t *)Malloc(sizeof(tcache_t));
	pcache->seccnt = seccnt;
	pcache->hdev = hdev;
	pcache->sector_size = sector_size;
	if (seccnt > 0) {
		pcache->secbufs = (sec_buf_t *)Malloc(sizeof(sec_buf_t) * seccnt);
		if (pcache->secbufs == NULL) {
			Free(pcache);
			return NULL;
		}

		Memset(pcache->secbufs, 0, sizeof(sec_buf_t) * seccnt);
		for (seci = 0; seci < pcache->seccnt; seci++) {
			pcache->secbufs[seci].secbuf = (ubyte *)Malloc(pcache->sector_size);
			pcache->secbufs[seci].sec = 0;
		}
		pcache->head = SECBUF_EOF;
		pcache->tail = SECBUF_EOF;

		pcache->use_cache = 1;
	}
	else {
		pcache->use_cache = 0;
	}

	return pcache;
}

int32
cache_readsector(
    IN  tcache_t * pcache,
    IN  int32 addr,
    OUT ubyte * ptr)
{
	int16 secindex;
	int32 ret;

	ret = CACHE_OK;

	if (!pcache->use_cache) {
		return HAI_readsector(pcache->hdev, addr, ptr);
	}

	if (!_find_in_list(pcache, addr, &secindex)) {
		secindex = _do_cache_miss(pcache, secindex, addr, TRUE);
		if (secindex < 0)
			ret = secindex;

	}
	else {
		_do_cache_hint(pcache, secindex);
	}

	if (ret >= 0) {
		Memcpy(ptr, pcache->secbufs[secindex].secbuf, pcache->sector_size);
	}

	return ret;
}

int32
cache_writesector(
    IN  tcache_t * pcache,
    IN  int32 addr,
    IN  ubyte * ptr)
{
	int16 secindex;
	int32 ret;

	ret = CACHE_OK;

	if (!pcache->use_cache) {
		/* Marvell fixed: was HAI_readsector */
		return HAI_writesector(pcache->hdev, addr, ptr);
	}

	if (!_find_in_list(pcache, addr, &secindex)) {
		secindex = _do_cache_miss(pcache, secindex, addr, FALSE);
		if (secindex < 0)
			ret = secindex;

	}
	else {
		_do_cache_hint(pcache, secindex);
	}

	if (ret >= 0) {
		Memcpy(pcache->secbufs[secindex].secbuf, ptr, pcache->sector_size);
		pcache->secbufs[secindex].is_dirty = 1;
	}

	return ret;
}

int32
cache_destroy(
	IN	tcache_t * pcache)
{
	int32 ret;
	int16 seci;

	ret = CACHE_OK;
	if (pcache->use_cache) {
		for (seci = 0; seci < pcache->seccnt; seci++) {
			if (pcache->secbufs[seci].is_dirty) {
				DBG("%s:write sector %d\n", __FUNCTION__, seci);
				if (HAI_writesector(pcache->hdev, pcache->secbufs[seci].sec,
						pcache->secbufs[seci].secbuf) != HAI_OK) {
					ret = ERR_CACHE_HARDWARE_FAIL;
					break;
				}
			}
		}

		for (seci = 0; seci < pcache->seccnt; seci++) {
			Free(pcache->secbufs[seci].secbuf);
		}

		Free(pcache->secbufs);
	}

	Free(pcache);

	return ret;
}

/*----------------------------------------------------------------------------------------------------*/

int16
_do_cache_miss(
	IN	tcache_t * pcache,
	IN	int16 secindex,
	IN	uint32 sec,
	IN	BOOL is_read) /* Marvell fixed: prevent sector read on write miss */
{
	int16 ret_sec_index;

	ret_sec_index = secindex;
	if (secindex == pcache->seccnt) {
		/* The cache list is full, we have to get the last element to swap to disk. */
		DBG("%s:cache is full, del %d from cache.\n", __FUNCTION__, pcache->tail);
		if (pcache->secbufs[pcache->tail].is_dirty) {
			if (HAI_writesector(pcache->hdev, pcache->secbufs[pcache->tail].sec,
					pcache->secbufs[pcache->tail].secbuf) != HAI_OK) {
				ret_sec_index = ERR_CACHE_HARDWARE_FAIL;
			}
		}
		secindex = pcache->tail;
		_del_from_list(pcache, pcache->tail);
	}

	if (ret_sec_index >= 0 && (!is_read || (HAI_readsector(pcache->hdev, sec,
			pcache->secbufs[secindex].secbuf) == HAI_OK))) {
		pcache->secbufs[secindex].sec = sec;
		pcache->secbufs[secindex].is_dirty = 0;
		_insert_into_list(pcache, secindex);
		ret_sec_index = secindex;
	}
	else {
		ret_sec_index = ERR_CACHE_HARDWARE_FAIL;
	}

	return ret_sec_index;
}

void
_do_cache_hint(
	IN	tcache_t * pcache,
	IN	int16 secindex)
{
	_del_from_list(pcache, secindex);
	_insert_into_list(pcache, secindex);
}

void
_insert_into_list(
	IN	tcache_t * pcache,
	IN	int16 sec_index)
{
	if (pcache->head == SECBUF_EOF) {
		/* list is empty. */
		pcache->secbufs[sec_index].pre = SECBUF_EOF;
		pcache->secbufs[sec_index].next = SECBUF_EOF;
		pcache->head = sec_index;
		pcache->tail = sec_index;
	}
	else {
		/* insert into the head of the list. */
		pcache->secbufs[sec_index].pre = SECBUF_EOF;
		pcache->secbufs[sec_index].next = pcache->head;
		pcache->secbufs[pcache->head].pre = sec_index;
		pcache->head = sec_index;
	}
}

void
_del_from_list(
	IN	tcache_t * pcache,
	IN	int16 sec_index)
{
	//ASSERT(pcache->secbufs[sec_index].sec != 0);

	DBG("%s:sec_index = %d\n", __FUNCTION__, sec_index);
	if (pcache->secbufs[sec_index].pre == SECBUF_EOF) {
		/* the element is the first one. */
		pcache->head = pcache->secbufs[sec_index].next;
	}
	else {
		pcache->secbufs[pcache->secbufs[sec_index].pre].next = pcache->secbufs[sec_index].next;
	}

	if (pcache->secbufs[sec_index].next != SECBUF_EOF) {
		/* modify the next element's pre field only when next element is not EOF. */
		pcache->secbufs[pcache->secbufs[sec_index].next].pre = pcache->secbufs[sec_index].pre;
	}
	else {
		pcache->tail = pcache->secbufs[sec_index].pre;
	}
}

int16
_hash_function(
	IN	tcache_t * pcache,
	IN	uint32 sec)
{
	return sec % pcache->seccnt;
}

BOOL
_find_in_list(
	IN	tcache_t * pcache,
	IN	uint32 sec,
	OUT	int16 * pret)
{
	int16 start_index;
	BOOL is_found;
	int16 cur_index;

	start_index = _hash_function(pcache, sec);
	cur_index = start_index;
	while (1) {
		DBG("%s:cur_index = %d\n", __FUNCTION__, cur_index);
		if (pcache->secbufs[cur_index].sec == sec) {
			is_found = TRUE;
			*pret = cur_index;
			break;
		}
		else if (pcache->secbufs[cur_index].sec == 0) {
			is_found = FALSE;
			*pret = cur_index;
			break;
		}

		cur_index++;
		cur_index = cur_index % pcache->seccnt;
		if (cur_index == start_index) {
			is_found = FALSE;
			*pret = pcache->seccnt;
			break;
		}
	}

	return is_found;
}

