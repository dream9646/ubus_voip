/******************************************************************
 *
 * Copyright (C) 2013 5V Technologies Ltd.
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
 * Module  : acl
 * File    : vacltest_util.h
 *
 ******************************************************************/

#ifndef __VACLTEST_UTIL_H__
#define __VACLTEST_UTIL_H__

#include <strings.h>
#include <endian.h>
#include <stdio.h>
#include <time.h>

#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>

#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */
#define LINE_MAXSIZE	2048

#define dbprintf(level, fmt, args...)	util_dbprintf(level, 1, "%s:%d: %s(): " fmt, util_purename(__FILE__), __LINE__, __FUNCTION__, ##args)
#define malloc_safe(size)		util_malloc_safe(__FILE__, __LINE__, (char*)__FUNCTION__, size)
#define free_safe(addr)			util_free_safe(__FILE__, __LINE__, (char*)__FUNCTION__, (void *)&(addr))


int util_fdprintf(int fd, char *fmt, ...);
int util_dbprintf(int debug_level, unsigned char log_time, char *fmt, ...);
char *util_purename(char *path);
void *util_malloc_safe(char *filename, int linenum, char *funcname, size_t size);
void util_free_safe(char *filename, int linenum, char *funcname, void **ptr);
int switch_hw_fvt2510_uni_port_total(void);

#endif
