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
 * File    : alu_p2p.c
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
#include "platform.h"

#define P2P_UNI_PORT	0x0
#define P2P_UNKNOWN	0xff

static short p2p_cfg_g = 0;

static int
igmp_filter_alu_p2p_raw_filter(struct cpuport_info_t *pktinfo)
{
	if (!omci_env_g.port2port_enable) return MCAST_FILTER_CONTINUE;
	if (THREAD_ENV.igmp_proxy_snooping) return MCAST_FILTER_CONTINUE; // only applicable in snooping mode
	
	if (pktinfo->rx_desc.bridge_port_me != NULL && pktinfo->rx_desc.router_mode == 0)
	{
		if (pktinfo->rx_desc.logical_port_id != 5 && pktinfo->rx_desc.logical_port_id != 6)
		{
			struct switch_mac_tab_entry_t mac_tab_entry;

                        pktinfo->rx_desc.p2p_cfg = P2P_UNKNOWN;
			
			memset(&mac_tab_entry,0,sizeof(struct switch_mac_tab_entry_t ));
			mac_tab_entry.vid = 0;
			memcpy(&mac_tab_entry.mac_addr, pktinfo->frame_ptr+6, 6);
                	switch_hw_g.mac_entry_get(&mac_tab_entry);
			
			int i = 0;
			for (i = 0 ; i< 12; i++)
			{
                                if (mac_tab_entry.port_bitmap & (1 << i ))
				{
					pktinfo->rx_desc.p2p_cfg = i;
					p2p_cfg_g = i;
					break;
				}
                        }

			if(pktinfo->rx_desc.p2p_cfg == 5 )//pon port
			{
				pktinfo->rx_desc.p2p_cfg = pktinfo->rx_desc.logical_port_id;
				dbprintf_igmp(LOG_WARNING, "igmp p2p enable in pon port skip to (%d)!\n"
				      ,pktinfo->rx_desc.p2p_cfg );
				return MCAST_FILTER_CONTINUE;
			}
			
                        struct switch_port_info_t port_info;
                                                
			if (pktinfo->rx_desc.p2p_cfg == 6) pktinfo->rx_desc.p2p_cfg = 8;
                        pktinfo->rx_desc.logical_port_id = P2P_UNI_PORT;
                        switch_get_port_info_by_logical_port_id(pktinfo->rx_desc.logical_port_id , &port_info);
			pktinfo->rx_desc.bridge_port_me = port_info.bridge_port_me;

                        if (!pktinfo->rx_desc.bridge_port_me)
			{
                                dbprintf_igmp(LOG_WARNING, "P2P Fail to get bridge port descriptor!\n");
				return MCAST_FILTER_STOP;
			}							
		}
	}
	
	return MCAST_FILTER_CONTINUE;
}

static int
igmp_filter_alu_p2p_pre_filter(struct igmp_clientinfo_t *igmp_clientinfo)
{
	MCAST_FILTER_NO_IPV6(igmp_clientinfo)
	if (!omci_env_g.port2port_enable) return MCAST_FILTER_CONTINUE;
	if (THREAD_ENV.igmp_proxy_snooping) return MCAST_FILTER_CONTINUE; // only applicable in snooping mode
	
	if (igmp_clientinfo->pktinfo->rx_desc.bridge_port_me != NULL && igmp_clientinfo->pktinfo->rx_desc.router_mode == 0)
	{
		if (igmp_clientinfo->pktinfo->rx_desc.logical_port_id != 5 && igmp_clientinfo->pktinfo->rx_desc.logical_port_id != 6)
		{
			igmp_clientinfo->p2p_cfg_g = p2p_cfg_g;
		}	
	}
		
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_alu_p2p_forward_modify(struct mcast_mbship_t *h)
{
	
	if (omci_env_g.port2port_enable)
	{
		if (THREAD_ENV.igmp_proxy_snooping == 0)
		{
			unsigned char old = h->hw_port_mask;
#if 1 // Workaround for source ip wasn't leant in l2-table
			if (h->p2p_cfg == 0xff)
				h->hw_port_mask = switch_get_uni_logical_portmask(); // invalid source port, flooding to all LAN ports
			else
#endif
				h->hw_port_mask = h->p2p_cfg;
			
			if (old != h->hw_port_mask)
				dbprintf_igmp(LOG_WARNING, "update %x to gip %x @ %d to %d\n", h->client_ip.ipv4.s_addr, h->group_ip.ipv4.s_addr, old, h->hw_port_mask);
			
			if (h->p2p_cfg != 0xff) h->hw_port_mask = 1 << h->hw_port_mask;
		}
	}
			
	return MCAST_FILTER_CONTINUE;
}

// module callbacks
static const char * igmp_filter_alu_p2p_name(void)
{
	return "alu_p2p";
}

static int igmp_filter_alu_p2p_init(void)
{	
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_alu_p2p_deinit(void)
{
	return MCAST_FILTER_CONTINUE;
}

struct igmp_filter_module_t alu_p2p_filter =
{
        .priority = 10,
	.filter_name = igmp_filter_alu_p2p_name,
        .init = igmp_filter_alu_p2p_init,
        .deinit = igmp_filter_alu_p2p_deinit,
        .raw_filter = igmp_filter_alu_p2p_raw_filter,
        .pre_filter = igmp_filter_alu_p2p_pre_filter,
        .forward = igmp_filter_alu_p2p_forward_modify
};


          

