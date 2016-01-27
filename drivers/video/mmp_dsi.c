/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 *
 */
#include <config.h>
#include <common.h>
#include <malloc.h>
#include <mmp_disp.h>
#include <linux/types.h>
#include <asm/io.h>
#include "tc35876x.h"

#define dsi_ex_pixel_cnt                0
#define dsi_hex_en                      0
/* (Unit: Mhz) */
#define dsi_lpclk                       3

#define to_dsi_bcnt(timing, bpp)        (((timing) * (bpp)) >> 3)
static unsigned int dsi_lane[5] = {0, 0x1, 0x3, 0x7, 0xf};

/* dsi phy timing */
static struct dsi_phy_timing phy = {
	.hs_prep_constant	= 40,    /* Unit: ns. */
	.hs_prep_ui		= 4,
	.hs_zero_constant	= 145,
	.hs_zero_ui		= 10,
	.hs_trail_constant	= 60,
	.hs_trail_ui		= 4,
	.hs_exit_constant	= 100,
	.hs_exit_ui		= 0,
	.ck_zero_constant	= 300,
	.ck_zero_ui		= 0,
	.ck_trail_constant	= 60,
	.ck_trail_ui		= 0,
	.req_ready		= 0x3c,
	.wakeup_constant        = 1000000,
	.wakeup_ui      = 0,
	/*
	 * According to the D-PHY spec, Tlpx >= 50 ns
	 * For the panel otm1281a, it works unstable when Tlpx = 50,
	 * so increase to 60ns.
	 */
	.lpx_constant       = 60,
	.lpx_ui     = 0,
};


static unsigned char dsi_bit(unsigned int index, unsigned char *pdata)
{
	unsigned char ret;
	unsigned int cindex, bindex;
	cindex = index / 8;
	bindex = index % 8;

	if (pdata[cindex] & (0x1 << bindex))
		ret = (unsigned char) 0x1;
	else
		ret = (unsigned char) 0x0;

	return ret;
}

static unsigned char calculate_ecc(unsigned char *pdata)
{
	unsigned char ret;
	unsigned char p[8];

	p[7] = (unsigned char) 0x0;
	p[6] = (unsigned char) 0x0;

	p[5] = (unsigned char) (
	(
		dsi_bit(10, pdata) ^
		dsi_bit(11, pdata) ^
		dsi_bit(12, pdata) ^
		dsi_bit(13, pdata) ^
		dsi_bit(14, pdata) ^
		dsi_bit(15, pdata) ^
		dsi_bit(16, pdata) ^
		dsi_bit(17, pdata) ^
		dsi_bit(18, pdata) ^
		dsi_bit(19, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(22, pdata) ^
		dsi_bit(23, pdata)
		)
	);
	p[4] = (unsigned char) (
		dsi_bit(4, pdata) ^
		dsi_bit(5, pdata) ^
		dsi_bit(6, pdata) ^
		dsi_bit(7, pdata) ^
		dsi_bit(8, pdata) ^
		dsi_bit(9, pdata) ^
		dsi_bit(16, pdata) ^
		dsi_bit(17, pdata) ^
		dsi_bit(18, pdata) ^
		dsi_bit(19, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(22, pdata) ^
		dsi_bit(23, pdata)
	);
	p[3] = (unsigned char) (
	(
		dsi_bit(1, pdata) ^
		dsi_bit(2, pdata) ^
		dsi_bit(3, pdata) ^
		dsi_bit(7, pdata) ^
		dsi_bit(8, pdata) ^
		dsi_bit(9, pdata) ^
		dsi_bit(13, pdata) ^
		dsi_bit(14, pdata) ^
		dsi_bit(15, pdata) ^
		dsi_bit(19, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(23, pdata)
		)
	);
	p[2] = (unsigned char) (
	(
		dsi_bit(0, pdata) ^
		dsi_bit(2, pdata) ^
		dsi_bit(3, pdata) ^
		dsi_bit(5, pdata) ^
		dsi_bit(6, pdata) ^
		dsi_bit(9, pdata) ^
		dsi_bit(11, pdata) ^
		dsi_bit(12, pdata) ^
		dsi_bit(15, pdata) ^
		dsi_bit(18, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(22, pdata)
		)
	);
	p[1] = (unsigned char) (
		(
		dsi_bit(0, pdata) ^
		dsi_bit(1, pdata) ^
		dsi_bit(3, pdata) ^
		dsi_bit(4, pdata) ^
		dsi_bit(6, pdata) ^
		dsi_bit(8, pdata) ^
		dsi_bit(10, pdata) ^
		dsi_bit(12, pdata) ^
		dsi_bit(14, pdata) ^
		dsi_bit(17, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(22, pdata) ^
		dsi_bit(23, pdata)
		)
	);
	p[0] = (unsigned char) (
		(
		dsi_bit(0, pdata) ^
		dsi_bit(1, pdata) ^
		dsi_bit(2, pdata) ^
		dsi_bit(4, pdata) ^
		dsi_bit(5, pdata) ^
		dsi_bit(7, pdata) ^
		dsi_bit(10, pdata) ^
		dsi_bit(11, pdata) ^
		dsi_bit(13, pdata) ^
		dsi_bit(16, pdata) ^
		dsi_bit(20, pdata) ^
		dsi_bit(21, pdata) ^
		dsi_bit(22, pdata) ^
		dsi_bit(23, pdata)
		)
	);
	ret = (unsigned char)(
		p[0] |
		(p[1] << 0x1) |
		(p[2] << 0x2) |
		(p[3] << 0x3) |
		(p[4] << 0x4) |
		(p[5] << 0x5)
	);
	return   ret;
}

static void dsi_lanes_enable(struct dsi_info *di, int en,
		unsigned version)
{
	struct dsi_regs *dsi = (struct dsi_regs *)(uintptr_t)di->regs;
	void *phy_ctrl2_reg = (!DISP_GEN4(version)) ?
		(&dsi->phy_ctrl2) : (&dsi->phy.phy_ctrl2);
	u32 reg1 = readl(phy_ctrl2_reg) &
		(~DSI_PHY_CTRL_2_CFG_CSR_LANE_EN_MASK);
	u32 reg2 = readl(&dsi->cmd1) &
		~(0xf << DSI_CPU_CMD_1_CFG_TXLP_LPDT_SHIFT);

	if (en) {
		reg1 |= (dsi_lane[di->lanes] <<
				DSI_PHY_CTRL_2_CFG_CSR_LANE_EN_SHIFT);
		reg2 |= (dsi_lane[di->lanes] <<
				DSI_CPU_CMD_1_CFG_TXLP_LPDT_SHIFT);
	}

	writel(reg1, phy_ctrl2_reg);
	writel(reg2, &dsi->cmd1);
}

static void dsi_send_cmds(struct mmp_disp_info *fbi, u8 *parameter,
		u8 count, u8 lp)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;
	struct dsi_regs *dsi = (struct dsi_regs *)(uintptr_t)di->regs;
	u32 send_data = 0, waddr, timeout, tmp, i, turnaround;
	u32 len;

	/* write all packet bytes to packet data buffer */
	for (i = 0; i < count; i++) {
		send_data |= parameter[i] << ((i % 4) * 8);
		if (!((i + 1) % 4)) {
			writel(send_data, &dsi->dat0);
			waddr = DSI_CFG_CPU_DAT_REQ_MASK |
				DSI_CFG_CPU_DAT_RW_MASK |
				((i - 3) << DSI_CFG_CPU_DAT_ADDR_SHIFT);
			writel(waddr, &dsi->cmd3);
			/* wait write operation done */
			timeout = 1000;
			do {
				timeout--;
				tmp = readl(&dsi->cmd3);
			} while ((tmp & DSI_CFG_CPU_DAT_REQ_MASK) && timeout);
			if (!timeout)
				printf("DSI write data to the packet data buffer not done.\n");
			send_data = 0;
		}
	}

	/* handle last none 4Byte align data */
	if (i % 4) {
		writel(send_data, &dsi->dat0);
		waddr = DSI_CFG_CPU_DAT_REQ_MASK |
			DSI_CFG_CPU_DAT_RW_MASK |
			((4 * (i / 4)) << DSI_CFG_CPU_DAT_ADDR_SHIFT);
		writel(waddr, &dsi->cmd3);
		/* wait write operation done */
		timeout = 1000;
		do {
			timeout--;
			tmp = readl(&dsi->cmd3);
		} while ((tmp & DSI_CFG_CPU_DAT_REQ_MASK) && timeout);
		if (!timeout)
			printf("DSI write data to the packet data buffer not done.\n");
		send_data = 0;
	}

	if (parameter[0] == DSI_DI_DCS_READ ||
	    parameter[0] == DSI_DI_GENERIC_READ1 ||
	    parameter[0] == DSI_DI_SET_MAX_PKT_SIZE)
		turnaround = 0x1;
	else
		turnaround = 0x0;

	len = count;

	if ((parameter[0] == DSI_DI_DCS_LWRITE ||
		parameter[0] == DSI_DI_GENERIC_LWRITE) &&
		(!(lp || DISP_GEN4(mi->version))))
		len = count - 6;

	waddr = DSI_CFG_CPU_CMD_REQ_MASK |
		((count == 4) ? DSI_CFG_CPU_SP_MASK : 0) |
		(turnaround << DSI_CFG_CPU_TURN_SHIFT) |
		(lp ? DSI_CFG_CPU_TXLP_MASK : 0) |
		(len << DSI_CFG_CPU_WC_SHIFT);

	/* send out the packet */
	writel(waddr, &dsi->cmd0);
	/* wait packet be sent out */
	timeout = 1000;
	do {
		timeout--;
		tmp = readl(&dsi->cmd0);
	} while ((tmp & DSI_CFG_CPU_CMD_REQ_MASK) && timeout);
	if (!timeout)
		printf("DSI send out packet maybe failed.\n");
}

static unsigned short gs_crc16_generation_code = 0x8408;
static unsigned short calculate_crc16(unsigned char *pdata, unsigned
		short count)
{
	unsigned short byte_counter;
	unsigned char bit_counter;
	unsigned char data;
	unsigned short crc16_result = 0xFFFF;
	if (count > 0) {
		for (byte_counter = 0; byte_counter < count;
			byte_counter++) {
			data = *(pdata + byte_counter);
			for (bit_counter = 0; bit_counter < 8; bit_counter++) {
				if (((crc16_result & 0x0001) ^ ((0x0001 *
					data) & 0x0001)) > 0)
					crc16_result = ((crc16_result >> 1)
					& 0x7FFF) ^ gs_crc16_generation_code;
				else
					crc16_result = (crc16_result >> 1)
					& 0x7FFF;
				data = (data >> 1) & 0x7F;
			}
		}
	}
	return crc16_result;
}

void dsi_cmd_array_tx(struct mmp_disp_info *fbi, struct dsi_cmd_desc cmds[],
		int count)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;
	struct dsi_cmd_desc cmd_line;
	u8 command, parameter[DSI_MAX_DATA_BYTES], len, *temp;
	u32 crc, loop;
	int i, j;

	for (loop = 0; loop < count; loop++) {
		cmd_line = cmds[loop];
		command = cmd_line.data_type;
		len = cmd_line.length;
		memset(parameter, 0x00, len + 6);
		parameter[0] = command & 0xff;
		switch (command) {
		case DSI_DI_DCS_SWRITE:
		case DSI_DI_DCS_SWRITE1:
		case DSI_DI_DCS_READ:
		case DSI_DI_GENERIC_READ1:
		case DSI_DI_SET_MAX_PKT_SIZE:
			memcpy(&parameter[1], cmd_line.data, len);
			len = 4;
			break;
		case DSI_DI_GENERIC_LWRITE:
		case DSI_DI_DCS_LWRITE:
			parameter[1] = len & 0xff;
			parameter[2] = 0;
			memcpy(&parameter[4], cmd_line.data, len);
			crc = calculate_crc16(&parameter[4], len);
			parameter[len + 4] = crc & 0xff;
			parameter[len + 5] = (crc >> 8) & 0xff;
			len += 6;
			break;
		default:
			printf("%s: data type not supported 0x%8x\n",
			       __func__, command);
			break;
		}

		parameter[3] = calculate_ecc(parameter);

		/* support to send lower power mode command on multi lanes */
		if (cmd_line.lp && !(DISP_GEN4_LITE(fbi->mi->version) ||
			DISP_GEN4_PLUS(fbi->mi->version))) {
			temp = malloc(sizeof(u8) * DSI_MAX_DATA_BYTES
					* di->lanes);
			if (temp == NULL)
				return;
			for (i = 0; i < len * di->lanes; i += di->lanes)
				for (j = 0; j < di->lanes; j++)
					temp[i + j] = parameter[i/di->lanes];
			len = di->lanes * len;
			memcpy(parameter, temp, len);
			free(temp);
		}

		/* send dsi commands */
		dsi_send_cmds(fbi, parameter, len, cmd_line.lp);

		if (cmd_line.delay)
			udelay(cmd_line.delay * 1000);
	}
}

void dsi_cmd_array_rx(struct mmp_disp_info *fbi, struct dsi_buf *dbuf,
		struct dsi_cmd_desc cmds[], int count)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;
	struct dsi_regs *dsi = (struct dsi_regs *)(uintptr_t)di->regs;

	u8 parameter[DSI_DI_SET_MAX_PKT_SIZE];
	u32 i, rx_reg, timeout, tmp, packet,
	    data_pointer, byte_count;

	memset(dbuf, 0x0, sizeof(struct dsi_buf));
	dsi_cmd_array_tx(fbi, cmds, count);

	timeout = 1000;
	do {
		timeout--;
		tmp = readl(&dsi->irq_status);
	} while (((tmp & 0x4) == 0) && timeout);
	if (!timeout) {
		printf("error: dsi didn't receive packet, irq status 0x%x\n",
		       readl(&dsi->irq_status));
		return;
	}
	if (tmp & IRQ_RX_TRG2)
		printf("ACK received\n");
	if (tmp & IRQ_RX_TRG1)
		printf("TE trigger received\n");
	if (tmp & IRQ_RX_ERR) {
		printf("error: ACK with error report\n");
		tmp = readl(&dsi->rx0_header);
	}

	packet = readl(&dsi->rx0_status);

	data_pointer = (packet & RX_PKT0_PKT_PTR_MASK) >> RX_PKT0_PKT_PTR_SHIFT;
	tmp = readl(&dsi->rx_ctrl1);
	byte_count = tmp & RX_PKT_BCNT_MASK;

	memset(parameter, 0x00, byte_count);
	for (i = data_pointer; i < data_pointer + byte_count; i++) {
		rx_reg = readl(&dsi->rx_ctrl);
		rx_reg &= ~RX_PKT_RD_PTR_MASK;
		rx_reg |= RX_PKT_RD_REQ | (i << RX_PKT_RD_PTR_SHIFT);
		writel(rx_reg, &dsi->rx_ctrl);
		count = 10000;
		do {
			count--;
			rx_reg = readl(&dsi->rx_ctrl);
		} while (rx_reg & RX_PKT_RD_REQ && count);
		if (!count)
			printf("error: Rx packet FIFO error\n");
		parameter[i - data_pointer] = rx_reg & 0xff;
	}
	switch (parameter[0]) {
	case DSI_DI_ACK_ERR_RESP:
		/* printf("error: Acknowledge with error report\n"); */
		break;
	case DSI_DI_EOTP:
		printf("error: End of Transmission packet\n");
		break;
	case DSI_DI_GEN_READ1_RESP:
	case DSI_DI_DCS_READ1_RESP:
		dbuf->data_type = parameter[0];
		dbuf->length = 1;
		memcpy(dbuf->data, &parameter[1], dbuf->length);
		break;
	case DSI_DI_GEN_READ2_RESP:
	case DSI_DI_DCS_READ2_RESP:
		dbuf->data_type = parameter[0];
		dbuf->length = 2;
		memcpy(dbuf->data, &parameter[1], dbuf->length);
		break;
	case DSI_DI_GEN_LREAD_RESP:
	case DSI_DI_DCS_LREAD_RESP:
		dbuf->data_type = parameter[0];
		dbuf->length = (parameter[2] << 8) | parameter[1];
		memcpy(dbuf->data, &parameter[4], dbuf->length);
		break;
	}
}

void dsi_set_dphy(struct mmp_disp_info *fbi)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;
	struct dsi_regs *dsi = (struct dsi_regs *)(uintptr_t)di->regs;
	int ui, lpx_clk, lpx_time, ta_get, ta_go, wakeup, reg;
	int hs_prep, hs_zero, hs_trail, hs_exit, ck_zero, ck_trail, ck_exit;
	void *phy_timing0_reg, *phy_timing1_reg,
	     *phy_timing2_reg, *phy_timing3_reg;

	mi->sclk_src /= 1000000;
	ui = 1000/mi->sclk_src + 1;

	lpx_clk = (phy.lpx_constant + phy.lpx_ui * ui) / DSI_ESC_CLK_T + 1;
	lpx_time = lpx_clk * DSI_ESC_CLK_T;

	/* Below is for NT35451 */
	ta_get = lpx_time * 5 / DSI_ESC_CLK_T - 1;
	ta_go = lpx_time * 4 / DSI_ESC_CLK_T - 1;

	wakeup = phy.wakeup_constant;
	wakeup = wakeup / DSI_ESC_CLK_T + 1;

	hs_prep = phy.hs_prep_constant + phy.hs_prep_ui * ui;
	hs_prep = hs_prep / DSI_ESC_CLK_T + 1;

	/* Our hardware added 3-byte clk automatically.
	 * 3-byte 3 * 8 * ui.
	 */
	hs_zero = phy.hs_zero_constant + phy.hs_zero_ui * ui -
		(hs_prep + 1) * DSI_ESC_CLK_T;
	hs_zero = (hs_zero - (3 * ui << 3)) / DSI_ESC_CLK_T + 4;
	if (hs_zero < 0)
		hs_zero = 0;

	hs_trail = phy.hs_trail_constant + phy.hs_trail_ui * ui;
	hs_trail = ((8 * ui) >= hs_trail) ? (8 * ui) : hs_trail;
	hs_trail = hs_trail / DSI_ESC_CLK_T + 1;
	if (hs_trail > 3)
		hs_trail -= 3;
	else
		hs_trail = 0;

	hs_exit = phy.hs_exit_constant + phy.hs_exit_ui * ui;
	hs_exit = hs_exit / DSI_ESC_CLK_T + 1;

	ck_zero = phy.ck_zero_constant + phy.ck_zero_ui * ui -
		(hs_prep + 1) * DSI_ESC_CLK_T;
	ck_zero = ck_zero / DSI_ESC_CLK_T + 1;

	ck_trail = phy.ck_trail_constant + phy.ck_trail_ui * ui;
	ck_trail = ck_trail / DSI_ESC_CLK_T + 1;

	ck_exit = hs_exit;

	/* bandgap ref enabled by default */
	if (!DISP_GEN4(fbi->mi->version)) {
		reg = readl(&dsi->phy_rcomp0);
		reg |= DPHY_BG_VREF_EN;

		phy_timing0_reg = &dsi->phy_timing0;
		phy_timing1_reg = &dsi->phy_timing1;
		phy_timing2_reg = &dsi->phy_timing2;
		phy_timing3_reg = &dsi->phy_timing3;
		writel(reg, &dsi->phy_rcomp0);
	} else {
		reg = readl(&dsi->phy.phy_rcomp0);
		reg |= DPHY_BG_VREF_EN_DC4;

		phy_timing0_reg = &dsi->phy.phy_timing0;
		phy_timing1_reg = &dsi->phy.phy_timing1;
		phy_timing2_reg = &dsi->phy.phy_timing2;
		phy_timing3_reg = &dsi->phy.phy_timing3;
		writel(reg, &dsi->phy.phy_rcomp0);
	}

	/* timing_0 */
	reg = (hs_exit << DSI_PHY_TIME_0_CFG_CSR_TIME_HS_EXIT_SHIFT)
		| (hs_trail << DSI_PHY_TIME_0_CFG_CSR_TIME_HS_TRAIL_SHIFT)
		| (hs_zero << DSI_PHY_TIME_0_CDG_CSR_TIME_HS_ZERO_SHIFT)
		| (hs_prep);
	writel(reg, phy_timing0_reg);

	reg = (ta_get << DSI_PHY_TIME_1_CFG_CSR_TIME_TA_GET_SHIFT)
		| (ta_go << DSI_PHY_TIME_1_CFG_CSR_TIME_TA_GO_SHIFT)
		| wakeup;
	writel(reg, phy_timing1_reg);

	reg = (ck_exit << DSI_PHY_TIME_2_CFG_CSR_TIME_CK_EXIT_SHIFT)
		| (ck_trail << DSI_PHY_TIME_2_CFG_CSR_TIME_CK_TRAIL_SHIFT)
		| (ck_zero << DSI_PHY_TIME_2_CFG_CSR_TIME_CK_ZERO_SHIFT)
		| lpx_clk;
	writel(reg, phy_timing2_reg);

	reg = (lpx_clk << DSI_PHY_TIME_3_CFG_CSR_TIME_LPX_SHIFT)
		| phy.req_ready;
	writel(reg, phy_timing3_reg);
	/* calculated timing on brownstone:
	 * DSI_PHY_TIME_0 0x06080204
	 * DSI_PHY_TIME_1 0x6d2bfff0
	 * DSI_PHY_TIME_2 0x603130a
	 * DSI_PHY_TIME_3 0xa3c
	 */
}

#if defined(DEBUG_MMP_DISP)
#define DSIW(x, y)  do {writel(x, y); \
			printf("address %x, value %x\n", y, x); \
			} while (0)
#else
#define DSIW(x, y)  writel(x, y)
#endif
static void dsi_set_ctrl0(struct dsi_regs *dsi,
		u32 mask, u32 set)
{
	u32 tmp;

	tmp = readl(&dsi->ctrl0);
	tmp &= ~mask;
	tmp |= set;
	DSIW(tmp, &dsi->ctrl0);
}

void dsi_reset(struct dsi_info *di, int hold)
{
	struct dsi_regs *dsi = (struct dsi_regs *)(uintptr_t)di->regs;

	/* software reset DSI module */
	dsi_set_ctrl0(dsi, 0, DSI_CTRL_0_CFG_SOFT_RST);
	/* Note: there need some delay after set DSI_CTRL_0_CFG_SOFT_RST */
	udelay(1000);
	dsi_set_ctrl0(dsi, (u32)DSI_CTRL_0_CFG_LCD1_EN, 0);

	if (!hold) {
		dsi_set_ctrl0(dsi, DSI_CTRL_0_CFG_SOFT_RST, 0);
		udelay(1000);
		dsi_set_ctrl0(dsi, DSI_CTRL_0_CFG_SOFT_RST_REG, 0);
	}
}

void dsi_set_video_panel(struct mmp_disp_info *fbi)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;
	struct dsi_regs *dsi = (struct dsi_regs *)(uintptr_t)di->regs;
	struct dsi_lcd_regs *dsi_lcd = &dsi->lcd1;
	u32 hsync_b, hbp_b, hact_b, hex_b, hfp_b, httl_b;
	u32 hsync, hbp, hact, httl, v_total;
	u32 hsa_wc, hbp_wc, hact_wc, hex_wc, hfp_wc, hlp_wc;
	u32 bpp = di->bpp, hss_bcnt = 4, hse_bct = 4, lgp_over_head = 6, reg;
	u32 slot_cnt0, slot_cnt1;
	union dsi_lcd_ctrl1 lcd_ctrl1;

	void *phy_ctrl2_reg = (!DISP_GEN4(fbi->mi->version)) ?
		(&dsi->phy_ctrl2) : (&dsi->phy.phy_ctrl2);
	if (di->id & 2)
		dsi_lcd = &dsi->lcd2;

	v_total = mi->modes->yres + mi->modes->upper_margin + mi->modes->lower_margin
		+ mi->modes->vsync_len;

	hact_b = to_dsi_bcnt(mi->modes->xres, bpp);
	hfp_b = to_dsi_bcnt(mi->modes->right_margin, bpp);
	hbp_b = to_dsi_bcnt(mi->modes->left_margin, bpp);
	hsync_b = to_dsi_bcnt(mi->modes->hsync_len, bpp);
	hex_b = to_dsi_bcnt(dsi_ex_pixel_cnt, bpp);
	httl_b = hact_b + hsync_b + hfp_b + hbp_b + hex_b;
	slot_cnt0 = (httl_b - hact_b) / di->lanes  + 3;
	slot_cnt1 = slot_cnt0;

	hact = hact_b / di->lanes;
	hbp = hbp_b / di->lanes;
	hsync = hsync_b / di->lanes;
	httl = (hact_b + hfp_b + hbp_b + hsync_b) / di->lanes;

	/* word count in the unit of byte */
	hsa_wc = (di->burst_mode == DSI_BURST_MODE_SYNC_PULSE) ?
		(hsync_b - hss_bcnt - lgp_over_head) : 0;

	/* Hse is with backporch */
	hbp_wc = (di->burst_mode == DSI_BURST_MODE_SYNC_PULSE) ?
		(hbp_b - hse_bct - lgp_over_head)
		: (hsync_b + hbp_b - hss_bcnt - lgp_over_head);

	hfp_wc = ((di->burst_mode == DSI_BURST_MODE_BURST) && (dsi_hex_en == 0)) ?
		(hfp_b + hex_b - lgp_over_head - lgp_over_head) :
		(hfp_b - lgp_over_head - lgp_over_head);

	hact_wc =  ((mi->modes->xres) * bpp) >> 3;

	/* disable Hex currently */
	hex_wc = 0;

	/*  There is no hlp with active data segment.  */
	hlp_wc = (di->burst_mode == DSI_BURST_MODE_SYNC_PULSE) ?
		(httl_b - hsync_b - hse_bct - lgp_over_head) :
		(httl_b - hss_bcnt - lgp_over_head);

	/* FIXME - need to double check the (*3) is bytes_per_pixel
	 * from input data or output to panel
	 */
	/* dsi_lane_enable - Set according to specified DSI lane count */
	DSIW(dsi_lane[di->lanes] << DSI_PHY_CTRL_2_CFG_CSR_LANE_EN_SHIFT,
	     phy_ctrl2_reg);
	DSIW(dsi_lane[di->lanes] << DSI_CPU_CMD_1_CFG_TXLP_LPDT_SHIFT,
	     &dsi->cmd1);

	/* SET UP LCD1 TIMING REGISTERS FOR DSI BUS */
	/* NOTE: Some register values were obtained by trial and error */
	DSIW((hact << 16) | httl, &dsi_lcd->timing0);
	DSIW((hsync << 16) | hbp, &dsi_lcd->timing1);
	/*
	 * For now the active size is set really low (we'll use 10) to allow
	 * the hardware to attain V Sync. Once the DSI bus is up and running,
	 * the final value will be put in place for the active size (this is
	 * done below). In a later stepping of the processor this requirement
	 * will not be required.
	 */
	DSIW(((mi->modes->yres)<<16) | (v_total), &dsi_lcd->timing2);
	DSIW(((mi->modes->vsync_len) << 16) | (mi->modes->upper_margin),
	     &dsi_lcd->timing3);

	/* SET UP LCD1 WORD COUNT REGISTERS FOR DSI BUS */
	/* Set up for word(byte) count register 0 */
	DSIW((hbp_wc << 16) | hsa_wc, &dsi_lcd->wc0);
	DSIW((hfp_wc << 16) | hact_wc, &dsi_lcd->wc1);
	DSIW((hex_wc << 16) | hlp_wc, &dsi_lcd->wc2);

	DSIW((slot_cnt0 << 16) | slot_cnt0, &dsi_lcd->slot_cnt0);
	DSIW((slot_cnt1 << 16) | slot_cnt1, &dsi_lcd->slot_cnt1);

	/* Configure LCD control register 1 FOR DSI BUS */
	lcd_ctrl1.val = readl(&dsi_lcd->ctrl1);
	lcd_ctrl1.bit.input_fmt = di->rgb_mode;
	lcd_ctrl1.bit.burst_mode = di->burst_mode;
	lcd_ctrl1.bit.lpm_frame_en = di->lpm_line_en;
	lcd_ctrl1.bit.last_line_turn = di->last_line_turn;
	lcd_ctrl1.bit.hex_slot_en = 0;
	lcd_ctrl1.bit.hsa_pkt_en = di->hsa_en;
	lcd_ctrl1.bit.hse_pkt_en = di->hse_en;
	if (di->burst_mode == DSI_BURST_MODE_SYNC_PULSE)
		lcd_ctrl1.bit.hse_pkt_en = 1;
	else
		lcd_ctrl1.bit.hsa_pkt_en = 0;
	lcd_ctrl1.bit.hbp_pkt_en = di->hbp_en;
	lcd_ctrl1.bit.hfp_pkt_en = di->hfp_en;
	lcd_ctrl1.bit.hex_pkt_en = 0;
	lcd_ctrl1.bit.hlp_pkt_en = di->hlp_en;
	lcd_ctrl1.bit.vsync_rst_en = 1;
	if (!DISP_GEN4(fbi->mi->version)) {
		/* These bits were removed for DC4 */
		lcd_ctrl1.bit.lpm_line_en = di->lpm_line_en;
		lcd_ctrl1.bit.hact_pkt_en = di->hact_en;
		lcd_ctrl1.bit.m2k_en = 0;
		lcd_ctrl1.bit.all_slot_en = 0;
	} else {
		/*
		 * FIXME: some extend features in DC4, default enable
		 * 1. word count auto calculation.
		 * 2. check timing before request DPHY for TX.
		 * 3. auto vsync delay count calculation.
		 */
		lcd_ctrl1.bit.auto_wc_dis = 0;
		lcd_ctrl1.bit.hact_wc_en = 0;
		lcd_ctrl1.bit.timing_check_dis = 0;
		lcd_ctrl1.bit.auto_dly_dis = 0;
	}
	DSIW(lcd_ctrl1.val, &dsi_lcd->ctrl1);

	/*Start the transfer of LCD data over the DSI bus*/
	/* DSI_CTRL_1 */
	reg = readl(&dsi->ctrl1);
	reg &= ~(DSI_CTRL_1_CFG_LCD2_VCH_NO_MASK
			| DSI_CTRL_1_CFG_LCD1_VCH_NO_MASK);
	reg |= 0x1 << ((di->id & 1) ? DSI_CTRL_1_CFG_LCD2_VCH_NO_SHIFT
			: DSI_CTRL_1_CFG_LCD1_VCH_NO_SHIFT);

	reg &= ~(DSI_CTRL_1_CFG_EOTP);
	if (di->eotp_en)
		reg |= DSI_CTRL_1_CFG_EOTP;	/* EOTP */

	DSIW(reg, &dsi->ctrl1);
	/* DSI_CTRL_0 */
	reg = (di->master_mode ? 0 : DSI_CTRL_0_CFG_LCD1_SLV) |
		 DSI_CTRL_0_CFG_LCD1_TX_EN | DSI_CTRL_0_CFG_LCD1_EN;
	if (di->id & 2)
		reg = reg << 1;
	dsi_set_ctrl0(dsi, DSI_CTRL_0_CFG_LCD1_SLV, reg);
	udelay(100000);
}

/*
 * FIXME: The read id commands is only for sharp panel.
 */
#define SHARP_NO_DELAY	0
static u8 sharp_pkt_size_cmd[] = {0x1};
static u8 read_id1[] = {0xda};
static u8 read_id2[] = {0xdb};
static u8 read_id3[] = {0xdc};

static u8 tft_pkt_size_cmd[] = {0x1};
static u8 read_tft_id[] = {0xF4};
#define TFT_1080P_ID 0x96

#define SHARP_1080P_ID 0x38
#define SHARP_QHD_ID 0x8000
#define HX8394A_720P_ID 0x83941A
#define HX8394D_JT_720P_ID 0x83940D

static struct dsi_cmd_desc tft_read_id_cmds[] = {
	{DSI_DI_SET_MAX_PKT_SIZE, 1, 0, sizeof(tft_pkt_size_cmd),
		tft_pkt_size_cmd},
	{DSI_DI_DCS_READ, 1, 0, sizeof(read_tft_id), read_tft_id},
};

static struct dsi_cmd_desc sharp_read_id1_cmds[] = {
	{DSI_DI_SET_MAX_PKT_SIZE, 1, SHARP_NO_DELAY, sizeof(sharp_pkt_size_cmd),
		sharp_pkt_size_cmd},
	{DSI_DI_DCS_READ, 1, SHARP_NO_DELAY, sizeof(read_id1), read_id1},
};

static struct dsi_cmd_desc sharp_read_id2_cmds[] = {
	{DSI_DI_SET_MAX_PKT_SIZE, 1, SHARP_NO_DELAY, sizeof(sharp_pkt_size_cmd),
		sharp_pkt_size_cmd},
	{DSI_DI_DCS_READ, 1, SHARP_NO_DELAY, sizeof(read_id2), read_id2},
};

static struct dsi_cmd_desc sharp_read_id3_cmds[] = {
	{DSI_DI_SET_MAX_PKT_SIZE, 1, SHARP_NO_DELAY, sizeof(sharp_pkt_size_cmd),
		sharp_pkt_size_cmd},
	{DSI_DI_DCS_READ, 1, SHARP_NO_DELAY, sizeof(read_id3), read_id3},
};


static int panel_read_ftf_id(struct mmp_disp_info *fbi)
{
	struct dsi_buf *dbuf;
	u32 read_id = 0;

	dbuf = malloc(sizeof(struct dsi_buf));
	if (dbuf) {
		dsi_cmd_array_rx(fbi, dbuf, tft_read_id_cmds,
				 ARRAY_SIZE(tft_read_id_cmds));
		read_id |= dbuf->data[0];
		free(dbuf);
		printf("read tft panel id:0x%x\n", read_id);
		return read_id;
	} else {
		printf("%s: can't alloc dsi rx buffer\n", __func__);
		return -1;
	}
}

static int panel_read_sharp_id(struct mmp_disp_info *fbi)
{
	struct dsi_buf *dbuf;
	u32 read_id = 0;

	dbuf = malloc(sizeof(struct dsi_buf));
	if (dbuf) {
		dsi_cmd_array_rx(fbi, dbuf, sharp_read_id1_cmds,
				 ARRAY_SIZE(sharp_read_id1_cmds));
		read_id |= dbuf->data[0] << 16;
		dsi_cmd_array_rx(fbi, dbuf, sharp_read_id2_cmds,
				 ARRAY_SIZE(sharp_read_id2_cmds));
		read_id |= dbuf->data[0] << 8;
		dsi_cmd_array_rx(fbi, dbuf, sharp_read_id3_cmds,
				 ARRAY_SIZE(sharp_read_id3_cmds));
		read_id |= dbuf->data[0];
		free(dbuf);
		printf("read sharp panel id:0x%x\n", read_id);
		return read_id;
	} else {
		printf("%s: can't alloc dsi rx buffer\n", __func__);
		return -1;
	}
}

/*
 * OTM8108b: read panel id
 */
static u8 otm8018b_pkt_size_cmd[] = {0x4};
static u8 otm8018b_read_id[] = {0xa1};
#define OTM8018B_ID 0x8009
#define OTM1283A_ID 0x1283

static struct dsi_cmd_desc otm8018b_read_id_cmds[] = {
	{DSI_DI_SET_MAX_PKT_SIZE, 1, SHARP_NO_DELAY, sizeof(otm8018b_pkt_size_cmd),
		otm8018b_pkt_size_cmd},
	{DSI_DI_DCS_READ, 1, 0, sizeof(otm8018b_read_id), otm8018b_read_id},
};

static int panel_read_otm_id(struct mmp_disp_info *fbi)
{
	struct dsi_buf *dbuf;
	u32 read_id = 0;

	dbuf = malloc(sizeof(struct dsi_buf));
	if (dbuf) {
		dsi_cmd_array_rx(fbi, dbuf, otm8018b_read_id_cmds,
				ARRAY_SIZE(otm8018b_read_id_cmds));
		read_id |= dbuf->data[0]<<8;
		read_id |= dbuf->data[1];
		printf("get panel supply is %x\n",read_id);

		read_id = 0;
		read_id |= dbuf->data[2]<<8;
		read_id |= dbuf->data[3];

		free(dbuf);
		printf("read otm panel id: 0x%x\n", read_id);
		return read_id;
	} else {
		printf("%s: can't alloc dsi rx buffer\n", __func__);
		return -1;
	}
}

static u8 nt35521_pkt_set_cmd[] = {0xF0,0x55,0xaa,0x52,0x08,0x01};
static u8 nt35521_pkt_size_cmd[] = {0x3};
static u8 read_nt35521_id[] = {0xC5};
#define NT35521_720P_ID 0x21

static struct dsi_cmd_desc nt35521_set_cmds[] = {
	{DSI_DI_DCS_LWRITE, 1, 1, sizeof(nt35521_pkt_set_cmd),
		nt35521_pkt_set_cmd},
};

static struct dsi_cmd_desc nt35521_read_id_cmds[] = {
	{DSI_DI_SET_MAX_PKT_SIZE, 1, 0, sizeof(nt35521_pkt_size_cmd),
		nt35521_pkt_size_cmd},
	{DSI_DI_DCS_READ, 1, 0, sizeof(read_nt35521_id), read_nt35521_id},
};

static int panel_read_nt35521_id(struct mmp_disp_info *fbi)
{
	struct dsi_buf *dbuf;
	u32 read_id = 0;
	
	dbuf = malloc(sizeof(struct dsi_buf));
	if (dbuf) {
		dsi_cmd_array_tx(fbi, nt35521_set_cmds,
			ARRAY_SIZE(nt35521_set_cmds));
		
		dsi_cmd_array_rx(fbi, dbuf, nt35521_read_id_cmds,
				 ARRAY_SIZE(nt35521_read_id_cmds));
		read_id |= dbuf->data[1];
		free(dbuf);
		printf("read nt35521 panel id:0x%x\n", read_id);
		return read_id;
	} else {
		printf("%s: can't alloc dsi rx buffer\n", __func__);
		return -1;
	}
}

static u8 r63315_pkt_set_cmd[] = {0xb0,0x04};
static u8 r63315_pkt_size_cmd[] = {0x4};
static u8 read_r63315_id[] = {0xbf};
#define R63315_1080P_ID 0x3315

static struct dsi_cmd_desc r63315_set_cmds[] = {
	{DSI_DI_GENERIC_LWRITE, 1, 1, sizeof(r63315_pkt_set_cmd),
		r63315_pkt_set_cmd},
};

static struct dsi_cmd_desc r63315_read_id_cmds[] = {
	{DSI_DI_SET_MAX_PKT_SIZE, 1, 0, sizeof(r63315_pkt_size_cmd),
		r63315_pkt_size_cmd},
	{DSI_DI_GENERIC_READ1, 1, 0, sizeof(read_r63315_id), read_r63315_id},
};

static int panel_read_r63315_id(struct mmp_disp_info *fbi)
{
	struct dsi_buf *dbuf;
	u32 read_id = 0;

	dbuf = malloc(sizeof(struct dsi_buf));
	if (dbuf) {
		dsi_cmd_array_tx(fbi, r63315_set_cmds,
			ARRAY_SIZE(r63315_set_cmds));

		dsi_cmd_array_rx(fbi, dbuf, r63315_read_id_cmds,
				 ARRAY_SIZE(r63315_read_id_cmds));
		if(dbuf->data[2]== 0x33 || dbuf->data[3] == 0x15)
			read_id = R63315_1080P_ID;
		free(dbuf);
		printf("read r63315 panel id:0x%x\n", read_id);
		return read_id;
	} else {
		printf("%s: can't alloc dsi rx buffer\n", __func__);
		return -1;
	}
}

/*
 * FL10802 read id
 */
#define FL10802_ID 0x0307
/* for panel fl10802a */
static u8 fl10802a_pkt_size_cmd[] = {0x06, 0x0};
static u8 fl10802a_manufaceture_init[6] = {0xB9, 0xF1, 0x08, 0x01};
static u8 read_id_0xfl10802a[] = {0xD0, 0x00};
static struct dsi_cmd_desc fl10802a_read_id_cmds[] = {
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(fl10802a_manufaceture_init),
		fl10802a_manufaceture_init},
	{DSI_DI_SET_MAX_PKT_SIZE, 1, 0, sizeof(fl10802a_pkt_size_cmd),
		fl10802a_pkt_size_cmd},
	{DSI_DI_DCS_READ, 1, 0, sizeof(read_id_0xfl10802a), read_id_0xfl10802a},
};

static int panel_read_fl10802_id(struct mmp_disp_info *fbi)
{
	struct dsi_buf *dbuf;
	u32 read_id = 0;

	dbuf = malloc(sizeof(struct dsi_buf));
	if (dbuf) {
		/* check whether it is fl10802a panel */
		dsi_cmd_array_rx(fbi, dbuf, fl10802a_read_id_cmds,
				ARRAY_SIZE(fl10802a_read_id_cmds));
		read_id = 0;
		read_id |= dbuf->data[0]<<8;
		read_id |= dbuf->data[5];
		free(dbuf);
		printf("read fl10802 panel id: 0x%x\n", read_id);
		return read_id;
	}

	printf("%s: can't alloc dsi rx buffer\n", __func__);
	return -1;
}

static int panel_read_id(struct mmp_disp_info *fbi)
{
	int read_id;

	read_id = panel_read_nt35521_id(fbi);
	if (NT35521_720P_ID == read_id)
		return read_id;

	read_id = panel_read_ftf_id(fbi);
	if (TFT_1080P_ID == read_id)
		return read_id;
	read_id = panel_read_sharp_id(fbi);
	if (HX8394A_720P_ID == read_id ||
			SHARP_1080P_ID == read_id ||
			SHARP_QHD_ID == read_id ||
			HX8394D_JT_720P_ID == read_id)
		return read_id;
	read_id = panel_read_r63315_id(fbi);
	if (R63315_1080P_ID == read_id)
		return read_id;
	read_id = panel_read_otm_id(fbi);
	if (OTM8018B_ID == read_id || OTM1283A_ID == read_id)
		return read_id;

	read_id = panel_read_fl10802_id(fbi);
	if (FL10802_ID == read_id)
		return read_id;

	return -1;
}

static void dsi_set_dphy_ctrl1(struct dsi_info *di,
		u32 mask, u32 set, unsigned version)
{
	struct dsi_regs *dsi = (struct dsi_regs *)(uintptr_t)di->regs;
	void *phy_ctrl1_reg = (!DISP_GEN4(version)) ?
		(&dsi->phy_ctrl1) : (&dsi->phy.phy_ctrl1);
	u32 tmp;

	tmp = readl(phy_ctrl1_reg);
	tmp &= ~mask;
	tmp |= set;
	writel(tmp, phy_ctrl1_reg);
}

void mmp_disp_dsi_init_dphy(struct mmp_disp_info *fbi)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;

	/* reset and de-assert DSI controller */
	dsi_reset(di, 0);

	/* digital and analog power on */
	dsi_set_dphy_ctrl1(di, 0x0, DPHY_CFG_VDD_VALID,
			   fbi->mi->version);

	/* turn on DSI continuous clock for HS */
	dsi_set_dphy_ctrl1(di, 0x0, 0x1,
			   fbi->mi->version);

	/* set dphy */
	dsi_set_dphy(fbi);

	/* enable data lanes */
	dsi_lanes_enable(di, 0, fbi->mi->version);
	dsi_lanes_enable(di, 1, fbi->mi->version);

	if (mi->phy_type == DSI) {
		/*
		 * If dynamic_detect_panel flag is set and
		 * detected panel id does not match the default one,
		 * need to reset panel related info
		 * and do the intial flow again.
		 */
		if (mi->dynamic_detect_panel == 1) {
			g_panel_id = panel_read_id(fbi);
			if (mi->update_panel_info)
				mi->update_panel_info(mi);
			mi->dynamic_detect_panel = 0;
		}
	}
}

int mmp_disp_dsi_init_panel(struct mmp_disp_info *fbi)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;
	int ret = 0;

	/* enable data lanes */
	dsi_lanes_enable(di, 0, fbi->mi->version);
	dsi_lanes_enable(di, 1, fbi->mi->version);

	/* init panel settings via dsi */
	if (mi->phy_type == DSI)
		mi->dsi_panel_config(fbi);

	/*  reset the bridge */
	if (mi->phy_type == DSI2DPI) {
		if (mi->xcvr_reset)
			mi->xcvr_reset();
		else
			tc358765_reset();
	}
	/* set dsi controller */
	dsi_set_video_panel(fbi);

	/* set dsi to dpi conversion chip */
	if (mi->phy_type == DSI2DPI) {
		ret = dsi_set_tc358765(fbi);
		if (ret < 0)
			printf("dsi2dpi_set error!\n");
	}

	return 0;
}
