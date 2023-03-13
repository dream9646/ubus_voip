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
 * File    : preassign.c
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
#include "igmp_mbship.h"
#include "mcast_switch.h"
#include "mcast_const.h"

static unsigned char macaddr_zero[6] = {0,0,0,0,0,0};

static int ip_to_mac( union ipaddr *ip, unsigned char mac[6])
{
	if (IS_IPV6_ADDR(*ip))
	{
		mac[0] = 0x33;
		mac[1] = 0x33;

		mac[2] = ip->ipv6.s6_addr[12];
		mac[3] = ip->ipv6.s6_addr[13];
		mac[4] = ip->ipv6.s6_addr[14];
		mac[5] = ip->ipv6.s6_addr[15];
	}
	else
	{
		mac[0] = 0x01;
		mac[1] = 0x00;
		mac[2] = 0x5e;

		mac[3] = (ip->ipv4.s_addr & 0x7f0000)>>16;
		mac[4] = (ip->ipv4.s_addr & 0xff00)>>8;
		mac[5] = ip->ipv4.s_addr & 0xff;
	}
	return 0;
}

static int
igmp_switch_mac_preassigned_filter_match(union ipaddr *ip)
{
	unsigned char i, j = 0;
	unsigned char mcast_entry_mac[6];

	ip_to_mac(ip, mcast_entry_mac);

	if (IS_IPV6_ADDR(*ip))
	{	// IPv6 mcast could be matched by L2 cast only
		for (i = 0; i < ENV_SIZE_IGMP_PREASSIGN_LIST; i++)
		{
			if (memcmp(omci_env_g.igmp_preassign_list[i].mac, macaddr_zero, 6) != 0)
			{
				for (j = 0; j < 6; j++)
				{
					if ((omci_env_g.igmp_preassign_list[i].mac[j] & omci_env_g.igmp_preassign_list[i].mask[j]) == (mcast_entry_mac[j] & omci_env_g.igmp_preassign_list[i].mask[j]))
					    	return 1;
				}
			}
		}
	}
	else
	{	// IPv4 mcast could be matched by L2 or IPv4 mcast 
		for (i = 0; i < ENV_SIZE_IGMP_PREASSIGN_LIST; i++)
		{
			if (memcmp(omci_env_g.igmp_preassign_list[i].mac, macaddr_zero, 6) != 0)
			{
				for (j = 0; j < 6; j++)
				{
					if ((omci_env_g.igmp_preassign_list[i].mac[j] & omci_env_g.igmp_preassign_list[i].mask[j]) == (mcast_entry_mac[j] & omci_env_g.igmp_preassign_list[i].mask[j]))
					    	return 1;
				}
			}
			else if (omci_env_g.igmp_preassign_list[i].ipaddr != 0)
			{
				unsigned int ipaddr = htonl(omci_env_g.igmp_preassign_list[i].ipaddr);
				unsigned int mask = htonl(omci_env_g.igmp_preassign_list[i].ipaddr_mask);
				if ((ipaddr & mask) == (ip->ipv4.s_addr & mask))
					return 1;
			}
		}
	}
	return 0;
}

static int
igmp_switch_preassign_init(void)
{
	struct switch_mac_tab_entry_t igmp_mac_tab_entry;
	unsigned char i, j;

	for (i = 0; i < ENV_SIZE_IGMP_PREASSIGN_LIST; i++)
	{
		if (omci_env_g.igmp_preassign_list[i].ipaddr != 0)
		{	// add ipv4 
			memset(&igmp_mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
			igmp_mac_tab_entry.port_bitmap = switch_get_all_logical_portmask_without_cpuext();
			igmp_mac_tab_entry.vid = 0;
			igmp_mac_tab_entry.ipmcast.src_ip.ipv4.s_addr = 0;
			igmp_mac_tab_entry.ipmcast.dst_ip.ipv4.s_addr = htonl(omci_env_g.igmp_preassign_list[i].ipaddr);
			igmp_switch_mcast_entry_add(&igmp_mac_tab_entry);

		}
		else if (memcmp(omci_env_g.igmp_preassign_list[i].mac, macaddr_zero, 6) != 0)
		{	// add L2 (for ipv6 or ipv4)
			memset(&igmp_mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
			igmp_mac_tab_entry.port_bitmap = switch_get_all_logical_portmask_without_cpuext();
			igmp_mac_tab_entry.vid = 0;
			for (j = 0; j < 6; j++)
			{
				igmp_mac_tab_entry.mac_mask[j] = 0xff;
				igmp_mac_tab_entry.mac_addr[j] = omci_env_g.igmp_preassign_list[i].mac[j];
			}

			switch_hw_g.mac_entry_add(&igmp_mac_tab_entry, 1);
		}
	}
	
	return 0;
}

static int
igmp_switch_preassign_deinit(void)
{
	struct switch_mac_tab_entry_t igmp_mac_tab_entry;
	int i;
	for (i = 0; i < ENV_SIZE_IGMP_PREASSIGN_LIST; i++)
	{
		if(omci_env_g.igmp_preassign_list[i].ipaddr != 0)
		{
			igmp_mac_tab_entry.ipmcast.dst_ip.ipv4.s_addr = htonl(omci_env_g.igmp_preassign_list[i].ipaddr);
			igmp_switch_mcast_entry_del(&igmp_mac_tab_entry);

		}
		else if (memcmp(omci_env_g.igmp_preassign_list[i].mac, macaddr_zero, 6) != 0)
		{
			memset(&igmp_mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
			switch_hw_g.mac_entry_del(&igmp_mac_tab_entry);
		}
	}
	return 0;
}

static int
igmp_filter_preassign_filter(struct igmp_grprec_t *r)
{
        MCAST_FILTER_NO_IPV6(r)
        	
	if (igmp_switch_mac_preassigned_filter_match(&r->rec->group_ip))
		r->flag |= 1 << NO_IGMP_SWITCH_FORWARD;
	
	return MCAST_FILTER_CONTINUE;
}

// module callbacks
static const char * igmp_filter_preassign_name(void)
{
	return "preassign";
}

static int igmp_filter_preassign_init(void)
{	
	igmp_switch_preassign_init();
	
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_preassign_deinit(void)
{
	igmp_switch_preassign_deinit();
	
	return MCAST_FILTER_CONTINUE;
}

struct igmp_filter_module_t preassign_filter =
{
        .priority = 10,
	.filter_name = igmp_filter_preassign_name,
        .init = igmp_filter_preassign_init,
        .deinit = igmp_filter_preassign_deinit,
        .filter = igmp_filter_preassign_filter
};


          

