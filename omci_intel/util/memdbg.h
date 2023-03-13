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
 * File    : memdbg.h
 *
 ******************************************************************/

#ifndef __MEMDBG_H__
#define __MEMDBG_H__

/* set this to 1 to enable memdbg command in cli for memory debug */
#define MEMDBG_ENABLE	1

/* memdbg.c */
int memdbg_add(void *addr, int size, char *filename, int linenum, char *funcname);
int memdbg_del(void *addr, char *filename, int linenum, char *funcname);
int memdbg_dump(int fd);

#endif
