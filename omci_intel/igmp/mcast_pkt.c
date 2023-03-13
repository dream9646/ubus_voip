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
#include "cpuport_util.h"
#include "mcast_timer.h"
#include "platform.h"



int
mcast_pkt_ip_cmp(union ipaddr *src_ip, union ipaddr *dst_ip)
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
mcast_pkt_ip_cpy(union ipaddr *dst_ip, union ipaddr *src_ip)
{
	int i;
	for (i = 0 ; i < sizeof(union ipaddr) ; i++)
		*((unsigned char *)dst_ip + i) = *((unsigned char *)src_ip + i);

	return 0;
}

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
		if (pktinfo->tci.ethertype == ETH_P_IPV6)
		{
			offset = 0;	// raw socket does not give ipv6 header
			iphead = pktinfo->frame_ptr;
			goto MLD_CHECK;
		}
		else
			return -1;
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

	MLD_CHECK:	
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
		if (pktinfo->tci.ethertype == ETH_P_IPV6)
		{
			offset = 0;	// raw socket does not give ipv6 header
			iphead = pktinfo->frame_ptr;
			goto MLD_CHECK;
		}
		else
			return -1;
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

	MLD_CHECK:	
	// Inspect MLD type
	if ((*(iphead + offset) == MLD_LISTENER_QUERY))
		return CPUPORT_MATCH;
	
	return CPUPORT_UNMATCH;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
static int
mcast_pkt_parse_mld_type(unsigned char *ipv6_ptr, struct igmp_clientinfo_t *igmp_clientinfo)
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

int
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

int
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
	char *msg = NULL;

	switch (igmp_clientinfo->igmp_msg_type) 
	{
	case IGMP_V1_MEMBERSHIP_REPORT:
		msg="IGMPv1 Report";
		igmp_clientinfo->group_num = 1;
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip, igmp_head+4, sizeof(struct in_addr));
		igmp_group->msg_type = IGMP_V1_MEMBERSHIP_REPORT;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;
	case IGMP_MEMBERSHIP_QUERY:
		msg="IGMPv1 Query";
                igmp_clientinfo->igmp_version = IGMP_V1_BIT;      // defaults to v1 query
		igmp_clientinfo->max_response_time = *(igmp_head+1);
                // v2 query has max response time value
                if (igmp_clientinfo->max_response_time)
		{
			igmp_clientinfo->igmp_version = IGMP_V2_BIT;
			msg="IGMPv2 Query";
		}
                        
                // deal with v3 query, at least 4 bytes longer than v1/v2
                        
                if (total_length - ip_head_length > 8) // source query is longer than 12 bytes
                {
                        igmp_clientinfo->igmp_version = IGMP_V3_BIT;
                        msg="IGMPv3 Query";
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
                msg="IGMPv2 Report";
		igmp_clientinfo->group_num = 1;
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip, igmp_head+4, sizeof(struct in_addr)) ;
		memset(&igmp_group->src_ip, 0, sizeof(struct in_addr));
		igmp_group->msg_type = IGMP_V2_MEMBERSHIP_REPORT;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;	
	case IGMP_V3_MEMBERSHIP_REPORT:
		msg="IGMPv3 Report";
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
		msg="IGMPv2 Leave";
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		igmp_clientinfo->group_num = 1;
		memcpy(&igmp_group->group_ip, igmp_head+4, sizeof(struct in_addr));
		igmp_group->msg_type = IGMP_V2_LEAVE_GROUP;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;
	case MLD_LISTENER_QUERY:
		msg="MLD Query";
		igmp_clientinfo->group_num = 1;
		igmp_clientinfo->max_response_time = *((unsigned short *)(igmp_head+4)); // two bytes in MLD
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip.ipv6, igmp_head+8, sizeof(struct in6_addr));
		igmp_group->msg_type = MLD_LISTENER_QUERY;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;
	case MLD_LISTENER_REPORT:
		msg="MLDv1 Report";
		igmp_clientinfo->group_num = 1;
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip.ipv6, igmp_head+8, sizeof(struct in6_addr)) ;
		memset(&igmp_group->src_ip.ipv6, 0, sizeof(struct in6_addr));
		igmp_group->msg_type = MLD_LISTENER_REPORT;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;
	case MLD_LISTENER_REDUCTION:
		msg="MLDv1 Done";
		igmp_group = malloc_safe(sizeof(struct igmp_group_t));
		INIT_LIST_NODE(&igmp_group->entry_node);
		memcpy(&igmp_group->group_ip.ipv6, igmp_head+8, sizeof(struct in6_addr));
		igmp_group->msg_type = MLD_LISTENER_REDUCTION;
		list_add(&igmp_group->entry_node, &igmp_clientinfo->group_list);
		break;
	case MLD_V2_LISTENER_REPORT:
		msg="MLDv2 Report";
		group_num = ntohs(*(unsigned short *)(igmp_head + 6));
		igmp_clientinfo->group_num = group_num;
		current_group_ptr = igmp_head + 8;
		payload = igmp_clientinfo->pktinfo->frame_len - ip_head_length;
		offset = 0;
			
		for (i = 0; i < group_num; i++)
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
	
	dbprintf_igmp(LOG_WARNING, "recv %s(%d) packet\n", msg, igmp_clientinfo->igmp_msg_type);
	
	return 0;
}

int
mcast_pkt_parse_clientinfo_router_mode(struct igmp_clientinfo_t *igmp_clientinfo)
{
	unsigned char *igmp_head = NULL;
	int ip_head_length, total_length;
	unsigned int payload = 0;

	struct cpuport_info_t *pktinfo = igmp_clientinfo->pktinfo;
	memcpy(&igmp_clientinfo->rx_desc , &pktinfo->rx_desc, sizeof(struct cpuport_desc_t));

	if (pktinfo->tci.ethertype == ETH_P_IP)
	{
		if (*(unsigned char *)(pktinfo->nh + 9) != 0x2)
		{
			dbprintf_igmp(LOG_INFO, "discard wrong protocol number:0x%x\n", *(unsigned char *)(pktinfo->nh + 9));
			return -1;	
		}
		
		dbprintf_igmp(LOG_INFO, "recv %s igmp packet\n", (pktinfo->tci.ethertype == ETH_P_PPP_SES)?"ppp_ip":"ipv4");
		ip_head_length = (*(pktinfo->frame_ptr) & 0x0f) * 4;
	        total_length = ntohs(*(unsigned short *)(pktinfo->nh + 2));
		igmp_head = pktinfo->frame_ptr + ip_head_length; 
		igmp_clientinfo->igmp_msg_type = *(pktinfo->frame_ptr+ ip_head_length);
		memcpy(&igmp_clientinfo->client_ip,pktinfo->frame_ptr+12,sizeof(struct in_addr));
		payload = igmp_clientinfo->pktinfo->frame_len-(2+ip_head_length);
	}
	else if (pktinfo->tci.ethertype == ETH_P_IPV6)
	{
		dbprintf_igmp(LOG_INFO,"recv %s igmp packet\n", (pktinfo->tci.ethertype == ETH_P_PPP_SES)?"ppp_ipv6":"ipv6");
		struct in6_addr *ip6 = (struct in6_addr *)(pktinfo->frame_ptr + pktinfo->frame_len);
		igmp_clientinfo->client_ip.ipv6 = *ip6;
		igmp_clientinfo->dst_ip.ipv6 = *(ip6 + 1);
		igmp_clientinfo->igmp_msg_type = *(pktinfo->frame_ptr);
		igmp_head = pktinfo->frame_ptr;			
	}
	else
	{
		dbprintf_igmp(LOG_WARNING,"recv non ipv4/ipv6/ppp_ipv4/ppp_ipv6 packet!\n");
		return -1;
	}
	


	igmp_clientinfo->vid = 0;
        
	igmp_clientinfo->access_time = igmp_timer_get_cur_time();
	
        igmp_clientinfo->igmphdr = igmp_head;
        
	return igmp_pkt_build_igmp_group(igmp_clientinfo, &total_length, &ip_head_length, &payload);
}

int
mcast_pkt_parse_clientinfo(struct igmp_clientinfo_t *igmp_clientinfo)
{
	unsigned char *igmp_head = NULL;
	int ip_head_length, total_length;
	unsigned int payload = 0;
	int MAYBE_UNUSED tag_num = -1;
        
        struct cpuport_info_t *pktinfo = igmp_clientinfo->pktinfo;
	memcpy(&igmp_clientinfo->rx_desc , &pktinfo->rx_desc, sizeof(struct cpuport_desc_t));

	tag_num = igmp_pkt_tag_num_get(pktinfo);
	
	if (pktinfo->tci.ethertype == ETH_P_IP ||	// 0x800
	    (pktinfo->tci.ethertype == ETH_P_PPP_SES && pktinfo->pppoe_proto == PPP_IP)) // 0x8864, ppp sub protocol 0x21
	{
		if (*(unsigned char *)(pktinfo->nh + 9) != 0x2)	// must be 0x2
		{
			dbprintf_igmp(LOG_INFO, "discard wrong protocol number:0x%x\n", *(unsigned char *)(pktinfo->nh + 9));
			return -1;	
		}
		
		dbprintf_igmp(LOG_INFO, "recv %s igmp packet\n", (pktinfo->tci.ethertype == ETH_P_PPP_SES)?"ppp_ip":"ipv4");
		ip_head_length = (*(char*)pktinfo->nh & 0x0f) * 4;
                total_length = ntohs(*(unsigned short *)(pktinfo->nh + 2));
		igmp_head = pktinfo->nh + ip_head_length; 
		igmp_clientinfo->igmp_msg_type = *(igmp_head);
		memcpy(&igmp_clientinfo->client_ip, pktinfo->nh+12,sizeof(struct in_addr));	// 12 is offset from ipv4 hdr to sipv4
		memcpy(&igmp_clientinfo->dst_ip, pktinfo->nh+16,sizeof(struct in_addr));	// 12 is offset from ipv4 hdr to sipv4

	}
	else if (pktinfo->tci.ethertype == ETH_P_IPV6 ||	// 0x86dd
		   (pktinfo->tci.ethertype == ETH_P_PPP_SES && pktinfo->pppoe_proto == PPP_IPV6)) 	// 0x8864, ppp sub protocol 0x57
	{
		dbprintf_igmp(LOG_INFO,"recv %s igmp packet\n", (pktinfo->tci.ethertype == ETH_P_PPP_SES)?"ppp_ipv6":"ipv6");
		ip_head_length = mcast_pkt_parse_mld_type(pktinfo->nh, igmp_clientinfo);
		if (ip_head_length == CPUPORT_UNMATCH) return -1;
		igmp_head = pktinfo->nh + ip_head_length; 
		memcpy(&igmp_clientinfo->client_ip , pktinfo->nh + 8 , sizeof(struct in6_addr));	// 8 is offset from ipv6 hdr to sipv6
	}
	else
	{
		dbprintf_igmp(LOG_WARNING,"recv non ipv4/ipv6/ppp_ipv4/ppp_ipv6 packet!\n");
		return -1;
	}
	
	if (pktinfo->tci.otci.tpid != 0)
		igmp_clientinfo->vid = pktinfo->tci.otci.vid;
	else if (pktinfo->tci.itci.tpid != 0)
		igmp_clientinfo->vid = pktinfo->tci.itci.vid;
	else
		igmp_clientinfo->vid = 0;
        
	memcpy(igmp_clientinfo->client_mac, pktinfo->frame_ptr+CPUPORT_MAC_ADDR_LEN, CPUPORT_MAC_ADDR_LEN);

	igmp_clientinfo->access_time = igmp_timer_get_cur_time();
	
	payload = pktinfo->frame_len - (igmp_head - pktinfo->frame_ptr);
	
	igmp_clientinfo->igmphdr = igmp_head;
	
	return igmp_pkt_build_igmp_group(igmp_clientinfo, &total_length, &ip_head_length, &payload);
}

