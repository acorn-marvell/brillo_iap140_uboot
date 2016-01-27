/*
 * Command for accessing DataFlash.
 *
 * Copyright (C) 2008 Atmel Corporation
 */
#include <common.h>
#include <asm/io.h>

typedef volatile unsigned int   VUINT_T;
typedef unsigned int   UINT_T;

#define	TMR2_BASE	0xD4080000
#define	TMR2_WFAR	(TMR2_BASE+0x009C)
#define	TMR2_WSAR	(TMR2_BASE+0x00A0)
#define	TMR2_WICR	(TMR2_BASE+0x0080)
#define	TMR2_CER	(TMR2_BASE+0x0084)
#define	TMR2_WCR	(TMR2_BASE+0x0098)
#define	TMR2_WMR	(TMR2_BASE+0x0068)
#define	TMR2_WMER	(TMR2_BASE+0x0064)

#define WDT_GEN_RESET   1
#define REG_RTC_BR0 	0xD4010014
#define APBC_BASE_ADDR          (0xD4015000)
#define APBC_RTC_CLK_RST        (0x28)

static void wdt_access(void)
{
	*(VUINT_T *)TMR2_WFAR = 0xbaba;
	*(VUINT_T *)TMR2_WSAR = 0xeb10;
}

static void wdt_reset_counter(void)
{
	wdt_access();
	*(VUINT_T *)TMR2_CER = 0x7;

	wdt_access();
	*(VUINT_T *)TMR2_WCR = 0x1;
}

static void wdt_enable(unsigned int reset_int)
{
	wdt_access();
	*(VUINT_T *)TMR2_WMER = 0x3;

	udelay(100);
	udelay(100);
}

static void wdt_set_match(unsigned int match)
{
	wdt_access();
	*(VUINT_T *)TMR2_WMR = 0xffff & match;

	udelay(100);
	udelay(100);
}

void wdt_test(unsigned int seconds, unsigned int reset_int)
{

	// wdt 13Mhz
	*(VUINT_T *)0xD4050200 = 0x7;
	udelay(10);
	*(VUINT_T *)0xD4050200 = 0x3;
	udelay(100);
	*(VUINT_T *)0xD4051020 |= 0x10;

	wdt_reset_counter();
	// the wdt timer seems only use 256hz
	// see the emei register TimersCP section for details
	wdt_set_match(seconds * 256);
	wdt_enable(reset_int);
	printf("-------- wdt rest --------------\n");

	while (1);
}

void do_wdt_reset(void)
{
	*(VUINT_T *)(0xd401e080) = 0xD0C1;
	*(VUINT_T *)(0xd401900c) &=  ~(0x1<<3);
	udelay(1);	//can't be removed.

	wdt_test(2, WDT_GEN_RESET);
}

static int do_fobm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long cmd = 'b'<<24 | 'o'<<16 | 'o'<<8 | 't';
	udelay(10);

	unsigned int rtc_clk = *(unsigned int *) (APBC_BASE_ADDR + APBC_RTC_CLK_RST);
	*(unsigned int *) (APBC_BASE_ADDR + APBC_RTC_CLK_RST) = (rtc_clk | 0x83) & ~(1<<2);

	do {
		__raw_writel(cmd, REG_RTC_BR0);
	} while (__raw_readl(REG_RTC_BR0) != cmd);
	udelay(10);
	printf("fobm %x",REG_RTC_BR0);
	do_wdt_reset();
	return 0;
}

U_BOOT_CMD(
	fobm,	1,	1,	do_fobm,
	"reboot to obm flash mode",
	"fobm ");
