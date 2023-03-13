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
 * File    : calix_txme.c
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
igmp_filter_calix_txme_pre_filter(struct igmp_clientinfo_t *igmp_clientinfo)
{
        MCAST_FILTER_NO_IPV6(igmp_clientinfo)
        	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
	{
		struct meinfo_t *miptr=meinfo_util_miptr(249);
		struct me_t *me249=NULL;
		int has_me249=0, i=0;
		list_for_each_entry(me249, &miptr->me_instance_list, instance_node) {
			if (me249 != NULL)
			{
				has_me249 = 1;
				break;
			}
		}
		if (!has_me249) return MCAST_FILTER_CONTINUE;	// me 249 does not exist
		
		for (i = 0 ; i < 8 ; i++)
		{
			if (igmp_clientinfo->tx_meid[i] != 0) return MCAST_FILTER_CONTINUE;
		}
		
		dbprintf_igmp(LOG_WARNING, "IGMP_JOIN_FAILED_VLAN_TAGGING\n");
        	return MCAST_FILTER_STOP;
	}
	
        return MCAST_FILTER_CONTINUE;
}

// module callbacks
static const char * igmp_filter_calix_txme_name(void)
{
	return "calix_txme";
}

static int igmp_filter_calix_txme_init(void)
{	
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_calix_txme_deinit(void)
{
	return MCAST_FILTER_CONTINUE;
}

struct igmp_filter_module_t calix_txme_filter =
{
        .priority = 10,
	.filter_name = igmp_filter_calix_txme_name,
        .init = igmp_filter_calix_txme_init,
        .deinit = igmp_filter_calix_txme_deinit,
        .pre_filter = igmp_filter_calix_txme_pre_filter,
};


          

