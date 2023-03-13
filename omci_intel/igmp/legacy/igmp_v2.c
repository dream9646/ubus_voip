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
 * Module  : igmp v2 module
 * File    : igmp_v2.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "util_inet.h"
#include "list.h"
#include "util.h"
#include "cpuport.h"
#include "switch.h"
#include "cpuport_frame.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"
#include "batchtab_cb.h"
#include "batchtab.h"

#include "mcast_pkt.h"
#include "mcast_const.h"
#include "igmp_mbship.h"
#include "mcast.h"
#include "mcast_main.h"
#include "mcast_timer.h"
#include "mcast_switch.h"
#include "igmp_omci.h"
#include "mcast_router.h"
#include "mcast_proxy.h"
#include "mcast_snoop.h"

extern int timer_id_non_fast_leave_g;
extern uint32_t allhosts_group;          /* All hosts addr in net order */

static struct igmp_clientinfo_t * make_igmp_clientinfo_t_copy(struct igmp_clientinfo_t *igmp_clientinfo)
{
        struct igmp_clientinfo_t *copy = malloc(sizeof(struct igmp_clientinfo_t));
        memcpy(copy, igmp_clientinfo, sizeof(struct igmp_clientinfo_t));
        copy->pktinfo = NULL;
        INIT_LIST_HEAD(&copy->group_list);
        
	if (!list_empty(&igmp_clientinfo->group_list))
        {
                struct igmp_group_t *group_pos=NULL, *group_n=NULL;

		list_for_each_entry_safe(group_pos, group_n, &igmp_clientinfo->group_list, entry_node)
                {
                        struct igmp_group_t *g = malloc_safe(sizeof(struct igmp_group_t));
                        memcpy(g, group_pos, sizeof(struct igmp_group_t));
			INIT_LIST_NODE(&g->entry_node);
                        list_add(&g->entry_node, &copy->group_list);
		}
	}
        
        return copy;      
}

static int 
igmp_mbship_query_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	struct me_t *bridge_port_me = (*igmp_clientinfo)->rx_desc.bridge_port_me;
	if (!bridge_port_me)
	{
		dbprintf_igmp(LOG_WARNING,"bridge_port_me == NULL\n");
		return -1;
	}

	if (igmp_config_g.igmp_function == IGMP_FUNCTION_TRANSPARENT)
	{
		if (omci_env_g.igmp_proxy_snooping == 0)
		{
			igmp_switch_frame_send( *igmp_clientinfo );
		}
	}
        
	return 0;
}

static int 
igmp_mbship_mld_query_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	struct me_t *bridge_port_me = (*igmp_clientinfo)->rx_desc.bridge_port_me;
	if (!bridge_port_me)
	{
		dbprintf_igmp(LOG_WARNING,"bridge_port_me == NULL\n");
		return -1;
	}

	if (igmp_config_g.igmp_function == IGMP_FUNCTION_TRANSPARENT)
	{
		if ( omci_env_g.igmp_proxy_snooping == 0)
		{
			igmp_switch_frame_send( *igmp_clientinfo );
		}
	}
	
	return 0;
}

static int 
igmp_clientinfo_has_tx_me(struct igmp_clientinfo_t *igmp_clientinfo)
{
        if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
	{
		struct meinfo_t *miptr=meinfo_util_miptr(249);
		struct me_t *me249=NULL;
		int has_me249=0, i=0;
		list_for_each_entry(me249, &miptr->me_instance_list, instance_node) {
			if (me249 != NULL)
			{
				has_me249 = 1;
				break;
			}
		}
		if (!has_me249) return 1;	// me 249 does not exist
		for (i = 0 ; i < 8 ; i++)
		{
			if (igmp_clientinfo->tx_meid[i] != 0) return 1;
		}
        	return 0;
	}
	else 
	{
		return 1;
	}
}

static int check_exceed_group_member_count(struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct igmp_mbship_t *mbship_pos=NULL, *mbship_n=NULL;
	int count = 0;

        struct igmp_group_t *pos=NULL, *n=NULL;
	list_for_each_entry_safe(pos, n, &(igmp_clientinfo)->group_list, entry_node)
	{
		list_for_each_entry_safe(mbship_pos, mbship_n, &igmp_mbship_db_g.mbship_list, entry_node)
		{
			if (mbship_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
			if (mbship_pos->uni_vid != igmp_clientinfo->vid) continue;
			if (igmp_pkt_ip_cmp(&pos->group_ip, &mbship_pos->group_ip)) continue;
		
			count++;
		}
		break;	
	}
	
	return (count >= (igmp_config_g.max_group_member))?1:0;
}


// igmp_v2 specific filter
// this is bad as packet comes often, building list takes time, and fragments memory
// it's better to change underlying membership datastructure - TODO
struct igmp_v2_group_t {
	union ipaddr gip;    
        struct list_head node;
};

int 
igmp_mbship_total_group_add_is_allow(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo)
{
	int ret = 1;
	
	if (igmp_config_g.group_limit == 0) return 1;
	
	// don't do this filter if it's not calix
	// OMCI ME310 does have its own max stream group and mspt does specifiy its own max stream group
	// this does raises the question if all mspt group limit should follow the non mstp stream limit
	// for non calix module, use the igmp_omci_max_allowed_limit_filter only
	// calix module, this will run along igmp_omci_max_allowed_limit_filter - weird mess
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) != 0) return 1;
	
	LIST_HEAD(igmp_group_g);
	
	struct igmp_group_t *client_pos=NULL, *client_n=NULL;
	struct igmp_mbship_t *mbship_pos=NULL, *mbship_n=NULL;

	if (igmp_mbship_db ==NULL || igmp_clientinfo==NULL)
	{
		dbprintf_igmp(LOG_WARNING, "igmp_mbship_db == NULL\n");
		return 0;
	}
	
	// build a linklist of current groups
	int grp_count = 0;
	struct igmp_v2_group_t *grp_pos=NULL, *grp_n=NULL;
	list_for_each_entry_safe(mbship_pos, mbship_n, &igmp_mbship_db->mbship_list, entry_node) {
		if (mbship_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
		if (mbship_pos->uni_vid != igmp_clientinfo->vid) continue;
		int found = 0;
		
		list_for_each_entry_safe(grp_pos, grp_n, &igmp_group_g, node) {
			if (igmp_pkt_ip_cmp(&grp_pos->gip,&mbship_pos->group_ip) == 0)
			{
				found = 1;	// match exisiting 
				break;
			}
		}
		
		if (found) continue;	// not new
		struct igmp_v2_group_t *grp_new = malloc_safe(sizeof(struct igmp_v2_group_t));
        	memcpy(&grp_new->gip, &mbship_pos->group_ip, sizeof(union ipaddr));
		list_add(&(grp_new->node), &igmp_group_g);
		grp_count++;
	}
		
	list_for_each_entry_safe(client_pos, client_n, &igmp_clientinfo->group_list, entry_node) {
	
		dbprintf_igmp(LOG_WARNING, "grp_count = %d\n", grp_count);
			
		list_for_each_entry_safe(grp_pos, grp_n, &igmp_group_g, node) {
			if (igmp_pkt_ip_cmp(&grp_pos->gip, &client_pos->group_ip) == 0) goto grp_cleanup;	// this is exisiting group, allow join
		}
					
		if ((grp_count + 1) > igmp_config_g.group_limit) ret = 0;	// exceeed group count limit
		break;	// essentially handle v1/v2 only. igmpv3 can have a report with multiple group joins, behavior is yet defined
	}

grp_cleanup:
	// list cleanup
	list_for_each_entry_safe(grp_pos, grp_n, &igmp_group_g, node) {
		list_del(&grp_pos->node);
		free_safe(grp_pos);
	}
			
	return ret;
}

                          
static int 
igmp_v1_mbship_report_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	struct me_t *bridge_port_me = NULL;
	int join_check;
	struct igmp_group_t *pos=NULL, *n=NULL;

	dbprintf_igmp(LOG_WARNING,"pkt uni vid %d\n",(*igmp_clientinfo)->vid);

	bridge_port_me = (*igmp_clientinfo)->rx_desc.bridge_port_me;
	if (!bridge_port_me)
	{
		dbprintf_igmp(LOG_WARNING,"bridge_port_me == NULL \n");
		return -1;
	}

        if (!igmp_clientinfo_has_tx_me(*igmp_clientinfo))
        {
                // ME 249/171 failure case
                dbprintf_igmp(LOG_WARNING,"IGMP_JOIN_FAILED_VLAN_TAGGING\n");
                return 0;
        }

        if (check_exceed_group_member_count(*igmp_clientinfo))
        {
		dbprintf_igmp(LOG_WARNING,"exceed group member limit\n");
		return 0;
	}
	        
	join_check = igmp_omci_check_join_is_allowed( bridge_port_me, *igmp_clientinfo );

	if (join_check == IGMP_JOIN_DENIED_ACL_FILTERED)
	{
	
		dbprintf_igmp(LOG_WARNING,"IGMP_JOIN_DENIED_ACL_FILTERED\n");
		return 0;
	}

	if (igmp_mbship_total_group_add_is_allow(&igmp_mbship_db_g, *igmp_clientinfo) == 0)
		return 0;

	dbprintf_igmp(LOG_WARNING,"igmp_function:%d \n",igmp_config_g.igmp_function);
	if (igmp_config_g.igmp_function == IGMP_FUNCTION_TRANSPARENT )
	{
		if ( igmp_omci_check_msgrate_is_allowed(bridge_port_me, (*igmp_clientinfo)->vid))
		{
			igmp_omci_update_msgrate(bridge_port_me, (*igmp_clientinfo)->vid);
			if ((*igmp_clientinfo)->pktinfo->rx_desc.router_mode && omci_env_g.igmp_proxy_snooping)
			{
				if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                                        igmp_v3_lw_acceptigmp((*igmp_clientinfo)->pktinfo);
                                else
                                        acceptigmp((*igmp_clientinfo)->pktinfo);       
			}
			else
			{
				igmp_switch_frame_send(*igmp_clientinfo);
			}
		}
		else
			return 0;
	}
	
	igmp_omci_update_join(bridge_port_me, *igmp_clientinfo);

	list_for_each_entry_safe( pos, n, &(*igmp_clientinfo)->group_list, entry_node) {
		igmp_mbship_add_client ( &igmp_mbship_db_g , *igmp_clientinfo, pos);
		igmp_switch_mac_table_client_update(*igmp_clientinfo , &igmp_mbship_db_g, pos);
	}

	dbprintf_igmp(LOG_WARNING,"bridge membership\n");
	
	return 0;
}

static int 
igmp_v2_mbship_report_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	struct me_t *bridge_port_me = NULL;
	int join_check;
	struct igmp_group_t *pos=NULL, *n=NULL;

	bridge_port_me = (*igmp_clientinfo)->rx_desc.bridge_port_me;
	if (!bridge_port_me)
	{
		dbprintf_igmp(LOG_WARNING,"bridge_port_me == NULL\n");
		return -1;
	}

        if (!igmp_clientinfo_has_tx_me(*igmp_clientinfo))
        {
                // ME 249/171 failure case
                dbprintf_igmp(LOG_WARNING,"IGMP_JOIN_FAILED_VLAN_TAGGING\n");
                return 0;
        }
        
        if (check_exceed_group_member_count(*igmp_clientinfo))
        {
		dbprintf_igmp(LOG_WARNING,"exceed group member limit\n");
		return 0;
	}
	
	join_check = igmp_omci_check_join_is_allowed(bridge_port_me, *igmp_clientinfo);

	if (join_check == IGMP_JOIN_DENIED_ACL_FILTERED)
	{	
		dbprintf_igmp(LOG_WARNING,"IGMP_JOIN_DENIED_ACL_FILTERED\n");
		return 0;
	}
                                        
	if (igmp_mbship_total_group_add_is_allow(&igmp_mbship_db_g, *igmp_clientinfo) == 0)
	{
		dbprintf_igmp(LOG_WARNING,"total group override\n");
		return 0;
	}

	dbprintf_igmp(LOG_WARNING,"igmp_function:%d rx_desc.router_mode:%d\n",igmp_config_g.igmp_function,(*igmp_clientinfo)->pktinfo->rx_desc.router_mode);
	if (igmp_config_g.igmp_function == IGMP_FUNCTION_TRANSPARENT )	// appears to be 0 for all OLT so far - not used to set snooping or proxy
	{
		if ( igmp_omci_check_msgrate_is_allowed(bridge_port_me, (*igmp_clientinfo)->vid))
		{
			igmp_omci_update_msgrate(bridge_port_me, (*igmp_clientinfo)->vid);
			if ((*igmp_clientinfo)->pktinfo->rx_desc.router_mode && omci_env_g.igmp_proxy_snooping)
			{
				if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                                        igmp_v3_lw_acceptigmp((*igmp_clientinfo)->pktinfo);
                                else
                                        acceptigmp((*igmp_clientinfo)->pktinfo);       
			}
			else
			{
				igmp_switch_frame_send( *igmp_clientinfo );
			}
		}
		else
		{
			dbprintf_igmp(LOG_WARNING,"igmp message rate override\n");
			return 0;
		}
	}

	igmp_omci_update_join(bridge_port_me, *igmp_clientinfo);

	igmp_mbship_rm_client_groups_in_mbship_db(&igmp_mbship_db_g,*igmp_clientinfo, 0); 
	if( join_check == IGMP_JOIN_ALREADY_BY_SAME_CLIENT)
	{ 
		dbprintf_igmp(LOG_WARNING,"IGMP_JOIN_ALREADY_BY_SAME_CLIENT\n");
		return 0;
	}
	
	list_for_each_entry_safe(pos, n, &(*igmp_clientinfo)->group_list, entry_node) {
		if (omci_env_g.olt_alu_mcast_support) alu_igmp_update_group(&igmp_mbship_db_g ,pos);
		igmp_mbship_add_client ( &igmp_mbship_db_g , *igmp_clientinfo, pos);
		igmp_switch_mac_table_client_update(*igmp_clientinfo , &igmp_mbship_db_g, pos);
	}
	dbprintf_igmp(LOG_WARNING,"bridge membership\n");
	
	return 0;
}

static int igmp_v2_router_leave_forward_check(struct igmp_clientinfo_t **igmp_clientinfo)
{
        struct igmp_group_t *pos = NULL, *n = NULL;
        
        list_for_each_entry_safe(pos, n, &(*igmp_clientinfo)->group_list, entry_node) {
                struct switch_mac_tab_entry_t mac_tab_entry;
                memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
                *(unsigned int *)&mac_tab_entry.ipmcast.dst_ip = *(unsigned int *)&pos->group_ip;
                if (igmp_switch_mcast_entry_get(&mac_tab_entry) == 0)
                {
                        return (mac_tab_entry.port_bitmap)?0:1;
                }                
        }
        
        return 0;				
}

static int 
igmp_v2_mbship_leave_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	struct me_t *bridge_port_me = NULL;
	struct igmp_group_t *pos=NULL,*n=NULL;
         
	if (list_empty(&igmp_mbship_db_g.mbship_list)) return 0;

	bridge_port_me = (*igmp_clientinfo)->rx_desc.bridge_port_me ;
	if (!bridge_port_me)
	{
		dbprintf_igmp(LOG_WARNING,"bridge_port_me == NULL\n");
		return -1;
	}
        
	if(igmp_config_g.igmp_function == IGMP_FUNCTION_TRANSPARENT)
	{
		if (igmp_omci_check_msgrate_is_allowed(bridge_port_me, (*igmp_clientinfo)->vid))
		{
			igmp_omci_update_msgrate(bridge_port_me, (*igmp_clientinfo)->vid);
			if (TRUE == igmp_config_g.immediate_leave)
			{
				igmp_mbship_rm_mbship_db_groups_in_client(&igmp_mbship_db_g,*igmp_clientinfo);
				igmp_omci_update_leave(bridge_port_me, *igmp_clientinfo );

				list_for_each_entry_safe( pos, n, &(*igmp_clientinfo)->group_list,  entry_node ) {
					igmp_mbship_del_client (&igmp_mbship_db_g, *igmp_clientinfo, pos);
					igmp_switch_mac_table_client_update(*igmp_clientinfo , &igmp_mbship_db_g, pos);
				}

			}
			if ( (*igmp_clientinfo)->pktinfo->rx_desc.router_mode && omci_env_g.igmp_proxy_snooping )
			{
                                if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                                        igmp_v3_lw_acceptigmp((*igmp_clientinfo)->pktinfo);
                                else
                                        if (igmp_v2_router_leave_forward_check(igmp_clientinfo)) acceptigmp((*igmp_clientinfo)->pktinfo);
			}
			else
			{
				igmp_switch_frame_send(*igmp_clientinfo );
			}
		}
		else
		{
			dbprintf_igmp(LOG_WARNING,"igmp message rate override\n");
			return 0;
		}

	}
	dbprintf_igmp(LOG_WARNING,"bridge membership\n");
	
	return 0;
			
}

static int 
igmp_alu_v3_mbship_report_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	struct igmp_group_t *pos=NULL, *n=NULL;
	unsigned short group_num=0;
	char ip_addr[INET6_ADDRSTRLEN];
    //struct cpuport_info_t *pktinfo = (*igmp_clientinfo)->pktinfo; 
    
	list_for_each_entry_safe(pos, n, &(*igmp_clientinfo)->group_list, entry_node) {
		if (pos->record_type == MODE_IS_EXCLUDE || pos->record_type == CHANGE_TO_EXCLUDE_MODE)
		{
			if (IS_IPV6_ADDR(pos->group_ip) ) 
				util_inet_ntop(AF_INET6, &pos->group_ip, (char *)ip_addr, sizeof(ip_addr) );
			else 
				util_inet_ntop(AF_INET, &pos->group_ip, (char *)ip_addr, sizeof(ip_addr) );
			
			dbprintf_igmp(LOG_WARNING,"join is granted from alu olt %s\n", ip_addr);
			alu_igmp_set_group_signal(&igmp_mbship_db_g ,pos, 1);
		}
		else if (pos->record_type == MODE_IS_INCLUDE || pos->record_type == CHANGE_TO_INCLUDE_MODE )
		{
			if (IS_IPV6_ADDR(pos->group_ip) ) 
				util_inet_ntop(AF_INET6, &pos->group_ip, (char *)ip_addr, sizeof(ip_addr) );
			else 
				util_inet_ntop(AF_INET, &pos->group_ip, (char *)ip_addr, sizeof(ip_addr) );
			dbprintf_igmp(LOG_WARNING,"leave is permitted from alu olt %s\n", ip_addr);
			alu_igmp_set_group_signal(&igmp_mbship_db_g ,pos, 0);
		}
		igmp_switch_mac_table_client_update(*igmp_clientinfo , &igmp_mbship_db_g, pos );
	}
	
	// TODO: Delete igmp_clientinfo only if this IGMP is pure leave message (no any join)
	if (!group_num && FALSE == igmp_config_g.immediate_leave)
	{
		timer_id_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100,(void *)make_igmp_clientinfo_t_copy(*igmp_clientinfo));
		dbprintf_igmp(LOG_WARNING,"IGMP leave: Delete igmp_clientinfo, timer_id_non_fast_leave_g %d\n",timer_id_non_fast_leave_g );
	}
	
	return 0;
}

static int 
igmp_v3_mbship_report_worker(struct igmp_clientinfo_t **igmp_clientinfo)
{
	// TODO: Temporary solution to call v2 join/leave function
	struct igmp_group_t *pos=NULL, *n=NULL;
	unsigned short group_num=0;

	list_for_each_entry_safe( pos, n, &(*igmp_clientinfo)->group_list, entry_node ) {
		if (pos->record_type == MODE_IS_EXCLUDE || pos->record_type == CHANGE_TO_EXCLUDE_MODE )
		{
			dbprintf_igmp(LOG_WARNING,"IGMP V3 join\n");
			igmp_v2_mbship_report_worker(igmp_clientinfo);
			group_num++;
		}
		else if (pos->record_type == MODE_IS_INCLUDE || pos->record_type == CHANGE_TO_INCLUDE_MODE )
		{
			dbprintf_igmp(LOG_WARNING,"IGMP V3 leave pos->group_ip=%x;\n", (unsigned int *)&(pos->group_ip));
			igmp_v2_mbship_leave_worker(igmp_clientinfo);
		}
	}
	
	// TODO: Delete igmp_clientinfo only if this IGMP is pure leave message (no any join)
	if (!group_num && FALSE == igmp_config_g.immediate_leave)
	{
		if (igmp_config_g.last_member_query_interval) 
			timer_id_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100,(void *)*igmp_clientinfo);
		else
			timer_id_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_NONE_FAST_LEAVE, 10000,(void *)make_igmp_clientinfo_t_copy(*igmp_clientinfo));

		dbprintf_igmp(LOG_WARNING,"IGMP leave: Delete igmp_clientinfo, timer_id_non_fast_leave_g %d\n",timer_id_non_fast_leave_g );             
	}
	
	return 0;
}

static int
igmp_main_is_join_from_wan(struct igmp_clientinfo_t *igmp_clientinfo) 
{
	if (igmp_clientinfo->rx_desc.logical_port_id == 5)
	{
		dbprintf_igmp(LOG_WARNING,"drop the join packet from WAN port.\n");
		return -1;
	}
	
	return 0;
}

int 
igmp_main_pkt_process(struct igmp_clientinfo_t *clientinfo)
{
	struct igmp_clientinfo_t **igmp_clientinfo = &clientinfo;
        
        dbprintf_igmp(LOG_WARNING,"igmp_msg_type: 0x%x\n", (*igmp_clientinfo)->igmp_msg_type);
        
	if ((*igmp_clientinfo)->signature != IGMP_CLIENTINFO_SIGNATURE)
	{
		dbprintf_igmp(LOG_ERR,"igmp_clientinfo with invalid signature? ignored!\n");
		return -1;
	}
	
	igmp_v3_lw_router_compatibility_filter(igmp_clientinfo);
	igmp_v3_lw_mbship_report_sanity_check(igmp_clientinfo);
        
        switch ((*igmp_clientinfo)->igmp_msg_type)
	{
	case IGMP_MEMBERSHIP_QUERY:

		dbprintf_igmp(LOG_WARNING,"IGMP_MEMBERSHIP_QUERY\n");
		if ((*igmp_clientinfo)->rx_desc.logical_port_id != switch_get_wan_logical_portid()) break;
		igmp_mbship_query_worker(igmp_clientinfo);
		break;

	case IGMP_V1_MEMBERSHIP_REPORT:

		dbprintf_igmp(LOG_WARNING,"IGMP_V1_MEMBERSHIP_REPORT\n");
		if (igmp_main_is_join_from_wan(*igmp_clientinfo) < 0) break;
		if (omci_env_g.igmp_v1_ignore) break;
		igmp_v1_mbship_report_worker(igmp_clientinfo);
		break;

	case IGMP_V2_MEMBERSHIP_REPORT:
		dbprintf_igmp(LOG_WARNING,"IGMP_V2_MEMBERSHIP_REPORT\n");
		if (igmp_main_is_join_from_wan(*igmp_clientinfo) < 0) break;
		igmp_v2_mbship_report_worker(igmp_clientinfo);
		break;

	case IGMP_V2_LEAVE_GROUP:
			
		dbprintf_igmp(LOG_WARNING,"IGMP_V2_LEAVE_GROUP\n");
		if (igmp_main_is_join_from_wan(*igmp_clientinfo) < 0) break;
		igmp_v2_mbship_leave_worker(igmp_clientinfo);
		if (FALSE == igmp_config_g.immediate_leave)
		{
			timer_id_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100, (void *)make_igmp_clientinfo_t_copy(*igmp_clientinfo));
			dbprintf_igmp(LOG_WARNING,"IGMP leave: Delete igmp_clientinfo, timer_id_non_fast_leave_g %d\n",timer_id_non_fast_leave_g );
		}
		break;

	case IGMP_V3_MEMBERSHIP_REPORT:
			
		if (!(igmp_config_g.igmp_version & IGMP_V3_BIT) && !omci_env_g.olt_alu_mcast_support)
		{
			dbprintf_igmp(LOG_WARNING,"IGMPv2 Only.IGMPv3 packets discarded\n");
			break;
                }
			
		dbprintf_igmp(LOG_WARNING,"IGMP_V3_MEMBERSHIP_REPORT\n");
		if (igmp_main_is_join_from_wan(*igmp_clientinfo) < 0 && omci_env_g.olt_alu_mcast_support)
		{
			igmp_alu_v3_mbship_report_worker(igmp_clientinfo);
			break;
		}
			
		if (igmp_main_is_join_from_wan(*igmp_clientinfo) < 0) break;
		igmp_v3_lw_mbship_report_worker(igmp_clientinfo);
                //igmp_v3_mbship_report_worker(igmp_clientinfo);
		break;

	case MLD_LISTENER_QUERY:

		dbprintf_igmp(LOG_WARNING,"MLD_LISTENER_QUERY\n");
		if ((*igmp_clientinfo)->rx_desc.logical_port_id != switch_get_wan_logical_portid()) break;
		igmp_mbship_mld_query_worker(igmp_clientinfo);
		break;
		
	case MLD_LISTENER_REPORT:
		
		dbprintf_igmp(LOG_WARNING,"MLD_LISTENER_REPORT\n");
		if (igmp_main_is_join_from_wan(*igmp_clientinfo) < 0) break;
		igmp_v2_mbship_report_worker(igmp_clientinfo);
		break;

	case MLD_LISTENER_REDUCTION:
			
		dbprintf_igmp(LOG_WARNING,"MLD_LISTENER_REDUCTION\n");
		if (igmp_main_is_join_from_wan(*igmp_clientinfo) < 0) break;
		igmp_v2_mbship_leave_worker(igmp_clientinfo);
		if (FALSE == igmp_config_g.immediate_leave)
		{
			timer_id_non_fast_leave_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_NONE_FAST_LEAVE, igmp_config_g.last_member_query_interval*100,(void *)make_igmp_clientinfo_t_copy(*igmp_clientinfo));
			dbprintf_igmp(LOG_WARNING,"MLD leave: Delete igmp_clientinfo\n");
		}
		break;

	case MLD_V2_LISTENER_REPORT:
			
		dbprintf_igmp(LOG_WARNING,"MLD_V2_LISTENER_REPORT\n");
		if (igmp_main_is_join_from_wan(*igmp_clientinfo) < 0) break;
		igmp_v3_mbship_report_worker(igmp_clientinfo);
		break;

	default:
		dbprintf_igmp(LOG_WARNING,"default return -1\n");
		return -1;
	}
	
	return 0;
}

