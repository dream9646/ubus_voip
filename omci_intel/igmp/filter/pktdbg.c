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
 * File    : pktdbg.c
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
#include "cli.h"

#include "cpuport_sniffer.h"
#include "mcast.h"
#include "mcast_router.h"
#include "mcast_const.h"
#include "platform.h"

static short pktdbg_enable = 0;

static int
igmp_filter_pktdbg_prefilter(struct igmp_clientinfo_t *igmp_clientinfo)
{
	if (!pktdbg_enable)	return MCAST_FILTER_CONTINUE;
	
	struct igmp_group_t *pos, *n;
			
        util_fdprintf(util_dbprintf_get_console_fd(), "<--\n");
	list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {
        	unsigned char tmp[1024] ="";
		char group_ip[INET6_ADDRSTRLEN] = "";
		char src_ip[INET6_ADDRSTRLEN] = "";
		char typestr[32] = "";
		char modestr[32] = "";
		switch(pos->msg_type)
		{
		case IGMP_V1_MEMBERSHIP_REPORT:
			snprintf(typestr, sizeof(typestr), "IGMPv1 Report");
			break;
		case IGMP_MEMBERSHIP_QUERY:
			snprintf(typestr, sizeof(typestr), "IGMP Query");
			break;
		case IGMP_V2_MEMBERSHIP_REPORT:
			snprintf(typestr, sizeof(typestr), "IGMPv2 Report");
			break;
		case IGMP_V3_MEMBERSHIP_REPORT:
			snprintf(typestr, sizeof(typestr), "IGMPv3 Report");
			break;
		case IGMP_V2_LEAVE_GROUP:
			snprintf(typestr, sizeof(typestr), "IGMPv2 Leave");
			break;
		case MLD_LISTENER_QUERY:
			snprintf(typestr, sizeof(typestr), "MLD Query");
			break;
		case MLD_LISTENER_REPORT:
			snprintf(typestr, sizeof(typestr), "MLDv1 Report");
			break;
		case MLD_LISTENER_REDUCTION:
			snprintf(typestr, sizeof(typestr), "MLDv1 Done");
			break;
		case MLD_V2_LISTENER_REPORT:
			snprintf(typestr, sizeof(typestr), "MLDv2 Report");
			break;
		default:
			snprintf(typestr, sizeof(typestr), "Unknown Type");						
		}
		
		switch(pos->record_type)
		{
		case MODE_IS_INCLUDE:
			snprintf(modestr, sizeof(modestr), "IS_INCLUDE");
			break;
		case MODE_IS_EXCLUDE:
			snprintf(modestr, sizeof(modestr), "IS_EXCLUDE");
			break;
		case CHANGE_TO_INCLUDE_MODE:
			snprintf(modestr, sizeof(modestr), "TO_INCLUDE_MODE");
			break;
		case CHANGE_TO_EXCLUDE_MODE:
			snprintf(modestr, sizeof(modestr), "TO_EXCLUDE_MODE");
			break;
		case ALLOW_NEW_SOURCES:
			snprintf(modestr, sizeof(modestr), "ALLOW_NEW_SOURCES");
			break;
		case BLOCK_OLD_SOURCES:
			snprintf(modestr, sizeof(modestr), "BLOCK_OLD_SOURCES");
			break;
		default:
			snprintf(modestr, sizeof(modestr), "-");	
		}
		inet_ipaddr(pos->group_ip, group_ip);
		inet_ipaddr(pos->src_ip, src_ip);
        	snprintf(tmp, sizeof(tmp), "type:0x%2x(%s) gip:%s sip:%s mode:0x%x(%s)\n", pos->msg_type, typestr, group_ip, src_ip, pos->record_type, modestr);
        	util_fdprintf(util_dbprintf_get_console_fd(), "%s", tmp);
	}
	util_fdprintf(util_dbprintf_get_console_fd(), "-->\n");
        
        return MCAST_FILTER_CONTINUE;
}


// module callbacks
static const char * igmp_filter_pktdbg_name(void)
{
	return "pktdbg";
}

static int igmp_filter_pktdbg_init(void)
{	
	
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_pktdbg_deinit(void)
{	
	return MCAST_FILTER_CONTINUE;
}

static int
igmp_filter_pktdbg_raw_filter(struct cpuport_info_t *pktinfo)
{
	if (!pktdbg_enable)	return MCAST_FILTER_CONTINUE;
	// print raw packet
	cpuport_sniffer_print_pkt(util_dbprintf_get_console_fd(), "\n", pktinfo->frame_ptr, pktinfo->frame_len, pktinfo->rx_desc.router_mode);
	
	return MCAST_FILTER_CONTINUE;	
}

static int
igmp_filter_pktdbg_cli(int fd, int argc, char *argv[])
{
	if (strcmp(argv[0], "igmp") == 0)
	{
		if (argc==4 && strcmp(argv[1], igmp_filter_pktdbg_name()) == 0)
		{
			if (strcmp(argv[2], "enable") == 0)
			{
				pktdbg_enable = util_atoi(argv[3]);
				return CLI_OK;		
			}
		}
	}
	
	return CLI_ERROR_OTHER_CATEGORY;
}

struct igmp_filter_module_t pktdbg_filter =
{
        .priority = 10,
	.filter_name = igmp_filter_pktdbg_name,
        .init = igmp_filter_pktdbg_init,
        .deinit = igmp_filter_pktdbg_deinit,
        .raw_filter = igmp_filter_pktdbg_raw_filter,
        .pre_filter = igmp_filter_pktdbg_prefilter,
        .cli = igmp_filter_pktdbg_cli
};


          

