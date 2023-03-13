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
 * File    : util.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"

static int util_debug_level=4;

void
util_set_debug_level(int level)
{
	util_debug_level=level;
}

int
util_dbprintf(int debug_level, char *fmt, ...)
{
	va_list ap;
	int ret;

	if (debug_level > util_debug_level)
		return 0;

	va_start(ap, fmt);
	ret = vfprintf(stderr, fmt, ap);
	va_end(ap);
	return (ret);
}

char *
util_trim(char *s)
{
	int i;

	if (s) {
		i = strlen(s);
		while ((--i) > 0 && isspace(s[i]))
			s[i] = 0;
	}
	if (s) {
		while (isspace(*s))
			s++;
	}
	return s;
}

int
util_str2array(char *s, char *delim, char *array[], int array_size)
{
	int dlen = strlen(delim);
	int i = 0;
	char *p;

	while (s != NULL) {
		p = strstr(s, delim);
		if (p != NULL) {
			*p = 0x0;
			array[i] = util_trim(s);
			i++;
			s = p + dlen;
			if (i == array_size)
				break;
		} else {
			break;
		}
	}
	if (s != NULL && i < array_size) {
		array[i] = util_trim(s);
		i++;
	}

	return i;
}

int
util_hexstr2mem(unsigned char *mem, int mem_total, char *str)
{
	char buff[1024];
	char *a[128];
	int n, i, j;
	
	strncpy(buff, str, 1024);
	buff[1023] = 0;

	n = util_str2array(buff, " ", a, 128);
	if (n>mem_total) 
		n=mem_total;
		
	for (i=j=0; i < n; i++) {	
		if (strlen(a[i])==2) {
			char *p;
			mem[j]=(unsigned char) strtol(a[i], &p, 16);
			j++;
		}
	}
	return j;
}

int
util_viewfile(char *filename, int lines)
{
	static char pager[256]={0};
	char buf[256];

	if (pager[0]==0) {
		struct stat statbuf;
		char *p=getenv("LESS");		
		if (p) {
			strncpy(pager, p, 256);
		} else if (stat("/usr/bin/less", &statbuf)==0 ||
			   stat("/bin/less", &statbuf)==0) {
			strncpy(pager, "less -X -Q -f -r -s -z -2", 256); 
		} else {
			strncpy(pager, "more", 256);
		}
	}

	if (lines >= 23) {
		sprintf(buf, "%s %s", pager, filename);
		system(buf);
	} else if (lines > 0) {
		sprintf(buf, "cat %s", filename);
		system(buf);
	}
	return 0;
}

int
util_thread_create(pthread_t *thread, int policy, int priority, 
			void*(*funcptr)(void *), void *dataptr)
{
	pthread_attr_t pattr;
	struct sched_param psch;

	pthread_attr_init(&pattr);
	
	//pattr.__scope=PTHREAD_SCOPE_SYSTEM;
	pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setschedpolicy(&pattr, policy);
	psch.sched_priority= priority;
	pthread_attr_setschedparam(&pattr,&psch);

	/* specify explicit scheduling */
	pthread_attr_setinheritsched (&pattr, PTHREAD_EXPLICIT_SCHED);

	pthread_create(thread, &pattr, funcptr, dataptr);
	
	return 0;
}

int
util_udp_server(int port)
{
	int sockfd;
	struct sockaddr_in local_addr;
	int on=1;

	// open udp socket
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on));

	// bind local address so client can send to this server
	bzero((char *) &local_addr, sizeof(struct sockaddr_in));
	local_addr.sin_family      = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port        = htons(port);

	if (bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0)
		return -2;

	return(sockfd);
}

int
util_ipstr_to_sockaddr(struct sockaddr_in *sockaddr, char *ipstr, unsigned short port)
{
	bzero((char *) sockaddr, sizeof(struct sockaddr_in));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_addr.s_addr=inet_addr(ipstr);	/* ip */
	sockaddr->sin_port = htons(port);

	if (sockaddr->sin_addr.s_addr==INADDR_NONE) 	/* ip invalid */
		return -1;	
	return 0;
}

void *
util_malloc_safe(char *filename, int linenum, char *funcname, size_t size)
{
	void *ptr=malloc(size);

	if (ptr==NULL) {
		util_dbprintf(omci_env_g.debug_level, LOG_EMERG, 0, "%s:%d: %s(): malloc(%d) return null?\n", filename, linenum, funcname, size);
		exit(1);
	}
	memset(ptr, 0x00, size);
	return ptr;
}

