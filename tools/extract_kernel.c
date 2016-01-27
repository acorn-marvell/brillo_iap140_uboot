/* Extract kernel from ram dump file
 *
 *  Copyright (C) 2013 Marvell International Ltd.
 *  All rights reserved.
 *
 *  2013-2  Guoqing Li <ligq@marvell.com>
 *          extract git info and kernel symbols from dump file
 *
 *  Usage: extract_kernel <dump file>
 *
 */

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <emmd_rsv.h>
#include <configs/mv-common.h>
#include <stddef.h>

#define GIT_LEN_MAX	1024
#define DEF_FILELEN	256
#define KSYM_NAME_LEN	128
#define KSYM_TOKEN_TB_LEN	128
#define KALLSYMS_NUM_MAX	65536
#define GIT_INFO_FILE	"git_info.log"
#define KERNEL_LOG_FILE	"kernel.log"
#define SYSTEM_MAP_FILE	"System.map.log"

typedef unsigned int       u32;
typedef unsigned short     u16;
typedef unsigned char      u8;

static struct ram_tag_info ram_tags[20];
static char commit[20][50];
static u8 git_info[GIT_LEN_MAX];
static unsigned long kallsyms_addresses[KALLSYMS_NUM_MAX];
static unsigned long ks_addr;
static u8 kallsyms_names[KALLSYMS_NUM_MAX * KSYM_NAME_LEN];
static unsigned long ks_name_addr;
static unsigned long kallsyms_num_syms;
static u8 kallsyms_token_table[KALLSYMS_NUM_MAX * KSYM_TOKEN_TB_LEN];
static unsigned long ks_token_tb_addr;
static u16 kallsyms_token_index[KALLSYMS_NUM_MAX];
static unsigned long ks_token_id_addr;
static unsigned long kallsyms_markers[KALLSYMS_NUM_MAX];
static unsigned long ks_marker_addr;

static Elf32_Ehdr elf32_header;
static Elf64_Ehdr elf64_header;
static Elf32_Phdr *elf32_phdr;
static Elf64_Phdr *elf64_phdr;
static int arch;

static void usage(int exit_status)
{
	puts(
		"Extract_kernel 1.0 (C) 2013 by Guoqing Li\n"
		"\n"
		"Syntax:	extract_kernel [options] inputfile\n"
		"Options:\n"
		"  -h     Help output\n"
		"  -g     create git info file\n"
		"  -l     create kernel log file\n"
		"Where: 'inputfile' is the dump file to extract\n"
	);
	exit(exit_status);
}

/*
 * Expand a compressed symbol data into the resulting uncompressed string,
 * given the offset to where the symbol is in the compressed stream.
 */
static u32 kallsyms_expand_symbol(u32 off, char *result)
{
	int len, skipped_first = 0;
	const u8 *tptr, *data;

	/* Get the compressed symbol length from the first symbol byte. */
	data = &kallsyms_names[off];
	len = *data;
	data++;

	/*
	 * Update the offset to return the offset for the next symbol on
	 * the compressed stream.
	 */
	off += len + 1;

	/*
	 * For every byte on the compressed symbol data, copy the table
	 * entry for that byte.
	 */
	while (len) {
		tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				*result = *tptr;
				result++;
			} else {
				skipped_first = 1;
			}
			tptr++;
		}
	}

	*result = '\0';

	/* Return to offset to the next symbol. */
	return off;
}

/*
 * Get symbol type information. This is encoded as a single char at the
 * beginning of the symbol name.
 */
static char kallsyms_get_symbol_type(u32 off)
{
	/*
	 * Get just the first code, look it up in the token table,
	 * and return the first char from this token.
	 */
	return kallsyms_token_table[kallsyms_token_index\
		[kallsyms_names[off + 1]]];
}

/*
 * Find the offset on the compressed stream given and index in the
 * kallsyms array.
 */
static u32 get_symbol_offset(unsigned long pos)
{
	const u8 *name;
	int i;

	/*
	 * Use the closest marker we have. We have markers every 256 positions,
	 * so that should be close enough.
	 */
	name = &kallsyms_names[kallsyms_markers[pos >> 8]];

	/*
	 * Sequentially scan all the symbols up to the point we're searching
	 * for. Every symbol is stored in a [<len>][<len> bytes of data] format,
	 * so we just need to add the len to the current pointer for every
	 * symbol we wish to skip.
	 */
	for (i = 0; i < (pos & 0xFF); i++)
		name = name + (*name) + 1;

	return name - kallsyms_names;
}

static int read_file_by_vaddr(int fd, void *buf,
		unsigned long vaddr, size_t count)
{
	int i;
	off_t offset;

	if (arch == 64) {
		for (i = 0; i < elf64_header.e_phnum; i++) {
			if (vaddr >= (elf64_phdr[i].p_vaddr) &&
					vaddr <= (elf64_phdr[i].p_memsz
					+ elf64_phdr[i].p_vaddr)) {
				offset = vaddr - elf64_phdr[i].p_vaddr
					+ elf64_phdr[i].p_offset;
				lseek(fd, offset, SEEK_SET);
				return read(fd, buf, count);
			}
		}
	} else {
		for (i = 0; i < elf32_header.e_phnum; i++) {
			if (vaddr >= (elf32_phdr[i].p_vaddr) &&
					vaddr <= (elf32_phdr[i].p_memsz
					+ elf32_phdr[i].p_vaddr)) {
				offset = vaddr - elf32_phdr[i].p_vaddr
					+ elf32_phdr[i].p_offset;
				lseek(fd, offset, SEEK_SET);
				return read(fd, buf, count);
			}
		}
	}

	return 0;
}

static void extract_kallsyms(int fd)
{
	int pos;
	char type;
	u32 nameoff, off;
	unsigned long value;
	char name[KSYM_NAME_LEN];
	FILE *file;

	printf("\tCreating System.map.log...\n");

	read_file_by_vaddr(fd, kallsyms_addresses, ks_addr,
			sizeof(unsigned long) * kallsyms_num_syms);
	read_file_by_vaddr(fd, kallsyms_markers, ks_marker_addr,
			sizeof(unsigned long) * (kallsyms_num_syms >> 8));
	read_file_by_vaddr(fd, kallsyms_names, ks_name_addr,
			sizeof(u8) * kallsyms_num_syms * KSYM_NAME_LEN);
	read_file_by_vaddr(fd, kallsyms_token_table, ks_token_tb_addr,
			sizeof(u8) * kallsyms_num_syms * KSYM_TOKEN_TB_LEN);
	read_file_by_vaddr(fd, kallsyms_token_index, ks_token_id_addr,
			sizeof(u16) * kallsyms_num_syms);

	file = fopen(SYSTEM_MAP_FILE, "w");

	if (file == NULL) {
		printf("Can not create System.map.log\n");
		return;
	}

	nameoff = get_symbol_offset(0);
	for (pos = 0; pos < kallsyms_num_syms; pos++) {
		value = kallsyms_addresses[pos];
		type = kallsyms_get_symbol_type(nameoff);
		off = kallsyms_expand_symbol(nameoff, name);
		nameoff += (off - nameoff);
		fprintf(file, "%p %c %s\n", (void *)value, type, name);
	}

	fclose(file);
}

static unsigned long get_value_for_sym(char *sym_name)
{
	int pos;
	u32 nameoff, off;
	unsigned long value = 0;
	char name[KSYM_NAME_LEN];

	nameoff = get_symbol_offset(0);
	for (pos = 0; pos < kallsyms_num_syms; pos++) {
		value = kallsyms_addresses[pos];
		off = kallsyms_expand_symbol(nameoff, name);
		nameoff += (off - nameoff);
		if (!strcmp(sym_name, name))
			return value;
	}

	return 0;
}

/*
 * Extract git information.
 */
static void extract_gitinfo(int fd)
{
	char branch[100];
	int i, j, off = 0, branch_len, commit_len[20];
	FILE *file;
	u32 git_info_num;
	unsigned long ginfo_num_addr, ginfo_addr;

	printf("\tCreating git_info.log...\n");

	ginfo_num_addr = get_value_for_sym("git_info_num");
	ginfo_addr = get_value_for_sym("git_info");

	if (ginfo_num_addr == 0 || ginfo_addr == 0) {
		printf("\t Could not get info\n");
		return;
	}

	read_file_by_vaddr(fd, &git_info_num, ginfo_num_addr, sizeof(u32));
	read_file_by_vaddr(fd, git_info, ginfo_addr, sizeof(git_info));

	file = fopen(GIT_INFO_FILE, "w");

	if (file == NULL) {
		printf("Can not create git_info.log\n");
		return;
	}

	off += 1;
	branch_len = git_info[0];

	for (i = 0; i < branch_len; i++)
		branch[i] = git_info[i + off];
	branch[i] = '\0';
	fprintf(file, "%s\n", branch);

	off += branch_len;
	for (i = 0; i < git_info_num - 1; i++) {
		commit_len[i] = git_info[off];
		off += 1;
		for (j = 0; j < commit_len[i]; j++)
			commit[i][j] = git_info[j + off];
		off += commit_len[i];
		commit[i][j] = '\0';
		fprintf(file, "%s\n", commit[i]);
	}

	fclose(file);
}

/*
 * get kernel log buffer
 */
static void get_kernel_log(int fd)
{
	int i, off = 0, zero_flag = 0;
	char tmp;
	FILE *file;
	unsigned long log_buf_addr, log_buf_addr_addr, log_buf_len_addr;
	unsigned int log_buf_len;

	file = fopen(KERNEL_LOG_FILE, "w");

	if (file == NULL) {
		printf("Can not create kernel.log\n");
		return;
	}

	printf("\tCreating kernel.log...\n");

	log_buf_addr_addr = get_value_for_sym("log_buf");
	log_buf_len_addr = get_value_for_sym("log_buf_len");

	if (log_buf_addr_addr == 0 || log_buf_len_addr == 0) {
		printf("\t Could not get log info\n");
		return;
	}

	read_file_by_vaddr(fd, &log_buf_addr, log_buf_addr_addr,
		sizeof(unsigned long));
	read_file_by_vaddr(fd, &log_buf_len, log_buf_len_addr,
		sizeof(unsigned int));

	for (i = 0; i < log_buf_len; i++) {
		read_file_by_vaddr(fd, &tmp, log_buf_addr + i, sizeof(char));
		off++;
		if (tmp != 0x0) {
			fprintf(file, "%c", tmp);
			zero_flag = 0;
		} else {
			zero_flag++;
		}
		if (zero_flag > 3) {
			fprintf(file, "\n");
			zero_flag = 0;
		}
	}

	fclose(file);
}

static int read_file_by_paddr(int fd, void *buf,
		unsigned long paddr, size_t count)
{
	int i;
	off_t offset;

	if (arch == 64) {
		for (i = 0; i < elf64_header.e_phnum; i++) {
			if (paddr >= elf64_phdr[i].p_paddr &&
					paddr <= (elf64_phdr[i].p_memsz
					+ elf64_phdr[i].p_paddr)) {
				offset = paddr - elf64_phdr[i].p_paddr +
					elf64_phdr[i].p_offset;
				lseek(fd, offset, SEEK_SET);
				return read(fd, buf, count);
			}
		}
	} else {
		for (i = 0; i < elf32_header.e_phnum; i++) {
			if (paddr >= elf32_phdr[i].p_paddr &&
					paddr <= (elf32_phdr[i].p_memsz
					+ elf32_phdr[i].p_paddr)) {
				offset = paddr - elf32_phdr[i].p_paddr +
					elf32_phdr[i].p_offset;
				lseek(fd, offset, SEEK_SET);
				return read(fd, buf, count);
			}
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int c, fd, tags_num, i, phdr_size, ret = 1;
	char inputfile[DEF_FILELEN];
	u32 version, sum = 0, check_sum = 0xffffffff;
	u32 cgit_info = 0, ckernel_log = 0;
	char elf_ident[EI_NIDENT];
	unsigned long ram_tag_paddr;

	while ((c = getopt(argc, argv, "hg:l:")) > 0) {
		switch (c) {
		case 'h':
			usage(0);
			break;
		case 'g':
			cgit_info = strtol(optarg, NULL, 10);
			break;
		case 'l':
			ckernel_log = strtol(optarg, NULL, 10);
			break;
		default:
			break;
		}
	}

	c = argc - optind;
	if (c > 4 || c < 1)
		usage(1);

	strcpy(inputfile, argv[optind]);

	/* Make sure the output is sent as soon as we printf() */
	setbuf(stdout, NULL);

	printf("Extracting git_info and kallsyms from '%s'...\n", inputfile);

	fd = open(inputfile, O_RDONLY);
	if (fd < 0) {
		usage(1);
		exit(1);
	}

	ret = read(fd, &elf_ident, EI_NIDENT);
	if (ret != EI_NIDENT)
		goto fault;

	lseek(fd, 0x0, SEEK_SET);
	if (elf_ident[EI_CLASS] == ELFCLASS32) {
		arch = 32;
		ret = read(fd, &elf32_header, sizeof(elf32_header));
		if (ret	!= sizeof(elf32_header))
			goto fault;
		phdr_size = elf32_header.e_phnum * sizeof(Elf32_Phdr);
		elf32_phdr = malloc(phdr_size);

		if (elf32_phdr == NULL) {
			printf("Alloc phdr failed\n");
			ret = -ENOMEM;
			goto fault;
		}

		lseek(fd, elf32_header.e_phoff, SEEK_SET);
		ret = read(fd, elf32_phdr, phdr_size);
		if (ret != phdr_size)
			goto fault;
	} else if (elf_ident[EI_CLASS] == ELFCLASS64) {
		arch = 64;
		ret = read(fd, &elf64_header, sizeof(elf64_header));
		if (ret	!= sizeof(elf64_header))
			goto fault;
		phdr_size = elf64_header.e_phnum * sizeof(Elf64_Phdr);
		elf64_phdr = malloc(phdr_size);

		if (elf64_phdr == NULL) {
			printf("Alloc phdr failed\n");
			ret = -ENOMEM;
			goto fault;
		}

		lseek(fd, elf64_header.e_phoff, SEEK_SET);
		ret = read(fd, elf64_phdr, phdr_size);
		if (ret != phdr_size)
			goto fault;
	} else {
		printf("ELF header not support\n");
		ret = -EINVAL;
		goto fault;
	}


	ram_tag_paddr = CRASH_BASE_ADDR + offsetof(struct emmd_page, ram_tag);

	ret = read_file_by_paddr(fd, ram_tags, ram_tag_paddr,
			sizeof(struct ram_tag_info));
	if (ret != sizeof(struct ram_tag_info))
		goto fault;

	if (strcmp(ram_tags[0].name, "magic_id")) {
		printf("The %s does not support extract\n", inputfile);
		ret = -EINVAL;
		goto fault;
	}

	tags_num = ram_tags[0].data;

	ret = read_file_by_paddr(fd, &ram_tags[1],
			ram_tag_paddr + sizeof(struct ram_tag_info),
			sizeof(struct ram_tag_info) * tags_num);
	if (ret != sizeof(struct ram_tag_info) * tags_num)
		goto fault;

	version = ram_tags[1].data;
	switch (version) {
	case 1:
		for (i = 2; i < tags_num + 1; i++) {
			if (!strcmp(ram_tags[i].name,
						"kallsyms_addrs"))
				ks_addr = ram_tags[i].data;
			else if (!strcmp(ram_tags[i].name,
						"kallsyms_names"))
				ks_name_addr = ram_tags[i].data;
			else if (!strcmp(ram_tags[i].name,
						"kallsyms_num"))
				kallsyms_num_syms = ram_tags[i].data;
			else if (!strcmp(ram_tags[i].name,
						"kallsyms_token_tb"))
				ks_token_tb_addr = ram_tags[i].data;
			else if (!strcmp(ram_tags[i].name,
						"kallsyms_token_id"))
				ks_token_id_addr = ram_tags[i].data;
			else if (!strcmp(ram_tags[i].name,
						"kallsyms_markers"))
				ks_marker_addr = ram_tags[i].data;
			else if (!strcmp(ram_tags[i].name,
						"check_sum"))
				check_sum = ram_tags[i].data;
			else
				printf("ram tag:%s not support\n",
						ram_tags[i].name);
		}
		break;
	default:
		printf("The version:%d not support\n", version);
		ret = -EINVAL;
		goto fault;
		break;
	}

	sum = ks_addr + ks_name_addr + kallsyms_num_syms +
		ks_token_tb_addr + ks_token_id_addr + ks_marker_addr;
	if (sum != check_sum) {
		printf("Check sum failed\n");
		ret = -EINVAL;
		goto fault;
	}

	extract_kallsyms(fd);

	if (cgit_info)
		extract_gitinfo(fd);

	if (ckernel_log)
		get_kernel_log(fd);

	printf("Done\n");

fault:
	close(fd);
	if (elf32_phdr != NULL)
		free(elf32_phdr);
	if (elf64_phdr != NULL)
		free(elf64_phdr);

	if (ret <= 0) {
		printf("Extract failed with %d\n", ret);
		exit(1);
	}

	return 0;
}
