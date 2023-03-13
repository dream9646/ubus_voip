/******************************************************************
 *
 * Copyright (C) 2011 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI protocol stack
 * Module  : util
 * File    : util_misc.h
 *
 ******************************************************************/

#ifndef __UTIL_MISC_H__
#define __UTIL_MISC_H__

/* util_misc.c */
int util_readfile_lines(const char *pathname, char *lines[], int max);
int util_writefile_lines(const char *pathname, char *lines[], int total);
void util_free_lines(char *lines[], int total);
long util_readfile(const char *pathname, char **data);
long util_readcmd(const char *cmd, char **data);
int util_templatefill(char **data, char *label, char *fill);
int util_is_same_subnet(char *ipstr1, char *ipstr2, char *netmaskstr);
char *util_escape(const char *str);
char *util_unescape(const char *str);
int util_str2html(char *buf1, char *buf2, int buf2size);
int util_password2star(char *buff);

#endif
