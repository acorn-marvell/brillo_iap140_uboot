/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Xiaofan Tian <tianxf@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/cpu.h>

bool pxa_amp_uart_enabled(void)
{
	return smp_hw_cpuid() == *(unsigned int *)CONFIG_AMP_SYNC_ADDR;
}

static void pxa_amp_set_master(unsigned int cpuid)
{
	*(unsigned int *)CONFIG_AMP_SYNC_ADDR = cpuid;
}

bool pxa_is_warm_reset(void)
{
	return *(unsigned int *)CONFIG_WARM_RESET;
}

void pxa_amp_init(void)
{
	if (!pxa_is_warm_reset()) {
		BUG_ON(smp_hw_cpuid());
		pxa_amp_set_master(0);
	}
}

unsigned int pxa_amp_reloc_end(void)
{
	unsigned int reloc_end[CONFIG_NR_CPUS - 1] = CONFIG_SYS_AMP_RELOC_END;

	if (smp_hw_cpuid())
		return reloc_end[smp_hw_cpuid() - 1];
	else
		return CONFIG_SYS_RELOC_END;
}

int do_amp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int cpu;

	if (argc != 2)
		return cmd_usage(cmdtp);

	/* Disable I&D cache */
	icache_disable();
	dcache_disable();

	cpu = simple_strtoul(argv[1], NULL, 10);
	/* update core busy status in CORE_BUSY_ADDR */
	*(unsigned int *)CONFIG_CORE_BUSY_ADDR &= ~(1 << cpu);
	pxa_cpu_reset(cpu, CONFIG_SYS_TEXT_BASE);

	/* Enable I&D cache */
	dcache_enable();
	icache_enable();
	return 0;
}

U_BOOT_CMD(
	amp, 2, 1, do_amp,
	"Release other cpu from reset mode",
	"Usage:\namp [cpu id]"
);

int do_uartsw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int cpu;

	if (argc != 2)
		return cmd_usage(cmdtp);
	cpu = simple_strtoul(argv[1], NULL, 10);
	if (cpu >= CONFIG_NR_CPUS) {
		printf("Invalid CPU number\n");
		return -1;
	}

	pxa_amp_set_master(cpu);

	return 0;
}
U_BOOT_CMD(
	uartsw, 2, 0, do_uartsw,
	"Switch UART to another cpu",
	"Usage:\nuartsw [cpu id]"
);

int do_cpuid(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int cpu;

	cpu = smp_hw_cpuid();
	printf("This is cpu %d\n", cpu);

	return 0;
}

U_BOOT_CMD(
	cpuid, 1, 1, do_cpuid,
	"Show current CPU ID",
	"Usage:\ncpuid"
);
