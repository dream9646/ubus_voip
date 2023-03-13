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
 * File    : alu_signal.c
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

static LIST_HEAD(alu_signal_list);

struct igmp_alu_pkt_t {
	struct cpuport_info_t	*pkt;
	union ipaddr		client_ip;
	struct list_head	node;
};

struct igmp_alu_group_t {
	union ipaddr	group_ip;
	short int signal;
	struct list_head	pkt_node;	// store a list of join/leave pkts before signal
	struct list_head	entry_node;
};

#define IGMP_ALU_SIGNAL_OFF	0xff
#define IGMP_ALU_SIGNAL_ON	0xfe

static struct cpuport_info_t *
cpuport_clone_pktinfo(struct cpuport_info_t *pktinfo)
{
	struct cpuport_info_t *pktinfo2;
	unsigned char *buf_ptr;

	if (pktinfo == NULL)
		return NULL;

	pktinfo2 = (struct cpuport_info_t *)malloc_safe(sizeof(struct cpuport_info_t));
	if (pktinfo2 == NULL) {
		dbprintf_igmp(LOG_WARNING, "fail to malloc pktinfo for alu signal\n");
		return NULL;
	}
	buf_ptr = (unsigned char *)malloc_safe(2*CPUPORT_VTAG_LEN + pktinfo->frame_len + 8);	// 8 is for safety
	if (buf_ptr == NULL) {
		dbprintf_igmp(LOG_WARNING, "fail to malloc pktinfo->buf_ptr for alu_signal\n");
		free_safe(pktinfo2);
		return NULL;
	}

	pktinfo->history = pktinfo2;	// orig pktinfo has a pointer to the cloned pkt for tracking
	memcpy(pktinfo2, pktinfo, sizeof(struct cpuport_info_t));	// copy pktinfo
	pktinfo2->buf_ptr = buf_ptr;
	pktinfo2->buf_len = pktinfo->frame_len; // the buf len doesn't count the len before the frame start
	pktinfo2->frame_ptr = pktinfo2->buf_ptr + 2*CPUPORT_VTAG_LEN;
	memcpy(pktinfo2->frame_ptr, pktinfo->frame_ptr, pktinfo->frame_len);	// copy pkt frame
	// recalc nh pointer 
	pktinfo2->nh = pktinfo2->frame_ptr + ((void*)pktinfo->nh - (void*)pktinfo->frame_ptr);

	return pktinfo2;
}

/*
	1. alu signalling first bypasses incoming IGMPv2 report/leave from UNI
	2. supported alu OLT will issue a signal in IGMPv3 with dst ip 0.0.x.x 
	3. ONT intercepts IGMPv3 signal and sets necessary forwarding
	Refer to vendor documentation on the specifics 
*/

static void igmp_alu_v2_remove_signal_group(struct igmp_group_t *igmp_group)
{
	struct igmp_alu_group_t *alu_pos=NULL, *alu_n=NULL;

	list_for_each_entry_safe(alu_pos, alu_n, &alu_signal_list, entry_node) {
		if (mcast_pkt_ip_cmp(&igmp_group->group_ip, &alu_pos->group_ip)) continue;
		
		struct igmp_alu_pkt_t *p, *p_n;
		list_for_each_entry_safe(p, p_n, &alu_pos->pkt_node, node) {
			if (p->pkt == NULL) continue;
			if (p->pkt->buf_ptr) free_safe(p->pkt->buf_ptr);
			free_safe(p->pkt);
			list_del(&p->node);	// free unsent alu_pkt
			free_safe(p);
		}
		
                list_del(&alu_pos->entry_node);
		free_safe(alu_pos);
	}
}

static int alu_igmp_get_group_signal(union ipaddr *igmp_group)
{ 
	struct igmp_alu_group_t *alu_pos=NULL, *alu_n=NULL;
	
	list_for_each_entry_safe(alu_pos, alu_n, &alu_signal_list, entry_node) {
		if (mcast_pkt_ip_cmp( igmp_group, &alu_pos->group_ip))
			continue;
		
		return alu_pos->signal;
	}
	
	return 0;   
}

static struct igmp_alu_group_t *alu_igmp_get_signal_group(struct igmp_group_t *igmp_group)
{
	struct igmp_alu_group_t *alu_pos=NULL, *alu_n=NULL;
	
	list_for_each_entry_safe(alu_pos, alu_n, &alu_signal_list, entry_node) {
		if (mcast_pkt_ip_cmp(&igmp_group->group_ip, &alu_pos->group_ip))
			continue;
		return alu_pos;
	}
	return NULL;
}

static void alu_igmp_set_group_signal(struct igmp_group_t *igmp_group, short signal)
{ 
	struct igmp_alu_group_t *alu_pos=NULL, *alu_n=NULL;
	
	list_for_each_entry_safe(alu_pos, alu_n, &alu_signal_list, entry_node) {
		if (mcast_pkt_ip_cmp(&igmp_group->group_ip, &alu_pos->group_ip))
			continue;
		
		alu_pos->signal = signal;
	} 
}

static int alu_igmp_create_signal_group(struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group)
{
	struct igmp_alu_group_t *g;
		
	// new group
	g = malloc_safe(sizeof(struct igmp_alu_group_t));
	mcast_pkt_ip_cpy(&g->group_ip, &igmp_group->group_ip);
	g->signal = IGMP_ALU_SIGNAL_OFF;
	INIT_LIST_HEAD(&g->pkt_node);
	INIT_LIST_NODE(&g->entry_node);    
	list_add(&g->entry_node, &alu_signal_list);
	
	return 0;      
}

static void dump_all_signals()
{
	struct igmp_alu_group_t *alu_pos=NULL, *alu_n=NULL;
	char gip[INET6_ADDRSTRLEN];
	
	list_for_each_entry_safe(alu_pos, alu_n, &alu_signal_list, entry_node) {
		util_inet_ntop(AF_INET,  &alu_pos->group_ip, (char *)gip, sizeof(gip));
		struct igmp_alu_pkt_t *p, *p_n;
		int i = 0;
		list_for_each_entry_safe(p, p_n, &alu_pos->pkt_node, node) {
			i++;
		}
		dbprintf_igmp(LOG_INFO,"signal group - %s, count - %d\n", gip, i);
	}
}

static int 
igmp_alu_v3_mbship_report_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	struct igmp_group_t *pos=NULL, *n=NULL;
	char ip_addr[INET6_ADDRSTRLEN];

	if ((*igmp_clientinfo)->rx_desc.logical_port_id != switch_get_wan_logical_portid()) return 0;	// igmpv3 signal comes from OLT
	
	util_inet_ntop(AF_INET,  &(*igmp_clientinfo)->dst_ip, (char *)ip_addr, sizeof(ip_addr));
	if ((*igmp_clientinfo)->dst_ip.ipv4.s_addr & 0xFFFF0000)	// check if dip is 0.0.x.x
	{
		dbprintf_igmp(LOG_INFO, "IGMPv3 with dip: %s is not a valid signal packet\n", ip_addr);
		return 0;	
	}
	else
	{
		dbprintf_igmp(LOG_INFO, "IGMPv3 with dip: %s is a signal packet\n", ip_addr);
	}
	
	list_for_each_entry_safe(pos, n, &(*igmp_clientinfo)->group_list, entry_node) {
		struct igmp_alu_group_t *sg = alu_igmp_get_signal_group(pos);
		if (sg == NULL) {
			// receive signal pkt but doesn't exist in signal group, buffer this group at first and wait for join coming
			dbprintf_igmp(LOG_INFO, "signal group is null\n");
			alu_igmp_create_signal_group((*igmp_clientinfo), pos);
			sg = alu_igmp_get_signal_group(pos);
			if (sg == NULL) {
				dbprintf_igmp(LOG_INFO, "signal group is still null\n");
				return -1;	// signal packet does not exist anymore - discard
			}
		}
		int isLeave = 0;
		if (pos->record_type == MODE_IS_EXCLUDE || pos->record_type == CHANGE_TO_EXCLUDE_MODE)
		{
			util_inet_ntop(AF_INET, &pos->group_ip, (char *)ip_addr, sizeof(ip_addr));			
			dbprintf_igmp(LOG_INFO, "[ALU]join %s is permitted by alu olt\n", ip_addr);
			alu_igmp_set_group_signal(pos, IGMP_ALU_SIGNAL_ON);
		}
		else if (pos->record_type == MODE_IS_INCLUDE || pos->record_type == CHANGE_TO_INCLUDE_MODE)
		{
			util_inet_ntop(AF_INET, &pos->group_ip, (char *)ip_addr, sizeof(ip_addr));
			dbprintf_igmp(LOG_INFO, "[ALU]leave %s is permitted from alu olt\n", ip_addr);
			alu_igmp_set_group_signal(pos, IGMP_ALU_SIGNAL_OFF);
			isLeave = 1;
		}
		else {
			dbprintf_igmp(LOG_INFO, "[ALU]unknown record type %d from alu olt\n", pos->record_type);
		}
		
		// empty all queued packet on this group
		struct igmp_alu_pkt_t *p, *p_n;
		list_for_each_entry_safe(p, p_n, &sg->pkt_node, node) {
			igmp_proxy_send_cmd(IGMP_CMD_RUN_PKT, (void *)p->pkt);
			list_del(&p->node);	// remove from the list, packet will be freed as normal packet
			free_safe(p);
		}
		
		if (isLeave) igmp_alu_v2_remove_signal_group(pos);
		
	}
	
	return -1;	// don't continue processing signal packet
}

static int igmp_alu_pkt_exist(struct igmp_alu_group_t *sg, union ipaddr *client_ip)
{
	struct igmp_alu_pkt_t *p, *p_n;
	list_for_each_entry_safe(p, p_n, &sg->pkt_node, node) {
		if (mcast_pkt_ip_cmp(&p->client_ip, client_ip) == 0) return 1;	
	}	
	return 0;
}

static int 
igmp_alu_v2_mbship_report_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	struct igmp_group_t *pos=NULL, *n=NULL;
	char ip_addr[INET6_ADDRSTRLEN];

        list_for_each_entry_safe(pos, n, &(*igmp_clientinfo)->group_list, entry_node) {
        	util_inet_ntop(AF_INET, &pos->group_ip, (char *)ip_addr, sizeof(ip_addr));
        	int signal = alu_igmp_get_group_signal(&pos->group_ip);
        	if (signal == 0) alu_igmp_create_signal_group((*igmp_clientinfo), pos);
        	
        	signal = alu_igmp_get_group_signal(&pos->group_ip);
        	if (signal == IGMP_ALU_SIGNAL_OFF)
		{
			util_inet_ntop(AF_INET, &pos->group_ip, (char *)ip_addr, sizeof(ip_addr));
			// modify packet type so it skips normal snooping
        		(*igmp_clientinfo)->igmp_msg_type = IGMP_SPECIAL;
        		
        		struct igmp_alu_group_t *sg = alu_igmp_get_signal_group(pos);
			if (igmp_alu_pkt_exist(sg, &((*igmp_clientinfo)->client_ip))) continue; // join by the same client
			   		
        		struct igmp_alu_pkt_t *pkt = malloc_safe(sizeof(struct igmp_alu_group_t));
			pkt->pkt = cpuport_clone_pktinfo((*igmp_clientinfo)->pktinfo);
			mcast_pkt_ip_cpy(&pkt->client_ip, &((*igmp_clientinfo)->client_ip));
			INIT_LIST_NODE(&pkt->node);
			list_add(&pkt->node, &sg->pkt_node);	// queue this packet for rejoin
			dbprintf_igmp(LOG_INFO, "[ALU]waiting for alu signal, join %s\n", ip_addr);
			return 0;
		}
		
		dbprintf_igmp(LOG_INFO, "[ALU]join %s is already signalled, normal path\n", ip_addr);
	}
	
	return 0;	
}

static int 
igmp_alu_v2_mbship_leave_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	struct igmp_group_t *pos=NULL, *n=NULL;
	char ip_addr[INET6_ADDRSTRLEN];

        list_for_each_entry_safe(pos, n, &(*igmp_clientinfo)->group_list, entry_node) {
        	util_inet_ntop(AF_INET, &pos->group_ip, (char *)ip_addr, sizeof(ip_addr));
        	int signal = alu_igmp_get_group_signal(&pos->group_ip);
        	if (signal == 0) return 0;		// no signal group, no join, leave does nothing
        	if (signal == IGMP_ALU_SIGNAL_OFF)	// has signal group but in usignalled state - joined but receive leave before signal 
        	{
        		igmp_alu_v2_remove_signal_group(pos);	// remove this signal group
			dbprintf_igmp(LOG_INFO, "[ALU]leave %s is already signalled, normal path\n", ip_addr);
        		return 0;
		}
        	
		// modify packet type so it skips normal snooping
        	(*igmp_clientinfo)->igmp_msg_type = IGMP_SPECIAL;
        	struct igmp_alu_group_t *sg = alu_igmp_get_signal_group(pos);
		if (igmp_alu_pkt_exist(sg, &((*igmp_clientinfo)->client_ip))) continue;	// leave by the same client
			    		
        	struct igmp_alu_pkt_t *pkt = malloc_safe(sizeof(struct igmp_alu_group_t));
		pkt->pkt = cpuport_clone_pktinfo((*igmp_clientinfo)->pktinfo);
		mcast_pkt_ip_cpy(&pkt->client_ip, &((*igmp_clientinfo)->client_ip));
		INIT_LIST_NODE(&pkt->node);
		list_add(&pkt->node, &sg->pkt_node);	// queue the leave packet
		dbprintf_igmp(LOG_INFO, "[ALU]waiting for alu signal, leave %s\n", ip_addr);
		return 0;	
	}
	
	return 0;	
}

static int
igmp_filter_alu_signalling_pre_filter(struct igmp_clientinfo_t *igmp_clientinfo)
{
	// inspect incoming igmp v2/v3 packet
	// for incoming report/leave, first check if it's signal packet
	//	no -> set packet status to be forwarded & bypass all processing
	//      yes -> modify signal packet and proceed normal processing
	
	MCAST_FILTER_NO_IPV6(igmp_clientinfo)
	
	int ret = MCAST_FILTER_CONTINUE;
	
	if (!omci_env_g.olt_alu_mcast_support)
	{
		dbprintf_igmp(LOG_WARNING, "alu signal filter present but OLT is not supported -> BYPASS\n");
		return ret;	// bypass this filter
	}
	
	if (THREAD_ENV.igmp_proxy_snooping)
	{
		dbprintf_igmp(LOG_ERR, "alu signal filter present but ONT is not in L2 mode -> CAUTION\n");
		ret = MCAST_FILTER_CONTINUE;
		return ret;
	}
	
	dump_all_signals();
	
	switch (igmp_clientinfo->igmp_msg_type)
	{
	case IGMP_V2_MEMBERSHIP_REPORT:
		ret = igmp_alu_v2_mbship_report_worker(&igmp_clientinfo);
		break;
	case IGMP_V2_LEAVE_GROUP:
		ret = igmp_alu_v2_mbship_leave_worker(&igmp_clientinfo);
		break;
	case IGMP_V3_MEMBERSHIP_REPORT:
		ret = igmp_alu_v3_mbship_report_worker(&igmp_clientinfo);
		break;
	default:
		break;	
	}
					
	return ret;
}

 
// module callbacks
static const char * igmp_filter_alu_signalling_name(void)
{
	return "alu_signalling";
}

static int igmp_filter_alu_signalling_init(void)
{
	INIT_LIST_HEAD(&alu_signal_list);
	
	return MCAST_FILTER_CONTINUE;
}

static int igmp_filter_alu_signalling_deinit(void)
{
	struct igmp_alu_group_t *alu_pos=NULL, *alu_n=NULL;

	list_for_each_entry_safe(alu_pos, alu_n, &alu_signal_list, entry_node) {
		struct igmp_alu_pkt_t *p, *p_n;
		list_for_each_entry_safe(p, p_n, &alu_pos->pkt_node, node) {
			if (p->pkt == NULL) continue;
			if (p->pkt->buf_ptr) free_safe(p->pkt->buf_ptr);
			free_safe(p->pkt);
			list_del(&p->node);	// free unsent alu_pkt
			free_safe(p);
		}
		
                list_del(&alu_pos->entry_node);
		free_safe(alu_pos);
	}
		
	return MCAST_FILTER_CONTINUE;
}

struct igmp_filter_module_t alu_signal_filter =
{
        .priority = 2,
	.filter_name = igmp_filter_alu_signalling_name,
        .init = igmp_filter_alu_signalling_init,
        .deinit = igmp_filter_alu_signalling_deinit,
        .pre_filter = igmp_filter_alu_signalling_pre_filter,
};


          

