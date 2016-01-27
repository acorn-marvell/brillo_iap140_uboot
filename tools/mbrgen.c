#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <u-boot/crc.h>

/* following part is used by part_efi.h */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long long u64;
#ifndef __packed
#define __packed			__attribute__((packed))
#endif

#include <part_efi.h>

#define DEFAULT_MBR_NAME1	"primary_gpt"
#define DEFAULT_MBR_NAME2	"second_gpt"
#define SECTOR_SIZE		512
#define GPT_HEADER_SIZE		512
#define MAX_PART_ENTRY		128
#define MAX_NAME_SZ		36
#define GPT_SIZE		128
#define ENTRY_NUM		128
#define FIRST_USABLE_LBA	34
#define HEADER_SIZE		92

#ifdef WIN32
#define OUTPUT		"%I64x"
#define OUTPUT_D	"%I64d"
#define SEPERATOR       '\\'
#else
#define OUTPUT		"%llx"
#define OUTPUT_D	"%lld"
#define SEPERATOR       '/'
#endif

static void usage(void);
static char mbrfile1[PATH_MAX + 1], mbrfile2[PATH_MAX + 1], *cmdname;
static int show_content;
static FILE *desp, *mbr;
static void random_uuid(unsigned char *str)
{
	int i;
	for (i = 0; i < 16; i++)
		str[i] = (unsigned char) (256.0 * (rand() / (RAND_MAX + 1.0)));
}

static void reformat_part_size(u64 *start, u64 *end, int num)
{
	int i;
	for (i = 0; i < num; i++) {
		if (start[i] > end[i]) {
			fprintf(stderr, "The %d entry's start "OUTPUT" exceed end "OUTPUT"\n",
				i, start[i], end[i]);
			exit(EXIT_FAILURE);
		}
		if (end[i] > start[i + 1]) {
			fprintf(stderr,
				"The %d entry's end "OUTPUT"exceed the next start "OUTPUT"\n",
				i, end[i], start[i + 1]);
			exit(EXIT_FAILURE);
		}
		if (start[i] == end[i])
			end[i] = (start[i + 1] - SECTOR_SIZE);
		start[i] /= SECTOR_SIZE;
		end[i] /= SECTOR_SIZE;
	}
}

int main(int argc, char **argv)
{
	char oneline[100], filename[MAX_PART_ENTRY][36];
	gpt_header pri_header, sec_header;
	char gpt_header_pad[GPT_HEADER_SIZE - sizeof(gpt_header)] = {0};
	legacy_mbr leg_mbr;
	gpt_entry *entry;
	unsigned long long start[MAX_PART_ENTRY + 1], end[MAX_PART_ENTRY + 1], dev_sz;
	unsigned int i, j, num;
	char *output = NULL;
	int ret;
	cmdname = argv[0];
	while (--argc > 0 && **++argv == '-') {
		while (*++*argv) {
			switch (**argv) {
			case 's':
				show_content = 1;
				break;
			case 'o':
				if (--argc <= 0)
					usage();
				output = *++argv;
				goto NXTARG;
			default:
				usage();
			}
		}
NXTARG:
		;
	}

	if (output) {
		num = strlen(output);
		if (output[num - 1] == SEPERATOR)
			output[num - 1] = 0;
		ret = access(output, F_OK);
		if (ret) {
#ifndef __MINGW32__
			ret = mkdir(output, S_IRWXU | S_IRWXG | S_IROTH);
#else
			ret = mkdir(output);
#endif
			if (ret)
				perror("mkdir fail");
		}
	}
	if (argc != 2) {
		usage();
		exit(EXIT_FAILURE);
	}

	if (sscanf(argv[1], "0%*[xX]%100s", oneline) == 1)
		dev_sz = strtoull(oneline, 0, 16);
	else
		dev_sz = strtoull(argv[1], 0, 10);
	desp = fopen(*argv, "r");
	if (!desp) {
		fprintf(stderr, "Can't open %s: %s\n", *argv, strerror(errno));
		exit(EXIT_FAILURE);
	}

	num = 0;
	while (fgets(oneline, sizeof(oneline), desp) != NULL) {
		switch (sscanf(oneline, "%s\t"OUTPUT"\t"OUTPUT"\n",
			filename[num], &start[num], &end[num])) {
		case 2:
			end[num] = start[num];
			break;
		case 3:
			end[num] -= SECTOR_SIZE;
			break;
		default:
			fprintf(stderr, "desp file err\n");
			fclose(desp);
			exit(EXIT_FAILURE);
		}
		if (strlen(filename[num]) > MAX_NAME_SZ) {
			fprintf(stderr, "The %dth entry name %s exceed max size of %x\n",
				num, filename[num], MAX_NAME_SZ);
			fclose(desp);
			exit(EXIT_FAILURE);
		}
		num++;
	}

	fclose(desp);
	if (num == 0)
		return 0;
	start[num] = dev_sz;
	end[num] = dev_sz;
	if (end[num - 1] > dev_sz) {
		fprintf(stderr, "The last entry end 0x"OUTPUT"exceed device size 0x"OUTPUT"\n",
			end[num - 1], dev_sz);
		exit(EXIT_FAILURE);
	}
	reformat_part_size(start, end, num);

	entry = malloc(sizeof(gpt_entry) * ENTRY_NUM);
	if (!entry) {
		fprintf(stderr, "malloc fail\n");
		exit(EXIT_FAILURE);
	}

	if (output) {
		sprintf(mbrfile1, "%s%c%s_"OUTPUT_D, output, SEPERATOR,
			DEFAULT_MBR_NAME1, dev_sz);
		sprintf(mbrfile2, "%s%c%s_"OUTPUT_D, output, SEPERATOR,
			DEFAULT_MBR_NAME2, dev_sz);
	} else {
		sprintf(mbrfile1, "%s_"OUTPUT_D, DEFAULT_MBR_NAME1, dev_sz);
		sprintf(mbrfile2, "%s_"OUTPUT_D, DEFAULT_MBR_NAME2, dev_sz);
	}
	memset(entry, 0, sizeof(gpt_entry) * ENTRY_NUM);
	for (i = 0; i < num; i++) {
		memcpy(&entry[i].partition_type_guid, &PARTITION_BASIC_DATA_GUID,
		       sizeof(efi_guid_t));
		random_uuid((unsigned char *)&entry[i].unique_partition_guid);
		memcpy(&entry[i].starting_lba, &start[i], 8);
		memcpy(&entry[i].ending_lba, &end[i], 8);
		for (j = 0; j < strlen(filename[i]); j++)
			entry[i].partition_name[j] = filename[i][j];
	}

	dev_sz /= SECTOR_SIZE;
	memset(&pri_header, 0, sizeof(pri_header));
	pri_header.signature = cpu_to_le64(GPT_HEADER_SIGNATURE);
	pri_header.revision = cpu_to_le32(GPT_HEADER_REVISION_V1);
	pri_header.header_size = cpu_to_le32(HEADER_SIZE);
	pri_header.my_lba = cpu_to_le64(1);
	pri_header.alternate_lba = cpu_to_le64(dev_sz - 1);
	pri_header.first_usable_lba = cpu_to_le64(FIRST_USABLE_LBA);
	pri_header.last_usable_lba = cpu_to_le64(dev_sz - FIRST_USABLE_LBA);
	random_uuid((unsigned char *)&pri_header.disk_guid);
	pri_header.partition_entry_lba = cpu_to_le64(2);
	pri_header.num_partition_entries = cpu_to_le32(ENTRY_NUM);
	pri_header.sizeof_partition_entry = cpu_to_le32(GPT_SIZE);
	pri_header.partition_entry_array_crc32 = cpu_to_le32(
		crc32(0, (const unsigned char *)entry, sizeof(gpt_entry) * MAX_PART_ENTRY)
		);

	pri_header.header_crc32 = cpu_to_le32(
		crc32(0, (const unsigned char *)&pri_header, HEADER_SIZE)
		);

	memcpy(&sec_header, &pri_header, sizeof(pri_header));
	memset(&sec_header.header_crc32, 0, 4);

	sec_header.my_lba = cpu_to_le64(dev_sz - 1);
	sec_header.alternate_lba = 0;
	sec_header.alternate_lba = cpu_to_le64(1);
	sec_header.header_crc32 = cpu_to_le32(
		crc32(0, (const unsigned char *)&sec_header, HEADER_SIZE)
		);

	memset(&leg_mbr, 0, sizeof(leg_mbr));

	leg_mbr.partition_record[0].sector = 1;
	leg_mbr.partition_record[0].sys_ind = EFI_PMBR_OSTYPE_EFI_GPT;
	leg_mbr.partition_record[0].end_head = 0xfe;
	leg_mbr.partition_record[0].end_sector = 0xff;
	leg_mbr.partition_record[0].end_cyl = 0xff;
	leg_mbr.partition_record[0].start_sect = cpu_to_le32(1);
	leg_mbr.partition_record[0].nr_sects = cpu_to_le32(dev_sz);
	leg_mbr.signature = cpu_to_le16(MSDOS_MBR_SIGNATURE);

	mbr = fopen(mbrfile1, "wb");
	if (!mbr) {
		fprintf(stderr, "Can't create %s: %s\n", mbrfile1, strerror(errno));
		exit(EXIT_FAILURE);
	}

	fwrite(&leg_mbr, 1, sizeof(leg_mbr), mbr);
	fwrite(&pri_header, 1, sizeof(pri_header), mbr);
	fwrite(gpt_header_pad, 1, sizeof(gpt_header_pad), mbr);
	fwrite(entry, 1, sizeof(gpt_entry) * ENTRY_NUM, mbr);
	fclose(mbr);

	mbr = fopen(mbrfile2, "wb");
	if (!mbr) {
		fprintf(stderr, "Can't create %s: %s\n", mbrfile2, strerror(errno));
		exit(EXIT_FAILURE);
	}

	fwrite(entry, 1, sizeof(gpt_entry) * ENTRY_NUM, mbr);
	fwrite(&sec_header, 1, sizeof(sec_header), mbr);
	fwrite(gpt_header_pad, 1, sizeof(gpt_header_pad), mbr);
	fclose(mbr);

	printf("GPT create sucessfull!!!\n"
		"Please write the primary mbr first from offset 0 to mmc device offset 0 with the size of 0x22\n"
		"And write the second mbr to mmc device offset 0x"OUTPUT" with the size of 0x21\n",
		dev_sz - FIRST_USABLE_LBA + 1);
	return 0;
}

static void usage()
{
	fprintf(stderr, "Usage:\n"
		"cmdname %s [partition description file] [mmc device size] [-o [gpt store place]]\n"
		"---------------------------------------------------------\n"
		"Partition description file example:\n"
		"bootloader      0               0x100000\n"
		"ramdisk         0x100000        0x140000\n"
		"kernel          0x980000        0xc80000\n"
		"system          0x3bc0000       0xabc0000\n"
		"---------------------------------------------------------\n"
		"The description file is created by [NAME] [Start offset] [End offset]\n"
		"If [End offset] is not specified, it would auto calculate it out by\n"
		"its following partition\n"
		, cmdname);
}
