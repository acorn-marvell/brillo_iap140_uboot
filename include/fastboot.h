/*
 * Copyright 2011, Marvell Semiconductor Inc.
 * Lei Wen <leiwen@marvell.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Back ported to the 8xx platform (from the 8260 platform) by
 * Murray.Jensen@cmst.csiro.au, 27-Jan-01.
 */
#ifndef __FASTBOOT_H_
#define __FASTBOOT_H_	1

void rcv_cmd(void);
void fb_set_buf(void *buf);
void fb_tx_status(const char *status);
void fb_tx_data(void *data, unsigned long len);
int fb_get_rcv_len(void);
void *fb_get_buf(void);
void fb_init(void);
void fb_run(void);
void fb_halt(void);

extern u32 read_cpuid_id(void);

#endif	/* __FASTBOOT_H_ */
