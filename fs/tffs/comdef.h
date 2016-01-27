/*
 * comdef.h
 *
 * Common definiation of TFFS.
 * head file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _COM_DEF_H
#define _COM_DEF_H

/*
 * type redefiniation
 */
typedef char	byte;
typedef short	int16;
typedef int		int32;

typedef unsigned char	ubyte;
typedef unsigned short	uint16;
typedef unsigned int	uint32;

#define IN
#define OUT
#define INOUT

#ifndef NULL
#define NULL	(0)
#endif

#ifndef TRUE
#define TRUE	(1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif

#ifndef BOOL
#define BOOL int32
#endif

/* Marvell fixed: TFFS_FAT_CACHE */
#define TFFS_FAT_CACHE 32
#define TFFS_FILE_CACHE 0
#define TFFS_FAT_MIRROR /* Sync FAT1 to FAT0 */
#endif
