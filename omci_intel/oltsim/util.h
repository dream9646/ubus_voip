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
 * Module  : oltsim
 * File    : util.h
 *
 ******************************************************************/

#ifndef __UTIL_H__
#define __UTIL_H__

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <linux/sockios.h>

#if 1				// definition from /usr/include/syslog.h
#define	LOG_EMERG	0	/* system is unusable */
#define	LOG_ALERT	1	/* action must be taken immediately */
#define	LOG_CRIT	2	/* critical conditions */
#define	LOG_ERR		3	/* error conditions */
#define	LOG_WARNING	4	/* warning conditions */
#define	LOG_NOTICE	5	/* normal but significant condition */
#define	LOG_INFO	6	/* informational */
#define	LOG_DEBUG	7	/* debug-level messages */
#endif

#define dbprintf(level, fmt, args...)        util_dbprintf(level, "%s:%d: %s(): " fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
#define malloc_safe(size)		util_malloc_safe((char*)__FILE__, __LINE__, (char*)__FUNCTION__, size)

/* util.c */
void util_set_debug_level(int level);
int util_dbprintf(int debug_level, char *fmt, ...);
char *util_trim(char *s);
int util_str2array(char *s, char *delim, char *array[], int array_size);
int util_hexstr2mem(unsigned char *mem, int mem_total, char *str);
int util_viewfile(char *filename, int lines);
int util_thread_create(pthread_t *thread, int policy, int priority, void *(*funcptr)(void *), void *dataptr);
int util_udp_server(int port);
int util_ipstr_to_sockaddr(struct sockaddr_in *sockaddr, char *ipstr, unsigned short port);
void *util_malloc_safe(char *filename, int linenum, char *funcname, size_t size);

#endif
