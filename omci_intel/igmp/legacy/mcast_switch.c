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
 * File    : mcast_switch.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "list.h"
#include "util.h"

#include "mcast_const.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "cpuport_util.h"
#include "switch.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "mcast.h"
#include "meinfo.h"
#include "switch.h"
#include "mcast_switch.h"
#include "mcast_timer.h"
#include "hwresource.h"
#include "igmp_omci.h"
#include "util_inet.h"

struct list_head ipv6_mac_member_list;

static unsigned char macaddr_zero[6] = {0,0,0,0,0,0};

static int 
igmp_switch_ip_to_mac( union ipaddr *ip, unsigned char mac[6])
{
	if (IS_IPV6_ADDR(*ip)) {
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

	igmp_switch_ip_to_mac(ip, mcast_entry_mac);

	if (IS_IPV6_ADDR(*ip))
	{	// IPv6 mcast could be matched by L2 cast only
		for (i = 0; i < ENV_SIZE_IGMP_PREASSIGN_LIST; i++)
		{
			if (memcmp(omci_env_g.igmp_preassign_list[i].mac, macaddr_zero, 6)!=0)
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
			if (memcmp(omci_env_g.igmp_preassign_list[i].mac, macaddr_zero, 6)!=0)
			{
				for (j = 0; j < 6; j++)
				{
					if ((omci_env_g.igmp_preassign_list[i].mac[j] & omci_env_g.igmp_preassign_list[i].mask[j]) == (mcast_entry_mac[j] & omci_env_g.igmp_preassign_list[i].mask[j]))
					    	return 1;
				}
			}
			else if (omci_env_g.igmp_preassign_list[i].ipaddr!=0)
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

int igmp_switch_preassign_list_exist(union ipaddr *ip)
{
	return igmp_switch_mac_preassigned_filter_match(ip);
}

int 
igmp_switch_igmp_extract_set(int enable)
{
	unsigned int i = 0;

	if (switch_hw_g.igmp_extract_enable_set == NULL)
		return -1;

	for (i=0 ; i< SWITCH_LOGICAL_PORT_TOTAL ; i++)
	{
		if (((1<<i) & switch_get_all_logical_portmask_without_cpuext()) == 0)
			continue;
		if (switch_hw_g.igmp_extract_enable_set(i, enable ) < 0)
			return -1;
	}
	return 0;
}

int 
igmp_switch_igmp_extract_get(unsigned int *enable)
{
	if (switch_hw_g.igmp_extract_enable_get == NULL)
		return -1;

	return switch_hw_g.igmp_extract_enable_get(0, enable);
}

int 
igmp_switch_mcast_entry_add(struct switch_mac_tab_entry_t *mac_tab_entry) 
{
	if (switch_hw_g.ipmcastaddr_add == NULL)
		return -1;

	if( switch_hw_g.ipmcastaddr_add(mac_tab_entry) < 0)
	{
		dbprintf_igmp(LOG_WARNING,"igmp multicast entry add err\n");
		return -1;
	}

	return 0;
}

int 
igmp_switch_mcast_entry_del(struct switch_mac_tab_entry_t *mac_tab_entry) 
{

	if (igmp_switch_mac_preassigned_filter_match( &mac_tab_entry->ipmcast.dst_ip ))
		return 0;

	if(switch_hw_g.ipmcastaddr_del == NULL)
		return -1;


	// check if entry hits static ACL
	if (igmp_omci_check_is_in_static_acl(&mac_tab_entry->ipmcast.dst_ip)) return 0;

	if( switch_hw_g.ipmcastaddr_del(mac_tab_entry) < 0)
	{
		dbprintf_igmp(LOG_WARNING,"igmp multicast entry del err\n");
		return -1;
	}

	return 0;
}


int 
igmp_switch_mcast_entry_update(struct switch_mac_tab_entry_t *mac_tab_entry) 
{
	if(switch_hw_g.ipmcastaddr_add == NULL)
		return -1;

	if (switch_hw_g.ipmcastaddr_add (mac_tab_entry) < 0)
		return -1;

	return 0;
}


int 
igmp_switch_mcast_entry_get(struct switch_mac_tab_entry_t *mac_tab_entry) 
{
	return switch_hw_g.ipmcastaddr_get(mac_tab_entry);
}




static int
igmp_switch_ipv6_mcast_pbmp(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_group_t *igmp_group)
{
	struct igmp_mbship_t *mb_pos=NULL, *mb_n=NULL;
	int pbmp = 0;

	list_for_each_entry_safe(mb_pos, mb_n, &igmp_mbship_db->mbship_list, entry_node) {
		if (mb_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
		if (igmp_group->group_ip.ipv6.s6_addr32[3] == mb_pos->group_ip.ipv6.s6_addr32[3])  //compare MAC address that transform from IPv6
			pbmp |= 1 << mb_pos->logical_port_id ;
	}
	return pbmp;
}

static struct ipv6_mac_member_t *
igmp_switch_ipv6_mac_search(struct list_head *ipv6_mac_member_list, struct igmp_group_t *igmp_group )
{

	struct ipv6_mac_member_t *pos, *n;
	list_for_each_entry_safe(pos, n, ipv6_mac_member_list, node) {
		if (igmp_group->group_ip.ipv6.s6_addr32[3] == pos->ipv6.s6_addr32[3])  //compare MAC address that transform from IPv6
			return pos;
	}
	return NULL;
}


static int
igmp_switch_ipv6_mac_member_del(struct list_head *ipv6_mac_member_list,struct igmp_group_t *igmp_group)
{
	struct ipv6_mac_member_t *pos, *n;
	list_for_each_entry_safe(pos, n, ipv6_mac_member_list, node) {
		if (igmp_group->group_ip.ipv6.s6_addr32[3] == pos->ipv6.s6_addr32[3])
		{  //compare MAC address that transform from IPv6
			list_del(&pos->node);
			free_safe(pos);
			return 0;
		}
	}
	return -1;
}

static int
igmp_switch_ipv6_mac_member_add(struct list_head *ipv6_mac_member_list,struct igmp_group_t *igmp_group)
{
	struct ipv6_mac_member_t *pos, *n, *new;
	unsigned char found = 0;

	list_for_each_entry_safe(pos, n, ipv6_mac_member_list, node) {
		if (igmp_group->group_ip.ipv6.s6_addr32[3] == pos->ipv6.s6_addr32[3])
		{  //compare MAC address that transform from IPv6
			found = 1;
			break;
		}
	}

	if (!found)
	{
		new = malloc_safe(sizeof(struct ipv6_mac_member_t));
		INIT_LIST_NODE(&new->node);
		memcpy(&(new->ipv6), &(igmp_group->group_ip.ipv6), sizeof(struct in6_addr));
		igmp_switch_ip_to_mac(&(igmp_group->group_ip), new->mac_addr);
		list_add(&new->node,ipv6_mac_member_list);
	}
	return 0;
}


int
igmp_switch_mac_table_client_update(struct igmp_clientinfo_t *igmp_clientinfo , struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_group_t *igmp_group)
{
	int pbmp =0;
	struct me_t *bridge_port_me,*ibridge_port_me;

	struct igmp_mbship_t *mb_pos=NULL, *mb_n=NULL;
	list_for_each_entry_safe(mb_pos, mb_n, &igmp_mbship_db->mbship_list, entry_node) {
		if (mb_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
		if (igmp_pkt_ip_cmp(&igmp_group->group_ip ,&mb_pos->group_ip))
			continue;

		//update switch
		//if ( mb_pos->vid != igmp_clientinfo->active_uni_vid)
		//	continue;
		if (omci_env_g.port2port_enable)
		{
			if (omci_env_g.igmp_proxy_snooping) 
				pbmp |= ( 1 << mb_pos->logical_port_id);
			else
			{

#if 1 // Workaround for source ip wasn't leant in l2-table
				if(mb_pos->p2p_cfg==0xff)
					pbmp = switch_get_uni_logical_portmask(); // invalid source port, flooding to all LAN ports
				else
#endif
					pbmp |= (1 << mb_pos->p2p_cfg);
			}
		}
		else
		{
			if ( omci_env_g.igmp_proxy_snooping == 0)
			{
				bridge_port_me = meinfo_me_get_by_meid(47,mb_pos->src_port_meid);
				ibridge_port_me = hwresource_public2private(bridge_port_me);
				if (meinfo_util_me_attr_data(ibridge_port_me,4) != 1) // now, only allow port type == uni
					continue;

				pbmp |= (1 << meinfo_util_me_attr_data(ibridge_port_me,6));
			}else
				pbmp |= (1 << mb_pos->logical_port_id);

		}
		if (omci_env_g.olt_alu_mcast_support && !alu_igmp_get_group_signal(igmp_mbship_db, &igmp_group->group_ip)) {
			pbmp = 0;
		}
	}

	struct switch_mac_tab_entry_t mac_entry;
	memset(&mac_entry,0,sizeof(struct switch_mac_tab_entry_t));

	if(!mb_pos->ani_vid && igmp_clientinfo && igmp_clientinfo->ani_vid) mb_pos->ani_vid = igmp_clientinfo->ani_vid;
	mac_entry.vid = mb_pos->ani_vid;
	igmp_pkt_ip_cpy(&mac_entry.ipmcast.dst_ip,&igmp_group->group_ip);

	char group_ip[INET6_ADDRSTRLEN];
	memset(group_ip,0,INET6_ADDRSTRLEN);
	if (IS_IPV6_ADDR(igmp_group->group_ip))
	{// IP multicast is replaced by L2 multicast in IPv6
		mac_entry.port_bitmap = igmp_switch_ipv6_mcast_pbmp(igmp_mbship_db, igmp_group);
		util_inet_ntop(AF_INET6, &mac_entry.ipmcast.dst_ip, (char *)group_ip, sizeof(group_ip));
	}
	else
	{
		mac_entry.port_bitmap = pbmp;
		util_inet_ntop(AF_INET, &mac_entry.ipmcast.dst_ip, (char *)group_ip, sizeof(group_ip));
	}

	dbprintf_igmp(LOG_WARNING, "ip mcast %s portmask (0x%x)\n", group_ip, mac_entry.port_bitmap);

	// skip ACL list
	if (igmp_omci_check_is_in_static_acl(&mac_entry.ipmcast.dst_ip)) return 0;
    
	//skip preassigned mac address
	if (igmp_switch_mac_preassigned_filter_match(&mac_entry.ipmcast.dst_ip))
		return 0;

	struct ipv6_mac_member_t *ipv6_mac_member = NULL;
	if (pbmp == 0)
	{
		if (IS_IPV6_ADDR(igmp_group->group_ip))
		{// IP multicast is replaced by L2 multicast in IPv6
			ipv6_mac_member = igmp_switch_ipv6_mac_search(&ipv6_mac_member_list, igmp_group);
			if (ipv6_mac_member)
			{
				igmp_pkt_ip_cpy(&mac_entry.ipmcast.dst_ip, (union ipaddr *)&ipv6_mac_member->ipv6);
				igmp_switch_ipv6_mac_member_del(&ipv6_mac_member_list, igmp_group);
			}
		}

		if (switch_hw_g.ipmcastaddr_del == NULL)
			return -1;
		if (switch_hw_g.ipmcastaddr_del(&mac_entry) < 0)
			return -1;
	}
	else
	{
		if (IS_IPV6_ADDR(igmp_group->group_ip)) // IP multicast is replaced by L2 multicast in IPv6
			igmp_switch_ipv6_mac_member_add(&ipv6_mac_member_list,igmp_group );

		if (switch_hw_g.ipmcastaddr_add == NULL)
			return -1;
		if (switch_hw_g.ipmcastaddr_add(&mac_entry)<0)
			return -1;
	}
	return 0;
}

int 
igmp_switch_frame_send(struct igmp_clientinfo_t *igmp_clientinfo)
{
	unsigned char i=0,j=0;
	struct me_t *txport_me=NULL,*itxport_me=NULL,*irxport_me=NULL;
	struct meinfo_t *port_miptr=NULL;

	port_miptr = meinfo_util_miptr(47);
	if (port_miptr == NULL) 
	{
		dbprintf_igmp(LOG_ERR, "class=%d, null miptr?\n", 47);
		return 0;
	}

	irxport_me = hwresource_public2private(igmp_clientinfo->rx_desc.bridge_port_me);
	if (1 >= meinfo_util_me_attr_data(irxport_me , 4))
	{ //from cpu/uni to wan

		list_for_each_entry(txport_me, &port_miptr->me_instance_list, instance_node) {

			igmp_clientinfo->pktinfo->tx_desc.bridge_port_me = NULL;
			itxport_me = hwresource_public2private(txport_me);

			if (meinfo_util_me_attr_data(itxport_me, 4) != 2) //to wan
				continue;
			if (meinfo_util_me_attr_data(txport_me, 3) == 6) //skip multicast IWTP
				continue;
			if (omci_env_g.port2port_enable == 0 && meinfo_util_me_attr_data(igmp_clientinfo->rx_desc.bridge_port_me, 1) != meinfo_util_me_attr_data(txport_me,1))
				continue;
			igmp_clientinfo->pktinfo->tx_desc.logical_port_id = 5;
			igmp_clientinfo->pktinfo->tx_desc.bridge_port_me = txport_me;
                        
			for (i = 0 ; i < 8 ; i++)
			{
				if (igmp_clientinfo->tx_meid[i] != txport_me->meid) 
					continue;
				dbprintf_igmp(LOG_WARNING, "frame send to logical port id %d bridge port meid %d(0x%x) i=%d\n",igmp_clientinfo->pktinfo->tx_desc.logical_port_id, igmp_clientinfo->pktinfo->tx_desc.bridge_port_me->meid,igmp_clientinfo->pktinfo->tx_desc.bridge_port_me->meid,i);
				cpuport_util_replace_tag( igmp_clientinfo->pktinfo, &igmp_clientinfo->tx_tci[i]);
				cpuport_frame_send ( igmp_clientinfo->pktinfo);
				break;
			}
		}
	}
	else if (2 == meinfo_util_me_attr_data(irxport_me , 4) || 4 == meinfo_util_me_attr_data(irxport_me , 4))
	{ //from wan to uni
		int trunk_group_send = 0;

		list_for_each_entry(txport_me, &port_miptr->me_instance_list, instance_node) {

			igmp_clientinfo->pktinfo->tx_desc.bridge_port_me = NULL;
			itxport_me = hwresource_public2private(txport_me);
			if (meinfo_util_me_attr_data(itxport_me, 4) != 1 && 0 != meinfo_util_me_attr_data(itxport_me, 4)) //to uni or veip(wifi)
				continue;

			igmp_clientinfo->pktinfo->tx_desc.logical_port_id = meinfo_util_me_attr_data(itxport_me,6);
			if ((1<<igmp_clientinfo->pktinfo->tx_desc.logical_port_id) & omci_env_g.trunk_group_portmask)
			{
				if (trunk_group_send > 0)
					continue;	// dont send more than once to same group of trunk ports
			}

			// look into igmp_clientinfo->tx_meid[i] for matching internal port meid
 			for (i = 0 ; i < 8 ; i++)
			 {
				if (igmp_clientinfo->tx_meid[i] != txport_me->meid) 
					continue;
				igmp_clientinfo->pktinfo->tx_desc.bridge_port_me = txport_me;
				break;
			}
			if (igmp_clientinfo->pktinfo->tx_desc.bridge_port_me == NULL) continue;


			if (omci_env_g.igmp_query_pass_through==1)
			{ // only untag packets pass
				if(igmp_clientinfo->pktinfo->tci.itci.tpid == 0 && igmp_clientinfo->pktinfo->tci.otci.tpid == 0)
				{
					cpuport_frame_send (igmp_clientinfo->pktinfo);
					if ((1<<igmp_clientinfo->pktinfo->tx_desc.logical_port_id) & omci_env_g.trunk_group_portmask)
						trunk_group_send++;
				}
			}
			else if (omci_env_g.igmp_query_pass_through==2)
			{ // all kinds of packet pass
				cpuport_frame_send ( igmp_clientinfo->pktinfo);
				if ((1<<igmp_clientinfo->pktinfo->tx_desc.logical_port_id) & omci_env_g.trunk_group_portmask)
					trunk_group_send++;
			}
			else if (igmp_clientinfo->tx_meid[i] != 0)
			{
				if (meinfo_util_me_attr_data(itxport_me, 4) == 1 )
				{
					cpuport_util_replace_tag( igmp_clientinfo->pktinfo, &igmp_clientinfo->tx_tci[i]);
					if (omci_env_g.port2port_enable)
					{
						struct me_t *veip;
						veip = meinfo_me_get_by_meid(422,7);
						if (meinfo_util_me_attr_data(veip , 2) == 65535 )
						{
							for (j = 0 ; j < 4 ; j++)
							{
								igmp_clientinfo->pktinfo->tx_desc.logical_port_id= j;
								cpuport_frame_send ( igmp_clientinfo->pktinfo);
								if ((1<<igmp_clientinfo->pktinfo->tx_desc.logical_port_id) & omci_env_g.trunk_group_portmask)
									trunk_group_send++;
							}
							return 0;
						}
						else
						{
							cpuport_frame_send ( igmp_clientinfo->pktinfo);
							if ((1<<igmp_clientinfo->pktinfo->tx_desc.logical_port_id) & omci_env_g.trunk_group_portmask)
								trunk_group_send++;
						}
					}
					else 					
					{
						cpuport_frame_send ( igmp_clientinfo->pktinfo);
						if ((1<<igmp_clientinfo->pktinfo->tx_desc.logical_port_id) & omci_env_g.trunk_group_portmask)
							trunk_group_send++;
					}
				}
				/*
				else if (0 == meinfo_util_me_attr_data(itxport_me, 4) && strcmp(omci_env_g.olt_proprietary_support,"alu") == 0) {
					if ( strlen(omci_env_g.wifi_if)){
						cpuport_util_replace_tag( igmp_clientinfo->pktinfo, &igmp_clientinfo->tx_tci[i]);
							if (strcmp(omci_env_g.wifi_if,"wlan1") == 0 )
								igmp_clientinfo->pktinfo->tx_desc.logical_port_id= 8;
							else if (strcmp(omci_env_g.wifi_if,"wlan0") == 0 )
								igmp_clientinfo->pktinfo->tx_desc.logical_port_id= 7;
						cpuport_frame_send ( igmp_clientinfo->pktinfo);
					}
				}
				*/
			}
			dbprintf_igmp(LOG_WARNING, "send to logical port %d meid %d(0x%x)\n",igmp_clientinfo->pktinfo->tx_desc.logical_port_id, igmp_clientinfo->pktinfo->tx_desc.bridge_port_me->meid,igmp_clientinfo->pktinfo->tx_desc.bridge_port_me->meid);
			i++;
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
		if (omci_env_g.igmp_preassign_list[i].ipaddr !=0)
		{	// add ipv4 
			memset(&igmp_mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
			igmp_mac_tab_entry.port_bitmap = switch_get_all_logical_portmask_without_cpuext();
			igmp_mac_tab_entry.vid = 0;
			igmp_mac_tab_entry.ipmcast.src_ip.ipv4.s_addr = 0;
			igmp_mac_tab_entry.ipmcast.dst_ip.ipv4.s_addr = htonl(omci_env_g.igmp_preassign_list[i].ipaddr);
			igmp_switch_mcast_entry_add(&igmp_mac_tab_entry);

		}
		else if (memcmp(omci_env_g.igmp_preassign_list[i].mac, macaddr_zero, 6)!=0)
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
		else if (memcmp(omci_env_g.igmp_preassign_list[i].mac, macaddr_zero, 6)!=0)
		{
			memset(&igmp_mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));

			switch_hw_g.mac_entry_del(&igmp_mac_tab_entry);
		}
	}
	return 0;
}

int igmp_switch_ratelimit_set(int port, int upstream_rate)
{
	if (upstream_rate == 0 || upstream_rate > 250)
		upstream_rate = 250;		
	if (switch_hw_g.igmp_upstream_rate_set)
	{
		return switch_hw_g.igmp_upstream_rate_set(port, upstream_rate);
	}	
	return 0;	     
}

int 
igmp_switch_init(void)
{
//	igmp_switch_frame_recv_init(&igmpproxytask_dummy_fd_g[1]);

	INIT_LIST_HEAD( &ipv6_mac_member_list);

	// igmp_proxy_snooping=1 is proxy only, no snooping, allow multicast floods to UNI
	if(omci_env_g.igmp_proxy_snooping==0 || omci_env_g.igmp_proxy_snooping==2)
	{
		if (switch_hw_g.unknown_mcast_drop_set == NULL)
			return -2;

		if (switch_hw_g.unknown_mcast_drop_set(SWITCH_ENABLED) < 0)
			return -1;
	}
	switch_hw_g.ipmcast_mode_set();

	if (igmp_switch_igmp_extract_set(SWITCH_ENABLED) < 0)
		dbprintf_igmp(LOG_ERR,"igmp extract error\n");

	igmp_switch_preassign_init();

	return 0;
}

int 
igmp_switch_shutdown(void)
{
	igmp_switch_igmp_extract_set(SWITCH_DISABLED);

	if(switch_hw_g.unknown_mcast_drop_set(SWITCH_DISABLED) < 0)
		return -1;

	switch_hw_g.ipmcastaddr_flush();
	switch_hw_g.ipmcast_mode_clear();

	igmp_switch_preassign_deinit();

	return 0;
}


