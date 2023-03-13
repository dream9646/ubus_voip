/******************************************************************
 *
 * Copyright (C) 2016 5V Technologies Ltd.
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
 * Module  : igmp counter module - simple
 * File    : igmp_counter_simple.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "util.h"
#include "switch.h"
#include "mcast_const.h"
#include "mcast_pkt.h"
#include "mcast.h"
#include "igmp_counter_simple.h"

#define IGMP_COUNTER_MODULE_NAME        "simple"
static LIST_HEAD(igmp_counter_g);

struct igmp_pkt_state_t {
	enum IGMP_PKT_STATE state;
	int (*state_handler)(struct igmp_clientinfo_t *igmp_clientinfo);
};

static struct igmp_pkt_vid_msg_counter_t *
igmp_counter_locate_by_vid(unsigned int vid)
{
	struct igmp_pkt_vid_msg_counter_t *pos;
	list_for_each_entry(pos, &igmp_counter_g, node) {
		if (pos->vid == vid)
			return pos;
	}
	return NULL;
}

static struct igmp_pkt_vid_msg_counter_t *
igmp_counter_alloc_by_vid(unsigned int vid, struct me_t *bridge_port_me)
{
	struct igmp_pkt_vid_msg_counter_t *igmp_pkt_vid_msg_counter = malloc_safe(sizeof(struct igmp_pkt_vid_msg_counter_t));
        memset(igmp_pkt_vid_msg_counter, 0, sizeof(struct igmp_pkt_vid_msg_counter_t));
	igmp_pkt_vid_msg_counter->vid = vid;
	igmp_pkt_vid_msg_counter->bridge_port_meid = bridge_port_me->meid; 
	list_add_tail(&(igmp_pkt_vid_msg_counter->node),&igmp_counter_g);
	return igmp_pkt_vid_msg_counter;
}

static int
igmp_counter_received_igmp_incr(unsigned int vid, struct me_t *bridge_port_me)
{
	struct igmp_pkt_vid_msg_counter_t *c = igmp_counter_locate_by_vid(vid);
	if (c == NULL)
		c = igmp_counter_alloc_by_vid(vid, bridge_port_me);
	c->received_igmp++;
	return 0;
}

static int
igmp_counter_total_join_incr(unsigned int vid,struct me_t *bridge_port_me)
{
	struct igmp_pkt_vid_msg_counter_t *c = igmp_counter_locate_by_vid(vid);
	if (c == NULL)
		c = igmp_counter_alloc_by_vid(vid, bridge_port_me);
	c->total_join++;
	return 0;
}

static int
igmp_counter_successful_join_incr(unsigned int vid, struct me_t *bridge_port_me)
{
	struct igmp_pkt_vid_msg_counter_t *c = igmp_counter_locate_by_vid(vid);
	if (c == NULL)
		c = igmp_counter_alloc_by_vid(vid, bridge_port_me);
	c->successful_join++;
	return 0;
}

static int
igmp_counter_total_leave_incr(unsigned int vid, struct me_t *bridge_port_me)
{
	struct igmp_pkt_vid_msg_counter_t *c = igmp_counter_locate_by_vid(vid);
	if (c == NULL)
		c = igmp_counter_alloc_by_vid(vid, bridge_port_me);
	c->total_leave++;
	return 0;
}

static int
igmp_counter_general_query_incr(unsigned int vid, struct me_t *bridge_port_me)
{
	struct igmp_pkt_vid_msg_counter_t *c = igmp_counter_locate_by_vid(vid);
	if (c == NULL)
		c = igmp_counter_alloc_by_vid(vid, bridge_port_me);
	c->general_query++;
	return 0;
}

static int
igmp_counter_specific_query_incr(unsigned int vid, struct me_t *bridge_port_me)
{
	struct igmp_pkt_vid_msg_counter_t *c = igmp_counter_locate_by_vid(vid);
	if (c == NULL)
		c = igmp_counter_alloc_by_vid(vid, bridge_port_me);
	c->specific_query++;
	return 0;
}

// state handler functions
static int parse_clientinfo_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{
        int vid = igmp_clientinfo->vid;
        struct me_t *me47 = igmp_clientinfo->pktinfo->rx_desc.bridge_port_me;
        
	// increment received igmp count
        if (igmp_clientinfo->pktinfo->rx_desc.logical_port_id < 6)
        {
                if (igmp_clientinfo->pktinfo->rx_desc.bridge_port_me)
                        igmp_counter_received_igmp_incr(vid, me47);
		else
                        dbprintf_igmp(LOG_WARNING,"recv pkt from port %d, which is mapped to null bport me?\n", igmp_clientinfo->pktinfo->rx_desc.logical_port_id);
        }

// This snippet is in router mode, not sure what it does
//*	if ( pktinfo->rx_desc.logical_port_id != 6 && pktinfo->rx_desc.logical_port_id  < 12 )
//*        {
//*		if (pktinfo->rx_desc.bridge_port_me)
//*			igmp_counter_received_igmp_incr(igmp_clientinfo->vid, pktinfo->rx_desc.bridge_port_me);
//*		else
//*			dbprintf_igmp(LOG_WARNING,"recv pkt from port %d, which is mapped to null bport me?\n", pktinfo->rx_desc.logical_port_id);
//*	}
                
        struct igmp_group_t *pos = NULL;
        
        switch (igmp_clientinfo->igmp_msg_type) 
	{
                case IGMP_V1_MEMBERSHIP_REPORT:
                        igmp_counter_total_join_incr(vid, me47);
			break;
		case IGMP_MEMBERSHIP_QUERY:
                        list_for_each_entry(pos, &igmp_clientinfo->group_list, entry_node) {
                                if (pos->group_ip.ipv4.s_addr) 
                                        igmp_counter_specific_query_incr(vid, me47);
                                else 
                                        igmp_counter_general_query_incr(vid, me47);
                                break;
                        }
			break;
		case IGMP_V2_MEMBERSHIP_REPORT:
			igmp_counter_total_join_incr(vid, me47);
			break;	
		case IGMP_V3_MEMBERSHIP_REPORT:
			igmp_counter_total_join_incr(vid, me47);
			break;
		case IGMP_V2_LEAVE_GROUP:
			igmp_counter_total_leave_incr(vid, me47);
			break;
		case MLD_LISTENER_QUERY:
			break;
		case MLD_LISTENER_REPORT:
			igmp_counter_total_join_incr(vid, me47);
			break;
		case MLD_LISTENER_REDUCTION:
			igmp_counter_total_leave_incr(vid, me47);
			break;
		case MLD_V2_LISTENER_REPORT:
			igmp_counter_total_join_incr(vid, me47);
                        break;		
		default:
			dbprintf_igmp(LOG_ERR,"unknown igmp msg type (%d)\n", igmp_clientinfo->igmp_msg_type);
			return -1;
	}                
        
        return 0;
}

static int tag_filter_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{

        int vid = igmp_clientinfo->vid;
        struct me_t *me47 = igmp_clientinfo->pktinfo->rx_desc.bridge_port_me;
        
        if (igmp_clientinfo->state.exec_ret == CONTINUE) igmp_counter_successful_join_incr(vid, me47);
        
        return 0;
}       

static struct igmp_pkt_state_t state_handler_table[]={
	{IGMP_PKT_PARSE_CLIENTINFO, parse_clientinfo_handler},
	{IGMP_PKT_AFTER_TAGFILTER, tag_filter_handler}
};
       

// module functions
int
igmp_counter_reset(void)
{
	struct igmp_pkt_vid_msg_counter_t *pos, *n;
	list_for_each_entry_safe(pos, n, &igmp_counter_g, node) {
		list_del(&pos->node);
		free_safe(pos);
	}
	return 0;
}

int
igmp_counter_init(void)
{
	return 0;
}

const char *igmp_counter_simple_name()
{
        return IGMP_COUNTER_MODULE_NAME;
}

int igmp_counter_simple_handler(struct igmp_clientinfo_t *igmp_clientinfo)
{
	int i;
	
        int nHandlers = sizeof(state_handler_table)/sizeof(struct igmp_pkt_state_t);
        
        for (i=0; i < nHandlers; i++) {
                if (state_handler_table[i].state != igmp_clientinfo->state.exec_state) continue;
                // matching state, execute handler
                state_handler_table[i].state_handler(igmp_clientinfo);      
	}
        
	return 0; 
}

int igmp_cli_simple(int fd)
{
	struct igmp_pkt_vid_msg_counter_t *pos, *n;

	list_for_each_entry_safe(pos, n, &igmp_counter_g, node) {

		if (pos->vid == 4096 )
                {
			util_fdprintf(fd, "untag\n");
		}
                else
                {
			util_fdprintf(fd, "vid: %d\n", pos->vid);
		}
                
		util_fdprintf(fd, "\treceived_igmp msg: %d\n", pos->received_igmp);
		util_fdprintf(fd, "\ttotal_join: %d\n", pos->total_join);
		util_fdprintf(fd, "\tsuccessful_join: %d\n", pos->successful_join);
		util_fdprintf(fd, "\ttotal_leave: %d\n", pos->total_leave);
		util_fdprintf(fd, "\tgeneral_query: %d\n", pos->general_query);
		util_fdprintf(fd, "\tspecific_query: %d\n\n\n", pos->specific_query);
	}
        
	return 0;
}

int igmp_counter_iterator_simple(int (*iterator_function)(struct igmp_pkt_vid_msg_counter_t *pos, void *data), void *iterator_data)
{
        struct igmp_pkt_vid_msg_counter_t *pos_counter=NULL;
        
        list_for_each_entry(pos_counter,  &igmp_counter_g, node) {
                (*iterator_function)(pos_counter, iterator_data);               
        }
        
        return 0;
}

struct igmp_counter_module_t simple =
{
        .module_name = igmp_counter_simple_name,
        .init = igmp_counter_init,
        .reset = igmp_counter_reset,
        .igmp_counter_handler = igmp_counter_simple_handler,
        .igmp_cli_handler = igmp_cli_simple,
        .igmp_counter_iterator = igmp_counter_iterator_simple
};
