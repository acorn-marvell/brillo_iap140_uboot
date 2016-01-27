/*
 * Copyright(C) 2011-2014 Marvell International Ltd.
 * Copyright(C) 2011-2014 Marvell XDB Team
 * All Rights Reserved
 *
 * License: can be distributed in source within Marvell
 *
 */

#include <common.h>
#include <linux/string.h>
#include <asm/io.h>
#include <asm/armv8/adtfile.h>

typedef struct etb_dmp_hdr {
	int magic;
	int size;
} etb_dmp_hdr;

typedef struct s_adtfile {
	ADTF_FILETYPE typ;
	int version;
	int record_length;
} adtfile;

#ifdef CONFIG_MV_ETB_DMP_ADDR

static char *dmp_buf = (void *)CONFIG_MV_ETB_DMP_ADDR;
static char *etb_base[CONFIG_NR_CLUS];
static char *etm_base[CONFIG_NR_CLUS];
static etb_dmp_hdr *dmp_hdr;
static int dmp_idx;

#define ETB_ADDRESS(CLUS)	(CONFIG_MV_ETB_REG_ADDR##CLUS)
#define ETM_ADDRESS(CLUS)	(CONFIG_MV_ETM_REG_ADDR##CLUS)
#define CPU2CLUS(CPU)		((CPU < 4) ? 0 : 1)
#define CPU2OFFSET(CPU)		((CPU < 4) ? CPU : (CPU - 4))

static int do_open(const char *name, char *prop)
{
	dmp_hdr = (etb_dmp_hdr *)dmp_buf;
	dmp_idx = sizeof(struct etb_dmp_hdr);
	return 0;
}

static int do_close(void)
{
	dmp_hdr->magic = 0x45544200;
	dmp_hdr->size  = dmp_idx;

	flush_cache((unsigned long)dmp_buf, 0x4000);
	return 0;
}

static int do_write(const void *buf, int count)
{
	memcpy(dmp_buf + dmp_idx, buf, (count << 2));
	dmp_idx += (count << 2);
	return (count << 2);
}

static int mstrlen(const char *str)
{
	int ret = 1;

	if (str) {
		ret = strlen(str);
		ret = (ret + 4) / 4;
	}
	return ret;
}

static int put_start(adtfile *fp, int type, int length)
{
	/* this is a fatal error */
	if (fp->record_length)
		return -2;

	fp->record_length = length;
	type |= length << 8;
	return do_write(&type, 1);
}

static int put_int(adtfile *fp, int val)
{
	--fp->record_length;
	return do_write(&val, 1);
}

static int put_nint(adtfile *fp, int count, const unsigned int *buf)
{
	fp->record_length -= count;
	return do_write(buf, count);
}

static int put_string(adtfile *fp, int length, const char *str)
{
	if (length && str) {
		fp->record_length -= length;
		return do_write(str, length);
	} else {
		/* write an empty string */
		char aBuffer[4] = {0, 0, 0, 0};
		fp->record_length -= 1;
		return do_write(aBuffer, 1);
	}

	return 0;
}

void *adtf_open(const char *name, ADTF_FILETYPE flag)
{
	static adtfile file = { 0 };
	int val;

	file.typ = flag;

	if (flag == ADTF_WRITE) {
		do_open(name, "wb");
		put_start(&file, ADTF_eStartRecord, 2);

		val = 0x41445446; /* ADTF */
		put_int(&file, val);
		val = 0x32303030; /* 2000 */
		put_int(&file, val);
		return &file;
	}

	return NULL;
}

int adtf_tool(void *pF, const char *pName,
	      const char *pVersion, const char *pTime)
{
	adtfile *pFile = (adtfile *) pF;
	int nameLen    = mstrlen(pName);
	int versionLen = mstrlen(pVersion);
	int timeLen    = mstrlen(pTime);

	put_start(pFile, ADTF_eToolDescription, nameLen + versionLen + timeLen);
	put_string(pFile, nameLen, pName);
	put_string(pFile, versionLen, pVersion);
	put_string(pFile, timeLen, pTime);
	return 0;
}

int adtf_architecture(void *pF, int cores, int traceSources,
		      int traceSinks, const char *pName)
{
	adtfile *pFile  = (adtfile *) pF;
	int     nameLen = mstrlen(pName);

	put_start(pFile, ADTF_eArchitectureDescription, nameLen + 3);
	put_int(pFile, cores);
	put_int(pFile, traceSources);
	put_int(pFile, traceSinks);
	put_string(pFile, nameLen, pName);
	return 0;
}

int adtf_tracesink(void *pF, int sinkId, const char *pName,
		   int count, unsigned int *pRegSet)
{
	adtfile *pFile  = (adtfile *) pF;
	int     nameLen = mstrlen(pName);

	put_start(pFile, ADTF_eTraceSink, 1 + nameLen + count);
	put_int(pFile, sinkId);
	put_string(pFile, nameLen, pName);
	put_nint(pFile, count, pRegSet);
	return 0;
}

int adtf_tracesinkinout(void *pF, int sinkId, const char *pName,
		        int sinkIdOut, int count, unsigned int *pRegSet)
{
	adtfile *pFile  = (adtfile *) pF;
	int     nameLen = mstrlen(pName);

	put_start(pFile, ADTF_eTraceSinkWOutput, 2 + nameLen + count);
	put_int(pFile, sinkId);
	put_string(pFile, nameLen, pName);
	put_int(pFile, sinkIdOut);
	put_nint(pFile, count, pRegSet);

	return 0;
}

int adtf_tracereplicator(void *pF, int sinkIdIn, const char *pName,
			 int sinkIdOut1, int sinkIdOut2)
{
	adtfile *pFile  = (adtfile *) pF;
	int     nameLen = mstrlen(pName);

	put_start(pFile, ADTF_eTraceReplicator, 3 + nameLen);
	put_int(pFile, sinkIdIn);
	put_string(pFile, nameLen, pName);
	put_int(pFile, sinkIdOut1);
	put_int(pFile, sinkIdOut2);
	return 0;
}

int adtf_tracefunnel(void *pF, int sinkId, const char *pName,
		     int count, unsigned int *pRegSet)
{
	adtfile *pFile  = (adtfile *) pF;
	int     nameLen = mstrlen(pName);

	put_start(pFile, ADTF_eTraceFunnel, 1 + nameLen + count);
	put_int(pFile, sinkId);
	put_string(pFile, nameLen, pName);
	put_nint(pFile, count, pRegSet);

	return 0;
}

int adtf_tracesource(void *pF, int id, int coreId, int sinkId,
		     const char *pName, int count, unsigned int *pRegSet)
{
	adtfile *pFile  = (adtfile *) pF;
	int     nameLen = mstrlen(pName);

	put_start(pFile, ADTF_eTraceSource, 3 + nameLen + count);
	put_int(pFile, id);
	put_int(pFile, coreId);
	put_int(pFile, sinkId);
	put_string(pFile, nameLen, pName);
	put_nint(pFile, count, pRegSet);

	return 0;
}

int adtf_tracebuffer(void *pF, int sinkId, unsigned int offset,
		     int count, void *pBuffer)
{
	adtfile *pFile = (adtfile *) pF;

	do {
		int cnt = count < ADTF_MAXLENGTH - 3 ? count : ADTF_MAXLENGTH - 3;

		put_start(pFile, ADTF_eTraceBuffer, 2 + cnt);
		put_int(pFile, sinkId);
		put_int(pFile, offset);
		put_nint(pFile, cnt, (const unsigned int *) pBuffer);
		count  -= cnt;
		offset += 4 * cnt;
		pBuffer = ((unsigned int *) pBuffer) + cnt;
	} while (0 < count);

	return 0;
}

int adtf_tracebuffer64(void *pF, int sinkId, unsigned long long offset,
		       int count, void *pBuffer)
{
	adtfile *pFile = (adtfile *) pF;

	do {
		int cnt = count < ADTF_MAXLENGTH - 4 ? count : ADTF_MAXLENGTH - 4;

		put_start(pFile, ADTF_eTraceBuffer, 3 + cnt);
		put_int(pFile, sinkId);
		put_int(pFile, (int) offset);
		put_int(pFile, (int) (offset >> 32));
		put_nint(pFile, cnt, (const unsigned int *) pBuffer);
		count  -= cnt;
		offset += 4 * cnt;
		pBuffer = ((unsigned int *) pBuffer) + cnt;
	} while (0 < count);

	return 0;
}

int adtf_core(void *pF, int coreId, int smpid, const char *pName, int count,
	      unsigned int *pRegSet)
{
	adtfile *pFile  = (adtfile *) pF;
	int     nameLen = mstrlen(pName);

	put_start(pFile, ADTF_eCoreDescription, 2 + nameLen + count);
	put_int(pFile, coreId);
	put_int(pFile, smpid);
	put_string(pFile, nameLen, pName);
	put_nint(pFile, count, pRegSet);

	return 0;
}

int adtf_close(void *pF)
{
	adtfile *pFile = (adtfile *) pF;

	if (pFile != NULL) {
		if (pFile->typ == ADTF_WRITE) {
			put_start(pFile, ADTF_eEndRecord, 0);
		}
		do_close();
	}

	return 0;
}

#define ETB_RDP		0x004
#define ETB_STS		0x00C
#define ETB_RRD		0x010
#define ETB_RRP		0x014
#define ETB_RWP		0x018
#define ETB_TRG		0x01C
#define ETB_CTL		0x020
#define ETB_MODE	0x028
#define ETB_FFCR	0x304
#define ETB_LAR		0xFB0

#define ETM_CFG		0x000
#define ETM_CTL		0x004
#define ETM_IDR		0x1E4
#define ETM_CCER	0x1E8
#define ETM_ID		0x040
#define ETM_OSLAR	0x300
#define ETM_OSLSR	0x304
#define ETM_PDSR	0x314
#define ETM_LAR		0xFB0
#define ETM_LSR		0xFB4

unsigned int reg_buff[CONFIG_NR_CPUS][32];
unsigned int etm_regs[CONFIG_NR_CPUS][64];
unsigned int etb_regs[CONFIG_NR_CPUS][16];
unsigned int etb_buff[CONFIG_NR_CPUS][1024];

#define ETB_REG(cpu, offset)	(etb_base[CPU2CLUS(cpu)] + (0x1000 * CPU2OFFSET(cpu)) + offset)
#define ETM_REG(cpu, offset)	(etm_base[CPU2CLUS(cpu)] + (0x1000 * CPU2OFFSET(cpu)) + offset)

static void _read_etm(int cpu, unsigned int *reg, unsigned int trace_id)
{
	u32 val;

	/* hardcode trace id in case ETM has been reset */
	if (trace_id) {
		reg[0] = 0x0;
		reg[1] = 0x0;
		reg[2] = 0x4100F400;
		reg[3] = trace_id;
		return;
	}

	writel(0xC5ACCE55, ETM_REG(cpu, ETM_LAR));
	val = readl(ETM_REG(cpu, ETM_LSR));
	if (val & (0x1 << 1))
		printf("Software Lock on cpu%d still locked!\n", cpu);

	/*Unlock the OS lock*/
	writel(0x0, ETM_REG(cpu, ETM_OSLAR));
	val = readl(ETM_REG(cpu, ETM_OSLSR));
	if (val & (0x1 << 0))
		printf("OS Lock on cpu%d still locked!\n", cpu);

	reg[0] = readl(ETM_REG(cpu, ETM_CFG));
	reg[1] = readl(ETM_REG(cpu, ETM_CTL));
	reg[2] = readl(ETM_REG(cpu, ETM_IDR));
	reg[3] = readl(ETM_REG(cpu, ETM_ID));

	writel(0x0, ETM_REG(cpu, ETM_LAR));
}

static void _read_etb(int cpu, unsigned int *reg, unsigned int *buf)
{
	u32 len = 0, val, timeout;
	s32 depth;

	writel(0xC5ACCE55, ETB_REG(cpu, ETB_LAR));

	/* flush etb */
	writel(0x1041, ETB_REG(cpu, ETB_FFCR));
	timeout = 0x10000;
	do {
		timeout--;
		if (!timeout) {
			printf("cpu %d flush etb timeout\n", cpu);
			break;
		}

		val = readl(ETB_REG(cpu, ETB_FFCR));

	} while (val & 0x40);

	/* disable capture */
	writel(0x0, ETB_REG(cpu, ETB_CTL));

	reg[0] = (unsigned int)(uintptr_t)ETB_REG(cpu, 0x0);
	reg[1] = readl(ETB_REG(cpu, ETB_RDP));
	reg[2] = readl(ETB_REG(cpu, 0x8));	/* 0x2 */
	reg[3] = readl(ETB_REG(cpu, ETB_STS));
	reg[4] = readl(ETB_REG(cpu, ETB_RRP));
	reg[5] = readl(ETB_REG(cpu, ETB_RWP));
	reg[6] = readl(ETB_REG(cpu, ETB_TRG));
	reg[7] = readl(ETB_REG(cpu, ETB_CTL));
	reg[8] = readl(ETB_REG(cpu, ETB_FFCR));

	if (reg[3] & 0x1) {
		writel(reg[5], ETB_REG(cpu, ETB_RRP));
		reg[3] &= ~0x1;		/* clear full bit */
		reg[4] = 0x0;		/* rd pointer = 0 */
		reg[5] = reg[1];	/* wt pointer = depth */

		printf("ETB is full\n");
		printf("ETB rrp 0x0%08x rwp 0x%08x\n",
			readl(ETB_REG(cpu, ETB_RRP)),
			readl(ETB_REG(cpu, ETB_RWP)));

	} else {
		printf("ETB is not full?!\n");
		writel(0x0, ETB_REG(cpu, ETB_RRP));
		reg[4] = 0x0;		/* rd pointer = 0 */
	}

	depth = reg[1];
	printf("cpu %d depth %d\n", cpu, depth);

	do {
		buf[len] = readl(ETB_REG(cpu, ETB_RRD));
		len++;
		depth--;

	} while (depth > 0);

	printf("cpu %d read etb 0x%x words\n", cpu, len);

	writel(0x0, ETB_REG(cpu, ETB_LAR));
}

static void _read_core_context(int cpu)
{
	memset(reg_buff[cpu], 0x0, sizeof(reg_buff[cpu]));
	memset(etm_regs[cpu], 0x0, sizeof(etm_regs[cpu]));
	memset(etb_regs[cpu], 0x0, sizeof(etb_regs[cpu]));
	memset(etb_buff[cpu], 0x0, sizeof(etb_buff[cpu]));

	_read_etm(cpu, etm_regs[cpu], cpu + 3);
	_read_etb(cpu, etb_regs[cpu], etb_buff[cpu]);
}

void adtf_dump_all_trace_buffer(void)
{
	int cpu;
	adtfile *file;
#ifdef CONFIG_ETM_POWER
	unsigned int val;
#endif

	if (!dmp_buf)
		return;

#ifdef CONFIG_MV_ETB_REG_ADDR1
	etb_base[1] = (void *)ETB_ADDRESS(1);
	etm_base[1] = (void *)ETM_ADDRESS(1);
#endif
#ifdef CONFIG_MV_ETB_REG_ADDR0
	etb_base[0] = (void *)ETB_ADDRESS(0);
	etm_base[0] = (void *)ETM_ADDRESS(0);
#endif

#ifdef CONFIG_ETM_POWER
	val = readl(0xD4282B8C);
	val |= (0x1 << 25);
	writel(val, 0xD4282B8C);
#endif

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++)
		_read_core_context(cpu);

	file = adtf_open("adtf", ADTF_WRITE);
	adtf_tool(file, "XDB", "V5.7.10", "2014/07/07 10:20:00");
	adtf_architecture(file, CONFIG_NR_CPUS, CONFIG_NR_CPUS,
			  CONFIG_NR_CPUS, "Coresight V8");

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++)
		adtf_core(file, cpu + CONFIG_CPU_OFFSET, 1, "CA53", 32, reg_buff[cpu]);

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++)
		adtf_tracesource(file, cpu + CONFIG_CPU_OFFSET, cpu + CONFIG_CPU_OFFSET,
			cpu + CONFIG_CPU_OFFSET, "ETM", 4, etm_regs[cpu]);

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++)
		adtf_tracesink(file, cpu + CONFIG_CPU_OFFSET, "ETB", 9, etb_regs[cpu]);

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++)
		adtf_tracebuffer(file, cpu + CONFIG_CPU_OFFSET, 0, 0x200, etb_buff[cpu]);

	adtf_close(file);
	return;
}

#endif	/* CONFIG_MV_ETB_DMP_ADDR */
