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
 * File    : port_validator.c
 *
 ******************************************************************/
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <linux/if_ether.h>

#include "util_inet.h"
#include "list.h"
#include "util.h"
#include "switch.h"

#include "mcast.h"
#include "me_related.h"
#include "hwresource.h"
#include "mcast_const.h"
#include "platform.h"

static struct me_t *igmp_router_bridge_port_me_of_lan_get(unsigned char logical_port_id)
{
	if (logical_port_id < 12)
	{
		struct me_t *meptr_422 = NULL; 
		struct meinfo_t *miptr = meinfo_util_miptr(422);
		list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
			if (meinfo_util_me_attr_data(meptr_422, 1) == 0) continue;
			if (meinfo_util_me_attr_data(meptr_422, 4) == ENV_BRIDGE_PORT_TYPE_CPU)
			{ // VEIP
				return hwresource_private2public(meptr_422);
			}
		}
		
		// cant find VEIP, find corresponding UNI instead
		list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
			if (meinfo_util_me_attr_data(meptr_422, 1) == 0) continue;
			if (meinfo_util_me_attr_data(meptr_422, 4) == ENV_BRIDGE_PORT_TYPE_UNI)
			{ // UNI
				if (meinfo_util_me_attr_data(meptr_422, 6) != logical_port_id) continue;
				dbprintf_igmp(LOG_WARNING, "UNI for logical_port_id %d found\n", logical_port_id);
				return hwresource_private2public(meptr_422);
			}
		}
		
		// cant find own UNI, use UNI0 instead (p2p_enable)
		list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
		if (meinfo_util_me_attr_data(meptr_422, 1) == 0) continue;
		if (meinfo_util_me_attr_data(meptr_422, 4) == ENV_BRIDGE_PORT_TYPE_UNI)
		       { // UNI
				if (meinfo_util_me_attr_data(meptr_422, 6) == 0 && omci_env_g.port2port_enable)
				{
					dbprintf_igmp(LOG_WARNING, "VEIP/UNI not found. UNI0 used intead\n");
					return hwresource_private2public(meptr_422);
				}
			}
		}                		
	}
	return NULL;
}

static int
igmp_filter_port_validator_pre_filter(struct igmp_clientinfo_t *igmp_clientinfo)
{
	MCAST_FILTER_NO_IPV6(igmp_clientinfo)
		
	int ret = MCAST_FILTER_CONTINUE;
	
	if (THREAD_ENV.igmp_proxy_snooping)
	{
		struct me_t *bridge_port_me = igmp_clientinfo->rx_desc.bridge_port_me;
		if (!bridge_port_me)
		{
			dbprintf_igmp(LOG_WARNING, "bridge_port_me == NULL\n");
			ret = MCAST_FILTER_STOP;
			return ret;
		}
	}

	if (omci_env_g.olt_alu_mcast_support)
	{
		dbprintf_igmp(LOG_WARNING, "bypass port validator - ALU\n");
		return ret;	// bypass this filter
	}
			
	switch (igmp_clientinfo->igmp_msg_type)
	{
	case IGMP_MEMBERSHIP_QUERY:
	case MLD_LISTENER_QUERY:
		break;
	case IGMP_V1_MEMBERSHIP_REPORT:
	case IGMP_V2_MEMBERSHIP_REPORT:
	case IGMP_V2_LEAVE_GROUP:
	case IGMP_V3_MEMBERSHIP_REPORT:
	case MLD_LISTENER_REPORT:
	case MLD_LISTENER_REDUCTION:
	case MLD_V2_LISTENER_REPORT:
		if (igmp_clientinfo->rx_desc.logical_port_id == switch_get_wan_logical_portid()) ret = MCAST_FILTER_STOP;	// report/leave comes from LAN only
		break;	
	}
	
	if (ret) dbprintf_igmp(LOG_WARNING, "Packets comes from invalid port\n");
	
	return ret;
}

// module callbacks
static const char * igmp_filter_port_validator_name(void)
{
	return "port_validator";
}

static int
igmp_filter_port_validator_raw_filter(struct cpuport_info_t *pktinfo)
{
	if (omci_env_g.port2port_enable) return MCAST_FILTER_CONTINUE;

	uint32_t allhosts_group = htonl(INADDR_ALLHOSTS_GROUP);
        
	if (THREAD_ENV.igmp_proxy_snooping == 0)
	{
		struct switch_port_info_t port_info;
		
		switch_get_port_info_by_logical_port_id(pktinfo->rx_desc.logical_port_id, &port_info);
		if (pktinfo->rx_desc.logical_port_id != switch_get_wan_logical_portid()) pktinfo->rx_desc.bridge_port_me = port_info.bridge_port_me;
		if (!pktinfo->rx_desc.bridge_port_me)
		{
			dbprintf_igmp(LOG_WARNING, "bridge_port_me null\n");
			return -1;
		}
	}
	else if (THREAD_ENV.igmp_proxy_snooping == 2)
	{
		if (pktinfo->tci.ethertype == ETH_P_IP)
		{
			struct ip *ip = (struct ip *)pktinfo->frame_ptr;
			if (!(ip->ip_p == 0 || ip->ip_src.s_addr == allhosts_group))
			{
				if (pktinfo->rx_desc.logical_port_id != switch_get_wan_logical_portid()) pktinfo->rx_desc.bridge_port_me = igmp_router_bridge_port_me_of_lan_get(pktinfo->rx_desc.logical_port_id);
				if (!pktinfo->rx_desc.bridge_port_me)
				{
					dbprintf_igmp(LOG_WARNING, "bridge_port_me null\n");
					return -1;
				}
			}
		}
		else if (pktinfo->tci.ethertype == ETH_P_IPV6)
		{
			if (pktinfo->rx_desc.logical_port_id != switch_get_wan_logical_portid()) pktinfo->rx_desc.bridge_port_me = igmp_router_bridge_port_me_of_lan_get(pktinfo->rx_desc.logical_port_id);
			if (!pktinfo->rx_desc.bridge_port_me)
			{
				dbprintf_igmp(LOG_WARNING, "bridge_port_me null\n");
				return -1;
			}
		}
	}
	
	return MCAST_FILTER_CONTINUE;
	
}

static int igmp_filter_port_validator_init(void)
{	
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_port_validator_deinit(void)
{
	return MCAST_FILTER_CONTINUE;
}

struct igmp_filter_module_t port_validator =
{
	.priority = 100,
	.filter_name = igmp_filter_port_validator_name,
	.raw_filter = igmp_filter_port_validator_raw_filter,
	.init = igmp_filter_port_validator_init,
	.deinit = igmp_filter_port_validator_deinit,
	.pre_filter = igmp_filter_port_validator_pre_filter,
};
