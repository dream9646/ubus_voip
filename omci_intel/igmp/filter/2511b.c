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
 * Module  : igmp
 * File    : 2511b.c
 *
 ******************************************************************/
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "util_inet.h"
#include "list.h"
#include "util.h"
#include "switch.h"

#include "mcast.h"
#include "mcast_const.h"

static int
igmp_filter_2511b_pre_filter(struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct igmp_group_t *pos, *n;
	
	MCAST_FILTER_NO_IPV6(igmp_clientinfo)
	
	if (strcmp("2511b", CHIP_NAME) != 0) return MCAST_FILTER_CONTINUE;
	
	dbprintf_igmp(LOG_INFO, "2511b does not support SSM (x, G), ASM(*, G) is always used\n");
		
        list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {
         
                if (pos->record_type == ALLOW_NEW_SOURCES || pos->record_type == CHANGE_TO_EXCLUDE_MODE || pos->record_type == MODE_IS_EXCLUDE)
                        pos->record_type = CHANGE_TO_EXCLUDE_MODE;
                else
                        pos->record_type = CHANGE_TO_INCLUDE_MODE;
                        
                pos->src_ip.ipv4.s_addr = 0;    // null source
	}
        
        return MCAST_FILTER_CONTINUE;
}

// module callbacks
static const char * igmp_filter_2511b_name(void)
{
	return "2511b";
}

static int igmp_filter_2511b_init(void)
{	
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_2511b_deinit(void)
{
	return MCAST_FILTER_CONTINUE;
}

struct igmp_filter_module_t x2511b_filter =
{
        .priority = 10,
	.filter_name = igmp_filter_2511b_name,
        .init = igmp_filter_2511b_init,
        .deinit = igmp_filter_2511b_deinit,
        .pre_filter = igmp_filter_2511b_pre_filter,
};


          

