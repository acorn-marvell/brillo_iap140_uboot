/*
 * Copyright (C) 2013 Marvell, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#ifndef __LEDS_88PM828X_H__
#define __LEDS_88PM828X_H__

#define PM828X_I2C_ADDR             0x10

/* Register map */
#define PM828X_CHIP_ID              0x00
#define PM828X_REV_ID               0x01
#define PM828X_STATUS               0x02
#define PM828X_STATUS_FLAG          0x03
#define PM828X_IDAC_CTRL0           0x04
#define PM828X_IDAC_CTRL1           0x05
#define PM828X_IDAC_RAMP_CLK_CTRL0  0x06
#define PM828X_MISC_CTRL            0x07
#define PM828X_IDAC_OUT_CURRENT0    0x08
#define PM828X_IDAC_OUT_CURRENT1    0x09

#define PM828X_ID                   0x28
#define PM828X_RAMP_MODE_DIRECT     0x1
#define PM828X_RAMP_MODE_NON_LINEAR 0x2
#define PM828X_RAMP_MODE_LINEAR     0x4
#define PM828X_RAMP_CLK_8K          0x0
#define PM828X_RAMP_CLK_4K          0x1
#define PM828X_RAMP_CLK_1K          0x2
#define PM828X_RAMP_CLK_500         0x3
#define PM828X_RAMP_CLK_250         0x4
#define PM828X_RAMP_CLK_125         0x5
#define PM828X_RAMP_CLK_62          0x6
#define PM828X_RAMP_CLK_31          0x7
#define PM828X_IDAC_CURRENT_20MA    0xA00
#define PM828X_EN_SHUTDN            (0x0 << 7)
#define PM828X_DIS_SHUTDN           (0x1 << 7)
#define PM828X_EN_PWM               (0x1 << 6)
#define PM828X_EN_SLEEP             (0x1 << 5)
#define PM828X_SINGLE_STR_CONFIG    0x1
#define PM828X_DUAL_STR_CONFIG      0x0

#define LED_MIN	0
#define LED_MAX	100

struct pm828x_data {
	uint16_t addr;

	u32 ramp_mode:3;     /* IDAC current ramp up/down mode */
	u32 idac_current:12; /* IDAC current setting */
	u32 ramp_clk:3;      /* set for linear or non-linear mode */
	u32 str_config:1;    /* single or dual string selection */

	u8 revision;
	u32 bvalue; /* Current brightness register value */

	u32 i2c_num;
};

int pm828x_read_id(struct pm828x_data *pdata, uint8_t *chip_id);
int pm828x_init(struct pm828x_data *pdata);

#endif /* __LEDS_88PM828X_H__ */
