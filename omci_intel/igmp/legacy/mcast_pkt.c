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
 * File    : igmp_pkt.c
 *
 ******************************************************************/

#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/ppp_defs.h>
#include <linux/if_ether.h>

#include "list.h"
#include "util.h"
#include "util_inet.h"
#include "mcast_const.h"
#include "mcast.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "cpuport_util.h"
#include "cpuport_vtagging_vfilter.h"
#include "switch.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "igmp_omci.h"
#include "mcast_timer.h"
#include "switch.h"
#include "mcast_switch.h"
#include "meinfo.h"
#include "hwresource.h"

extern char igmp_clientinfo_str_g[2048];

unsigned short alu_olt_ethertype = 0;

int
igmp_pkt_tag_num_get(struct cpuport_info_t *pktinfo)
{
	return cpuport_util_get_tagnum_from_tci(&pktinfo->tci);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

int
igmp_pkt_recv_is_igmp_join_leave(struct cpuport_info_t *pktinfo)
{
	unsigned char *iphead;
	unsigned int ip_head_len;

	if (omci_env_g.igmp_mcast_me_support) {
		struct meinfo_t *miptr_309 = meinfo_util_miptr(309);
		struct meinfo_t *miptr_310 = meinfo_util_miptr(310);
		if (list_empty(&miptr_309->me_instance_list) || list_empty(&miptr_310->me_instance_list)) {
			// skip without omci provisioning
			return -1;
		}
	}

	if (pktinfo == NULL || pktinfo->frame_ptr == NULL)  {
		dbprintf_igmp(LOG_WARNING,"pktinfo || pktinfo->frame_ptr == NULL\n"); 
		return -1;
	}

	if (pktinfo->rx_desc.router_mode)
	{
		iphead = pktinfo->frame_ptr;
	}
	else
	{
		if (pktinfo->tci.ethertype == ETH_P_IP || (pktinfo->tci.ethertype == ETH_P_PPP_SES && pktinfo->pppoe_proto == PPP_IP))
			iphead = pktinfo->nh;
		else
			return -1;
	}

	ip_head_len = (*iphead & 0xf) * 4;
	if ((*(iphead + 9) == 0x02)  && (*(iphead + ip_head_len) == IGMP_V1_MEMBERSHIP_REPORT
	    || *(iphead + ip_head_len) == IGMP_V2_MEMBERSHIP_REPORT || *(iphead + ip_head_len) == IGMP_V2_LEAVE_GROUP 
	    || *(iphead + ip_head_len) == IGMP_V3_MEMBERSHIP_REPORT) && (*iphead & 0x40))
		return 0;
	else if ((*(iphead + 9) == 0x00))
		return 0;

	return -1;
}

int
igmp_pkt_recv_is_igmp_query(struct cpuport_info_t *pktinfo)
{
	unsigned char *iphead=NULL;
	unsigned int ip_head_len;

	if (omci_env_g.igmp_mcast_me_support) {
		struct meinfo_t *miptr_309 = meinfo_util_miptr(309);
		struct meinfo_t *miptr_310 = meinfo_util_miptr(310);
		if (list_empty(&miptr_309->me_instance_list) || list_empty(&miptr_310->me_instance_list)) {
			// skip without omci provisioning
			return -1;
		}
	}

	if (pktinfo->rx_desc.router_mode)
		iphead = pktinfo->frame_ptr ;
	else
	{
		if (pktinfo->tci.ethertype == ETH_P_IP || (pktinfo->tci.ethertype == ETH_P_PPP_SES && pktinfo->pppoe_proto == PPP_IP))
			iphead = pktinfo->nh;
		else
			return -1;
	}

	ip_head_len = (*iphead & 0xf) * 4;
	if (*(iphead + 9) == 0x02 && *(iphead + ip_head_len) == IGMP_MEMBERSHIP_QUERY)
		return 0;

	return -1;
}

//for IPv6 MLD snooping Join Leave
int
igmp_pkt_recv_is_mld_join_leave(struct cpuport_info_t *pktinfo)
{
	unsigned char *iphead=NULL;
	int offset = 6;	// IPv6Hdr (4B) + Len(2B)
	
	if (omci_env_g.igmp_mcast_me_support) {
		struct meinfo_t *miptr_309 = meinfo_util_miptr(309);
		struct meinfo_t *miptr_310 = meinfo_util_miptr(310);
		if (list_empty(&miptr_309->me_instance_list) || list_empty(&miptr_310->me_instance_list)) {
			// skip without omci provisioning
			return -1;
		}
	}

	if (pktinfo->rx_desc.router_mode)
	{
		iphead = pktinfo->frame_ptr ;
		return -1;	// proxy mode for MLD is not supported
	}
	else
	{
		if (pktinfo->tci.ethertype == ETH_P_IPV6 || (pktinfo->tci.ethertype == ETH_P_PPP_SES && pktinfo->pppoe_proto == PPP_IPV6))
			iphead = pktinfo->nh;
		else
			return -1;
	}
	
	// Inspect IPv6 Next header: Hop-by-hop option
	while (*(iphead + offset) == IPPROTO_HOPOPTS)
	{
		offset += 34;
		if(offset >= 128) return CPUPORT_UNMATCH;
	}	
	// Inspect IPv6 Next header: ICMPv6
	if (*(iphead + offset) == IPPROTO_ICMPV6)
		offset += 2;
	else
		return CPUPORT_UNMATCH;
	
	// Inspect Router Alert
	if (*(iphead + offset) != 0x05)     return CPUPORT_UNMATCH;
	if ((*(unsigned short *)(iphead + offset + 2))) return CPUPORT_UNMATCH;
	offset += 6;
	
	// Inspect MLD type
	if((*(iphead + offset) == MLD_LISTENER_REPORT) || 
	   (*(iphead + offset) == MLD_LISTENER_REDUCTION) || 
	   (*(iphead + offset) == MLD_V2_LISTENER_REPORT))
		return CPUPORT_MATCH;
	
	return CPUPORT_UNMATCH;
}

int
igmp_pkt_recv_is_mld_query(struct cpuport_info_t *pktinfo)
{
	unsigned char *iphead=NULL;
	int offset = 6;	// IPv6Hdr (4B) + Len(2B)
	
	if (omci_env_g.igmp_mcast_me_support) {
		struct meinfo_t *miptr_309 = meinfo_util_miptr(309);
		struct meinfo_t *miptr_310 = meinfo_util_miptr(310);
		if (list_empty(&miptr_309->me_instance_list) || list_empty(&miptr_310->me_instance_list)) {
			// skip without omci provisioning
			return -1;
		}
	}

	if (pktinfo->rx_desc.router_mode)
	{
		iphead = pktinfo->frame_ptr ;
		return -1;	// proxy mode for MLD is not supported
	}
	else
	{
		if (pktinfo->tci.ethertype == ETH_P_IPV6 || (pktinfo->tci.ethertype == ETH_P_PPP_SES && pktinfo->pppoe_proto == PPP_IPV6))
			iphead = pktinfo->nh;
		else
			return -1;
	}

	// Inspect IPv6 Next header: Hop-by-hop option
	while(*(iphead + offset) == IPPROTO_HOPOPTS)
	{
		offset += 34;
		if(offset >= 128) return CPUPORT_UNMATCH;
	}
		
	// Inspect IPv6 Next header: ICMPv6
	if(*(iphead + offset) == IPPROTO_ICMPV6)
		offset += 2;
	else
		return CPUPORT_UNMATCH;
	
	// Inspect Router Alert
	if (*(iphead + offset) != 0x05)     return CPUPORT_UNMATCH;
	if ((*(unsigned short *)(iphead + offset + 2))) return CPUPORT_UNMATCH;
	offset += 6;
	
	// Inspect MLD type
	if ((*(iphead + offset) == MLD_LISTENER_QUERY))
		return CPUPORT_MATCH;
	
	return CPUPORT_UNMATCH;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
int
igmp_pkt_parse_mld_type(unsigned char *ipv6_ptr, struct igmp_clientinfo_t *igmp_clientinfo)
{
	int offset = 6;	// IPv6Hdr (4B) + Len(2B)
	
	// Inspect IPv6 Next header: Hop-by-hop option
	while(*(ipv6_ptr + offset) == IPPROTO_HOPOPTS)
	{
		offset += 34;
		if(offset >= 128) return CPUPORT_UNMATCH;
	}
	
	// Inspect IPv6 Next header: ICMPv6
	if (*(ipv6_ptr + offset) == IPPROTO_ICMPV6)
		offset += 2;
	else
		return CPUPORT_UNMATCH;
	
	// Inspect Router Alert
	if (*(ipv6_ptr + offset) != 0x05)     return CPUPORT_UNMATCH;
	if ((*(unsigned short *)(ipv6_ptr + offset + 2))) return CPUPORT_UNMATCH;
	offset += 6;
	
	// Inspect MLD type
	igmp_clientinfo->igmp_msg_type = *(ipv6_ptr + offset);
	
	return offset;
}

static int
igmp_pkt_upstream_tci_ctl(struct igmp_config_t *igmp_config, struct cpuport_info_t *pktinfo, struct cpuport_tci_t *dst_tci)
{
	int tag_num = 0;
	tag_num = igmp_pkt_tag_num_get(pktinfo);

	memset(dst_tci ,0 ,sizeof(struct cpuport_tci_t));

	switch (igmp_config->upstream_igmp_tag_ctl)
	{
	case 0:
		memcpy(&pktinfo->tci, dst_tci, sizeof(struct cpuport_tci_t));
	case 1:

		if (tag_num == 0)
		{
			if (omci_env_g.igmp_upstream_tpid) 
				dst_tci->itci.tpid = omci_env_g.igmp_upstream_tpid;
			else
				dst_tci->itci.tpid = 0x8100;
				
			dst_tci->itci.priority = igmp_config->upstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->upstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 1)
		{
			dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
			dst_tci->itci.priority = pktinfo->tci.itci.priority;
			dst_tci->itci.de = pktinfo->tci.itci.de;
			dst_tci->itci.vid = pktinfo->tci.itci.vid;
			if (omci_env_g.igmp_upstream_tpid) 
				dst_tci->otci.tpid = omci_env_g.igmp_upstream_tpid;
			else
				dst_tci->otci.tpid = 0x8100;
				
			dst_tci->otci.priority = igmp_config->upstream_igmp_tci >> 13;
			dst_tci->otci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
			dst_tci->otci.vid = igmp_config->upstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 2)
		{
			dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
			dst_tci->itci.priority = pktinfo->tci.itci.priority;
			dst_tci->itci.de = pktinfo->tci.itci.de;
			dst_tci->itci.vid = pktinfo->tci.itci.vid;
			dst_tci->otci.tpid = pktinfo->tci.otci.tpid;
			dst_tci->otci.priority = pktinfo->tci.otci.priority;
			dst_tci->otci.de = pktinfo->tci.otci.de;
			dst_tci->otci.vid = pktinfo->tci.otci.vid;
			if (omci_env_g.igmp_upstream_tpid) 
				dst_tci->ext1_tci.tpid = omci_env_g.igmp_upstream_tpid;
			else
				dst_tci->ext1_tci.tpid = 0x8100;
				
			dst_tci->ext1_tci.priority = igmp_config->upstream_igmp_tci >> 13;
			dst_tci->ext1_tci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
			dst_tci->ext1_tci.vid = igmp_config->upstream_igmp_tci & 0xfff;
		}

		break;
	case 2:
		if (tag_num == 0)
		{
			if (omci_env_g.igmp_upstream_tpid) 
				dst_tci->itci.tpid = omci_env_g.igmp_upstream_tpid;
			else
				dst_tci->itci.tpid = 0x8100;

			dst_tci->itci.priority = igmp_config->upstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->upstream_igmp_tci & 0xfff;

		}
		else if (tag_num == 1)
		{
			memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
			dst_tci->itci.priority = igmp_config->upstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->upstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 2)
		{
			dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
			dst_tci->itci.priority = pktinfo->tci.itci.priority;
			dst_tci->itci.de = pktinfo->tci.itci.de;
			dst_tci->itci.vid = pktinfo->tci.itci.vid;
			memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
			dst_tci->otci.priority = igmp_config->upstream_igmp_tci >> 13;
			dst_tci->otci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
			dst_tci->otci.vid = igmp_config->upstream_igmp_tci & 0xfff;
		}

		break;
	case 3:
		if (tag_num == 0)
		{
			if (omci_env_g.igmp_upstream_tpid) 
				dst_tci->itci.tpid = omci_env_g.igmp_upstream_tpid;
			else
				dst_tci->itci.tpid = 0x8100;

			dst_tci->itci.priority = igmp_config->upstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->upstream_igmp_tci & 0xfff;

		}
		else if (tag_num == 1)
		{
			memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
			dst_tci->itci.vid = igmp_config->upstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 2)
		{
			dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
			dst_tci->itci.priority = pktinfo->tci.itci.priority;
			dst_tci->itci.de = pktinfo->tci.itci.de;
			dst_tci->itci.vid = pktinfo->tci.itci.vid;
			memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
			dst_tci->otci.vid = igmp_config->upstream_igmp_tci & 0xfff;
		}
		break;
		// Customize behavior for BT (priority tag: replace vid, others: add tag (vid only)
	case 254:
		if (tag_num == 0)
		{
			if (omci_env_g.igmp_upstream_tpid)
				dst_tci->itci.tpid = omci_env_g.igmp_upstream_tpid;
			else
				dst_tci->itci.tpid = 0x8100;
	
			dst_tci->itci.priority = igmp_config->upstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->upstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 1)
		{
			if (pktinfo->tci.itci.vid == 0)
			{
				memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
				//dst_tci->itci.priority = igmp_config->upstream_igmp_tci >> 13;
				dst_tci->itci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
				dst_tci->itci.vid = igmp_config->upstream_igmp_tci & 0xfff;
			}
			else
			{
				dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
				dst_tci->itci.priority = pktinfo->tci.itci.priority;
				dst_tci->itci.de = pktinfo->tci.itci.de;
				dst_tci->itci.vid = pktinfo->tci.itci.vid;
				if (omci_env_g.igmp_upstream_tpid)
					dst_tci->otci.tpid = omci_env_g.igmp_upstream_tpid;
				else
					dst_tci->otci.tpid = 0x8100;
	
				dst_tci->otci.priority = pktinfo->tci.itci.priority;
				dst_tci->otci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
				dst_tci->otci.vid = igmp_config->upstream_igmp_tci & 0xfff;
			}
		}
		else if (tag_num == 2)
		{
			if (pktinfo->tci.otci.vid==0)
			{
				dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
				dst_tci->itci.priority = pktinfo->tci.itci.priority;
				dst_tci->itci.de = pktinfo->tci.itci.de;
				dst_tci->itci.vid = pktinfo->tci.itci.vid;
				memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
				//dst_tci->otci.priority = igmp_config->upstream_igmp_tci >> 13;
				dst_tci->otci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
				dst_tci->otci.vid = igmp_config->upstream_igmp_tci & 0xfff;
			}
			else
			{
				dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
				dst_tci->itci.priority = pktinfo->tci.itci.priority;
				dst_tci->itci.de = pktinfo->tci.itci.de;
				dst_tci->itci.vid = pktinfo->tci.itci.vid;
				dst_tci->otci.tpid = pktinfo->tci.otci.tpid;
				dst_tci->otci.priority = pktinfo->tci.otci.priority;
				dst_tci->otci.de = pktinfo->tci.otci.de;
				dst_tci->otci.vid = pktinfo->tci.otci.vid;
				if (omci_env_g.igmp_upstream_tpid)
					dst_tci->ext1_tci.tpid = omci_env_g.igmp_upstream_tpid;
				else
					dst_tci->ext1_tci.tpid = 0x8100;
	
				dst_tci->ext1_tci.priority = pktinfo->tci.otci.priority;
				dst_tci->ext1_tci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
				dst_tci->ext1_tci.vid = igmp_config->upstream_igmp_tci & 0xfff;
			}
		}
	
		break;
		// Customize behavior for BT (priority tag: replace vid, others: add tag
	case 255:
		if (tag_num == 0)
		{
			if (omci_env_g.igmp_upstream_tpid)
				dst_tci->itci.tpid = omci_env_g.igmp_upstream_tpid;
			else
				dst_tci->itci.tpid = 0x8100;
	
			dst_tci->itci.priority = igmp_config->upstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->upstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 1)
		{
			if (pktinfo->tci.itci.vid==0)
			{
				memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
				//dst_tci->itci.priority = igmp_config->upstream_igmp_tci >> 13;
				dst_tci->itci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
				dst_tci->itci.vid = igmp_config->upstream_igmp_tci & 0xfff;
			}
			else
			{
				dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
				dst_tci->itci.priority = pktinfo->tci.itci.priority;
				dst_tci->itci.de = pktinfo->tci.itci.de;
				dst_tci->itci.vid = pktinfo->tci.itci.vid;
				if (omci_env_g.igmp_upstream_tpid)
					dst_tci->otci.tpid = omci_env_g.igmp_upstream_tpid;
				else
					dst_tci->otci.tpid = 0x8100;
	
				dst_tci->otci.priority = igmp_config->upstream_igmp_tci >> 13;
				dst_tci->otci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
				dst_tci->otci.vid = igmp_config->upstream_igmp_tci & 0xfff;
			}
		} 
		else if (tag_num == 2)
		{
			if (pktinfo->tci.otci.vid==0)
			{
				dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
				dst_tci->itci.priority = pktinfo->tci.itci.priority;
				dst_tci->itci.de = pktinfo->tci.itci.de;
				dst_tci->itci.vid = pktinfo->tci.itci.vid;
				memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
				//dst_tci->otci.priority = igmp_config->upstream_igmp_tci >> 13;
				dst_tci->otci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
				dst_tci->otci.vid = igmp_config->upstream_igmp_tci & 0xfff;
			}
			else
			{
				dst_tci->itci.tpid = pktinfo->tci.itci.tpid;
				dst_tci->itci.priority = pktinfo->tci.itci.priority;
				dst_tci->itci.de = pktinfo->tci.itci.de;
				dst_tci->itci.vid = pktinfo->tci.itci.vid;
				dst_tci->otci.tpid = pktinfo->tci.otci.tpid;
				dst_tci->otci.priority = pktinfo->tci.otci.priority;
				dst_tci->otci.de = pktinfo->tci.otci.de;
				dst_tci->otci.vid = pktinfo->tci.otci.vid;
				if (omci_env_g.igmp_upstream_tpid)
					dst_tci->ext1_tci.tpid = omci_env_g.igmp_upstream_tpid;
				else
					dst_tci->ext1_tci.tpid = 0x8100;
	
				dst_tci->ext1_tci.priority = igmp_config->upstream_igmp_tci >> 13;
				dst_tci->ext1_tci.de = (igmp_config->upstream_igmp_tci >> 12) & 1;
				dst_tci->ext1_tci.vid = igmp_config->upstream_igmp_tci & 0xfff;
			}
		}
	
		break;
	default:
		return -1;
	}
	return 0;
}
static int
igmp_pkt_downstream_tci_ctl(struct igmp_config_t *igmp_config, struct cpuport_info_t *pktinfo, struct cpuport_tci_t *dst_tci)
{
	int tag_num = 0;
	tag_num = igmp_pkt_tag_num_get(pktinfo);

	memset(dst_tci ,0 ,sizeof(struct cpuport_tci_t));

	switch (igmp_config->downstream_igmp_tag_ctl)
	{
	case 0:
		memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
	case 1:
		if (tag_num == 2)
		{
			dst_tci->itci.priority = pktinfo->tci.itci.tpid;
			dst_tci->itci.de = pktinfo->tci.itci.de;
			dst_tci->itci.vid = pktinfo->tci.itci.vid;
			dst_tci->otci.priority = 0;
			dst_tci->otci.de = 0;
			dst_tci->otci.vid = 0;
		}
		else if (tag_num == 1 || tag_num == 0)
		{
			dst_tci->itci.priority = 0,
			dst_tci->itci.de = 0,
			dst_tci->itci.vid = 0,
			dst_tci->otci.priority = 0;
			dst_tci->otci.de = 0;
			dst_tci->otci.vid = 0;
		}
		break;
	case 2:
		if (tag_num == 0)
		{
			if (omci_env_g.igmp_downstream_tpid) 
				dst_tci->itci.tpid = omci_env_g.igmp_downstream_tpid;
			else
				dst_tci->itci.tpid = 0x8100;
				
			dst_tci->itci.priority = igmp_config->downstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->downstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->downstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 1)
		{
			if (omci_env_g.igmp_downstream_tpid) 
				dst_tci->otci.tpid = omci_env_g.igmp_downstream_tpid;
			else
				dst_tci->otci.tpid = 0x8100;
				
			dst_tci->otci.priority = igmp_config->downstream_igmp_tci >> 13;
			dst_tci->otci.de = (igmp_config->downstream_igmp_tci >> 12) & 1;
			dst_tci->otci.vid = igmp_config->downstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 2)
		{
			if (omci_env_g.igmp_downstream_tpid) 
				dst_tci->ext1_tci.tpid = omci_env_g.igmp_downstream_tpid;
			else
				dst_tci->ext1_tci.tpid = 0x8100;
				
			dst_tci->ext1_tci.priority = igmp_config->downstream_igmp_tci >> 13;
			dst_tci->ext1_tci.de = (igmp_config->downstream_igmp_tci >> 12) & 1;
			dst_tci->ext1_tci.vid = igmp_config->downstream_igmp_tci & 0xfff;
		}

		break;
	case 3:
		if (tag_num == 0)
		{
			if (omci_env_g.igmp_downstream_tpid) 
				dst_tci->itci.tpid = omci_env_g.igmp_downstream_tpid;
			else
				dst_tci->itci.tpid = 0x8100;

			dst_tci->itci.priority = igmp_config->downstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->downstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->downstream_igmp_tci & 0xfff;

		}
		else if (tag_num == 1)
		{
			memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
			dst_tci->itci.priority = igmp_config->downstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->downstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->downstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 2)
		{
			memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
			dst_tci->otci.priority = igmp_config->downstream_igmp_tci >> 13;
			dst_tci->otci.de = (igmp_config->downstream_igmp_tci >> 12) & 1;
			dst_tci->otci.vid = igmp_config->downstream_igmp_tci & 0xfff;
		}

		break;
	case 4:
		if (tag_num == 0)
		{
			if (omci_env_g.igmp_downstream_tpid) 
				dst_tci->itci.tpid = omci_env_g.igmp_downstream_tpid;
			else
				dst_tci->itci.tpid = 0x8100;

			dst_tci->itci.priority = igmp_config->downstream_igmp_tci >> 13;
			dst_tci->itci.de = (igmp_config->downstream_igmp_tci >> 12) & 1;
			dst_tci->itci.vid = igmp_config->downstream_igmp_tci & 0xfff;
		}
		else if (tag_num == 1)
		{
			memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
			dst_tci->itci.vid = igmp_config->downstream_igmp_tci & 0xfff ;
		} else if (tag_num == 2)
		{
			memcpy(dst_tci, &pktinfo->tci, sizeof(struct cpuport_tci_t));
			dst_tci->otci.vid = igmp_config->downstream_igmp_tci & 0xfff ;
		}
		break;
	case 5:
	case 6:
	case 7:
		dbprintf_igmp(LOG_WARNING,"IGMP downstream TCI ctl (%d) unsupport",igmp_config->downstream_igmp_tag_ctl); 				
		break;
	default:
		return -1;
	}
	return 0;
}

int
igmp_pkt_tag_filter_iteration (struct igmp_clientinfo_t *igmp_clientinfo)
{
	int rtn = 0;
	struct me_t *iuni0_port_me, *uni0_port_me;
	struct me_t *isrc_port_me, *idst_port_me;  
	unsigned int src_port_type, dst_port_type;
	unsigned int src_port_logical_id, dst_port_logical_id;
	struct cpuport_info_t *pktinfo;
	struct me_t *dst_port_me;
	unsigned  char i=0;
	unsigned short veip_meid=cpuport_util_get_veip_meid();
	struct me_t *veip_port_me=NULL;

	struct meinfo_t *miptr= meinfo_util_miptr(47);		
	if (miptr==NULL)
	{
		dbprintf_igmp(LOG_ERR, "class=47, null miptr?\n");
		return -1;
	}		
	
	pktinfo = igmp_clientinfo->pktinfo; 
	if (!pktinfo->rx_desc.bridge_port_me)
	{
		dbprintf_igmp(LOG_ERR, "rx_desc bridge_port_me == NULL (logical port id:%d)\n"
				,pktinfo->rx_desc.logical_port_id);
		return -1;
	}
	memset(igmp_clientinfo->tx_tci, 0, 8*sizeof(struct cpuport_tci_t));
	memset(igmp_clientinfo->tx_meid, 0, 8*sizeof(unsigned short));

	if ((isrc_port_me = hwresource_public2private(pktinfo->rx_desc.bridge_port_me)) == NULL)
	{
		dbprintf_igmp(LOG_ERR, "cannot find relate private source bridge port me.\n");
		return -1;
	}
	src_port_logical_id = meinfo_util_me_attr_data(isrc_port_me, 6);
	src_port_type = meinfo_util_me_attr_data(isrc_port_me, 4);	//attr 4 = port type
	
	// prepare uni0 me for port2port_enable
	iuni0_port_me = meinfo_me_get_by_meid(422, 0);
	uni0_port_me = hwresource_private2public(iuni0_port_me); 

	if (veip_meid != 0xffff) veip_port_me = meinfo_me_get_by_meid(47, veip_meid);

	list_for_each_entry(dst_port_me, &miptr->me_instance_list, instance_node) {
	#if 0
		//skip ports in different bridge
		if (meinfo_util_me_attr_data(dst_port_me, 1) != meinfo_util_me_attr_data(pktinfo->rx_desc.bridge_port_me, 1))
			continue;
	#endif
		if ((idst_port_me = hwresource_public2private(dst_port_me)) == NULL)
		{
			dbprintf_igmp(LOG_ERR, "cannot find related private destination bridge port me. meid=%d(0x%x)\n",dst_port_me->meid,dst_port_me->meid);
			continue;
		}
		
		dst_port_logical_id = meinfo_util_me_attr_data(idst_port_me, 6);
		pktinfo->tx_desc.bridge_port_me = dst_port_me;
	
		if (src_port_type <= 1)
		{ //CPU/UNI port
			int has_calix_vtagging_me = 0;

			if ((dst_port_type = meinfo_util_me_attr_data(idst_port_me, 4)) != 2)//only to wan port
				continue;
			if (meinfo_util_me_attr_data(dst_port_me, 3) == 6) //skip multicast IWTP
				continue;
	
			if (omci_env_g.port2port_enable == 0 && 
				meinfo_util_me_attr_data(pktinfo->rx_desc.bridge_port_me,1) != meinfo_util_me_attr_data(dst_port_me,1)) // skip different bridge
				continue;

			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
				unsigned short bport_meid = pktinfo->rx_desc.bridge_port_me->meid;
				unsigned short meid = meinfo_util_me_attr_data(pktinfo->rx_desc.bridge_port_me,4);
				if (meinfo_me_get_by_meid(249, meid) != NULL || meinfo_me_get_by_meid(249, bport_meid) != NULL )
					has_calix_vtagging_me = 1;
			}

			if (omci_env_g.igmp_mcast_me_support && igmp_config_g.upstream_igmp_tag_ctl && !has_calix_vtagging_me)
			{
				igmp_pkt_upstream_tci_ctl(&igmp_config_g, pktinfo,&igmp_clientinfo->tx_tci[i]);
				igmp_clientinfo->tx_meid[i] = dst_port_me->meid; 
				i++;
			}
			else
			{
				// while the src_port_me is the physical port where the pkt comes from,
				// th ingress_port_me is the srcport whose vlan translation rule would be referred
				struct me_t *ingress_port_me = pktinfo->rx_desc.bridge_port_me;
				// find equiv bport me if port2port_enable
				if (omci_env_g.port2port_enable)
				{
					if (iuni0_port_me && uni0_port_me) 
						ingress_port_me = uni0_port_me;
					else if (veip_port_me)
						ingress_port_me = veip_port_me;
				}
				// for wifi port, apply veip's vlan translation
				if ((src_port_logical_id == switch_get_wifi_logical_portid(0) || src_port_logical_id == switch_get_wifi_logical_portid(1)) && pktinfo->rx_desc.bridge_port_me && pktinfo->rx_desc.bridge_port_me->is_private)
				{
					if (veip_port_me)
					ingress_port_me = veip_port_me;
				}
				if (!ingress_port_me) continue;
						
				rtn = cpuport_vtagging_vfilter(ingress_port_me , pktinfo->tx_desc.bridge_port_me, &(pktinfo->tci), &igmp_clientinfo->tx_tci[i], pktinfo);
				if (rtn == CPUPORT_VTAGGING_VFILTER_BLOCK)
				{
					dbprintf_igmp(LOG_WARNING, "packet filtered or tagging fail from port %d to port %d \n", pktinfo->rx_desc.logical_port_id, dst_port_logical_id);
					continue;
				}
				else if (rtn == CPUPORT_VTAGGING_RULE_UNMATCH) // ? this wont happen as CPUPORT_VTAGGING_RULE_UNMATCH is treated as pass in cpuport_vtagging_vfilter()
				{
					// ? the classf_add_default_rules means default pass or not?
					dbprintf_igmp(LOG_WARNING, "packet tagging rule not found from port %d to port %d \n", pktinfo->rx_desc.logical_port_id, dst_port_logical_id);
					if (omci_env_g.classf_add_default_rules == 0)
					{
						dbprintf_igmp(LOG_WARNING, "default drop\n");
						continue;
					}
					else
					{
						dbprintf_igmp(LOG_WARNING, "default pass\n");
						memcpy(&igmp_clientinfo->tx_tci[i], &(pktinfo->tci), sizeof(struct cpuport_tci_t));
						igmp_clientinfo->tx_meid[i] = dst_port_me->meid; 
						i++;
					}
				}
				else if (rtn == CPUPORT_VTAGGING_VFILTER_PASS)
				{
					igmp_clientinfo->tx_meid[i] = dst_port_me->meid; 
					dbprintf_igmp(LOG_WARNING, "packet filtered or tagging success from port %d to port %d dst_port=%x, i=%d\n", pktinfo->rx_desc.logical_port_id, dst_port_logical_id, dst_port_me->meid, i);
					i++;
				}
			}
	
		}
		else if (src_port_type == 2 || src_port_type == 4) //WAN port
		{
			dst_port_type = meinfo_util_me_attr_data(idst_port_me, 4);
			dbprintf_igmp(LOG_WARNING, "dst_port_type: %d, passthrough: %d tag_ctl: %d", dst_port_type, omci_env_g.igmp_query_pass_through, igmp_config_g.downstream_igmp_tag_ctl);
			if (dst_port_type != 1 && dst_port_type != 0)//to uni/veip port
				continue;
	
			if (omci_env_g.igmp_query_pass_through==3) // make packets untag
			{
				igmp_config_g.downstream_igmp_tag_ctl = 1;	
				igmp_pkt_downstream_tci_ctl(&igmp_config_g, pktinfo, &igmp_clientinfo->tx_tci[i]);
				igmp_clientinfo->tx_meid[i] = dst_port_me->meid; 
				i++;
			}
			else if (omci_env_g.igmp_mcast_me_support && igmp_config_g.downstream_igmp_tag_ctl)
			{
				igmp_pkt_downstream_tci_ctl(&igmp_config_g, pktinfo, &igmp_clientinfo->tx_tci[i]);
				igmp_clientinfo->tx_meid[i] = dst_port_me->meid; 
				i++;
			}
			else
			{
				// while the dst_port_me is the physical port where the pkt would go out,
				// th egress_port_me is the dstport whose vlan translation rule would be referred
				struct me_t *egress_port_me = pktinfo->tx_desc.bridge_port_me;
				// find equiv bport me if port2port_enable
				if (omci_env_g.port2port_enable)
				{
					if (iuni0_port_me && uni0_port_me) 
						egress_port_me = uni0_port_me;
					else if (veip_port_me)	
						egress_port_me = veip_port_me;
				}
				// for wifi port, apply veip's vlan translation
				if ((dst_port_logical_id == switch_get_wifi_logical_portid(0) || dst_port_logical_id == switch_get_wifi_logical_portid(1)) && pktinfo->tx_desc.bridge_port_me && pktinfo->tx_desc.bridge_port_me->is_private)
				{
					if (veip_port_me)
					      	egress_port_me = veip_port_me;
				}
				if (!egress_port_me) continue;
				
				rtn = cpuport_vtagging_vfilter(pktinfo->rx_desc.bridge_port_me, egress_port_me, &(pktinfo->tci), &igmp_clientinfo->tx_tci[i], pktinfo);
				if (rtn == CPUPORT_VTAGGING_VFILTER_BLOCK)
				{
					dbprintf_igmp(LOG_WARNING, "packet filtered or tagging fail from port %d to port %d\n", pktinfo->rx_desc.logical_port_id, dst_port_logical_id);
					if (omci_env_g.port2port_enable)
						break;	// ds & port2port enabled, from one srcport to all UNIs will apply same vtagging_vfilter 
					continue;
				}
				else if (rtn == CPUPORT_VTAGGING_RULE_UNMATCH) 	// ? this wont happen as unmatch is treated as pass in cpuport_vtagging_vfilter()
				{
					// ? classf_add_default_rules means default pass if not matched?
					dbprintf_igmp(LOG_WARNING, "packet tagging rule not found from port %d to port %d \n", pktinfo->rx_desc.logical_port_id, dst_port_logical_id);
					if (omci_env_g.classf_add_default_rules == 0)
					{
						dbprintf_igmp(LOG_WARNING, "default drop\n");
						continue;
					}
					else
					{
						dbprintf_igmp(LOG_WARNING, "default pass\n");
						memcpy(&igmp_clientinfo->tx_tci[i], &(pktinfo->tci), sizeof(struct cpuport_tci_t));
						igmp_clientinfo->tx_meid[i] = dst_port_me->meid; 
						i++;
					}
				}
				else if (rtn == CPUPORT_VTAGGING_VFILTER_PASS)
				{
					igmp_clientinfo->tx_meid[i] = dst_port_me->meid; 
					i++;
				}
			}
		}
		if (i >= 8)	break;
	}
	return 0;
}

int
igmp_pkt_clientinfo_str(struct igmp_clientinfo_t *igmp_clientinfo)
{
	char client_ip[32];
	int len = 0;
	int i = 0;
	util_inet_ntop (AF_INET, (void *)(&igmp_clientinfo->client_ip) , client_ip, 32);
	len += snprintf(igmp_clientinfo_str_g, 2048, "group_num\t%d\nclient_ip\t%s\nclient_mac\t%02x:%02x:%02x:%02x:%02x:%02x\nvid\t%d\nigmp_version\t%d\nigmp_msg_type\t%d(0x%x)\n", igmp_clientinfo->group_num , client_ip, igmp_clientinfo->client_mac[0], igmp_clientinfo->client_mac[1], igmp_clientinfo->client_mac[2], igmp_clientinfo->client_mac[3], igmp_clientinfo->client_mac[4], igmp_clientinfo->client_mac[5], igmp_clientinfo->vid, igmp_clientinfo->igmp_version, igmp_clientinfo->igmp_msg_type, igmp_clientinfo->igmp_msg_type);

	
	len += snprintf(igmp_clientinfo_str_g+len, 2048-len, "tx_meid:\n\t");
	for (i=0 ; i< 8 ;  i++)
	{
		len += snprintf(igmp_clientinfo_str_g+len, 2048-len, "%d(0x%x)\t",igmp_clientinfo->tx_meid[i],igmp_clientinfo->tx_meid[i]);
	}

	len += snprintf(igmp_clientinfo_str_g+len, 2048-len, "\n\nsrc_tci:\n");
	len += snprintf(igmp_clientinfo_str_g+len, 2048-len, "\tmeid %d(0x%x)\n\titci\t\ttpid:0x%04x pri:%d de:%d vid:%d(0x%x)\n\totci\t\ttpid:0x%04x pri:%d de:%d vid:%d(0x%x)\n\text1_tci\ttpid:0x%04x pri:%d de:%d vid:%d(0x%x)\n\text2_tci\ttpid:0x%04x pri:%d de:%d vid:%d(0x%x)\n\n", ((igmp_clientinfo->rx_desc).bridge_port_me)->meid, ((igmp_clientinfo->rx_desc).bridge_port_me)->meid, igmp_clientinfo->pktinfo->tci.itci.tpid, igmp_clientinfo->pktinfo->tci.itci.priority, igmp_clientinfo->pktinfo->tci.itci.de, igmp_clientinfo->pktinfo->tci.itci.vid, igmp_clientinfo->pktinfo->tci.itci.vid, igmp_clientinfo->pktinfo->tci.otci.tpid, igmp_clientinfo->pktinfo->tci.otci.priority, igmp_clientinfo->pktinfo->tci.otci.de, igmp_clientinfo->pktinfo->tci.otci.vid, igmp_clientinfo->pktinfo->tci.otci.vid, igmp_clientinfo->pktinfo->tci.ext1_tci.tpid, igmp_clientinfo->pktinfo->tci.ext1_tci.priority, igmp_clientinfo->pktinfo->tci.ext1_tci.de, igmp_clientinfo->pktinfo->tci.ext1_tci.vid, igmp_clientinfo->pktinfo->tci.ext1_tci.vid, igmp_clientinfo->pktinfo->tci.ext2_tci.tpid, igmp_clientinfo->pktinfo->tci.ext2_tci.priority, igmp_clientinfo->pktinfo->tci.ext2_tci.de, igmp_clientinfo->pktinfo->tci.ext2_tci.vid, igmp_clientinfo->pktinfo->tci.ext2_tci.vid);

	len += snprintf(igmp_clientinfo_str_g+len, 2048-len, "\n\ndst_tci:\n");
	for (i=0 ; i< 8 ;  i++)
	{
		len += snprintf(igmp_clientinfo_str_g+len, 2048-len, "\tmeid %d(0x%x)\n\titci\t\ttpid:0x%04x pri:%d de:%d vid:%d(0x%x)\n\totci\t\ttpid:0x%04x pri:%d de:%d vid:%d(0x%x)\n\text1_tci\ttpid:0x%04x pri:%d de:%d vid:%d(0x%x)\n\text2_tci\ttpid:0x%04x pri:%d de:%d vid:%d(0x%x)\n\n", igmp_clientinfo->tx_meid[i], igmp_clientinfo->tx_meid[i], igmp_clientinfo->tx_tci[i].itci.tpid, igmp_clientinfo->tx_tci[i].itci.priority, igmp_clientinfo->tx_tci[i].itci.de, igmp_clientinfo->tx_tci[i].itci.vid, igmp_clientinfo->tx_tci[i].itci.vid, igmp_clientinfo->tx_tci[i].otci.tpid, igmp_clientinfo->tx_tci[i].otci.priority, igmp_clientinfo->tx_tci[i].otci.de, igmp_clientinfo->tx_tci[i].otci.vid, igmp_clientinfo->tx_tci[i].otci.vid, igmp_clientinfo->tx_tci[i].ext1_tci.tpid, igmp_clientinfo->tx_tci[i].ext1_tci.priority, igmp_clientinfo->tx_tci[i].ext1_tci.de, igmp_clientinfo->tx_tci[i].ext1_tci.vid, igmp_clientinfo->tx_tci[i].ext1_tci.vid, igmp_clientinfo->tx_tci[i].ext2_tci.tpid, igmp_clientinfo->tx_tci[i].ext2_tci.priority, igmp_clientinfo->tx_tci[i].ext2_tci.de, igmp_clientinfo->tx_tci[i].ext2_tci.vid, igmp_clientinfo->tx_tci[i].ext2_tci.vid);
	}
	return 0;
}

int
igmp_pkt_remove_igmp_v3_group (struct igmp_clientinfo_t *igmp_clientinfo, unsigned int uni_vid, struct in_addr group_ip, struct in_addr src_ip) 
{

	unsigned char *igmp_head = NULL;
	unsigned short group_num=0, source_num=0;
	unsigned char *cur_group_ptr=NULL, *cur_source_ptr=NULL;
	int ip_head_length ;
	int tag_num = -1;

	if (igmp_clientinfo->igmp_msg_type != IGMP_V3_MEMBERSHIP_REPORT) return 0;

	if (igmp_clientinfo->vid != uni_vid) return 0;

	tag_num = igmp_pkt_tag_num_get(igmp_clientinfo->pktinfo);
	ip_head_length = ((*(char *)igmp_clientinfo->pktinfo->nh) & 0x0f) * 4;
	igmp_head = igmp_clientinfo->pktinfo->nh + ip_head_length; 

	group_num = ntohs(*(unsigned short *)(igmp_head + 6));
	if (group_num > 1) return 0;

	cur_group_ptr = igmp_head + 8;
	//unsigned char *current_source_ptr=NULL;

	int i=0,j=0;
	for (i = 0; i < group_num ; i++)
	{
		//current_source_ptr = cur_group_ptr + 8;
		if(cur_group_ptr) source_num = ntohs(*(unsigned short *)(cur_group_ptr + 2));
		else break;

		if (memcmp(cur_group_ptr+4, &group_ip, sizeof(struct in_addr))==0)
		{
			for (j=0; j < source_num; j++)
			{
				if (src_ip.s_addr == 0) continue;
				cur_source_ptr = cur_group_ptr + 8;
				if (memcmp(cur_source_ptr, &src_ip, sizeof(struct in_addr))==0)
				{
					if (source_num == 1)
					{
						if(group_num == 1)
						{
							free_safe(igmp_clientinfo->pktinfo->buf_ptr);
							igmp_clientinfo->pktinfo->frame_len=0;
							return 0;
						}
						//group_len = aux len + source ip len + 8
						unsigned int group_len = (*(cur_group_ptr+1))*4 + source_num*4 + 8;
						unsigned char *end_ptr = igmp_clientinfo->pktinfo->frame_ptr + igmp_clientinfo->pktinfo->frame_len - group_len;
						unsigned char *current_ptr = cur_group_ptr;

						for (current_ptr = cur_group_ptr; current_ptr  < end_ptr ; current_ptr++) 
							*current_ptr = *(current_ptr + group_len);
						
						memset(current_ptr, 0, group_len);
						igmp_clientinfo->pktinfo->frame_len -= group_len;
						*(unsigned short *)(igmp_head + 6) = htons(group_num -1); //group number - 1
						return 0;
					}
					else
					{
						//src_len = aux len + source ip len + 8
						unsigned int src_len = 4;
						unsigned char *end_ptr  = igmp_clientinfo->pktinfo->frame_ptr + igmp_clientinfo->pktinfo->frame_len - src_len;
						unsigned char *current_ptr = cur_source_ptr;

						for (current_ptr = cur_source_ptr; current_ptr  < end_ptr ; current_ptr++) 
							*current_ptr = *(current_ptr + src_len);
						
						memset(current_ptr, 0, src_len);
						igmp_clientinfo->pktinfo->frame_len -= src_len;
						*(unsigned short *)(igmp_head + 6) = htons(source_num -1); //source number - 1
						return 0;
					}
				}
				cur_source_ptr += 4;
			}
		}
		cur_group_ptr = cur_source_ptr + *(cur_group_ptr+1)*4;
	}

	return 0;
}

int
igmp_pkt_remove_mld_v2_group (struct igmp_clientinfo_t *igmp_clientinfo, unsigned int uni_vid, struct in6_addr group_ipv6, struct in6_addr src_ipv6) 
{

	if (igmp_clientinfo->igmp_msg_type != MLD_V2_LISTENER_REPORT) return 0;
/*
	int active_vid = igmp_pkt_active_vid_before_tagging (igmp_clientinfo);
	if (active_vid != uni_vid)
		return 0;
*/
	unsigned char *igmp_head = NULL;
	unsigned short group_num=0, source_num=0;
	unsigned char *cur_group_ptr=NULL, *cur_source_ptr=NULL;
	int ip_head_length ;
	
	ip_head_length = igmp_pkt_parse_mld_type(igmp_clientinfo->pktinfo->frame_ptr, igmp_clientinfo);
	if(ip_head_length == CPUPORT_UNMATCH) return 0;
	igmp_head = igmp_clientinfo->pktinfo->frame_ptr + ip_head_length; 
	group_num = ntohs(*(unsigned short *)(igmp_head + 6));
	cur_group_ptr = igmp_head + 8;
	//unsigned char *current_source_ptr=NULL;

	int i=0,j=0;
	for (i = 0; i < group_num ; i++)
	{
		//current_source_ptr = cur_group_ptr + 8;
		if (cur_group_ptr)
			source_num = ntohs(*(unsigned short *)(cur_group_ptr + 2));
		else
			break;

		if (memcmp(cur_group_ptr+4, &group_ipv6, sizeof(struct in6_addr))==0)
		{
			for (j=0; j < source_num; j++)
			{
				if (src_ipv6.s6_addr32[0] == 0 && src_ipv6.s6_addr32[1] == 0 && src_ipv6.s6_addr32[2] == 0 && src_ipv6.s6_addr32[3] == 0)
					continue;
				cur_source_ptr = cur_group_ptr + 20;
				if (memcmp(cur_source_ptr, &src_ipv6, sizeof(struct in6_addr))==0)
				{
					if (source_num == 1)
					{
						if(group_num == 1)
						{
							free_safe(igmp_clientinfo->pktinfo->buf_ptr);
							igmp_clientinfo->pktinfo->frame_len=0;
							return 0;
						}
						//group_len = aux len + source ip len + 8
						unsigned int group_len = (*(cur_group_ptr+1))*4 + source_num*4 + 8;
						unsigned char *end_ptr = igmp_clientinfo->pktinfo->frame_ptr + igmp_clientinfo->pktinfo->frame_len - group_len;
						unsigned char *current_ptr = cur_group_ptr;

						for (current_ptr = cur_group_ptr; current_ptr  < end_ptr ; current_ptr++) 
							*current_ptr = *(current_ptr + group_len);
						
						memset(current_ptr, 0, group_len);
						igmp_clientinfo->pktinfo->frame_len -= group_len;
						*(unsigned short *)(igmp_head + 6) = htons(group_num -1); //group number - 1
						return 0;
					}
					else
					{
						//src_len = aux len + source ip len + 8
						unsigned int src_len = 16;
						unsigned char *end_ptr  = igmp_clientinfo->pktinfo->frame_ptr + igmp_clientinfo->pktinfo->frame_len - src_len;
						unsigned char *current_ptr = cur_source_ptr;

						for (current_ptr = cur_source_ptr; current_ptr  < end_ptr ; current_ptr++) 
							*current_ptr = *(current_ptr + src_len);
						
						memset(current_ptr, 0, src_len);
						igmp_clientinfo->pktinfo->frame_len -= src_len;
						*(unsigned short *)(igmp_head + 6) = htons(source_num -1); //source number - 1
						return 0;
					}
				}
				cur_source_ptr += 16;
			}
		}
		cur_group_ptr = cur_source_ptr + *(cur_group_ptr+1)*4;
	}

	return 0;
}

int
igmp_pkt_ip_cmp(union ipaddr *src_ip, union ipaddr *dst_ip)
{
	int i;
	for (i = 0 ; i < sizeof(union ipaddr) ; i++)
	{
		if (*((unsigned char *)src_ip + i) > *((unsigned char *)dst_ip + i))
			return 1;
		else if (*((unsigned char *)src_ip + i) < *((unsigned char *)dst_ip + i))
			return -1;
	}
	return 0;
}

int
igmp_pkt_ip_cpy(union ipaddr *dst_ip, union ipaddr *src_ip)
{
	int i;
	for (i = 0 ; i < sizeof(union ipaddr) ; i++)
		*((unsigned char *)dst_ip + i) = *((unsigned char *)src_ip + i);

	return 0;
}

static int igmp_pkt_build_igmp_group(struct igmp_clientinfo_t *igmp_clientinfo, int *tl, int *ihl, unsigned int *pl)
{
	unsigned int i = 0, j = 0, offset = 0;
	unsigned short group_num=0, source_num=0;
	unsigned char *current_group_ptr=NULL, *current_source_ptr=NULL;
	struct igmp_group_t *igmp_group = NULL;
	struct in_addr dst_addr;
	int total_length = *tl;
	int ip_head_length = *ihl;
	unsigned int payload = *pl;
	struct cpuport_info_t *pktinfo = igmp_clientinfo->pktinfo;
        unsigned char *igmp_head = igmp_clientinfo->igmphdr;
        
	switch (igmp_clientinfo->igmp_msg_type) 
	{
	case IGMP_V1_MEMBERSHIP_REPORT:
		igmp_clientinfo->group_num = 1;
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip, igmp_head+4, sizeof(struct in_addr));
		igmp_group->msg_type = IGMP_V1_MEMBERSHIP_REPORT;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;
	case IGMP_MEMBERSHIP_QUERY:
                igmp_clientinfo->igmp_version = IGMP_V1_BIT;      // defaults to v1 query
		igmp_clientinfo->max_response_time = *(igmp_head+1);
                // v2 query has max response time value
                if (igmp_clientinfo->max_response_time) igmp_clientinfo->igmp_version = IGMP_V2_BIT;
                        
                // deal with v3 query, at least 4 bytes longer than v1/v2
                        
                if (total_length - ip_head_length > 8) // source query is longer than 12 bytes
                {
                        igmp_clientinfo->igmp_version = IGMP_V3_BIT;
                        source_num = ntohs(*(unsigned short *)(igmp_head + 10));
                        j = 0;
                        do
			{
                                igmp_group = malloc_safe(sizeof(struct igmp_group_t));
                                memset(igmp_group, 0, sizeof(struct igmp_group_t));
                                INIT_LIST_NODE(&igmp_group->entry_node);
                                memcpy(&igmp_group->group_ip, igmp_head+4, sizeof(struct in_addr));
                                igmp_group->msg_type = IGMP_MEMBERSHIP_QUERY;
                                if (source_num)
                                {
                                        current_source_ptr = (igmp_head + 12) + 4 * j;
                                        memcpy(&igmp_group->src_ip, current_source_ptr, sizeof(struct in_addr));
                                }
                                list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
                                igmp_clientinfo->group_num++;
                                j++;                                        
                        } while (j < source_num);

                }
                else
        	{
                        igmp_clientinfo->group_num = 1;
                        igmp_group = malloc_safe(sizeof(struct igmp_group_t));
                        INIT_LIST_NODE(&igmp_group->entry_node);
                        memcpy(&igmp_group->group_ip, igmp_head+4, sizeof(struct in_addr));
                        igmp_group->msg_type = IGMP_MEMBERSHIP_QUERY;
                        list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
                }
		break;
	case IGMP_V2_MEMBERSHIP_REPORT:
		dst_addr.s_addr = inet_addr("224.0.0.2");
		if (memcmp(&dst_addr,igmp_head+4, sizeof(struct in_addr)) == 0)
		{
			dbprintf_igmp(LOG_WARNING, "invalid group ip 224.0.0.2\n");
			return -1;
		}

		igmp_clientinfo->group_num = 1;
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip, igmp_head+4, sizeof(struct in_addr)) ;
		memset(&igmp_group->src_ip, 0, sizeof(struct in_addr));
		igmp_group->msg_type = IGMP_V2_MEMBERSHIP_REPORT;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;	
	case IGMP_V3_MEMBERSHIP_REPORT:
		group_num = ntohs(*(unsigned short *)(igmp_head + 6));
		igmp_clientinfo->group_num = group_num;
		current_group_ptr = igmp_head + 8;
		payload = pktinfo->frame_len - (igmp_head - pktinfo->frame_ptr);
		offset = 0;
			
		for (i = 0; i < group_num ; i++)
		{
			current_source_ptr = current_group_ptr + 8;
			source_num = ntohs(*(unsigned short *)(current_group_ptr + 2));
			offset += (8 + source_num*4); // Type(1) + Aux(1) + NumSrc(2) + McastAddr(4) + SrcAddr(4*n)

			j=0;
			do
			{
				igmp_group = malloc_safe(sizeof(struct igmp_group_t));
				INIT_LIST_NODE(&igmp_group->entry_node);

				memcpy(&igmp_group->group_ip, current_group_ptr+4, sizeof(struct in_addr));
				if (igmp_group->group_ip.ipv4.s_addr==0)
				{
					free_safe(igmp_group);
					break;
				}
				if (source_num) memcpy(&igmp_group->src_ip, current_source_ptr, sizeof(struct in_addr));
				igmp_group->msg_type = IGMP_V3_MEMBERSHIP_REPORT;
				igmp_group->record_type = *current_group_ptr;

				list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
				if (source_num) current_source_ptr += 4;
				j++;
			} while (j < source_num && offset < payload);

			current_group_ptr = current_source_ptr + *(current_group_ptr+1)*4;
		}
		break;
	case IGMP_V2_LEAVE_GROUP:
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		igmp_clientinfo->group_num = 1;
		memcpy(&igmp_group->group_ip, igmp_head+4, sizeof(struct in_addr));
		igmp_group->msg_type = IGMP_V2_LEAVE_GROUP;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;

	case MLD_LISTENER_QUERY:
		igmp_clientinfo->group_num = 1;
		igmp_clientinfo->max_response_time = *((unsigned short *)(igmp_head+4)); // two bytes in MLD
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip.ipv6, igmp_head+8, sizeof(struct in6_addr));
		igmp_group->msg_type = MLD_LISTENER_QUERY;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;
	case MLD_LISTENER_REPORT:
		igmp_clientinfo->group_num = 1;
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip.ipv6, igmp_head+8, sizeof(struct in6_addr)) ;
		memset(&igmp_group->src_ip.ipv6, 0, sizeof(struct in6_addr));
		igmp_group->msg_type = MLD_LISTENER_REPORT;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;
	case MLD_LISTENER_REDUCTION:
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip.ipv6, igmp_head+8, sizeof(struct in6_addr));
		igmp_group->msg_type = MLD_LISTENER_REDUCTION;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;
	case MLD_V2_LISTENER_REPORT:
		group_num = ntohs(*(unsigned short *)(igmp_head + 6));
		igmp_clientinfo->group_num = group_num;
		current_group_ptr = igmp_head + 8;
		payload = igmp_clientinfo->pktinfo->frame_len - ip_head_length;
		offset = 0;
			
		for (i = 0; i < group_num ; i++)
		{
			current_source_ptr = current_group_ptr + 20;
			source_num = ntohs(*(unsigned short *)(current_group_ptr + 2));
			offset += (20 + source_num*16 + *(current_group_ptr + 1) * 4); // Type(1) + Aux(1) + NumSrc(2) + McastAddr(16) + SrcAddr(16*n)
				
			j=0;
			do
			{
				igmp_group = malloc_safe(sizeof(struct igmp_group_t));
				INIT_LIST_NODE(&igmp_group->entry_node);

				memcpy(&igmp_group->group_ip.ipv6, current_group_ptr+4, sizeof(struct in6_addr));
				if (igmp_group->group_ip.ipv6.s6_addr32[0]==0 && igmp_group->group_ip.ipv6.s6_addr32[1]==0 && igmp_group->group_ip.ipv6.s6_addr32[2]==0 && igmp_group->group_ip.ipv6.s6_addr32[3]==0)
				{
					free_safe(igmp_group);
					break;
				}
				
				if (source_num) memcpy(&igmp_group->src_ip.ipv6, current_source_ptr, sizeof(struct in6_addr));
				igmp_group->msg_type = MLD_V2_LISTENER_REPORT;
				igmp_group->record_type = *current_group_ptr;

				list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
				if (source_num) current_source_ptr += 16;
				j++;
				} while(j < source_num && offset < payload);

				current_group_ptr = current_source_ptr + *(current_group_ptr+1)*4;
		}
		break;

	default:
		dbprintf_igmp(LOG_WARNING, "unknown igmp msg type (%d)\n", igmp_clientinfo->igmp_msg_type);
		return -1;
	}
	
	return 0;
}

int
igmp_pkt_parse_clientinfo_router_mode(struct igmp_clientinfo_t *igmp_clientinfo)
{
	unsigned char *igmp_head = NULL;
	int ip_head_length, total_length;
	unsigned int payload = 0;

	struct cpuport_info_t *pktinfo = igmp_clientinfo->pktinfo;
	memcpy(&igmp_clientinfo->rx_desc , &pktinfo->rx_desc, sizeof(struct cpuport_desc_t));

	ip_head_length = (*(pktinfo->frame_ptr) & 0x0f) * 4;
        total_length = ntohs(*(unsigned short *)(pktinfo->nh + 2));
	igmp_head = pktinfo->frame_ptr + ip_head_length; 
	igmp_clientinfo->igmp_msg_type = *(pktinfo->frame_ptr+ ip_head_length);
	memcpy(&igmp_clientinfo->client_ip,pktinfo->frame_ptr+12,sizeof(struct in_addr));

	igmp_clientinfo->vid = 0;
	
	igmp_clientinfo->access_time = igmp_timer_get_cur_time();
		
        payload = igmp_clientinfo->pktinfo->frame_len-(2+ip_head_length);
        
        igmp_clientinfo->igmphdr = igmp_head;
        
	return igmp_pkt_build_igmp_group(igmp_clientinfo, &total_length, &ip_head_length, &payload);
}

int
igmp_pkt_parse_clientinfo(struct igmp_clientinfo_t *igmp_clientinfo)
{
	unsigned char *igmp_head = NULL;
	int ip_head_length, total_length;
	unsigned int payload = 0;
	int tag_num = -1;
	int no_igmp_profile_me = 0;
        
        struct cpuport_info_t *pktinfo = igmp_clientinfo->pktinfo;
	memcpy(&igmp_clientinfo->rx_desc , &pktinfo->rx_desc, sizeof(struct cpuport_desc_t));

	tag_num = igmp_pkt_tag_num_get(pktinfo);

	if (pktinfo->tci.ethertype == ETH_P_IP ||	// 0x800
	    (pktinfo->tci.ethertype == ETH_P_PPP_SES && pktinfo->pppoe_proto == PPP_IP)) // 0x8864, ppp sub protocol 0x21
	{
		dbprintf_igmp(LOG_WARNING, "recv %s igmp packet\n", (pktinfo->tci.ethertype == ETH_P_PPP_SES)?"ppp_ip":"ipv4");
		ip_head_length = (*(char*)pktinfo->nh & 0x0f) * 4;
                total_length = ntohs(*(unsigned short *)(pktinfo->nh + 2));
		igmp_head = pktinfo->nh + ip_head_length; 
		igmp_clientinfo->igmp_msg_type = *(igmp_head);
		memcpy(&igmp_clientinfo->client_ip, pktinfo->nh+12,sizeof(struct in_addr));	// 12 is offset from ipv4 hdr to sipv4

	}
	else if (pktinfo->tci.ethertype == ETH_P_IPV6 ||	// 0x86dd
		   (pktinfo->tci.ethertype == ETH_P_PPP_SES && pktinfo->pppoe_proto == PPP_IPV6)) 	// 0x8864, ppp sub protocol 0x57
	{
		dbprintf_igmp(LOG_WARNING,"recv %s igmp packet\n", (pktinfo->tci.ethertype == ETH_P_PPP_SES)?"ppp_ipv6":"ipv6");
		ip_head_length = igmp_pkt_parse_mld_type(pktinfo->nh, igmp_clientinfo);
		if (ip_head_length == CPUPORT_UNMATCH) return -1;
		igmp_head = pktinfo->nh + ip_head_length; 
		memcpy(&igmp_clientinfo->client_ip , pktinfo->nh + 8 , sizeof(struct in6_addr));	// 8 is offset from ipv6 hdr to sipv6
	}
	else
	{
		dbprintf_igmp(LOG_WARNING,"recv non ipv4/ipv6/ppp_ipv4/ppp_ipv6 packet!\n");
		return -1;
	}

	if (pktinfo->rx_desc.logical_port_id < 6 && omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU && alu_olt_ethertype != 0) 
		igmp_clientinfo->vid = alu_olt_ethertype;
	else if (pktinfo->tci.otci.tpid != 0)
		igmp_clientinfo->vid = pktinfo->tci.otci.vid;
	else if (pktinfo->tci.itci.tpid != 0)
		igmp_clientinfo->vid = pktinfo->tci.itci.vid;
	else
		igmp_clientinfo->vid = 0;	

        if (pktinfo->rx_desc.logical_port_id != 5)
	{
		no_igmp_profile_me = igmp_omci_read_igmp_profile_me(&igmp_config_g, pktinfo->rx_desc.bridge_port_me, igmp_clientinfo->vid);
		if (no_igmp_profile_me < 0 && omci_env_g.igmp_mcast_me_support)
		{
			dbprintf(LOG_WARNING,"igmp profile not found\n");
			return -1;
		}
	}
	    
	memcpy(igmp_clientinfo->client_mac, pktinfo->frame_ptr+CPUPORT_MAC_ADDR_LEN, CPUPORT_MAC_ADDR_LEN);

	igmp_clientinfo->access_time = igmp_timer_get_cur_time();
	
	payload = pktinfo->frame_len - (igmp_head - pktinfo->frame_ptr);
	
	igmp_clientinfo->igmphdr = igmp_head;
	
	return igmp_pkt_build_igmp_group(igmp_clientinfo, &total_length, &ip_head_length, &payload);
}

