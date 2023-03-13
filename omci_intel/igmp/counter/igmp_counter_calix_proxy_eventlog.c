/******************************************************************
 *
 * Copyright (C) 2016 5V Technologies Ltd.
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
 * Module  : igmp counter module - calix event log
 * File    : igmp_counter_calix_eventlog.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "meinfo.h"
#include "util.h"
#include "util_inet.h"
#include "hwresource.h"
#include "switch.h"
#include "mcast_const.h"
#include "mcast_pkt.h"
#include "mcast.h"
#include "mcast_timer.h"

#define IGMP_COUNTER_MODULE_NAME        "calix_proxy_eventlog"

// module functions
int
igmp_counter_calix_proxy_eventlog_reset(void)
{
//	FILE *slog = NULL;
//	struct stat fstat={0};

//      stat("/storage/igmpevent.log", &fstat);
//	slog = fopen("/storage/igmpevent.log", "w"); // trucate file to zero length
	
//	fclose(slog);
	return 0;
}

int
igmp_counter_calix_proxy_eventlog_init(void)
{	
	return 0;
}

const char *igmp_counter_calix_proxy_eventlog_name()
{
        return IGMP_COUNTER_MODULE_NAME;
}

int igmp_counter_calix_proxy_eventlog_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{				
	return 0;
}

int igmp_cli_counter_calix_proxy_eventlog(int fd)
{
	FILE *fp = popen("cat /proc/igmp.log > /storage/igmpevent.log", "r");

	if(fp) {
		pclose(fp);
	}
	
	return 0;
}
/*
int igmp_counter_iterator_calix_proxy_eventlog(int (*iterator_function)(struct igmp_pkt_calix_proxy_eventlog_t *pos, void *data), void *iterator_data)
{       
        return 0;
}
*/

struct igmp_counter_module_t calix_proxy_eventlog =
{
        .module_name = igmp_counter_calix_proxy_eventlog_name,
        .init = igmp_counter_calix_proxy_eventlog_init,
        .reset = igmp_counter_calix_proxy_eventlog_reset,
        .igmp_counter_handler = igmp_counter_calix_proxy_eventlog_handler,
        .igmp_cli_handler = igmp_cli_counter_calix_proxy_eventlog,
        .igmp_counter_iterator = NULL
};
