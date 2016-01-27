/*
 * crtdef.c
 *
 * C Run-Time Libraries functions definitions
 * implimentation file
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "crtdef.h"

void
Getcurtime(tffs_sys_time_t * ptime)
{
#if(0)
	time_t rawtime;
	struct tm * curtm;

	time(&rawtime);
	curtm = localtime(&rawtime);

	(ptime)->tm_sec = curtm->tm_sec;
	(ptime)->tm_min = curtm->tm_min;
	(ptime)->tm_hour = curtm->tm_hour;
	(ptime)->tm_mday = curtm->tm_mday;
	(ptime)->tm_mon = curtm->tm_mon;
	(ptime)->tm_year = curtm->tm_year;
#else
	/* Marvell fixed: no RTC in uboot for now */
	memset((void*)ptime, 0, sizeof(*ptime));
#endif
}
