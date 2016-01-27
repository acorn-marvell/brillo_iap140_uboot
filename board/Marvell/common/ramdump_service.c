/*
 * ramdump.c: PXA9XX ramdump support
 *
 * Copyright (C) 2008 MARVELL Corporation (antone@marvell.com)
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <config.h>
#include <linux/ctype.h>

#ifdef CONFIG_CMD_FAT
#include <fat.h>
#endif

#ifdef IH_ARCH_ALTERNATE
#undef CONFIG_ARMV8
#endif

#ifdef CONFIG_GZIP_COMPRESSED
#define CONFIG_ZLIB_DEFLATE
#endif

#ifdef CONFIG_RAMDUMP_TFFS
#ifndef CONFIG_FS_TFFS
#error CONFIG_RAMDUMP_TFFS requires CONFIG_FS_TFFS
#endif
#undef CONFIG_YAFFS2
#undef CONFIG_RD_RAM
#include "../../../fs/tffs/tffs.h"
#include "../../../fs/tffs/hai_dev.h"
#endif/*CONFIG_RAMDUMP_TFFS*/

#ifdef CONFIG_RAMDUMP_YAFFS2
#include "../../../fs/yaffs2/yaffsfs.h"
#ifndef CONFIG_YAFFS2
#error CONFIG_RAMDUMP_YAFFS2 requires CONFIG_YAFFS2
#endif
#undef CONFIG_FS_TFFS
#undef CONFIG_RD_RAM
#endif

#ifdef CONFIG_RD_RAM
#ifndef CONFIG_ZLIB_DEFLATE
#error CONFIG_RD_RAM requires CONFIG_ZLIB_DEFLATE
#endif
#undef CONFIG_FS_TFFS
#undef CONFIG_YAFFS2
#endif

#include <asm/byteorder.h>
#include <part.h>
#include <exports.h> /* do_reset() */
#include <asm/errno.h>

#ifdef CONFIG_ZLIB_DEFLATE
#include <malloc.h>
#include "../../../lib/zlib/zlib.h"
#endif
#include "ramdump_defs.h"
#include "ramdump.h"

/* #define RAMDUMP_DRYRUN */
#define RD_MAX_FILE_NAME_CHARS 40
#define RD_FILE_INDEX_CHARS 4 /* e.g. RAMDUMP0123.gz */

/* Might be set by default, but should not force the obsolete no FS mode */
#undef CONFIG_CMD_FAT

#if defined(CONFIG_FS_TFFS)
#define CONFIG_FS_PRESENT /* we're working with some filesystem */
#define CONFIG_RD_MMC
#define FS_HANDLE	tffs_handle_t
#define FILE_HANDLE	tfile_handle_t
#define DEV_HANDLE	struct tdev_t
#elif defined(CONFIG_YAFFS2)
#define CONFIG_FS_PRESENT /* we're working with some filesystem */
#define FS_HANDLE	void *
#define FILE_HANDLE	int
#define DEV_HANDLE	void *
#endif

struct ramdump_stream {
#ifdef CONFIG_CMD_FAT
	block_dev_desc_t	*dev_desc;
	int			part;
	fsdata			fsdata;
	dir_entry		dentry;
	__u32			curcluster;
#else
	FS_HANDLE		htffs;
	DEV_HANDLE		tdev;
	FILE_HANDLE		hfile;
#endif
	char			fname[RD_MAX_FILE_NAME_CHARS];
};
/* prototypes */
static int ramdump_file_write(const char *name,
			const struct rd_desc *desc, const char *text);
static int ramdump_getddrsize(unsigned *pbank0, unsigned *pbank1);
static int ramdump_getcpmem(unsigned *pcpaddr, unsigned *pcpsize);
static int ramdump_get_desc(struct rd_desc *desc,
			unsigned ddr0_size, unsigned ddr1_size);
static int ramdump_write_elf_header(struct ramdump_stream *rs,
			const struct rd_desc *desc);

static const char *rd_dir = "/rdlog";
static const char *ramdump_file_name =
#ifdef CONFIG_FS_TFFS
#ifdef CONFIG_ZLIB_DEFLATE
"RAMDUMP.gz";
#else
"RAMDUMP.bin";
#endif
#elif defined(CONFIG_YAFFS2)
#ifdef CONFIG_ZLIB_DEFLATE
"/flash/RAMDUMP.gz";
#else
"/flash/RAMDUMP.bin";
#endif
#define MOUNTPATH "/flash"
#else
"RAMDUMP.bin";
#endif
int ramdump_ext(unsigned ddr0_size, unsigned ddr1_size, int do_dump,
		unsigned mode)
{
	struct rdc_area *rdc;
	unsigned crc;
	int	ret = -1;
	struct rd_desc desc;

	if (ramdump_get_desc(&desc, ddr0_size, ddr1_size))
		return ret;
	rdc = (struct rdc_area *)desc.rdc_addr;
	printf("RAMDUMP: RDC=%lx: ", (ulong)rdc);
	if (rdc->header.signature != RDC_SIGNATURE) {
		printf("no signature found\n");
	} else {
		printf("signature found\n");
		do_dump = 3;
		crc = crc32_no_comp(0,
			(unsigned char *)(ulong)rdc->header.isram_pa,
			rdc->header.isram_size);
		printf("ISRAM area CRC: expected 0x%.8x, actual 0x%.8x\n",
			rdc->header.isram_crc32, crc);
		if (crc == rdc->header.isram_crc32)
			do_dump = 2;
	}
	if (do_dump) {
		/* Get error text from struct ramdump_state */
		const char *text = (rdc->header.signature == RDC_SIGNATURE) ?
			(const char *)(ulong)
			(rdc->header.ramdump_data_addr + 8) : "none";
		/* On architectures with low bank 0 location of CP area and RDC
		RDC cannot be used in rdp (parser) to detect the bank size:
		supply this info explicitely for rdp use */
		rdc->header.ddr_bank0_size = ddr0_size;

		ret = ramdump_file_write(ramdump_file_name, &desc, text);

		if ((ret && !(mode & RD_MODE_RESET_ON_FAILURE)) ||
		    (!ret && !(mode & RD_MODE_RESET_ON_SUCCESS)))
			return ret;
		/* clear ramdump indication for next time */
		/* do this regardless of the success/failure (e.g. no MMC)
			to prevent endless loop of attempts and resets */
		if ((rdc->header.signature == RDC_SIGNATURE) ||
		    (mode & RD_MODE_RESET_ON_FORCED)) {
			rdc->header.signature = 0;
			flush_dcache_range((unsigned long)&rdc->header.signature,
					   (unsigned long)&rdc->header.signature + 1);
			/* normal case, not forced from cmd */
			/* reset because OBM does not load zImage when RAMDUMP is pending */

			/* After data write to SD, the internal SD HW-controller
			 * needs some time for internal handling. Delay the reset.
			 */
			udelay(25/*MS*/ * 1000);
			reset_cpu(0);
		}
	}
	return ret;
}

int ramdump(unsigned ddr0_size, unsigned ddr1_size, int do_dump)
{
	return ramdump_ext(ddr0_size, ddr1_size, do_dump,
			RD_MODE_DEFAULT);
}

#ifdef CONFIG_ZLIB_DEFLATE
static void *ramdump_z_calloc(void *opaque, unsigned items, unsigned size)
{
	unsigned total = items*size;
	return malloc(total);
}

static void ramdump_z_free(void *opaque, void *p, unsigned unused)
{
	(void)unused;
	free(p);
}

#ifndef RAMDUMP_PROGRESS_PRINT_IN
#define RAMDUMP_PROGRESS_PRINT_IN 0x01000000
#endif
static void ramdump_z_print_status(z_stream *strm)
{
	static unsigned next_print_total;
	if (strm->avail_in)
		if ((unsigned)strm->total_in < next_print_total)
			return;
		else
			next_print_total = (unsigned)strm->total_in
				+ RAMDUMP_PROGRESS_PRINT_IN;
	else
		next_print_total = 0;
	printf("IN: %u OUT: %u RATIO: %d%%\n",
	       (unsigned)strm->total_in, (unsigned)strm->total_out,
	       (unsigned)((strm->total_out/1024)*100/(strm->total_in/1024 + 1)));
}
#endif

#if defined(CONFIG_FS_PRESENT) && !defined(CONFIG_RAMDUMP_SINGLE)
static int ramdump_get_fnum(const char *name, const char *base, int prefix_len)
{
	char sindex[RD_FILE_INDEX_CHARS+1];
	int i;
	if (strncmp(name, base, prefix_len))
		/* Prefix does not match */
		return -1;
	name += prefix_len;
	base += prefix_len;
	for (i = 0; i < RD_FILE_INDEX_CHARS; i++) {
		sindex[i] = name[i];
		if (!isdigit(sindex[i]))
			return -1;
	}

	if (strcmp(&name[i], &base[0]))
		/* Extension does not match */
		return -1;
	sindex[i] = 0;
	return simple_strtoul(sindex, NULL, 10);
}

static void ramdump_set_fnum(char *name, const char *base, int fnum)
{
	int i, extidx;
	char sindex[1+11]; /* handle larger numbers upto 32 bit */

	for (i = extidx = 0; base[i]; i++) {
		name[i] = base[i];
		if (base[i] == '.')
			extidx = i;
	}
	sprintf(sindex, "%.11d", fnum); /* fnum might be actually larger */
	i = strlen(sindex) - RD_FILE_INDEX_CHARS; /* mod in string form */
	strcpy(&name[extidx], &sindex[i]);
	strcat(name, &base[extidx]);
}
/* Account for all existing RAMDUMP files and assign a name for the new one */
static void ramdump_enumerate_files(FS_HANDLE htffs, const char *base,
					int *pfnum_min, int *pfnum_max)
{
	char fname[RD_MAX_FILE_NAME_CHARS];
#if defined(CONFIG_FS_TFFS)
	tdir_handle_t	hdir;
	dirent_t        de;
#elif defined(CONFIG_YAFFS2)
	struct yaffs_dirent *de;
	yaffs_DIR *hdir;
#else
#error Unsupported FS
#endif
	int i, nameidx, extidx, si;
	int fnum, fnum_max = -1, fnum_min = 0x7fffffff;
	/* Get the path only */
	for (i = nameidx = extidx = 0; base[i]; i++) {
		fname[i] = base[i];
		if (base[i] == '/')
			nameidx = i + 1;
		else if (base[i] == '.')
			extidx = i;
	}
	fname[nameidx] = 0;
	if (!extidx)
		extidx = i;
	fname[extidx] = 0;
	si = extidx - nameidx; /* file number start */
#if defined(CONFIG_FS_TFFS)
	if (TFFS_opendir(htffs, fname, &hdir) == TFFS_OK) {
		while (TFFS_readdir(hdir, &de) == TFFS_OK) {
			fnum = ramdump_get_fnum(de.d_name, &base[nameidx], si);
#elif defined(CONFIG_YAFFS2)
	hdir = yaffs_opendir(fname);
	if (hdir) {
		while ((de = yaffs_readdir(hdir)) != 0) {
			fnum = ramdump_get_fnum(de->d_name, &base[nameidx], si);
#else
#error Unsupported FS
#endif
		if (fnum >= 0) {
			if (fnum > fnum_max)
				fnum_max = fnum;
			if (fnum < fnum_min)
				fnum_min = fnum;
		}
	}
#if defined(CONFIG_FS_TFFS)
	TFFS_closedir(hdir);
#elif defined(CONFIG_YAFFS2)
	yaffs_closedir(hdir);
#else
#error Unsupported FS
#endif
	}
	if (pfnum_max)
		*pfnum_max = fnum_max;
	if (pfnum_min)
		*pfnum_min = fnum_min;
}
#endif

#if defined(CONFIG_FS_PRESENT)
int ramdump_nospace_helper(void *arg)
{
	struct ramdump_stream *rs = (struct ramdump_stream *)arg;
	int min_fnum;
	const char *name = ramdump_file_name;
	char fname[RD_MAX_FILE_NAME_CHARS];
	ramdump_enumerate_files(rs->htffs, name, &min_fnum, 0);
	ramdump_set_fnum(fname, name, min_fnum);
	printf("No space for RAMDUMP\n");
	if (strcmp(fname, rs->fname)) {
		/* cannot remove the file currently being written to */
		printf("delete %s ... ", fname);
#if defined(CONFIG_FS_TFFS)
		if (TFFS_rmfile(rs->htffs, fname) == TFFS_OK) {
#elif defined(CONFIG_YAFFS2)
		if (yaffs_unlink(fname) >= 0) {
#else
#error Unsupported FS
#endif
			printf("Deleted %s\n", fname);
			return 0; /* ok */
		} else {
			printf("Failed to delete %s\n", fname);
		}
	}
	return 1;
}
#endif

#if defined(CONFIG_FS_PRESENT)
/* Saves (text) in a file named like fname but with .txt extension */
static int ramdump_save_text(FS_HANDLE htffs, const char *fname,
				const char *text)
{
	char name[RD_MAX_FILE_NAME_CHARS];
	int i, len;
	FILE_HANDLE hfile;

	strcpy(name, fname);
	len = strlen(name);
	/* No extension: append .txt, otherwise replace it with .txt */
	for (i = len - 1; i; i--)
		if (name[i] == '.') {
			name[i] = 0;
			break;
		}
	strcat(name, ".txt");
	printf("Written %s\n", name);
#if defined(CONFIG_FS_TFFS)
	if (TFFS_fopen(htffs, (byte *)name, "w", &hfile) == TFFS_OK) {
		TFFS_fwrite(hfile, strlen(text), (ubyte *)text);
		TFFS_fclose(hfile);
		return 0;
	}
#elif defined(CONFIG_YAFFS2)
	hfile = yaffs_open(name, O_CREAT | O_RDWR | O_TRUNC,
			S_IREAD | S_IWRITE);
	if (hfile >= 0) {
		yaffs_write(hfile, text, strlen(text));
		yaffs_close(hfile);
		return 0;
	}
#else
#error Unsupported FS
#endif
	return 1;
}
#endif
/*
	Stream adaptation
*/
static int ramdump_stream_open(struct ramdump_stream *rs,
				block_dev_desc_t *dev_desc,
				int partition,
				const char *name,
				const char *text)
{
	int ret = 1;
	memset((void *)rs, 0, sizeof(*rs));
#if defined(CONFIG_CMD_FAT)
	rs->dev_desc = dev_desc;
	rs->part = partition;
	rs->curcluster = 0;
	if (!fat_register_device(rs->dev_desc, rs->part)) {
		ret = do_fat_find(name, &rs->fsdata, &rs->dentry, 0);
		printf("RAMDUMP placeholder file on SD: %s\n",
		       ret ? "none" : "found");
	}
#elif defined(CONFIG_FS_TFFS)
	rs->tdev.dev_desc = dev_desc;
	rs->tdev.part_offset = partition;
	if (TFFS_mount((byte *)&rs->tdev, &rs->htffs) == TFFS_OK) {
#if !defined(CONFIG_RAMDUMP_SINGLE)
		int max_fnum;

#ifdef CONFIG_RD_ALWAYS_SAVE_TO_SUBDIR
		/*
		 * This is not enabled, because some implementations want
		 * dumps in SD root, while others prefer a sub-directory.
		 * If rd_dir exists, use it, otherwise use SD root.
		 */
		ret = TFFS_mkdir(rs->htffs, (char *)rd_dir);
		if ((ret != TFFS_OK) && (ret != ERR_TFFS_DIR_ALREADY_EXIST))
			printf("Failed to mkdir %s (%d)\n", rd_dir, ret);
#endif
		ret = TFFS_chdir(rs->htffs, (char *)rd_dir);
		printf("\n===== Logging into SD  <%s>  directory ======\n",
		       ret == TFFS_OK ? rd_dir : "/");

		ramdump_enumerate_files(rs->htffs, name, 0, &max_fnum);
		ramdump_set_fnum(rs->fname, name, max_fnum + 1);
		printf("Writing %s\n", rs->fname);
		TFFS_bind_nospace_handler(rs->htffs, ramdump_nospace_helper, rs);
#else
		strcpy(rs->fname, name);
#endif
		if ((ramdump_save_text(rs->htffs, rs->fname, text) == 0) &&
		    (TFFS_fopen(rs->htffs, (byte *)rs->fname, "w",
				&rs->hfile) == TFFS_OK))
			ret = 0;
	}
#elif defined(CONFIG_YAFFS2)
	yaffs_StartUp();
	if (yaffs_mount(MOUNTPATH) >= 0) {
		int max_fnum;
		ramdump_enumerate_files(rs->htffs, name, 0, &max_fnum);
		ramdump_set_fnum(rs->fname, name, max_fnum + 1);
		printf("Writing %s\n", rs->fname);
		if (ramdump_save_text(rs->htffs, rs->fname, text) == 0) {
			rs->hfile = yaffs_open(rs->fname,
				O_CREAT | O_RDWR | O_TRUNC,
				S_IREAD | S_IWRITE);
			if (rs->hfile >= 0)
				ret = 0;
		}
	}
#else
#error Unsupported FS
#endif
	return ret;
}

static int ramdump_stream_close(struct ramdump_stream *rs)
{
#if defined(CONFIG_FS_TFFS)
	if (rs->hfile)
		TFFS_fclose(rs->hfile);
	TFFS_umount(rs->htffs);
#elif defined(CONFIG_YAFFS2)
	if (rs->hfile >= 0)
		yaffs_close(rs->hfile);
	yaffs_unmount(MOUNTPATH);
#else
#error Unsupported FS
#endif
	return 0;
}

static int ramdump_stream_write(struct ramdump_stream *rs, __u8 *data, unsigned size)
{
#if defined(RAMDUMP_DRYRUN)
	return size;
#elif defined(CONFIG_CMD_FAT)
	return fat_write(&rs->fsdata, &rs->dentry, &rs->curcluster, data, size);
#elif defined(CONFIG_FS_TFFS)
	return TFFS_fwrite(rs->hfile, size, data);
#elif defined(CONFIG_YAFFS2)
	int written, written_r;
	written = yaffs_write(rs->hfile, data, size);
	if ((written < 0) || ((unsigned)written < size)) {
		/* Assume no room and try to recover */
		if (ramdump_nospace_helper(rs))
			return written;
		data += written;
		size -= written;
		written_r = yaffs_write(rs->hfile, data, size);
		return written_r < 0 ? written_r : written + written_r;
	}
#else
	return 0;
#endif
}


static int ramdump_write(struct ramdump_stream *rs, __u8 *data, unsigned size)
{
	int ret;
#ifdef CONFIG_ZLIB_DEFLATE
#define OUTB_SIZE 0x10000
#define OUT_SECTOR 0x200
	static z_stream strm;
	static __u8 *out;
	static unsigned outheld;
	unsigned len;
	int flush;

	if (!strm.total_in) {
		out = (__u8 *)malloc(OUTB_SIZE);
		if (!out)
			return -1;
		/* allocate deflate state */
		strm.zalloc = ramdump_z_calloc;
		strm.zfree = ramdump_z_free;
		strm.opaque = Z_NULL;
		/*
		 * ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
		 * The below sets up gzip compression format
		 * Z_DEFAULT_COMPRESSION is twice slower, which gives
		 * almost the same compression ratio.
		 */
		ret = deflateInit2_(&strm, Z_BEST_SPEED, Z_DEFLATED,
			15 + 16, 8, Z_DEFAULT_STRATEGY,
			ZLIB_VERSION, sizeof(strm));
		if (ret != Z_OK) {
			printf("Failed deflateInit: %d\n", ret);
			return -1;
		}
		outheld = 0;
	}

	strm.next_in = data;
	strm.avail_in = size;
	flush = size ? Z_NO_FLUSH : Z_FINISH;

	do {
		strm.next_out = out+outheld;
		strm.avail_out = OUTB_SIZE-outheld;
		ret = deflate(&strm, flush);
		if (ret == Z_STREAM_ERROR) {
			printf("Deflate error\n");
			ret = -1;
			goto bail;
		}
		len = OUTB_SIZE - strm.avail_out;
		outheld = len%OUT_SECTOR;
		len -= outheld;
		ret = ramdump_stream_write(rs, out, len);
		if (ret != len) {
			printf("Failed to write %d bytes to SD\n", len);
			ret = -1;
			goto bail;
		}
		ramdump_z_print_status(&strm);
		memcpy(out, out+len, outheld); /* bytes not written yet */
	} while (strm.avail_out == 0);
	if (strm.avail_in) {
		printf("Deflate failed to consume %u bytes", strm.avail_in);
		ret = -1;
		goto bail;
	}
	ret = 0;
bail:
	if (ret || !size) {
		deflateEnd(&strm);
		if (outheld) {
			len = ramdump_stream_write(rs, out, outheld);
			if (len != outheld)
				ret = -1;
		}
		if (out)
			free(out);
		if (!ret)
			ramdump_z_print_status(&strm);
		strm.total_in = 0; /* not initialized */
	}
#else
	if (!size)
		return 0; /* nothing to close */
	ret = ramdump_stream_write(rs, data, size);
	if (ret != size) {
		printf("Failed to write %d bytes to SD\n", (int)size);
		return -1;
	}
	printf("Written %d bytes to SD\n", (int)ret);
	ret = 0;
#endif
	return ret;
}

static int ramdump_file_write(const char *name, const struct rd_desc *desc,
			const char *text)
{
#ifndef CONFIG_ESHEL
	int dev = CONFIG_RAMDUMP_MMC_DEV;
#else
	int dev = cpu_is_pxa1801_B1() ? 1 : 0; /* B1: SD is on MMC4, not MMC3 */
#endif
	int part = 1;
	struct ramdump_stream rs;
	block_dev_desc_t *dev_desc;
	long ret = 0;
	unsigned long t0;
	int s;

#ifdef CONFIG_RD_MMC
	const char *dev_name = "mmc";
	dev_desc = get_dev((char *)dev_name, dev);
	if (dev_desc == NULL) {
		puts("\n** Invalid MMC device for ramdump **\n");
		return 1;
	}
#else
	const char *dev_name = "flash";
#endif

	if (ramdump_stream_open(&rs, dev_desc, part, name, text)) {
		printf("\n** Unable to use %s %d:%d for ramdump **\n", dev_name, dev, part);
		return 1;
	}
	t0 = get_ticks();

	ret = ramdump_write_elf_header(&rs, desc);
	for (s = 0; !ret && (s < desc->nmems); s++)
		ret = ramdump_write(&rs, (__u8 *)desc->mems[s].start,
				desc->mems[s].end - desc->mems[s].start);

	if (!ret)
		/* End of stream */
		ramdump_write(&rs, 0, 0);

	ramdump_stream_close(&rs);
	if (!ret)
		printf("RAMDUMP complete (time elapsed %ldms)\n",
		       ((unsigned long)(get_ticks()-t0))/(CONFIG_SYS_HZ/1000));
	return ret;
}

/* Get DDR bank sizes from cmdline: mem=xxxM@0xaddr tokens */
static int ramdump_getddrsize(unsigned *pbank0, unsigned *pbank1)
{
	unsigned bank[] = { 0x10000000, 0 };
	char *cmdline = getenv("bootargs");
	const char prefix[] = " mem="; /* avoid match with cpmem */
	int i;
	if (cmdline) {
		char *p = cmdline;
		for (i = 0; p && (i < sizeof(bank)/sizeof(bank[0]));) {
			p = strstr(p, prefix);
			if (p) {
				p += sizeof(prefix)-1;
				bank[i++] =  simple_strtoul(p, &p, 10)*0x100000;
			}
		}
	}
	*pbank0 = bank[0];
	*pbank1 = bank[1];
	return 0;
}

static int ramdump_getcpmem(unsigned *pcpaddr, unsigned *pcpsize)
{
	const char prefix[] = " cpmem=";
	const char suffix[] = "@0";
	char *cmdline = getenv("bootargs");
	if (cmdline) {
		char *p = strstr(cmdline, prefix);
		if (p) {
			p += sizeof(prefix)-1;
			*pcpsize = simple_strtoul(p, &p, 10)*0x100000;
			p = strstr(p, suffix);
			if (p) {
				p += sizeof(suffix)-1;
				/* Allow forms 0x and 0 */
				p += (*p == 'x') ? 1 : -1;
				*pcpaddr = simple_strtoul(p, &p, 16);
			} else {
				*pcpaddr = DDR0_BASE;
			}
			return 0;
		}
	}
	return 1;
}

/*
 * Platform may supply a ramdump_desc object, otherwise
 * a default one is generated.
 * The platform ramdump_desc may be partial: default behavior
 * is enforced for non-initialized fields.
 */
extern const struct rd_desc ramdump_desc __attribute__((weak));
/*
 * ramdump_get_desc():
 * figures out the platform specific memory configuration
 * and returns a reference to an array of dump descriptors.
 */
static int ramdump_get_desc(struct rd_desc *desc,
			unsigned ddr0_size, unsigned ddr1_size)
{
	static struct rd_mem_desc lmems[3];
	int		s;
	ulong	size;

	if (&ramdump_desc) {
		/* Platform descriptor is present: copy and ammend if needed */
		*desc = ramdump_desc;
	} else {
		memset((void *)desc, 0, sizeof(*desc));
	}

	if (desc->mems && desc->nmems) {
		const struct rd_mem_desc *mems = desc->mems;
		for (s = 0, size = 0; s < desc->nmems; s++)
			if (mems[s].end > mems[s].start) {
				size += mems[s].end - mems[s].start;
			} else {
				printf("RAMDUMP: invalid mem desc 0x%.8lx..0x%.8lx\n",
				       mems[s].start, mems[s].end);
				return -1;
			}
		printf("RAMDUMP: %d platform memory descriptors, %ldMB\n",
		       s, size>>20);
	} else {
		#define va2pa(va) (va)
		/*
		 * Exclude the OBM/U-boot area, as the data in it is changing
		 * as we compress, and this likely causes bad CRC in .gz.
		 * U-boot memory map with stack/heap in 1MB below TEXT_BASE.
		 */
		unsigned uboot_base = (va2pa(CONFIG_SYS_TEXT_BASE)&0xfff00000)
							- 0x100000;
		if (!(ddr0_size+ddr1_size))
			ramdump_getddrsize(&ddr0_size, &ddr1_size);
		s = 0;
		lmems[s].start = DDR0_BASE;
		lmems[s++].end = uboot_base;
		lmems[s].start = uboot_base + 0x200000;
		lmems[s++].end = DDR0_BASE + ddr0_size;
		if (ddr1_size) {
			lmems[s].start = DDR1_BASE;
			lmems[s++].end = DDR1_BASE + ddr1_size;
		}
		desc->mems = lmems;
		desc->nmems = s;
	}

	if (!desc->rdc_addr) {
		unsigned cpmem_addr = DDR0_BASE, cpmem_size;
		ramdump_getcpmem(&cpmem_addr, &cpmem_size);

		desc->rdc_addr = (ulong)RDC_ADDRESS_CPA(cpmem_addr);
	}
	return 0;
}

/*
 * ELF definitions
 */
#define EI_NIDENT	16

#define PF_X            (1 << 0)        /* Segment is executable */
#define PF_W            (1 << 1)        /* Segment is writable */
#define PF_R            (1 << 2)        /* Segment is readable */
#define ELFMAG          "\177ELF"
#define SELFMAG		4

#define EI_CLASS        4               /* File class byte index */
#ifndef CONFIG_ARMV8
#define ELFCLASS      1               /* 32-bit objects */
#define EM_ARM	40              /* ARM */
#define VIRT_START	0xc0000000
#else
#define ELFCLASS      2               /* 64-bit objects */
#define EM_ARM	183              /* ARM */
#define VIRT_START	0xffffffc000000000
#endif

#define EI_DATA         5               /* Data encoding byte index */
#define ELFDATA2LSB     1               /* 2's complement, little endian */

#define EI_VERSION      6               /* File version byte index */
#define EV_CURRENT      1               /* Current version */

#define ET_CORE         4               /* Core file */

#define PT_LOAD         1               /* Loadable program segment */
#define PT_NOTE         4               /* Auxiliary information */

#ifndef CONFIG_ARMV8
/* 32-bit ELF base types and size. */
#define elf_addr unsigned int
#define elf_half unsigned short
#define elf_off unsigned int
#define elf_sword unsigned int
#define elf_word unsigned int
#else
/*64-bit ELF base types*/
#define elf_addr uint64_t
#define elf_half uint16_t
#define elf_off uint64_t
#define elf_sword int32_t
#define elf_word uint32_t
#endif

struct elf_hdr {
	unsigned char	e_ident[EI_NIDENT];
	elf_half	e_type;
	elf_half	e_machine;
	elf_word	e_version;
	elf_addr	e_entry;  /* Entry point */
	elf_off		e_phoff;
	elf_off		e_shoff;
	elf_word	e_flags;
	elf_half	e_ehsize;
	elf_half	e_phentsize;
	elf_half	e_phnum;
	elf_half	e_shentsize;
	elf_half	e_shnum;
	elf_half	e_shstrndx;
};

#ifndef CONFIG_ARMV8
struct elf_phdr {
	elf_word	p_type;
	elf_off		p_offset;
	elf_addr	p_vaddr;
	elf_addr	p_paddr;
	elf_off		p_filesz;
	elf_off		p_memsz;
	elf_word	p_flags;
	elf_off		p_align;
};
#else
struct elf_phdr {
	elf_word	p_type;
	elf_word	p_flags;
	elf_off		p_offset;
	elf_addr	p_vaddr;
	elf_addr	p_paddr;
	elf_off		p_filesz;
	elf_off		p_memsz;
	elf_off		p_align;
};
#endif

/*
 * ramdump_write_elf_header()
 * Generates a dump ELF header based on memory descriptors (mems),
 * and write it into the output stream (rs).
 */
static int ramdump_write_elf_header(struct ramdump_stream *rs,
				const struct rd_desc *desc)
{
	int	s;
	ulong sz, offset;
	int ret = -1;
	struct elf_hdr *ehdr;
	struct elf_phdr *phdr;
	void *elf_header = 0;
	const struct rd_mem_desc *mems = desc->mems;

	sz = sizeof(struct elf_hdr) +
			sizeof(struct elf_phdr) * (desc->nmems + 1);

	elf_header = malloc(sz);
	if (!elf_header)
		return ret;
	memset(elf_header, 0, sz);
	ehdr = (struct elf_hdr *)elf_header;

	memcpy(ehdr->e_ident, ELFMAG, SELFMAG);
	ehdr->e_ident[EI_CLASS] = ELFCLASS;
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_type = ET_CORE;
	ehdr->e_machine = EM_ARM;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_phoff = sizeof(struct elf_hdr);
	ehdr->e_ehsize = sizeof(struct elf_hdr);
	ehdr->e_phentsize = sizeof(struct elf_phdr);

	phdr = (struct elf_phdr *)(ehdr + 1);
	phdr->p_type = PT_NOTE;
	phdr->p_offset = sz;
	ehdr->e_phnum = desc->nmems + 1;

	phdr++; /* done with PT_NOTE; now generate pheaders for the mems */
	offset = sz;
	for (s = 0; s < (ehdr->e_phnum - 1); s++, phdr++) {
		phdr->p_type = PT_LOAD;
		phdr->p_offset = offset;
		phdr->p_vaddr = mems[s].start + VIRT_START
				- desc->phys_start;
		phdr->p_paddr = mems[s].start;
		phdr->p_filesz = mems[s].end - mems[s].start;
		phdr->p_memsz = phdr->p_filesz;
		phdr->p_flags = PF_X|PF_W|PF_R;
		offset += phdr->p_filesz;
	}

	/* done: write into the output stream */
	ret = ramdump_write(rs, elf_header, sz);
	free(elf_header);

	return ret;
}

int ramdump_cmd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned ddr0_size = 0;
	unsigned ddr1_size = 0;
	unsigned force = 1;
	unsigned mode = RD_MODE_DEFAULT;
	if (argc > 1)
		ddr0_size = simple_strtoul(argv[1], NULL, 16);
	if (argc > 2)
		ddr1_size = simple_strtoul(argv[2], NULL, 16);
	if (argc > 3)
		force = simple_strtoul(argv[3], NULL, 16);
	if (argc > 4)
		mode = simple_strtoul(argv[4], NULL, 16);

	return ramdump_ext(ddr0_size, ddr1_size, force, mode);
}

U_BOOT_CMD(
	ramdump,	5,	0,	ramdump_cmd,
	"check ramdump pending and handle if present\n",
	"[ddr0_size] [ddr1_size] [force] [mode]\n"
	"	DDR bank sizes: 0 to use platform defaults\n"
	"	force: 1 to dump when signature is not present\n"
	"	mode: OR of {1: reset if failed; 2: reset when done}"
);

/*
 * ddr_zero(): this is a common implementation to zero the unused DDR space
 * before booting the kernel.
 * This is needed on power-on to eliminate the random contents from DDR,
 * and thus improve the compression ratio and reduce dump size and time.
 * The board file should invoke this and pass the board-specific addresses:
 * start:	depends on objects loaded in DDR at this moment;
 * end:		depends on the DDR size.
 * The function automatically excludes some areas, e.g. the frame buffer.
 */
#define DDR_ZERO_TIME	/* print time spent */
#define FRAME_BUFFER_MAX_SIZE 0x02000000 /* hard to get this from the driver */

static int __zero_dcache_range(u64 start, u64 stop)
{
	return 0;
}

int zero_dcache_range(u64 start, u64 stop)
__attribute__((weak, alias("__zero_dcache_range")));

/*
 * zero_memory():
 * Handles misaligned start/end per block size returned by zero_dcache_range,
 * and cases where zero_dcache_range() returns 0, i.e. not implemented.
 */
static void zero_memory(u64 start, u64 end)
{
	unsigned long x;
	unsigned sz;

	sz = zero_dcache_range(start, end);
	if (!sz) {
		memset((void *)start, 0, end - start);
	} else {
		sz = sz - 1; /* alignment mask */
		x = (start + sz) & ~sz;
		if (x != start)
			memset((void *)start, 0, x - start);
		x = end & ~sz;
		if (x != end)
			memset((void *)x, 0, end - x);
	}
}

void ddr_zero(uint64_t start, uint64_t end)
{
	uint64_t raddr = 0, rsize = 0;
#ifdef DDR_ZERO_TIME
	unsigned long t0;
#endif
	printf("Power on zero DDR...\n");
#ifdef DDR_ZERO_TIME
	t0 = get_ticks();
#endif

	/* Exclude the frame buffer from the area we are about to clear */
#ifdef FRAME_BUFFER_ADDR
	raddr = FRAME_BUFFER_ADDR;
	rsize = FRAME_BUFFER_MAX_SIZE;
#endif
	if ((raddr + rsize) > start) {
		if (raddr > start)
			zero_memory(start, raddr);
		start = raddr + rsize;
	}

	if (end > start)
		zero_memory(start, end);
#ifdef DDR_ZERO_TIME
	printf("done in %dms\n",
	       (unsigned)((get_ticks()-t0)*1000/COUNTER_FREQUENCY));
#else
	printf("done\n");
#endif
}

int do_ddr0(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint64_t start = 0x10000000, end = 0x80000000;

	if (argc > 1)
		start = simple_strtoul(argv[1], NULL, 16);
	if (argc > 2)
		end = simple_strtoul(argv[2], NULL, 16);

	if (!start || !end)
		return 1;
	ddr_zero(start, end);
	return 0;
}

U_BOOT_CMD(
	ddr0, 3, 1, do_ddr0,
	"ZERO DDR",
	"Usage: ddr0 start_addr end_addr (default 0x10000000 0x80000000)"
);

int ramdump_attach_item(enum rdi_type type,
			const char *name,
			unsigned attrbits,
			unsigned nwords, ...)
{
	struct rdc_dataitem *rdi;
	unsigned size;
	unsigned offset = 0;
	va_list args;
	int	i;
	struct rd_desc desc;
	struct rdc_area *rdc_va;

	if (&ramdump_desc)
		desc = ramdump_desc;
	else
		return -EINVAL;

	rdc_va = (struct rdc_area *)desc.rdc_addr;
	if (rdc_va->header.signature != RDC_SIGNATURE) {
		printf("no signature found\n");
		return -EINVAL;
	}

	do {
		rdi = (struct rdc_dataitem *)&rdc_va->body.space[offset];
		size = rdi->size;
		if (size) {
			offset += size;
			if (offset > sizeof(rdc_va->body))
				return -ENOMEM;
		}
	} while (size);

	/* name should be present, and at most MAX_RDI_NAME chars */
	if (!name || (strlen(name) > sizeof(rdi->name)) || (nwords > 16))
		return -EINVAL;
	size = offsetof(struct rdc_dataitem, body)
		+ nwords * sizeof(unsigned long);

	if ((offset + size) > sizeof(rdc_va->body))
		return -ENOMEM;

	/* Allocated space: set pointer */
	rdi->size	= size;
	rdi->type	= type;
	rdi->attrbits	= attrbits;
	strncpy(rdi->name, name, sizeof(rdi->name));
	va_start(args, nwords);
	/* Copy the data words */
	for (i = 0; nwords; nwords--) {
		unsigned long w = va_arg(args, unsigned long);
		rdi->body.w[i++] = (unsigned)w;
#ifdef CONFIG_ARM64
		rdi->body.w[i++] = (unsigned)(w>>32);
#endif
	}
	va_end(args);
	return 0;
}

