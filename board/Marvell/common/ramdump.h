/*
 * ramdump.h: PXAXXX ramdump support public interface
 *
 * Copyright (C) 2008 MARVELL Corporation (antone@marvell.com)
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __RAMDUMP_H
#define __RAMDUMP_H

#include <common.h>

/*
 * Descriptor for a physical memory space to be included into the dump.
 * The platform may supply an array of such descriptors, that reflect
 * the RAM physical address(es, in case of multiple banks), size(s),
 * relevance of the contents in the dump per software memory map,
 * and software or hardware related restrictions.
 *
 * Examples of areas to be EXCLUDED from dump:
 * * U-boot RAM (vars/stacks/heap):
 *	 MUST be excluded to prevent bad crc in the gzip output.
 * * Secure only (read) RAM space:
 *   MUST be excluded to prevent an exception on access during dump.
 * * Areas containing code or RO contents that is available with the
 *	 matching software image. CAN be excluded to reduce dump size=time.
 *   Note, that such optimization disables visibility into the areas
 *   excluded for debug cases where retention and corruption is suspected.
 * * Areas used by hardware accelerators, which is not considered relevant
 *   for debug.
 *
 * Platform code MAY supply the description in the ramdump_desc object.
 * ramdump weak-references it, and if not present, generates the defaults.
 * ramdump_desc and the ramdump_desc.mems array MAY be const,
 * but can be also modified at run-time to accomodate the different
 * platform configurations.
 * If present, the valid (non-0) fields are used, otherwise
 * DEFAULTS are generated for those fields omitted as follows:
 * * rdc_addr: the legacy cpmem+RDC_OFFSET, where cpmem is obtained from
 *	 the current kernel cmdline cpmem= token and defaults to DDR0_BASE.
 * * phys_start: physical address corresponding to the kernel flat mapping
 *	 virtual base, e.g. 0xc0000000
 * * mems and nmems:
 *	 DDR address: defaults to DDR0_BASE;
 *	 DDR size: ddr0_size parameter passed to ramdump, if non-0, otherwise
 *	 is obtained from the current kernel cmdline mem= token.
 *	 ddr1_size or the second mem= token are used to detect the second
 *	 DDR bank presence and size, and the address is assumed DDR1_BASE.
 *	 When mem= token is used, both size and address are taken from it.
 *	 The only area excluded is the 2MB space u-boot occupies.
 */
struct rd_mem_desc {
	ulong	start;	/* area start physical address */
	ulong	end;	/* area end physical address (exclusive) */
};

struct rd_desc {
	ulong				rdc_addr; /* RDC physical address */
	ulong				phys_start; /* kernel virt start PA */
	const struct rd_mem_desc	*mems;	  /* areas to be dumped */
	int				nmems;	  /* number of areas in mems */
};

/*
 * RAMDUMP mode control:
 * - Reset or return to caller when done or failed (e.g. no SD).
 *   Reset also clears the RDC signature in RAM, return does not.
 * - Reset or return after done if dump was forced (do_dump),
 *   i.e. no RDC signature was detected.
 */
#define RD_MODE_RESET_ON_FAILURE	1 /* reset if dump failed? */
#define RD_MODE_RESET_ON_SUCCESS	2 /* reset after done? */
#define RD_MODE_RESET_ON_FORCED		4 /* reset after done if forced */

#define RD_MODE_DEFAULT	\
	(RD_MODE_RESET_ON_FAILURE | RD_MODE_RESET_ON_SUCCESS)
/*
 * ramdump()
 * Main public interface to trigger a dump/check.
 * Same is available in form of command: ramdump.
 *
 * do_dump = 0: check RDC signature and dump only if present.
 * do_dump != 0: dump unconditionally.
 *
 * ramdump_ext()
 * Has an extra mode parameter containing or-combined options RD_MODE_ above.
 * ramdump() follows RD_MODE_DEFAULT.
 */
int ramdump_ext(unsigned ddr0_size, unsigned ddr1_size, int do_dump,
		unsigned mode);

int ramdump(unsigned ddr0_size, unsigned ddr1_size, int do_dump);

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
void ddr_zero(uint64_t start, uint64_t end);


/* RDI related definitions and functions */
enum rdi_type {
	RDI_NONE, /* End of list */
	RDI_CBUF, /* virtual addr, size, opt current idx/ptr; refs supported */
	RDI_PBUF, /* physical addr, size in bytes; no refs */
	RDI_CUST, /* Custom: no 1st level parser for this */
};

/* Attrbits: OR these: for each data word tells if it's an address or a value */
#define RDI_ATTR_VAL(wordnum)	(0<<(wordnum))
#define RDI_ATTR_ADDR(wordnum)	(1<<(wordnum))

/*
 * RDI (data item) generic object attachment:
 *	type: for unknown types rdp will just print the data words suplied;
 *	name: string used by rdp as filename, at most MAX_RDI_NAME chars;
 *	attrbits: 1 bit per data word, 0=value, 1=address of a value;
 *		Callers can assume "value" is the default (0).
 *	nwords: number of data words, max 16;
 *	unsigned long data elements follow (32-bit or 64-bit)
 */
int ramdump_attach_item(enum rdi_type type,
			const char *name,
			unsigned attrbits,
			unsigned nwords, ...);

/*
 * Buffer (RDI_CBUF) type RDI object attachment:
 *	name:  string used by rdp as filename, at most MAX_RDI_NAME chars;
 *	buf: address of the pointer to the buffer;
 *	buf_size: address of an unsigned containing buffer size;
 *		OR an address of a pointer to the buffer end;
 *		rdp will automatically figure out which;
 *	cur_ptr: address of a pointer to the current byte inside the buffer;
 *		OR an address of an unsigned containing the current byte index;
 *		OR an address of an unsigned containing the total byte count
 *		(with no wrap-around);
 *		rdp will automatically figure out which;
 *	unit:	unit buf_size (and cur_ptr if is an index) are expressed in.
 */
static inline
int ramdump_attach_cbuffer(const char	*name,
			void		**buf,
			unsigned	*buf_size,
			unsigned	*cur_ptr,
			unsigned	unit)
{
	return ramdump_attach_item(RDI_CBUF, name,
		RDI_ATTR_ADDR(0) | RDI_ATTR_ADDR(1)
		| RDI_ATTR_ADDR(2), 4,
		(unsigned long)buf, (unsigned long)buf_size,
		(unsigned long)cur_ptr, (unsigned long)unit);
}

static inline
int ramdump_attach_pbuffer(const char	*name,
			unsigned	buf_physical,
			unsigned	buf_size)
{
	return ramdump_attach_item(RDI_PBUF, name,
		RDI_ATTR_VAL(0) | RDI_ATTR_VAL(1), 2,
		(unsigned)buf_physical, (unsigned)buf_size);
}
#endif /* __RAMDUMP_H */
