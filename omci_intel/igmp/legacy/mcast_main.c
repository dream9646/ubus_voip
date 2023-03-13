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
 * File    : igmp_main.c
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
#include "igmp_v2.h"

int timer_id_query_interval_g = 0;
int timer_id_non_fast_leave_g = 0;
int timer_id_router_non_fast_leave_g = 0;
int query_resp_timeout_id =0;
extern unsigned char p2p_cfg_g;
extern uint32_t allhosts_group;          /* All hosts addr in net order */

static int
igmp_main_clientinfo_free(struct igmp_clientinfo_t *igmp_clientinfo)
{
	if ( igmp_clientinfo == NULL )
		return 0;
        
	if (igmp_clientinfo->signature == 0) {
		dbprintf_igmp(LOG_ERR,"igmp_clientinfo already free? ignored!\n");
		return 0;
	}
	if (igmp_clientinfo->signature != IGMP_CLIENTINFO_SIGNATURE) {
		dbprintf_igmp(LOG_ERR,"igmp_clientinfo with invalid signature? ignored!\n");
		return -1;
	}

	if ( !list_empty(&igmp_clientinfo->group_list)  ) {
		struct igmp_group_t *group_pos=NULL, *group_n=NULL;

		list_for_each_entry_safe( group_pos, group_n, &igmp_clientinfo->group_list, entry_node) {
			list_del(&group_pos->entry_node);
			free_safe(group_pos);
		}
	}

	igmp_clientinfo->signature = 0;	// clear signature
        
	free_safe(igmp_clientinfo);
        
	return 0;
}

static int 
igmp_main_norlmal_leave_update(void *data_ptr)
{
	struct igmp_clientinfo_t *igmp_clientinfo = (struct igmp_clientinfo_t *)data_ptr; 
	struct me_t *bridge_port_me = igmp_clientinfo->rx_desc.bridge_port_me ;
	struct igmp_group_t *pos, *n;

	if (!bridge_port_me) {
		dbprintf_igmp(LOG_WARNING,"bridge_port_me == NULL\n");
		return -1;
	}

	// ugly check0, free already
	if (igmp_clientinfo->signature == 0) {
		dbprintf_igmp(LOG_ERR,"igmp_clientinfo already free? ignored!\n");
		return -1;
	}
	// ugly check1, invalid signature, corrupt?
	if (igmp_clientinfo->signature != IGMP_CLIENTINFO_SIGNATURE) {
		dbprintf_igmp(LOG_ERR,"igmp_clientinfo with invalid signature(0x%x)? ignored!\n", igmp_clientinfo->signature);
		return -1;
	}
	// ugly check2, corrupt?
	if (igmp_clientinfo->group_list.next == 0 || igmp_clientinfo->group_list.prev ==0) {
		dbprintf_igmp(LOG_ERR,"igmp_clientinfo with invalid group_list? ignored!\n");
		return -1;
	}
		
	igmp_mbship_rm_client_groups_in_mbship_db(&igmp_mbship_db_g , igmp_clientinfo, igmp_config_g.last_member_query_interval*100);
	
	igmp_mbship_rm_mbship_db_groups_in_client( &igmp_mbship_db_g, igmp_clientinfo );
	igmp_omci_update_leave( bridge_port_me, igmp_clientinfo );
	list_for_each_entry_safe( pos, n, &igmp_clientinfo->group_list, entry_node ) {
		igmp_mbship_del_client (&igmp_mbship_db_g, igmp_clientinfo, pos);
		igmp_switch_mac_table_client_update(igmp_clientinfo , &igmp_mbship_db_g, pos);
	}
	igmp_main_clientinfo_free(igmp_clientinfo);
	return 0;
}

struct igmp_pkt_stage_t {
	int (*stage_parse_handler)(struct igmp_clientinfo_t *igmp_clientinfo);
        int (*stage_tagfilter_handler)(struct igmp_clientinfo_t *igmp_clientinfo);
        int (*stage_bridge_handler)(struct igmp_clientinfo_t *igmp_clientinfo);
};

static int 
igmp_process_main( struct cpuport_info_t *pktinfo ) 
{
	struct igmp_clientinfo_t *igmp_clientinfo=NULL;
        
        struct igmp_pkt_stage_t sm = {
               .stage_parse_handler = NULL,
               .stage_tagfilter_handler = NULL,
               .stage_bridge_handler = NULL
        };
        
        int state = 0, ret = 0;
        
        dbprintf_igmp(LOG_INFO,"pktinfo rx logical port %d\n", pktinfo->rx_desc.logical_port_id);
        	
	igmp_clientinfo = malloc_safe(sizeof(struct igmp_clientinfo_t));
        
        if (igmp_clientinfo == NULL)
        {
                dbprintf_igmp(LOG_ERR, "igmp_clientinfo allocate failed!\n");
                return -1;
        }
        
        memset(igmp_clientinfo, 0, sizeof(struct igmp_clientinfo_t));
        igmp_clientinfo->pktinfo = pktinfo; 
	INIT_LIST_HEAD(&igmp_clientinfo->group_list);
	igmp_clientinfo->signature = IGMP_CLIENTINFO_SIGNATURE;	// mark signature, signature means this igmp_clientinfo is not free

	if ( pktinfo->rx_desc.router_mode && omci_env_g.igmp_proxy_snooping )
        {
                sm.stage_parse_handler = igmp_pkt_parse_clientinfo_router_mode;
                sm.stage_bridge_handler = igmp_main_pkt_process;
        }
        else
        {
                sm.stage_parse_handler = igmp_pkt_parse_clientinfo;
                sm.stage_tagfilter_handler = igmp_pkt_tag_filter_iteration;                                
                sm.stage_bridge_handler = igmp_main_pkt_process;
        }
        
	state = IGMP_PKT_PARSE_CLIENTINFO;
        IGMP_PKT_STATE_SET(igmp_clientinfo, state);
        while (state != IGMP_PKT_DONE) {
                ret = 0;
                switch (state)
                {
                case IGMP_PKT_PARSE_CLIENTINFO:
                        if (sm.stage_parse_handler) ret = sm.stage_parse_handler(igmp_clientinfo);
                        IGMP_PKT_STATE_RESULT_SET(igmp_clientinfo, (ret == 0)?CONTINUE:STOP)       
                        break;
                case IGMP_PKT_TAGFILTER:
                        if (sm.stage_tagfilter_handler) ret = sm.stage_tagfilter_handler(igmp_clientinfo);
                        IGMP_PKT_STATE_RESULT_SET(igmp_clientinfo, (ret == 0)?CONTINUE:STOP)
                        break;
                case IGMP_PKT_BRIDGE:
                        if (sm.stage_bridge_handler) ret = sm.stage_bridge_handler(igmp_clientinfo);
                        IGMP_PKT_STATE_RESULT_SET(igmp_clientinfo, (ret == 0)?CONTINUE:STOP)
                        break;
                default:
                        break;        
                }

		struct igmp_counter_module_t *m=NULL;
        
        	list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        		m->igmp_counter_handler(igmp_clientinfo);            
		}
                
                if (igmp_clientinfo->state.exec_ret == STOP) break;
                
                state++;    // advance to next state
                IGMP_PKT_STATE_SET(igmp_clientinfo, state);     
	}        
        
	igmp_main_clientinfo_free(igmp_clientinfo);
        
	return 0;
}

static void
igmp_main_pktinfo_free( struct cpuport_info_t *pktinfo)
{
	if (!pktinfo)
		return;
		
	util_get_uptime(&pktinfo->forward_time);
	if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > 5*1E6)
		dbprintf_cpuport(LOG_ERR, "packet is processed for more than 5 sec, system busy?\n");

	if (pktinfo->buf_len > CPUPORT_BUF_SIZE || 
	    pktinfo->buf_len < 64 ||
	    pktinfo->frame_len > CPUPORT_BUF_SIZE || 
	    pktinfo->frame_len > pktinfo->buf_len || 
	    pktinfo->frame_ptr < pktinfo->buf_ptr) {
	    	// it seems when task is terminated, an invalid pkt returned from fwk_msgq_recv()?
		dbprintf_cpuport(LOG_ERR, "packt pktinfo 0x%p is invalid! (buf_ptr %p, buf_len %d, frame_ptr %p, frame_len %d\n",
			pktinfo, pktinfo->buf_ptr, pktinfo->buf_len, pktinfo->frame_ptr, pktinfo->frame_len);
		return;
	}
		
	if (omci_env_g.cpuport_history_enable) cpuport_history_sync(pktinfo);
	if (pktinfo->buf_ptr) free_safe(pktinfo->buf_ptr);
	free_safe(pktinfo);
}

static int handle_igmp_cmd_qid()
{
        int ret;
        struct list_head *list_node=NULL;
        	
        if ((ret = fwk_msgq_set_isset(igmp_qsetid_g, igmp_cmd_qid_g)) < 0) {
                dbprintf_igmp(LOG_ERR, "igmp_cmd_qid_g isset err(%d)\n",ret);
	} else if (ret > 0) {
                if ((ret = fwk_msgq_recv(igmp_cmd_qid_g, &list_node)) < 0)
                {
		      dbprintf_igmp(LOG_ERR, "igmp_cmd_qid_g recv err(%d)\n",ret);
                }
                else
                {
		      struct igmp_cmd_msg_t *igmp_cmd_msg;
		      igmp_cmd_msg = list_entry(list_node, struct igmp_cmd_msg_t, q_entry);

		      if (omci_env_g.task_pause_mask & (1 << ENV_TASK_NO_IGMP)) {
			      if (igmp_cmd_msg)
				      free_safe(igmp_cmd_msg);
			      return 0;
		      }

		      omci_env_g.taskstate[ENV_TASK_NO_IGMP] = 1;

		      switch(igmp_cmd_msg->cmd){
                        case IGMP_CMD_ME_ADD_MSG:
                        case IGMP_CMD_ME_MODIFY_MSG:
                        case IGMP_CMD_ME_DEL_MSG:
			     igmp_omci_read_igmp_profile_me(&igmp_config_g, NULL, -1);
			     break;
                        case IGMP_CMD_ME_RELOAD:
			     igmp_omci_read_igmp_profile_me(&igmp_config_g, NULL, -1);
			     igmp_omci_check_mbships();
			     break;
                        case IGMP_CMD_TERMINATE:
                             igmp_omci_rm_all_active_group_list_group();
			     break;
			default:
			     dbprintf_igmp(LOG_ERR, "wrong IGMP msg cmd\n");
		      }
					 
		      free_safe(igmp_cmd_msg);
		}
        }
        
        return ret;
}

static int handle_igmp_query_qid()
{
        int ret;
        struct cpuport_info_t *pktinfo=NULL;
        struct list_head *list_node=NULL;
        
        if ( (ret = fwk_msgq_set_isset(igmp_qsetid_g, igmp_query_qid_g)) < 0 )
        {
	       dbprintf_igmp(LOG_ERR, "igmp_query_qid_g isset err:%d \n",ret);
	}
        else if (ret > 0 )
        {
                while ((ret = fwk_msgq_recv(igmp_query_qid_g, &list_node))==0)
                {
                        pktinfo = list_entry(list_node, struct cpuport_info_t, node);
                        
                        if ( pktinfo == NULL || pktinfo->rx_desc.bridge_port_me == NULL )
                                dbprintf_igmp(LOG_WARNING, "pktinfo is NULL or Bridge port me not found.\n");
			else
                        {
                                struct ip *ip;

				if (omci_env_g.task_pause_mask & (1 << ENV_TASK_NO_IGMP)) {
					igmp_main_pktinfo_free(pktinfo);
					continue;
				}

				omci_env_g.taskstate[ENV_TASK_NO_IGMP] = 1;

				pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_IGMP];
				util_get_uptime(&pktinfo->process_time);
				if (omci_env_g.cpuport_history_enable)
					cpuport_history_add(pktinfo);

                                ip = (struct ip *)pktinfo->frame_ptr;
                                if ( ip->ip_dst.s_addr == allhosts_group) 
                                        if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                                                igmp_v3_lw_acceptigmp(pktinfo);
                                        else
                                                acceptigmp(pktinfo);
				else
					igmp_process_main( pktinfo ); 
			}
			igmp_main_pktinfo_free( pktinfo);
                }
        }
        return ret;
}

static void dodbclear()
{
	struct igmp_mbship_t *mbship_pos, *mbship_n;
	int doclear = 0;
	list_for_each_entry_safe(mbship_pos, mbship_n, &igmp_mbship_db_g.mbship_list, entry_node) {
		if (igmp_config_g.igmp_version & IGMP_V3_BIT)	// v3 shouldn't have v2
		{
			if (mbship_pos->lw_igmpv3_signature != IGMP_V3_LW_SIGNATURE)
			{
				doclear = 1;
				break;
			}
		}
		else	// v2 shouldn't have v3
		{
			if (mbship_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE)
			{
				doclear = 1;
				break;
			}
		}			
	}
	
	if (doclear)
	{
		if (igmp_config_g.igmp_version & IGMP_V3_BIT)
		{
			igmp_mbship_db_deinit( &igmp_mbship_db_g );
		}
		else
		{
			igmp_v3_lw_mbship_db_deinit();
			mcast_proxy_db_deinit();
		}			
	}
}

static int handle_igmp_timer_qid()
{
        int ret;
        struct fwk_timer_msg_t *timer_msg=NULL;
        struct list_head *list_node=NULL;
        
        if ( (ret = fwk_msgq_set_isset(igmp_qsetid_g, igmp_timer_qid_g)) < 0)
        {
                dbprintf_igmp(LOG_ERR, "igmp_timer_qid_g isset err(%d) \n",ret);
	}
        else if (ret > 0)
        {
                if ((ret = fwk_msgq_recv(igmp_timer_qid_g, &list_node)) < 0)
                        dbprintf_igmp(LOG_ERR, "igmp_timer_qid_g recv err: %d\n",ret);
		else
                {
                        timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);

			if (omci_env_g.task_pause_mask & (1 << ENV_TASK_NO_IGMP)) {
				if (timer_msg)
					free_safe(timer_msg);
				return 0;
			}

			omci_env_g.taskstate[ENV_TASK_NO_IGMP] = 1;

			if (timer_msg->event == TIMER_EVENT_QUERY_INTERVAL )
                        {
                                // make sure returned timer id is the last one set
                                if (timer_msg->timer_id != timer_id_query_interval_g)
                                {
                                        dbprintf_igmp(LOG_ERR, "timer_id_query_interval_g mismatch(msg=%d, expect=%d), ignored\n",timer_msg->timer_id, timer_id_query_interval_g);
                                        free_safe(timer_msg);
                                        return ret;
                                }
                                
                                if (igmp_config_g.query_interval > 0) {
                                        timer_id_query_interval_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_QUERY_INTERVAL, igmp_config_g.query_interval*1000, NULL);
                                }
                                if ( timer_id_query_interval_g < 0 )
                                        dbprintf_igmp(LOG_ERR, "fwk_timer_create return(%d)\n", timer_id_query_interval_g );
				
                                if ( omci_env_g.igmp_proxy_snooping || IGMP_FUNCTION_PROXY == igmp_config_g.igmp_function ) {
                                        if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                                                sendigmpv3generalquery();
                                        else
                                                sendgeneralmembershipquery();
                                }
                                query_resp_timeout_id = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_QUERY_TIMEOUT_UPDATE, igmp_config_g.max_respon_time*100, NULL);
                                if ( query_resp_timeout_id < 0 )
                                        dbprintf_igmp(LOG_ERR, "fwk_timer_create return(%d)\n", query_resp_timeout_id );
                        }
                        else if ( timer_msg->event == TIMER_EVENT_QUERY_TIMEOUT_UPDATE )
                        {
				dodbclear();
				
				if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                                        igmp_mbship_lw_v3_timeout_update_routine(NULL);
                                else
                                        igmp_mbship_timeout_update_routine(NULL);
                        }
			else if ( timer_msg->event == TIMER_EVENT_NONE_FAST_LEAVE )
			{
                                if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                                        igmp_v3_lw_main_normal_leave_update(timer_msg->user_data);
                                else
                                        igmp_main_norlmal_leave_update(timer_msg->user_data);
                        }
                        else if ( timer_msg->event == TIMER_EVENT_ROUTER_NONE_FAST_LEAVE ) {
                                if ( omci_env_g.igmp_proxy_snooping )
                                        sendgroupspecificmemberquery(timer_msg->user_data); 
			}
			else if (timer_msg->event == TIMER_EVENT_ROUTER_COMPATIBITY)
			{
                                struct proxy_hpt_msg hpt_msg;
                                hpt_msg.timer_id = timer_msg->timer_id;
                                hpt_msg.mcastaddr = (uint32_t)timer_msg->user_data;
                                if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                                        igmp_proxy_v3_compatibility_update(&hpt_msg);		     
                        }
			else if (timer_msg->event & TIMER_EVENT_CUSTOM)
				igmp_custom_timer_handler(timer_msg); // call registered callbacks
			else
                                dbprintf_igmp(LOG_ERR, "fwk_timer unknown event(%d)\n", timer_msg->event );    
                                
                        free_safe(timer_msg);
		}
	}
	return ret;
}

static unsigned char macaddr_zero[6] = {0,0,0,0,0,0};

static int
igmp_switch_preassign_rewrite(void)
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

static int handle_igmp_join_leave_qid()
{
        int ret;
        struct cpuport_info_t *pktinfo=NULL;
        struct switch_port_info_t port_info;
        struct list_head *list_node=NULL;
        
        // receive igmp join/leave packets
	if ( (ret = fwk_msgq_set_isset(igmp_qsetid_g, igmp_join_leave_qid_g)) < 0 ){
                dbprintf_igmp(LOG_ERR, "igmp_join_leave_qid_g isset err:%d\n",ret);
	}
        else if (ret > 0 )
        { 
		while ((ret = fwk_msgq_recv(igmp_join_leave_qid_g, &list_node)) == 0)
                {
                        pktinfo = list_entry(list_node, struct cpuport_info_t, node);

			if (omci_env_g.task_pause_mask & (1 << ENV_TASK_NO_IGMP)) {
				igmp_main_pktinfo_free(pktinfo);
				continue;
			}

			omci_env_g.taskstate[ENV_TASK_NO_IGMP] = 1;

			pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_IGMP];
			util_get_uptime(&pktinfo->process_time);
			if (omci_env_g.cpuport_history_enable)
				cpuport_history_add(pktinfo);
                        
                        // this is dumb. we have no way of knowing l2-table being tempered.
                        // rewrite preassign list every time
                        igmp_switch_preassign_rewrite();
                        
			if ( pktinfo->rx_desc.bridge_port_me == NULL && pktinfo->rx_desc.router_mode == 1 )
                        {
                                if ( omci_env_g.igmp_proxy_snooping == 1 ) {
                                	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
        				{
        					dbprintf_igmp(LOG_ERR, "pure proxy without snoopying with calix OLT!\n");
					}
					else
					{
	                                        if (igmp_config_g.igmp_version & IGMP_V3_BIT)
	                                                igmp_v3_lw_acceptigmp(pktinfo);
	                                        else
	                                                acceptigmp(pktinfo);
                                        }
				} else if( omci_env_g.igmp_proxy_snooping == 2 ){
                                        dbprintf_igmp(LOG_WARNING,"rx logical port id:%d\n",pktinfo->rx_desc.logical_port_id);
					struct ip *ip = (struct ip *)pktinfo->frame_ptr;
					if (ip->ip_p == 0 || ip->ip_src.s_addr == allhosts_group) {
				                if (igmp_config_g.igmp_version & IGMP_V3_BIT)
                                                        igmp_v3_lw_acceptigmp(pktinfo);
                                                else
                                                        acceptigmp(pktinfo);
					} else {
						pktinfo->rx_desc.bridge_port_me = igmp_router_bridge_port_me_of_lan_get(pktinfo->rx_desc.logical_port_id);
                                                if (pktinfo->rx_desc.bridge_port_me)
                                                        igmp_process_main( pktinfo ); 
						else 
							dbprintf_igmp(LOG_WARNING,"port %d ME not found\n",pktinfo->rx_desc.logical_port_id);
					}
				}
                        }
                        else if (pktinfo->rx_desc.router_mode == 0 && omci_env_g.igmp_proxy_snooping)
                        {
                        	dbprintf_igmp(LOG_WARNING,"route mode but packet is not router mode\n");
			}
                        else if (pktinfo->rx_desc.bridge_port_me == NULL )
                        {
                                dbprintf_igmp(LOG_WARNING, "Bridge port me not found.\n");
			}
                        else
                        {
                                if ( pktinfo->rx_desc.logical_port_id == 5 )
                                {
				}
                                else if ( pktinfo->rx_desc.logical_port_id !=6 )
                                {
                                        if (!omci_env_g.igmp_proxy_snooping )
                                        {
                                                pktinfo->rx_desc.p2p_cfg = 0xff;
						if ( omci_env_g.port2port_enable )
                                                {
                                                        struct switch_mac_tab_entry_t mac_tab_entry;
                                                        memset(&mac_tab_entry,0,sizeof(struct switch_mac_tab_entry_t ));
							//mac_tab_entry.vid = (omci_env_g.ivlan_enable) ? 4095 : 0;
							mac_tab_entry.vid = 0;
                                                        memcpy(&mac_tab_entry.mac_addr, pktinfo->frame_ptr+6,6) ;
                                                        switch_hw_g.mac_entry_get(&mac_tab_entry);
									
							int i = 0;
							for ( i = 0 ; i< 12; i++) {
                                                                if ( mac_tab_entry.port_bitmap & (1 << i )) {
								        pktinfo->rx_desc.p2p_cfg = i;
                                                                        p2p_cfg_g = i;
									break;
								}
                                                        }
							if ( pktinfo->rx_desc.p2p_cfg ==6 ) 
                                                                pktinfo->rx_desc.p2p_cfg = 8;
                                                        pktinfo->rx_desc.logical_port_id = 0;
                                                        switch_get_port_info_by_logical_port_id( pktinfo->rx_desc.logical_port_id , &port_info );
							pktinfo->rx_desc.bridge_port_me = port_info.bridge_port_me;

                                                        if (!pktinfo->rx_desc.bridge_port_me) {
                                                                dbprintf_igmp(LOG_WARNING, "Fail to get bridge port descriptor!\n");
								igmp_main_pktinfo_free( pktinfo);
								continue;
							}
						}
                                                else
                                                {
                                                        switch_get_port_info_by_logical_port_id( pktinfo->rx_desc.logical_port_id , &port_info );
                                                        pktinfo->rx_desc.bridge_port_me = port_info.bridge_port_me;
                                                        if (!pktinfo->rx_desc.bridge_port_me) {
                                                                dbprintf_igmp(LOG_WARNING, "Fail to get bridge port descriptor!\n");
								igmp_main_pktinfo_free( pktinfo);
								continue;
							}
						}
					}
				}
						
				igmp_process_main( pktinfo ); 
			}
			igmp_main_pktinfo_free( pktinfo);
		}
        }
        
        return ret;
}

void *
igmp_main(void *ptr)
{
	long curtime;//, lasttime, difftime;
	int num;
//	int query_resp_timeout_id = -1;
	
	util_set_thread_name("omci-igmp");
	omci_env_g.taskname[ENV_TASK_NO_IGMP] = "IGMP";
	omci_env_g.taskid[ENV_TASK_NO_IGMP] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_IGMP] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_IGMP] = 0;
	omci_env_g.taskts[ENV_TASK_NO_IGMP] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_IGMP] = 0; //idle

	curtime = igmp_timer_get_cur_time();
	igmp_omci_read_igmp_profile_me(&igmp_config_g, NULL, -1);


	if (igmp_config_g.query_interval > 0)
		timer_id_query_interval_g = fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_QUERY_INTERVAL, igmp_config_g.query_interval*1000, NULL);
//	fwk_timer_create(igmp_timer_qid_g, TIMER_EVENT_PREVIEW_CYCLE, PREVIEW_CYCLE*1000, NULL);

	while(igmpproxytask_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_IGMP]++;
		omci_env_g.taskts[ENV_TASK_NO_IGMP] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_IGMP] = 0; //idle 

		if ((num = fwk_msgq_select(igmp_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) <= 0)
			continue;

		// receive igmp join/leave packets
		handle_igmp_join_leave_qid();

		// receive igmp query packets
		handle_igmp_query_qid();

		// receive timer timeout message
		handle_igmp_timer_qid();

		handle_igmp_cmd_qid();
	}

        igmp_mbship_db_deinit( &igmp_mbship_db_g );
	igmp_v3_lw_mbship_db_deinit();
	mcast_proxy_db_deinit();

	struct igmp_counter_module_t *m=NULL, *pos=NULL;
        list_for_each_entry_safe(m, pos,  &igmp_config_g.m_counter_list, m_node) {
        	list_del(&m->m_node);        
        }
			
	omci_env_g.taskstate[ENV_TASK_NO_IGMP] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_IGMP] = 0;
	
	dbprintf_igmp(LOG_ERR, "task terminate\n");
	return 0;
}
