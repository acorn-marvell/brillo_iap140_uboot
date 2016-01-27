/*
 * debug.c
 *
 * debug functions
 * implementation file
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "pubstruct.h"
#include "debug.h"

#define SECTOR_SIZE		512

void
print_sector(ubyte * secbuf, ubyte bychar)
{
	int i;

	printf("0x%04x ", 0);
	for (i = 0; i < SECTOR_SIZE; i++) {
		if (bychar) {
			printf("%2c ", secbuf[i]);
		}
		else {
			printf("%02x ", secbuf[i]);
		}

		if ((i + 1) % 16 == 0)
			printf("\n0x%04x ", i);
	}
	printf("\n");
}
