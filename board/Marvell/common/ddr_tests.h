#ifndef DDR_TESTS_H
#define DDR_TESTS_H

#define printw(hex_) printf("0x%08X", hex_)
#define printh(hex_) printf("0x%04X", hex_)
#define printb(hex_) printf("0x%02X", hex_)
#define rotate(value, number)  (value = ((value << (sizeof(uint32_t)*8-number))\
			| (value >> number)))
#define conv_dec(x) simple_strtoul(x, NULL, 10)
#define conv_hex(x) simple_strtoul(x, NULL, 16)

enum access_type {
	ACCESS_TYPE_BYTE = 0,
	ACCESS_TYPE_HWORD,
	ACCESS_TYPE_WORD,
};

enum print_mode {
	SILENT_MODE = 0,
	SIGN_MODE,
	MINIMAL_MODE,
	FULL_MODE,
};

enum mtsp_test_val {
	MTSP_OPERATION_OK = 0,
	MTSP_WRONG_PARAMETER,
	MTSP_READ_FAIL,  /* Reading pattern did not matched the writing*/
	MTSP_ERROR,  /*general error*/
	MTSP_DIR_ERR,  /*direction unknown*/
	MTSP_DRIVE_ERR,  /* drive unknown*/
	MTSP_CPU_UNKNOWN,
};

enum pattern_number {
	PATTERN_NUMBER_ONE = 0,
	PATTERN_NUMBER_TWO,
	PATTERN_NUMBER_THREE,
	PATTERN_NUMBER_FOUR,
	PATTERN_NUMBER_FIVE,
	PATTERN_NUMBER_SIX,
	PATTERN_NUMBER_SEVEN,
	PATTERN_NUMBER_EIGHT,
	PATTERN_NUMBER_NINE,
	PATTERN_NUMBER_TEN,
	PATTERN_NUMBER_ELEVEN,
	PATTERN_NUMBER_TWELVE,
	PATTERN_NUMBER_THIRTEEN,
	PATTERN_NUMBER_FOURTEEN,
	PATTERN_NUMBER_FIFTEEN,
	PATTERN_NUMBER_SIXTEEN,
	PATTERN_NUMBER_SEVENTEEN,
	PATTERN_NUMBER_EIGHTEEN,
	PATTERN_NUMBER_NINETEEN,
	PATTERN_NUMBER_TWENTY,
	PATTERN_NUMBER_TWENTYONE,
	PATTERN_NUMBER_TWENTYTWO,
	PATTERN_NUMBER_TWENTYTHREE,
	PATTERN_NUMBER_TWENTYFOUR,
	PATTERN_NUMBER_TWENTYFIVE,
	PATTERN_NUMBER_TWENTYSIX,
	PATTERN_NUMBER_TWENTYSEVEN,
	PATTERN_NUMBER_TWENTYEIGHT,
	PATTERN_NUMBER_TWENTYNINE,
	PATTERN_NUMBER_THIRTY,
	PATTERN_NUMBER_THIRTYONE,
	PATTERN_NUMBER_THIRTYTWO,
};

struct pattern_dword {
	uint32_t     first_pattern;
	uint32_t     second_pattern;
};

/* Functions Declerations*/
uint32_t mtsp_test(uintptr_t addr, uint32_t size, uint32_t pattern,
		uint32_t pre_read);
uint32_t memory_test(uintptr_t addr, uint32_t size, enum pattern_number pattern,
		uint8_t access, uint8_t mtsp);
int smtd(int argc, char * const *argv);
int mtsp(int argc, char * const *argv);

#ifdef CONFIG_SYS_MCK5_DDR
extern void MCK5_perf_cnt_clear(uint32_t cntmask);
extern void MCK5_perf_cnt_init(void);
extern void MCK5_perf_cnt_stop(void);
extern void MCK5_perf_cnt_get(uint32_t *total, uint32_t *busy,
		uint32_t *data);
extern int MCK5_sdram_size(int cs, ulong *size);
#endif

#endif
