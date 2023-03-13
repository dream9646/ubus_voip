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
 * File    : mcast_version.c
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
igmp_filter_mcast_version_pre_filter(struct igmp_clientinfo_t *igmp_clientinfo)
{
	MCAST_FILTER_NO_IPV6(igmp_clientinfo)
		
	int ret = MCAST_FILTER_CONTINUE;

	if (omci_env_g.olt_alu_mcast_support)
	{
		dbprintf_igmp(LOG_WARNING, "bypass mcast_version - ALU\n");
		return ret;	// bypass this filter
	}
		
	switch (igmp_clientinfo->igmp_msg_type)
	{
	case IGMP_MEMBERSHIP_QUERY:
		break;
	case IGMP_V1_MEMBERSHIP_REPORT:
		if (omci_env_g.igmp_v1_ignore) ret = MCAST_FILTER_STOP;	// ignore v1
		break;
	case IGMP_V2_MEMBERSHIP_REPORT:
	case IGMP_V2_LEAVE_GROUP:
		if (igmp_config_g.igmp_version < IGMP_V2_BIT) ret = MCAST_FILTER_STOP;
		break;
	case IGMP_V3_MEMBERSHIP_REPORT:
		if (igmp_config_g.igmp_version < IGMP_V3_BIT) ret = MCAST_FILTER_STOP;
		break;
	case MLD_LISTENER_QUERY:
	case MLD_LISTENER_REPORT:
	case MLD_LISTENER_REDUCTION:
	case MLD_V2_LISTENER_REPORT:
		dbprintf_igmp(LOG_INFO,"mld not supported\n");
		ret = MCAST_FILTER_STOP;	// not supported
		break;
	default:
		dbprintf_igmp(LOG_INFO,"uknown igmp/mld type: 0x%x\n", igmp_clientinfo->igmp_msg_type);
		ret = MCAST_FILTER_STOP;	
	}
	
	if (ret) dbprintf_igmp(LOG_WARNING, "Can't handle newer igmp version\n");
	
	return ret;
}

// module callbacks
static const char * igmp_filter_mcast_version_name(void)
{
	return "mcast_version";
}

static int igmp_filter_mcast_version_init(void)
{	
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_mcast_version_deinit(void)
{
	return MCAST_FILTER_CONTINUE;
}

struct igmp_filter_module_t mcast_version =
{
        .priority = 100,
	.filter_name = igmp_filter_mcast_version_name,
        .init = igmp_filter_mcast_version_init,
        .deinit = igmp_filter_mcast_version_deinit,
        .pre_filter = igmp_filter_mcast_version_pre_filter,
};


          

