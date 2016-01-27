#include <common.h>
#include <command.h>
#include <asm/io.h>

static void delay_1us(void)
{
	int i = 800;

	while (i--)
		;
}

static void delay_us(int us)
{
	while (us--)
		delay_1us();
}

#define TMR_CCR		(0x0000)
#define TMR_TN_MM(n, m)	(0x0004 + ((n) << 3) + (((n) + (m)) << 2))
#define TMR_CR(n)	(0x0028 + ((n) << 2))
#define TMR_SR(n)	(0x0034 + ((n) << 2))
#define TMR_IER(n)	(0x0040 + ((n) << 2))
#define TMR_PLVR(n)	(0x004c + ((n) << 2))
#define TMR_PLCR(n)	(0x0058 + ((n) << 2))
#define TMR_WMER	(0x0064)
#define TMR_WMR		(0x0068)
#define TMR_WVR		(0x006c)
#define TMR_WSR		(0x0070)
#define TMR_ICR(n)	(0x0074 + ((n) << 2))
#define TMR_WICR	(0x0080)
#define TMR_CER		(0x0084)
#define TMR_CMR		(0x0088)
#define TMR_ILR(n)	(0x008c + ((n) << 2))
#define TMR_WCR		(0x0098)
#define TMR_WFAR	(0x009c)
#define TMR_WSAR	(0x00A0)
#define TMR_CVWR(n)	(0x00A4 + ((n) << 2))
#define TMR_CRSR        (0x00B0)

#define TMR_CCR_CS_0(x)	(((x) & 0x3) << 0)
#define TMR_CCR_CS_1(x)	(((x) & 0x3) << 2)
#define TMR_CCR_CS_2(x)	(((x) & 0x3) << 5)

static int check_timer0_meta_stability(
		unsigned int *apb_timer_clk_reg, u32 timer_clock,
		unsigned int *timer_base, u32 timer_frequency, int delta)
{
	unsigned int reg32 = 0;
	unsigned int reg32_new = 0;
	int loops = 0;
	int risks = 0;
	int safe = 0;
	int start = 0;
	int end = 0;

	/* Initialize timer clock*/
	if (apb_timer_clk_reg)
		writel(timer_clock, apb_timer_clk_reg);

	/* Initialize timer frequency*/
	writel(timer_frequency, timer_base + TMR_CCR);

	/* Disable Timer0 clock */
	reg32 = readl(timer_base + TMR_CER);
	reg32 &= ~0x1; /* T0EN */
	writel(reg32, timer_base + TMR_CER);

	delay_us(1000);

	/* Timer0 count Mode */
	reg32 = readl(timer_base + TMR_CMR);
	reg32 |= 0x1; /* Mode 1 */
	writel(reg32, timer_base + TMR_CMR);

	/* En Timer0 clock */
	reg32 = readl(timer_base + TMR_CER);
	reg32 |= 0x1; /* T0EN */
	writel(reg32, timer_base + TMR_CER);

	delay_us(10000);

	reg32 = readl(timer_base + TMR_CR(0));
	reg32 = readl(timer_base + TMR_CR(0));
	start = reg32;
	while (loops++ < 1000000) {
		reg32_new =  readl(timer_base + TMR_CR(0));
		if (reg32 != reg32_new) {
			if ((reg32_new > reg32) && (reg32_new - reg32)
			    <= delta) {
				safe++;
			} else {
				risks++;
				printf("loop = %d, reg32_new = 0x%x, reg32 = 0x%x, reg32_new-reg32 = %d\n",
				       loops, reg32_new, reg32,
				       reg32_new-reg32);
				reg32_new =  readl(timer_base + TMR_CR(0));
			}

			reg32 = reg32_new;
		}
	}

	end = reg32_new;
	printf("start: %d ~end:%d [%d]-> check %d times,\n"
	       "find %d times safe and %d times risk\n",
	       start, end, end - start, loops - 1, safe, risks);
	return 0;
}

static int do_timer_stability_test(cmd_tbl_t *cmdtp, int flag,
				    int argc, char * const argv[])
{
	unsigned int *apb_timer_clk_reg;
	unsigned int *timer_base;
	u32 timer_clock;
	u32 timer_frequency;
	int delta = 0;
	int ret = 0;

	if (argc < 6) {
		printf("error:need 6 parameters!\n"
		       "timertest <apb_timer_clk_reg> <timer_clock> <timer_base> <timer_frequency> <delta>");
		return CMD_RET_FAILURE;
	}


	apb_timer_clk_reg = (unsigned int *)simple_strtoul(argv[1], NULL, 16);
	timer_clock = simple_strtoul(argv[2], NULL, 16);
	timer_base = (unsigned int *)simple_strtoul(argv[3], NULL, 16);
	timer_frequency = simple_strtoul(argv[4], NULL, 16);
	delta = simple_strtoul(argv[5], NULL, 10);

	if (!timer_base) {
		printf("error: timer_base couldn't be 0x0!\n");
		return CMD_RET_FAILURE;
	}

	printf("apb_timer_clk_reg:0x%x, timer_clock:0x%x, timer_base:0x%x,\n"
	       "timer_frequency:0x%x, delta:%d\n",
	       *apb_timer_clk_reg, timer_clock,
	       *timer_base, timer_frequency, delta);

	ret = check_timer0_meta_stability(apb_timer_clk_reg, timer_clock,
				    timer_base, timer_frequency, delta);
	return ret;
}

U_BOOT_CMD(timertest, CONFIG_SYS_MAXARGS, 0, do_timer_stability_test,
	   "run timer stability test\n",
	   "timertest <apb_timer_clk_reg> <timer_clock> <timer_base> <timer_frequency> <delta>"
	   "e.g. timertest 0xD4015024 0x33 0xD4014000 0x0 18\n");
