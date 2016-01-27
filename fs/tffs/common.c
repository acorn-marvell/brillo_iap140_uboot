/*
 * common.c
 *
 * Include routins to implemate common function.
 * implementation file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "common.h"

uint32
clus2sec(
	IN	tffs_t * ptffs,
	IN	uint32 clus)
{
	if (clus == ROOT_DIR_CLUS_FAT16) {
		return ptffs->sec_root_dir;
	}
	else {
		return ((clus - 2) * ptffs->pbs->sec_per_clus + ptffs->sec_first_data);
	}
}

uint32
copy_from_unicode(
    IN  uint16 * psrc,
    IN  uint32 len,
    OUT byte * pdst)
{
	uint32 i;

	for (i = 0; i < len; i++) {
		/* Marvell fixed: byte access to avoid misaligned 16-bit access */
		pdst[i] = *((byte *)&psrc[i]);
	}
	return i;
}

uint32
copy_to_unicode(
	IN	ubyte * psrc,
	IN	uint32 len,
	OUT	uint16 * pdst)
{
	uint32 i;

	for (i = 0; i < len; i++) {
		/* Marvell fixed: byte access to avoid misaligned 16-bit access */
		byte *pdstb = (byte *)&pdst[i];
		if (psrc[i] == 0xFF) {
			pdstb[0] = pdstb[1] = 0xFF;
		}
		else {
			pdstb[0] = psrc[i];
			pdstb[1] = 0;
		}
	}
	return i;
}

uint32
tokenize(
	IN	byte * string,
	IN	byte sep,
	OUT	byte * tokens[])
{
	byte * pcur;
	uint32 i;

	pcur = string;
	i = 0;
	if (*pcur == sep)
		pcur++;

	if (*pcur != '\0') {
		tokens[i] = pcur;
		i++;
	}
	while (*pcur != '\0') {
		if (*pcur == sep) {
			*pcur = '\0';
			if (*(pcur + 1) != '\0') {
				tokens[i] = pcur + 1;
				i++;
			}
		}
		pcur++;
	}
	return i;
}

byte *
dup_string(
	IN	byte * pstr)
{
	byte * pret;

	pret = (byte *)Malloc(Strlen(pstr) + 1);
	Strcpy(pret, pstr);

	return pret;
}

void
trip_blanks(
	IN	byte * pstr)
{
	byte * pread, * pwrite;

	pwrite = pstr;
	pread = pstr;

	while (*pread != '\0') {
		if (*pread == ' ') {
			pread++;
			continue;
		}

		*pwrite++ = *pread++;
	}
}

BOOL
divide_path(
	IN	byte * file_path,
	OUT	byte * pfname)
{
	byte * pcur;

	pcur = file_path + Strlen(file_path);
	while (pcur >= file_path && *pcur != '/')
		pcur--;

	Strcpy(pfname, pcur + 1);
	*(pcur + 1) = '\0';

	return TRUE;
}

