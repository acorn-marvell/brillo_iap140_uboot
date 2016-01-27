/*
 * (C) Copyright 2013
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#ifndef __CMDLINE_H__
#define __CMDLINE_H__
static inline void remove_cmdline_param(char *cmdline, char *str)
{
	char *p1 = cmdline;
	char *p2;
	char *p;
	int substr = 1;

	while ((p1 = strstr(p1, str)) != NULL) {
		if (p1 == cmdline)/*Judge whether it's a substring or not*/
			substr = 0;
		else if (*(p1-1) == ' ')
			substr = 0;
		else
			substr = 1;

		if (substr == 0) {
			/* process only when it's not a substring,
			   like "mem=" */
			p = p1;
			p2 = p1;
			while (*p2 != ' ' && *p2 != '\0')
				p2++;
			while (*p2++)
				*p++ = *p2;
			*p = '\0';
		} else {
			while (*p1 != ' ' && *p1 != '\0')
				p1++;
		}
	}
}

/* Only support the parameter with 'xxx=' format */
static inline char *get_cmdline_param(char *cmdline, char *str)
{
	char *p1 = cmdline;
	char *p2 = NULL;

	while ((p1 = strstr(p1, str)) != NULL) {
		while ((*p1 != ' ') && (*p1 != '\0')) {
			if (*p1 == '=') {
				p2 = p1+1;
				break;
			}
			p1++;
		}
	}

	return p2;
}
#endif
