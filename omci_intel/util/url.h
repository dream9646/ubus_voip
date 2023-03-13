/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI Protocol Stack
 * Module  : util
 * File    : url.h
 *
 ******************************************************************/

#ifndef __URL_H__
#define __URL_H__ 

/* url.c */
char *url_escape(const char *string, int inlength);
char *url_unescape(const char *string, int length);
int url_escape_file(const char *srcfile, const char *dstfile);
int url_unescape_file(const char *srcfile, const char *dstfile);

#endif
