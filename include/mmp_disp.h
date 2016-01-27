/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_MACH_PXA168FB_H
#define __ASM_MACH_PXA168FB_H

#include <linux/types.h>
#include <linux/list.h>

#define DISPLAY_CONTROLLER_BASE			0xD420B000
#ifdef CONFIG_PXA1U88
#define VDMA_CONTROLLER_BASE			0xD4209000
#else
#define VDMA_CONTROLLER_BASE			0xD427F000
#endif
#define LCD_TOP_CTRL				0x01DC
/* select max burst length and wait cycle */
#define TOP_CTRL_DEFAULT			0xfff0

/* DISP GEN4 Registers */
#define LCD_SHADOW_CTRL                         (0x2C0)
#define LCD_VERSION                             (0x0240)
#define DISP_GEN4(version)		((version) == 4 || (version) == 0x14 || (version) == 0x24)
#define DISP_GEN4_LITE(version)		((version) == 0x14)
#define DISP_GEN4_PLUS(version)		((version) == 0x24)

/* VDMA SQU Register */
#define LCD_PN_SQULN_CTRL                       (0x01E0)
#define LCD_SCALING_SQU_CTL_INDEX               (0x02AC)
#define LCD_VDMA_SEL_CTRL			(0x02F0)
/* select 4 sram lines by default */
#define VDMA_SRAM_LINES		(4)
#define AXI_RD_CNT_MAX_SHIFT			((fbi->mi->version == 0x14) ? (16) : (20))
#define AXI_RD_CNT_MAX(cnt)			((cnt) << AXI_RD_CNT_MAX_SHIFT)
#define DC_ENA(en)				((en) ? 2 : 0)
#define CH_ENA(en)				(en)

/* FIXME: By default we just enable vdma channel 0/2/3/4 clocks
 * and decompression clock */
#define VDMA_CLK_CTRL		(0xe2)

#define DSI1_REG_BASE				0xD420B800

/* DMA Control 0 Register */
#define LCD_SPU_DMA_CTRL0			0x0190
#define     CFG_ARBFAST_ENA(an)			((an)<<27)
/* for graphic part */
#define     CFG_GRA_HSMOOTH(smooth)		((smooth)<<14)

/* for video part */
#define     CFG_DMA_HSMOOTH(smooth)		((smooth)<<6)

/* DMA Control 1 Register */
#define LCD_SPU_DMA_CTRL1			0x0194
#define  CFG_VSYNC_INV(inv)                     ((inv)<<27)
#define  CFG_VSYNC_INV_MASK                     0x08000000
#define  CFG_ALPHA_MODE(amode)                  ((amode)<<16)
#define  CFG_ALPHA_MODE_MASK                    0x00030000
#define  CFG_ALPHA(alpha)                       ((alpha)<<8)
#define  CFG_ALPHA_MASK                         0x0000FF00

/* Smart or Dumb Panel Clock Divider */
#define LCD_CFG_SCLK_DIV			0x01A8
/* Dump LCD Panel Control Register */
#define LCD_SPU_DUMB_CTRL			0x01B8
/* LCD I/O Pads Control Register */
#define SPU_IOPAD_CONTROL			0x01BC
/* csc */
#define     CFG_CYC_BURST_LEN16			(1<<4)
#define     CFG_CYC_BURST_LEN8			(0<<4)

#define	SPU_IRQ_ISR				0x01C4

/* 32 bit LCD Mixed Overlay Control Register */
#define LCD_AFA_ALL2ONE                         0x01E8

/* LCD Interrupt Control Register */
#define     GRA_FRAME_IRQ0_ENA_MASK		0x08000000
#define     GRA_FRAME_IRQ1_ENA_MASK		0x04000000
#define     VSYNC_IRQ_ENA_MASK			0x00800000
#define     DUMB_FRAMEDONE_ENA_MASK		0x00400000
#define	    TVSYNC_IRQ_ENA_MASK			0x00001000
#define     TV_FRAME_IRQ0_ENA_MASK		0x00000800
#define     TV_FRAME_IRQ1_ENA_MASK              0x00000400
#define     TV_FRAMEDONE_ENA_MASK		0x00000100

#define  TV_DMA_FRAME_IRQ0_ENA_MASK             0x00008000
#define  TV_DMA_FRAME_IRQ1_ENA_MASK             0x00004000

/* FIXME - JUST GUESS */
#define	    PN2_DMA_FRAME_IRQ0_ENA_MASK		0x00000080
#define	    PN2_DMA_FRAME_IRQ1_ENA_MASK		0x00000040
#define     PN2_GRA_FRAME_IRQ0_ENA_MASK		0x00000008
#define     PN2_GRA_FRAME_IRQ1_ENA_MASK		0x04000004
#define     PN2_SYNC_IRQ_ENA_MASK               0x00000001

#define gf0_imask(id)	((id) ? (((id) & 1) ? TV_FRAME_IRQ0_ENA_MASK \
		: PN2_GRA_FRAME_IRQ0_ENA_MASK) : GRA_FRAME_IRQ0_ENA_MASK)
#define gf1_imask(id)	((id) ? (((id) & 1) ? TV_FRAME_IRQ1_ENA_MASK \
		: PN2_GRA_FRAME_IRQ1_ENA_MASK) : GRA_FRAME_IRQ1_ENA_MASK)
#define vsync_imask(id)	((id) ? (((id) & 1) ? TVSYNC_IRQ_ENA_MASK \
		: PN2_SYNC_IRQ_ENA_MASK) : VSYNC_IRQ_ENA_MASK)

#define display_done_imask(id)	((id) ? (((id) & 1) ? TV_FRAMEDONE_ENA_MASK\
	: (PN2_DMA_FRAME_IRQ0_ENA_MASK | PN2_DMA_FRAME_IRQ1_ENA_MASK))\
	: DUMB_FRAMEDONE_ENA_MASK)
#define LCD_PN2_CTRL0                           (0x02C8)
#define LCD_DUMB2_CTRL                          (0x02d8)
#define LCD_PN2_CTRL1                           (0x02DC)
#define LCD_PN2_SCLK_DIV                        (0x01EC)

#define LCD_TCLK_DIV		(0x009C)
#define clk_div(id)     ((id) ? ((id) & 1 ? LCD_TCLK_DIV : LCD_PN2_SCLK_DIV) \
		: LCD_CFG_SCLK_DIV)

/*
 * panel interface
 */
#define DPI             0
#define DSI2DPI         1
#define DSI             2

#define fb_base         0
#define fb_dual         1
#define FB_MODE_DUP     ((fbi->id == fb_base) && fb_mode && \
		gfx_info.fbi[fb_dual])

/* DSI burst mode */
#define DSI_BURST_MODE_SYNC_PULSE                       0x0
#define DSI_BURST_MODE_SYNC_EVENT                       0x1
#define DSI_BURST_MODE_BURST                            0x2

/* DSI input data RGB mode */
#define DSI_LCD_INPUT_DATA_RGB_MODE_565			0
#define DSI_LCD_INPUT_DATA_RGB_MODE_666PACKET		1
#define DSI_LCD_INPUT_DATA_RGB_MODE_666UNPACKET		2
#define DSI_LCD_INPUT_DATA_RGB_MODE_888			3

/* ------------< LCD register >------------ */
struct lcd_regs {
	/* Video Frame 0/1 Y/U/V/Command Starting Addr */
	u32 v_y0;
	u32 v_u0;
	u32 v_v0;
	u32 v_c0;
	union {
		/* frame1 start address for legacy LCD controller */
		u32 v_y1;
		/* SQU start address */
		u32 v_squln_y;
	};
	union {
		u32 v_u1;
		u32 v_squln_u;
	};
	union {
		u32 v_v1;
		u32 v_squln_v;
	};
	u32 v_c1;
	/* Video Y and C Line Length (Pitch) */
	u32 v_pitch_yc;
	/* Video U and V Line Length (Pitch) */
	u32 v_pitch_uv;
	/* Video Starting Point on Screen */
	u32 v_start;
	/* Video Source Size */
	u32 v_size;
	/* Video Destination Size (After Zooming) */
	u32 v_size_z;
	/* Graphic Frame 0/1 Starting Address */
	u32 g_0;
	union {
		u32 g_1;
		u32 g_squln;
	};
	/* Graphic Line Length (Pitch) */
	u32 g_pitch;
	/* Graphic Starting Point on Screen */
	u32 g_start;
	/* Graphic Source Size */
	u32 g_size;
	/* Graphic Destination Size (After Zooming) */
	u32 g_size_z;
	/* Hardware Cursor */
	u32 hc_start;
	/* Hardware Cursor */
	u32 hc_size;
	/* Screen Total Size */
	u32 screen_size;
	/* Screen Active Size */
	u32 screen_active;
	/* Screen Horizontal Porch */
	u32 screen_h_porch;
	/* Screen Vertical Porch */
	u32 screen_v_porch;
	/* Screen Blank Color */
	u32 blank_color;
	/* Hardware Cursor Color1 */
	u32 hc_Alpha_color1;
	/* Hardware Cursor Color2 */
	u32 hc_Alpha_color2;
	/* Video Y Color Key Control */
	u32 v_colorkey_y;
	/* Video U Color Key Control */
	u32 v_colorkey_u;
	/* Video V Color Key Control */
	u32 v_colorkey_v;
	/* VSYNC PulsePixel Edge Control */
	u32 vsync_ctrl;
};

#define intf_ctrl(id)		((id) ? (((id) & 1) ? LCD_TVIF_CTRL \
			: LCD_DUMB2_CTRL) : LCD_SPU_DUMB_CTRL)
#define dma_ctrl0(id)           ((id) ? (((id) & 1) ? LCD_TV_CTRL0 \
			: LCD_PN2_CTRL0) : LCD_SPU_DMA_CTRL0)
#define dma_ctrl1(id)           ((id) ? (((id) & 1) ? LCD_TV_CTRL1 \
			: LCD_PN2_CTRL1) : LCD_SPU_DMA_CTRL1)
#define dma_ctrl(ctrl1, id)     (ctrl1 ? dma_ctrl1(id) : dma_ctrl0(id))

/* 32 bit       TV Path DMA Control 0*/
#define LCD_TV_CTRL0                     (0x0080)
/* 32 bit       TV Path DMA Control 1*/
#define LCD_TV_CTRL1                     (0x0084)
/* 32 bit TV Path TVIF Control  Register */
#define LCD_TVIF_CTRL                    (0x0094)

/*
 * Buffer pixel format
 * bit0 is for rb swap.
 * bit12 is for Y UorV swap
 */
#define PIX_FMT_RGB565		0
#define PIX_FMT_BGR565		1
#define PIX_FMT_RGB1555		2
#define PIX_FMT_BGR1555		3
#define PIX_FMT_RGB888PACK	4
#define PIX_FMT_BGR888PACK	5
#define PIX_FMT_RGB888UNPACK	6
#define PIX_FMT_BGR888UNPACK	7
#define PIX_FMT_RGBA888		8
#define PIX_FMT_BGRA888		9
#define PIX_FMT_YUV422PACK	10
#define PIX_FMT_YVU422PACK	11
#define PIX_FMT_YUV422PLANAR	12
#define PIX_FMT_YVU422PLANAR	13
#define PIX_FMT_YUV420PLANAR	14
#define PIX_FMT_YVU420PLANAR	15
#define PIX_FMT_PSEUDOCOLOR	20
#define PIX_FMT_UYVY422PACK	(0x1000|PIX_FMT_YUV422PACK)

/* VDMA channel registers */
struct mmp_vdma_ch_reg {
	u32 dc_saddr;
	u32 dc_size;
	u32 ctrl;
	u32 src_size;
	u32 src_addr;
	u32 dst_addr;
	u32 dst_size;
	u32 pitch;
	u32 ram_ctrl0;
	u32 reserved0[0x3];
	u32 hdr_addr;
	u32 hdr_size;
	u32 hskip;
	u32 vskip;
	union {
		u32 dbg0;
		u32 ch_err_stat;
	};
	union {
		u32 dbg1;
		u32 ch_dbg1;
	};
	union {
		u32 dbg2;
		u32 ch_dbg2;
	};
	union {
		u32 dbg3;
		u32 ch_dbg3;
	};
	u32 ch_dbg4;
	u32 ch_dbg5;
	u32 ch_dbg6;
	u32 ch_dbg7;
	u32 reserved1[0x28];
};

/* VDMA controller registers */
struct mmp_vdma_reg {
	union {
		u32 irqr_0to3;
		u32 irq;
	};
	union {
		u32 irqr_4to7;
		u32 irq_en;
	};
	u32 irqm_0to3;
	u32 irqm_4to7;
	u32 irqs_0to3;
	u32 irqs_4to7;
	u32 reserved0[0x2];
	u32 main_ctrl;
	u32 clk_ctrl;
	u32 dec_ctrl;
	u32 dec_dbg;
	u32 reserved2[0x4];
	u32 top_dbg0;
	u32 top_dbg1;
	u32 top_dbg2;
	u32 top_dbg3;
	u32 top_dbg4;
	u32 top_dbg5;
	u32 top_dbg6;
	u32 top_dbg7;
	u32 reserved3[0x28];
	struct mmp_vdma_ch_reg ch0_reg;/* Channel 0: VDMA1 */
};

struct dsi_phy_timing {
	unsigned int hs_prep_constant;    /* Unit: ns. */
	unsigned int hs_prep_ui;
	unsigned int hs_zero_constant;
	unsigned int hs_zero_ui;
	unsigned int hs_trail_constant;
	unsigned int hs_trail_ui;
	unsigned int hs_exit_constant;
	unsigned int hs_exit_ui;
	unsigned int ck_zero_constant;
	unsigned int ck_zero_ui;
	unsigned int ck_trail_constant;
	unsigned int ck_trail_ui;
	unsigned int req_ready;
	unsigned int wakeup_constant;
	unsigned int wakeup_ui;
	unsigned int lpx_constant;
	unsigned int lpx_ui;
};

struct dsi_info {
	unsigned        id;
	unsigned        regs;
	unsigned        lanes;
	unsigned        bpp;
	unsigned        rgb_mode;
	unsigned        burst_mode;
	unsigned        master_mode;
	unsigned        lpm_line_en;
	unsigned        lpm_frame_en;
	unsigned        last_line_turn;
	unsigned        hex_slot_en;
	unsigned        all_slot_en;
	unsigned        hbp_en;
	unsigned        hact_en;
	unsigned        hfp_en;
	unsigned        hex_en;
	unsigned        hlp_en;
	unsigned        hsa_en;
	unsigned        hse_en;
	unsigned        eotp_en;
/* For DC4 DSI_VPN_CTRL_1 0xD420B904 bit [27..24] */
	unsigned        hact_wc_dis;
	unsigned        auto_wc_en;
	unsigned        timing_check_dis;
	unsigned        auto_dly_dis;

	struct dsi_phy_timing  *phy;
};

union dsi_lcd_ctrl1 {
	struct {
		u32 input_fmt:2;     /* 1:0 */
		u32 burst_mode:2;    /* 3:2 */
		u32 reserved0:4;     /* 7:4 */
		u32 lpm_line_en:1;   /* 8 */
		u32 lpm_frame_en:1;  /* 9 */
		u32 last_line_turn:1;/* 10 */
		u32 reserved1:3;     /* 13:11 */
		u32 hex_slot_en:1;   /* 14 */
		u32 all_slot_en:1;   /* 15 */
		u32 hsa_pkt_en:1;    /* 16 */
		u32 hse_pkt_en:1;    /* 17 */
		u32 hbp_pkt_en:1;    /* 18 */
		u32 hact_pkt_en:1;   /* 19 */
		u32 hfp_pkt_en:1;    /* 20 */
		u32 hex_pkt_en:1;    /* 21 */
		u32 hlp_pkt_en:1;    /* 22 */
		u32 reserved2:1;     /* 23 */
		u32 auto_dly_dis:1;  /* 24 */
		u32 timing_check_dis:1;/* 25 */
		u32 hact_wc_en:1;    /* 26 */
		u32 auto_wc_dis:1;   /* 27 */
		u32 reserved3:2;     /* 29:28 */
		u32 m2k_en:1;        /* 30 */
		u32 vsync_rst_en:1;  /* 31 */
	} bit;
	u32 val;
};

struct mmp_disp_info;
struct mmp_disp_plat_info {
	char    id[16];
	unsigned	index;
	unsigned	version;
	unsigned int    sclk_src;
	unsigned int    sclk_div;

	int             num_modes;
	struct lcd_videomode *modes;
	unsigned int max_fb_size;

	u32	bits_per_pixel;
	/*
	 * Pix_fmt
	 */
	unsigned        pix_fmt;

	/*
	 * Burst length
	 */
	unsigned        burst_len;

	/*
	 * I/O pin allocation.
	 */
	unsigned int    io_pin_allocation_mode;

	/*
	 * Dumb panel -- assignment of R/G/B component info to the 24
	 * available external data lanes.
	 */
	unsigned        dumb_mode:4;
	unsigned        panel_rgb_reverse_lanes:1;

	/*
	 * Dumb panel -- GPIO output data.
	 */
	unsigned        gpio_output_mask:8;
	unsigned        gpio_output_data:8;

	/*
	 * Dumb panel -- configurable output signal polarity.
	 */
	unsigned        invert_composite_blank:1;
	unsigned        invert_pix_val_ena:1;
	unsigned        invert_pixclock:1;
	unsigned        invert_vsync:1;
	unsigned        invert_hsync:1;
	unsigned        panel_rbswap:1;
	unsigned        active:1;
	unsigned        enable_lcd:1;

	/*
	 * panel interface
	 */
	unsigned int    phy_type;
	unsigned int    twsi_id;

	/*
	 * vdma option
	 */
	unsigned int vdma_enable;

	/* phy interface info */
	void *phy_info;

	/*
	 * dsi to dpi setting function
	 */
	int (*xcvr_reset)(void);

	/* init config for panel via dsi */
	void (*dsi_panel_config)(struct mmp_disp_info *);
	/* update panel */
	void (*update_panel_info)(struct mmp_disp_plat_info *);
	/* calculate clk */
	void (*calculate_sclk)(struct mmp_disp_info *, int dsiclk, int pathclk);

	/* dynamical panel detect flag*/
	int dynamic_detect_panel;
	unsigned int panel_manufacturer_id;
};

struct mmp_disp_info {
	int                     id;
	void			*reg_base;
	void			*vdma_reg_base;

	void                    *fb_start;
	int                     fb_size;
	int                     pix_fmt;
	unsigned                is_blanked:1;
	unsigned                edid:1;
	unsigned                cursor_enabled:1;
	unsigned                cursor_cfg:1;
	unsigned                panel_rbswap:1;
	unsigned                debug:1;
	unsigned                active:1;
	unsigned                enabled:1;
	unsigned                edid_en:1;

	/*
	 * 0: DMA mem is from DMA region.
	 * 1: DMA mem is from normal region.
	 */
	unsigned                mem_status:1;

	struct mmp_disp_plat_info	*mi;
	struct lcd_var_screeninfo	*var;
};

#define FB_SYNC_HOR_HIGH_ACT    1       /* horizontal sync high active  */
#define FB_SYNC_VERT_HIGH_ACT   2       /* vertical sync high active    */

/*       DSI Controller Registers       */
struct dsi_lcd_regs {
	u32 ctrl0;
	u32 ctrl1;
	u32 reserved1[2];
	u32 timing0;
	u32 timing1;
	u32 timing2;
	u32 timing3;
	u32 wc0;
	u32 wc1;
	u32 wc2;
	u32 reserved[1];
	u32 slot_cnt0;
	u32 slot_cnt1;
};

/* updated for DC4 */
struct dsi_phy {
	u32 phy_ctrl0;
	u32 phy_ctrl1;
	u32 phy_ctrl2;
	u32 phy_ctrl3;
	u32 phy_status0;
	u32 phy_status1;
	u32 phy_lprx0;
	u32 phy_lprx1;
	u32 phy_lptx0;
	u32 phy_lptx1;
	u32 phy_lptx2;
	u32 phy_status2;
	u32 phy_rcomp0;
	u32 phy_rcomp1;
	u32 reserved[2];
	u32 phy_timing0;
	u32 phy_timing1;
	u32 phy_timing2;
	u32 phy_timing3;
	u32 phy_code_0;
	u32 phy_code_1;
};

struct dsi_regs {
	u32 ctrl0;
	u32 ctrl1;
	u32 reserved1[2];
	u32 irq_status;
	u32 irq_mask;
	u32 reserved2[2];
	u32 cmd0;
	u32 cmd1;
	u32 cmd2;
	u32 cmd3;
	u32 dat0;
	u32 status0;
	u32 status1;
	u32 status2;
	u32 status3;
	u32 status4;
	u32 reserved3[1];

	u32 smt_status1;
	u32 smt_cmd;
	u32 smt_ctrl0;
	u32 smt_ctrl1;
	u32 smt_status0;

	u32 rx0_status;
	u32 rx0_header;
	u32 rx1_status;
	u32 rx1_header;
	u32 rx_ctrl;
	u32 rx_ctrl1;
	u32 rx2_status;
	u32 rx2_header;
	u32 reserved4[1];

	u32 phy_ctrl1;
	u32 phy_ctrl2;
	u32 phy_ctrl3;
	u32 phy_status0;
	u32 phy_status1;
	u32 reserved5[5];
	u32 phy_status2;

	u32 phy_rcomp0;
	u32 phy_rcomp1;
	u32 reserved6[2];
	u32 phy_timing0;
	u32 phy_timing1;
	u32 phy_timing2;
	u32 phy_timing3;
	u32 phy_code_0;
	u32 phy_code_1;
	u32 reserved7[2];
	u32 mem_ctrl;
	u32 tx_timer;
	u32 rx_timer;
	u32 turn_timer;
	u32 top_status0;
	u32 top_status1;
	u32 top_status2;
	u32 top_status3;

	struct dsi_lcd_regs lcd1;
	u32 reserved8[18];
	union {
		struct dsi_lcd_regs lcd2;
		struct dsi_phy phy;
	};
};

enum dsi_packet_di {
	/* for sleep in/out display on/off */
	DSI_DI_DCS_SWRITE = 0x5,
	/* for set_pixel_format */
	DSI_DI_DCS_SWRITE1 = 0x15,
	DSI_DI_GENERIC_LWRITE = 0x29,
	DSI_DI_DCS_LWRITE = 0x39,
	DSI_DI_DCS_READ = 0x6,
	DSI_DI_GENERIC_READ1 = 0x14,
	DSI_DI_SET_MAX_PKT_SIZE = 0x37,
	/* for video mode off */
	DSI_DI_PERIPHE_CMD_OFF = 0x22,
	/* for video mode on */
	DSI_DI_PERIPHE_CMD_ON = 0x32,
};

enum dsi_rx_packet_di {
	DSI_DI_ACK_ERR_RESP = 0x2,
	DSI_DI_EOTP = 0x8,
	DSI_DI_GEN_READ1_RESP = 0x11,
	DSI_DI_GEN_READ2_RESP = 0x12,
	DSI_DI_GEN_LREAD_RESP = 0x1A,
	DSI_DI_DCS_READ1_RESP = 0x21,
	DSI_DI_DCS_READ2_RESP = 0x22,
	DSI_DI_DCS_LREAD_RESP = 0x1C,
};

struct dsi_cmd_desc {
	enum dsi_packet_di data_type;
	u8  lp;		/*command tx through low power mode or hs mode */
	u32 delay;	/* time to delay */
	u32 length;	/* cmds length */
	u8 *data;
};

#define DSI_MAX_DATA_BYTES	256
struct dsi_buf {
	enum dsi_rx_packet_di data_type;
	u32 length; /* cmds length */
	u8 data[DSI_MAX_DATA_BYTES];
};

struct lcd_fix_screeninfo {
	char id[16];			/* identification string eg "TT Builtin" */
	unsigned long smem_start;	/* Start of frame buffer mem */
					/* (physical address) */
	u32 smem_len;			/* Length of frame buffer mem */
	u32 type;			/* see FB_TYPE_*		*/
	u32 type_aux;			/* Interleave for interleaved Planes */
	u32 visual;			/* see FB_VISUAL_*		*/
	u16 xpanstep;			/* zero if no hardware panning	*/
	u16 ypanstep;			/* zero if no hardware panning	*/
	u16 ywrapstep;		/* zero if no hardware ywrap	*/
	u32 line_length;		/* length of a line in bytes	*/
	unsigned long mmio_start;	/* Start of Memory Mapped I/O	*/
					/* (physical address) */
	u32 mmio_len;			/* Length of Memory Mapped I/O	*/
	u32 accel;			/* Indicate to driver which	*/
					/*  specific chip/card we have	*/
	u16 reserved[3];		/* Reserved for future compatibility */
};

struct lcd_bitfield {
	u32 offset;			/* beginning of bitfield	*/
	u32 length;			/* length of bitfield		*/
	u32 msb_right;
};

struct lcd_var_screeninfo {
	u32 xres;			/* visible resolution		*/
	u32 yres;
	u32 xres_virtual;		/* virtual resolution		*/
	u32 yres_virtual;
	u32 xoffset;			/* offset from virtual to visible */
	u32 yoffset;			/* resolution			*/

	u32 bits_per_pixel;		/* guess what			*/
	u32 grayscale;		/* != 0 Graylevels instead of colors */

	u32 nonstd;			/* != 0 Non standard pixel format */

	u32 activate;			/* see FB_ACTIVATE_*		*/

	u32 height;			/* height of picture in mm    */
	u32 width;			/* width of picture in mm     */

	/* Timing: All values in pixclocks, except pixclock (of course) */
	u32 pixclock;			/* pixel clock in ps (pico seconds) */
	u32 left_margin;		/* time from sync to picture	*/
	u32 right_margin;		/* time from picture to sync	*/
	u32 upper_margin;		/* time from sync to picture	*/
	u32 lower_margin;
	u32 hsync_len;		/* length of horizontal sync	*/
	u32 vsync_len;		/* length of vertical sync	*/
	u32 sync;			/* see FB_SYNC_*		*/
	u32 vmode;			/* see FB_VMODE_*		*/
	u32 rotate;			/* angle we rotate counter clockwise */
	u32 reserved[5];		/* Reserved for future compatibility */
};

struct lcd_info {
	int node;
	int flags;
	struct lcd_var_screeninfo var;	/* Current var */
	struct lcd_fix_screeninfo fix;	/* Current fix */
	struct lcd_videomode *mode;	/* current mode */

	char *screen_base;	/* Virtual address */
	unsigned long screen_size;	/* Amount of ioremapped VRAM or 0 */
	void *pseudo_palette;		/* Fake palette of 16 colors */
#define LCDINFO_STATE_RUNNING	0
#define LCDINFO_STATE_SUSPENDED	1
	u32 state;			/* Hardware state i.e suspend */
	/* From here on everything is device dependent */
	void *par;
};

struct lcd_videomode {
	const char *name;	/* optional */
	u32 refresh;		/* optional */
	u32 xres;
	u32 yres;
	u32 pixclock;
	u32 left_margin;
	u32 right_margin;
	u32 upper_margin;
	u32 lower_margin;
	u32 hsync_len;
	u32 vsync_len;
	u32 sync;
	u32 vmode;
	u32 flag;
};

#define DSI_CTRL_0_CFG_SOFT_RST				(1<<31)
#define DSI_CTRL_0_CFG_SOFT_RST_REG			(1<<30)
#define DSI_CTRL_0_CFG_LCD1_TX_EN			(1<<8)
#define CFG_CLR_PHY_FIFO				(1<<29)
#define CFG_RST_TXLP					(1<<28)
#define CFG_RST_CPU					(1<<27)
#define CFG_RST_CPN					(1<<26)
#define CFG_RST_VPN2					(1<<25)
#define CFG_RST_VPN1					(1<<24)
#define CFG_DSI_PHY_RST					(1<<23)
#define DSI_CTRL_0_CFG_LCD1_TX_EN			(1<<8)
#define DSI_CTRL_0_CFG_LCD1_SLV				(1<<4)
#define DSI_CTRL_0_CFG_LCD1_EN				(1<<0)

#define DSI_CTRL_1_CFG_EOTP				(1<<8)
#define DSI_CTRL_1_CFG_LCD2_VCH_NO_MASK			(3<<2)
#define DSI_CTRL_1_CFG_LCD2_VCH_NO_SHIFT		2
#define DSI_CTRL_1_CFG_LCD1_VCH_NO_MASK			(3<<0)
#define DSI_CTRL_1_CFG_LCD1_VCH_NO_SHIFT		0

/* LCD 1 Vsync Reset Enable */
#define	DSI_LCD1_CTRL_1_CFG_L1_VSYNC_RST_EN		(1<<31)
/* VPN Auto Word Count Disable */
#define	CFG_VPN_AUTO_WC_DIS				(1<<24)
/* VPN Hact Word Count Enable */
#define	CFG_VPN_HACT_WC_EN				(1<<26)
/* VPN hss/hse/hact Tx Timing Check Disable */
#define	CFG_VPN_TIMING_CHECK_DIS			(1<<25)
/* VPN Auto Vsync Delay Count Disable */
#define	CFG_VPN_AUTO_DLY_DIS				(1<<24)
/* Long Blanking Packet Enable */
#define	DSI_LCD1_CTRL_1_CFG_L1_HLP_PKT_EN		(1<<22)
/* Front Porch Packet Enable */
#define	DSI_LCD1_CTRL_1_CFG_L1_HFP_PKT_EN		(1<<20)
/* hact Packet Enable */
#define	DSI_LCD1_CTRL_1_CFG_L1_HACT_PKT_EN		(1<<19)
/* Back Porch Packet Enable */
#define	DSI_LCD1_CTRL_1_CFG_L1_HBP_PKT_EN		(1<<18)
/* hse Packet Enable */
#define	DSI_LCD1_CTRL_1_CFG_L1_HSE_PKT_EN		(1<<17)
/* hsa Packet Enable */
#define	DSI_LCD1_CTRL_1_CFG_L1_HSA_PKT_EN		(1<<16)
/* Turn Around Bus at Last h Line */
#define	DSI_LCD1_CTRL_1_CFG_L1_LAST_LINE_TURN		(1<<10)
/* Go to Low Power Every Frame */
#define	DSI_LCD1_CTRL_1_CFG_L1_LPM_FRAME_EN		(1<<9)
/* Go to Low Power Every Line */
#define	DSI_LCD1_CTRL_1_CFG_L1_LPM_LINE_EN		(1<<8)
/* DSI Transmission Mode for LCD 1 */
#define DSI_LCD1_CTRL_1_CFG_L1_BURST_MODE_SHIFT		2
/* LCD 1 Input Data RGB Mode for LCD 1 */
#define DSI_LCD2_CTRL_1_CFG_L1_RGB_TYPE_SHIFT		0

/* DSI_IRQ_ST 0x0010 DSI Interrupt Status Register */
#define IRQ_TA_TIMEOUT				(1<<29)
#define IRQ_RX_TIMEOUT				(1<<28)
#define IRQ_TX_TIMEOUT				(1<<27)
#define IRQ_RX_STATE_ERR			(1<<26)
#define IRQ_RX_ERR				(1<<25)
#define IRQ_RX_FIFO_FULL_ERR			(1<<24)
#define IRQ_PHY_FIFO_UNDERRUN			(1<<23)
#define IRQ_REQ_CNT_ERR				(1<<22)
#define IRQ_DPHY_ERR_SYNC_ESC			(1<<10)
#define IRQ_DPHY_ERR_ESC			(1<<9)
#define IRQ_DPHY_RX_LINE_ERR			(1<<8)
#define IRQ_RX_TRG3				(1<<7)
#define IRQ_RX_TRG2				(1<<6)
#define IRQ_RX_TRG1				(1<<5)
#define IRQ_RX_TRG0				(1<<4)
#define IRQ_RX_PKT				(1<<2)
#define IRQ_CPU_TX_DONE				(1<<0)

/* DSI_RX_PKT_ST_0 0x0060 DSI RX Packet 0 Status Register */
#define RX_PKT0_ST_VLD				(0x1 << 31)
#define RX_PKT0_ST_SP				(0x1 << 24)
#define RX_PKT0_PKT_PTR_MASK			(0x3f << 16)
#define RX_PKT0_PKT_PTR_SHIFT			16

/* DSI_RX_PKT_CTRL 0x0070 DSI RX Packet Read Control Register */
#define RX_PKT_RD_REQ				(0x1 << 31)
#define RX_PKT_RD_PTR_MASK			(0x3f << 16)
#define RX_PKT_RD_PTR_SHIFT			16
#define RX_PKT_RD_DATA_MASK			(0xff)
#define RX_PKT_RD_DATA_SHIFT			0

/* DSI_RX_PKT_CTRL_1 0x0074 DSI RX Packet Read Control1 Register */
#define RX_PKT_CNT_MASK				(0xf << 8)
#define RX_PKT_CNT_SHIFT			8
#define RX_PKT_BCNT_MASK			(0xff)
#define RX_PKT_BCNT_SHIFT			0

/* DSI_CPU_CMD_0 0x0020  DSI CPU Packet Command Register 0 */
#define DSI_CFG_CPU_CMD_REQ_MASK		(0x1 << 31)
#define DSI_CFG_CPU_CMD_REQ_SHIFT		31
#define DSI_CFG_CPU_SP_MASK			(0x1 << 30)
#define DSI_CFG_CPU_SP_SHIFT			30
#define DSI_CFG_CPU_TURN_MASK			(0x1 << 29)
#define DSI_CFG_CPU_TURN_SHIFT			29
#define DSI_CFG_CPU_TXLP_MASK			(0x1 << 27)
#define DSI_CFG_CPU_TXLP_SHIFT			27
#define DSI_CFG_CPU_WC_MASK			(0xff)
#define DSI_CFG_CPU_WC_SHIFT			0


/* DSI_CPU_CMD_3 0x002C DSI CPU Packet Command Register 3 */
#define DSI_CFG_CPU_DAT_REQ_MASK		(0x1 << 31)
#define DSI_CFG_CPU_DAT_REQ_SHIFT		31
#define DSI_CFG_CPU_DAT_RW_MASK			(0x1 << 30)
#define DSI_CFG_CPU_DAT_RW_SHIFT		30
#define DSI_CFG_CPU_DAT_ADDR_MASK		(0xff << 16)
#define DSI_CFG_CPU_DAT_ADDR_SHIFT		16

#define DPHY_CFG_VDD_VALID             (0x3 << 16)
/* DPHY BANDGAP REF ENABLE */
#define DPHY_BG_VREF_EN			(0x1 << 9)
#define DPHY_BG_VREF_EN_DC4		(0x1 << 18)

/* DPHY Data Lane Enable */
#define DSI_PHY_CTRL_2_CFG_CSR_LANE_EN_MASK             (0xf<<4)
#define	DSI_PHY_CTRL_2_CFG_CSR_LANE_EN_SHIFT		4

/*		Bit(s) DSI_CPU_CMD_1_RSRV_31_24 reserved */
/* LPDT TX Enable */
#define	DSI_CPU_CMD_1_CFG_TXLP_LPDT_SHIFT		20
/* Low Power TX Trigger Code */
#define	DSI_CPU_CMD_1_CFG_TXLP_TRIGGER_CODE_SHIFT	0

/* Length of HS Exit Period in tx_clk_esc Cycles */
#define	DSI_PHY_TIME_0_CFG_CSR_TIME_HS_EXIT_SHIFT	24
/* DPHY HS Trail Period Length */
#define	DSI_PHY_TIME_0_CFG_CSR_TIME_HS_TRAIL_SHIFT	16
/* DPHY HS Zero State Length */
#define	DSI_PHY_TIME_0_CDG_CSR_TIME_HS_ZERO_SHIFT	8

/* Time to Drive LP-00 by New Transmitter */
#define	DSI_PHY_TIME_1_CFG_CSR_TIME_TA_GET_SHIFT	24
/* Time to Drive LP-00 after Turn Request */
#define	DSI_PHY_TIME_1_CFG_CSR_TIME_TA_GO_SHIFT		16

/* DPHY CLK Exit Period Length */
#define	DSI_PHY_TIME_2_CFG_CSR_TIME_CK_EXIT_SHIFT	24
/* DPHY CLK Trail Period Length */
#define	DSI_PHY_TIME_2_CFG_CSR_TIME_CK_TRAIL_SHIFT	16
/* DPHY CLK Zero State Length */
#define	DSI_PHY_TIME_2_CFG_CSR_TIME_CK_ZERO_SHIFT	8

/* DPHY LP Length */
#define	DSI_PHY_TIME_3_CFG_CSR_TIME_LPX_SHIFT		8

/* DSI timings */
#if defined(CONFIG_PXA988) || defined(CONFIG_PXA1928)
#define DSI_ESC_CLK				52  /* Unit: Mhz */
#define DSI_ESC_CLK_T				19  /* Unit: ns */
#else
#define DSI_ESC_CLK				66  /* Unit: Mhz */
#define DSI_ESC_CLK_T				15  /* Unit: ns */
#endif

#ifdef CONFIG_PXA988
#define TIMING_MASTER_CONTROL			(0x01F4)
#define MASTER_ENH(id)				(1 << ((id) + 4))
#define MASTER_ENV(id)				(1 << ((id) + 6))
#else
#define TIMING_MASTER_CONTROL			(0x02F8)
#define MASTER_ENH(id)				(1 << (id))
#define MASTER_ENV(id)				(1 << ((id) + 4))
#endif

#define TIMING_MASTER_CONTROL_DC4			(0x02F8)
#define MASTER_ENH_DC4(id)				(1 << (id))
#define MASTER_ENV_DC4(id)				(1 << ((id) + 4))

#define TIMING_MASTER_CONTROL_DC4_LITE			(0x01BC)
#define MASTER_ENH_DC4_LITE(id)				(1 << ((id) + 16))
#define MASTER_ENV_DC4_LITE(id)				(1 << ((id) + 17))

#define DSI_START_SEL_SHIFT(id)		(((id) << 1) + 8)
#define timing_master_config(path, dsi_id, lcd_id) \
	(MASTER_ENH(path) | MASTER_ENV(path) | \
	(((lcd_id) + ((dsi_id) << 1)) << DSI_START_SEL_SHIFT(path)))
#define timing_master_config_dc4(path, dsi_id, lcd_id) \
	(MASTER_ENH_DC4(path) | MASTER_ENV_DC4(path) | \
	(((lcd_id) + ((dsi_id) << 1)) << DSI_START_SEL_SHIFT(path)))
#define timing_master_config_dc4_lite(path) \
	(MASTER_ENH_DC4_LITE(path) | MASTER_ENV_DC4_LITE(path))

void calculate_dsi_clk(struct mmp_disp_plat_info *mi);
void *mmp_disp_init(struct mmp_disp_plat_info *mi);
void set_dsi_low_power_mode(struct mmp_disp_info *fbi);
void dsi_cmd_array_tx(struct mmp_disp_info *fbi,
		struct dsi_cmd_desc cmds[], int count);
void mmp_disp_dsi_init_dphy(struct mmp_disp_info *fbi);
int mmp_disp_dsi_init_panel(struct mmp_disp_info *fbi);
int handle_disp_rsvmem(struct mmp_disp_plat_info *mi);
void show_debugmode_logo(struct mmp_disp_plat_info *mi, const unsigned char *picture);
#ifdef CONFIG_PXA1U88
void lcd_calculate_sclk(struct mmp_disp_info *fb, int dsiclk, int pathclk);
#endif
extern int g_panel_id;
extern unsigned long g_disp_start_addr;
extern unsigned long g_disp_buf_size;
extern void pxa1928_pll3_set_rate(unsigned long rate);

#endif /* __ASM_MACH_PXA168FB_H */
