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
 * File    : igmp_mbship.c
 *
 ******************************************************************/

#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>


#include "list.h"
#include "util.h"
#include "util_inet.h"
#include "meinfo.h"
#include "hwresource.h"
#include "env.h"

#include "switch.h"
#include "mcast_const.h"
#include "mcast.h"
#include "cpuport.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "switch.h"
#include "mcast_switch.h"
#include "mcast_timer.h"
#include "igmp_omci.h"
#include "mcast_router.h"

#include "mcast_snoop.h"

struct igmp_mbship_db_t igmp_mbship_db_g;


short alu_igmp_get_group_signal(struct igmp_mbship_db_t *igmp_mbship_db, union ipaddr *igmp_group)
{ 
	struct igmp_alu_group_t *alu_pos=NULL, *alu_n=NULL;
	
	list_for_each_entry_safe(alu_pos, alu_n, &igmp_mbship_db->alu_signal_list, entry_node) {
		if (igmp_pkt_ip_cmp(igmp_group, &alu_pos->group_ip))
			continue;
		
		return alu_pos->signal;
	}
	
	return 0;   
}

void alu_igmp_set_group_signal(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_group_t *igmp_group, short signal)
{ 
	struct igmp_alu_group_t *alu_pos=NULL, *alu_n=NULL;
	
	list_for_each_entry_safe(alu_pos, alu_n, &igmp_mbship_db->alu_signal_list, entry_node) {
		if (igmp_pkt_ip_cmp(&igmp_group->group_ip, &alu_pos->group_ip))
			continue;
		
		alu_pos->signal = signal;
	} 
}

void alu_igmp_update_group(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_group_t *igmp_group)
{
	struct igmp_alu_group_t *alu_pos=NULL, *alu_n=NULL;
	struct igmp_alu_group_t *g;
	
	list_for_each_entry_safe(alu_pos, alu_n, &igmp_mbship_db->alu_signal_list, entry_node) {
		if (igmp_pkt_ip_cmp(&igmp_group->group_ip, &alu_pos->group_ip) == 0)
			return;
	}
	// new group
	g = malloc_safe(sizeof(struct igmp_alu_group_t));
	*(unsigned int *)&g->group_ip = *(unsigned int *)&igmp_group->group_ip;
	igmp_pkt_ip_cpy(&g->group_ip, &igmp_group->group_ip);
	g->signal = 0;    
	list_add(&g->entry_node, &igmp_mbship_db->alu_signal_list);      
}

int
igmp_mbship_db_init(struct igmp_mbship_db_t *igmp_mbship_db)
{
	INIT_LIST_HEAD(&igmp_mbship_db->mbship_list);
	INIT_LIST_HEAD(&igmp_mbship_db->preview_acl_list);
	INIT_LIST_HEAD(&igmp_mbship_db->apgt_list);
	INIT_LIST_HEAD(&igmp_mbship_db->alu_signal_list);
	INIT_LIST_HEAD(&igmp_mbship_db->mcast_snoop_list);
	INIT_LIST_HEAD(&igmp_mbship_db->mcast_proxy_list);
//      pthread_mutex_init(&igmp_fdb->igmp_fdb_mutex , NULL); 
	return 0;
}

int
igmp_mbship_db_deinit(struct igmp_mbship_db_t *igmp_mbship_db)
{
	struct igmp_mbship_preview_acl_t *preview_acl_pos, *preview_acl_n;
	struct igmp_mbship_apgt_t *apgt_pos,*apgt_n;
	struct igmp_mbship_t *mbship_pos, *mbship_n;

	list_for_each_entry_safe( preview_acl_pos, preview_acl_n, &igmp_mbship_db->preview_acl_list, entry_node ){
		list_del(&preview_acl_pos->entry_node);
		free_safe(preview_acl_pos);
	}
	list_for_each_entry_safe( apgt_pos, apgt_n, &igmp_mbship_db->apgt_list, entry_node ){
		list_del(&apgt_pos->entry_node);
		free_safe(apgt_pos);
	}
	
	igmp_omci_remove_bridge_port_rate_list();
	
	list_for_each_entry_safe( mbship_pos, mbship_n, &igmp_mbship_db->mbship_list, entry_node ){
		if (mbship_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
                list_del(&mbship_pos->entry_node);
		free_safe(mbship_pos);
	}
	return 0;
}

struct igmp_mbship_preview_acl_t *
igmp_mbship_search_preview_acl_list(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group)
{
	struct igmp_mbship_preview_acl_t *pos, *n;
	list_for_each_entry_safe(pos, n, &igmp_mbship_db->preview_acl_list, entry_node){
		if (igmp_pkt_ip_cmp(&pos->group_ip,&igmp_group->group_ip))
			continue;
		if (igmp_pkt_ip_cmp(&pos->src_ip,&igmp_group->src_ip))
			continue;
		if (igmp_pkt_ip_cmp(&pos->client_ip,&igmp_clientinfo->client_ip))
			continue;
		return pos;
	}
	return NULL;
}

struct igmp_mbship_apgt_t *
igmp_mbship_search_apgt_list(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group)
{
	struct igmp_mbship_apgt_t *pos, *n;
	list_for_each_entry_safe( pos, n, &igmp_mbship_db->preview_acl_list, entry_node){
		if (igmp_pkt_ip_cmp(&pos->group_ip,&igmp_group->group_ip))
			continue;
		if (igmp_pkt_ip_cmp(&pos->src_ip,&igmp_group->src_ip))
			continue;
		if(igmp_clientinfo->vid != pos->uni_vid)
			continue;
		return pos;
	}
	return NULL;
}

int
igmp_mbship_add_client(struct igmp_mbship_db_t *igmp_mbship_db , struct igmp_clientinfo_t *igmp_clientinfo , struct igmp_group_t *igmp_group)
{
	struct igmp_mbship_t *igmp_mbship=NULL;

	igmp_mbship = malloc_safe(sizeof(struct igmp_mbship_t));
	igmp_mbship->src_port_meid = igmp_clientinfo->rx_desc.bridge_port_me->meid;
	igmp_mbship->logical_port_id = igmp_clientinfo->rx_desc.logical_port_id;
	igmp_mbship->p2p_cfg = igmp_clientinfo->rx_desc.p2p_cfg;
	igmp_mbship->uni_vid = igmp_clientinfo->vid;
        igmp_mbship->ani_vid = igmp_clientinfo->ani_vid;

	igmp_pkt_ip_cpy(&igmp_mbship->group_ip, &igmp_group->group_ip);
	igmp_pkt_ip_cpy(&igmp_mbship->src_ip, &igmp_group->src_ip);
	igmp_pkt_ip_cpy(&igmp_mbship->client_ip, &igmp_clientinfo->client_ip);
	if (omci_env_g.igmp_proxy_snooping == 0) {
		memcpy(igmp_mbship->client_mac, igmp_clientinfo->client_mac , sizeof(unsigned char)*6);
	}
	igmp_mbship->allowed_type = igmp_group->allowed_type;
	igmp_mbship->access_time = igmp_clientinfo->access_time;

	if (igmp_mbship->allowed_type == PREVIEW_ACL_AUTH) {
		struct igmp_mbship_preview_acl_t *preview_client=NULL;
		preview_client = igmp_mbship_search_preview_acl_list(igmp_mbship_db, igmp_clientinfo, igmp_group);
		if (preview_client == NULL)
		{
			preview_client = malloc_safe(sizeof(struct igmp_mbship_preview_acl_t));
			preview_client->last_join_time = igmp_mbship->access_time; 
			preview_client->repeat_count = 1; 
			preview_client->preview_len = igmp_group->preview_length; 
			igmp_pkt_ip_cpy(&preview_client->group_ip, &igmp_group->group_ip);
			igmp_pkt_ip_cpy(&preview_client->src_ip, &igmp_group->src_ip);
			igmp_pkt_ip_cpy(&preview_client->client_ip, &igmp_clientinfo->client_ip);
			list_add( &preview_client->entry_node, &igmp_mbship_db->preview_acl_list );
		}
		if (preview_client != NULL)
		{
			if ((igmp_timer_get_cur_time()-preview_client->last_join_time)> preview_client->preview_len) 
				preview_client->repeat_count++;
		}
	}

	if (igmp_mbship->allowed_type == ALLOWED_PREVIEW_GROUP)
	{
		struct igmp_mbship_apgt_t *apgt_client=NULL;
		apgt_client = igmp_mbship_search_apgt_list( igmp_mbship_db, igmp_clientinfo, igmp_group);	
		if (apgt_client == NULL) return 0;
	}

	INIT_LIST_NODE(&igmp_mbship->entry_node);
	list_add( &igmp_mbship->entry_node, &igmp_mbship_db->mbship_list );
	
	return MBSHIP_CLIENT_ADD_OK;
}


int 
igmp_mbship_del_client (struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group) 
{
	struct igmp_mbship_t *mbship_pos=NULL, *mbship_n=NULL;

	if (igmp_mbship_db == NULL)
	{
		dbprintf_igmp(LOG_WARNING,"igmp_mbship_db == NULL\n");
		return MBSHIP_CLIENT_DEL_NOT_EXIST; 
	}

	if (igmp_clientinfo == NULL)
	{
		dbprintf_igmp(LOG_WARNING,"igmp_clientinfo == NULL\n");
		return MBSHIP_CLIENT_DEL_NOT_EXIST; 
	}

	list_for_each_entry_safe(mbship_pos, mbship_n, &igmp_mbship_db->mbship_list, entry_node) {

		if (mbship_pos->src_port_meid != igmp_clientinfo->rx_desc.bridge_port_me->meid)
			continue;
		if (mbship_pos->logical_port_id != igmp_clientinfo->rx_desc.logical_port_id)
			continue;
		if (mbship_pos->uni_vid != igmp_clientinfo->vid) 
			continue;
		if (igmp_pkt_ip_cmp(&mbship_pos->group_ip,&igmp_group->group_ip))
			continue;
		if (igmp_pkt_ip_cmp(&mbship_pos->src_ip,&igmp_group->src_ip))
			continue;
		if (igmp_pkt_ip_cmp(&mbship_pos->client_ip,&igmp_clientinfo->client_ip))
			continue;
		if (omci_env_g.igmp_proxy_snooping == 0)
		{
			if (memcmp(mbship_pos->client_mac, igmp_clientinfo->client_mac, sizeof(unsigned char)*6 ))
				continue;
		}
		
                if (mbship_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
                
		list_del(&mbship_pos->entry_node);
		free_safe(mbship_pos);
	}
	return MBSHIP_CLIENT_DEL_OK;
}

// remove client groups that are already in the mbship db
int
igmp_mbship_rm_client_groups_in_mbship_db(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo, long timeout)
{
	if (igmp_mbship_db == NULL)
	{
		dbprintf_igmp(LOG_WARNING,"igmp_mbship_db == NULL\n");
		return 0;
	}

	if (igmp_clientinfo == NULL)
	{
		dbprintf_igmp(LOG_WARNING,"igmp_clientinfo == NULL\n");
		return 0;
	}

	struct igmp_group_t *client_pos=NULL, *client_n=NULL;
	struct igmp_mbship_t *mbship_pos=NULL, *mbship_n=NULL;
	long current = igmp_timer_get_cur_time();

	list_for_each_entry_safe( mbship_pos, mbship_n, &igmp_mbship_db->mbship_list, entry_node) {

		if (mbship_pos->uni_vid != igmp_clientinfo->vid) 
			continue;
		if (igmp_pkt_ip_cmp(&mbship_pos->client_ip,&igmp_clientinfo->client_ip))
			continue;
		if (omci_env_g.igmp_proxy_snooping == 0)
		{
			if (memcmp(mbship_pos->client_mac, igmp_clientinfo->client_mac, sizeof(unsigned char)*6))
				continue;
		}
		
		if (mbship_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
		list_for_each_entry_safe( client_pos, client_n, &igmp_clientinfo->group_list, entry_node) {
			if (igmp_pkt_ip_cmp(&mbship_pos->group_ip,&client_pos->group_ip))
				continue;
			if (igmp_pkt_ip_cmp(&mbship_pos->src_ip,&client_pos->src_ip))
				continue;
			if (timeout != 0 && current - mbship_pos->access_time > timeout) 
				continue;

			mbship_pos->access_time = current;
			igmp_clientinfo->group_num -= 1;
		//	if ( igmp_clientinfo->igmp_msg_type == IGMP_V3_MEMBERSHIP_REPORT )
		//		igmp_pkt_remove_igmp_v3_group(igmp_clientinfo,mbship_pos->uni_vid,client_pos->group_ip,mbship_pos->src_ip); 
		//	if ( igmp_clientinfo->igmp_msg_type == MLD_V2_LISTENER_REPORT )
		//		igmp_pkt_remove_mld_v2_group(igmp_clientinfo,mbship_pos->uni_vid,client_pos->group_ipv6,mbship_pos->src_ipv6); 
			list_del(&client_pos->entry_node);
			free_safe(client_pos);
		}
	}
	return 0;
}


int
igmp_mbship_client_access_time_update(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group)
{
	struct igmp_mbship_t *mb_pos= NULL,*mb_n= NULL;
	list_for_each_entry_safe(mb_pos, mb_n, &igmp_mbship_db->mbship_list, entry_node) {
		if (mb_pos->src_port_meid != igmp_clientinfo->rx_desc.bridge_port_me->meid)
			continue;
		if (mb_pos->logical_port_id != igmp_clientinfo->rx_desc.logical_port_id)
			continue;
		if (mb_pos->uni_vid != igmp_clientinfo->vid)
			continue;
		if (igmp_pkt_ip_cmp(&mb_pos->group_ip,&igmp_group->group_ip))
			continue;
		if (igmp_pkt_ip_cmp(&mb_pos->client_ip,&igmp_clientinfo->client_ip))
			continue;
		if (igmp_pkt_ip_cmp(&mb_pos->src_ip,&igmp_group->src_ip))
			continue;

		if (omci_env_g.igmp_proxy_snooping == 0)
		{
			if (memcmp(mb_pos->client_mac, igmp_clientinfo->client_mac,6))
				continue;
		}
		
		if (mb_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
		
		mb_pos->access_time = igmp_timer_get_cur_time();
	}
	return 0;
}

int 
igmp_mbship_rm_mbship_db_groups_in_client(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct igmp_mbship_t *mb_pos= NULL,*mb_n= NULL;
	struct igmp_group_t *client_pos= NULL,*client_n= NULL; 


	list_for_each_entry_safe(client_pos, client_n, &igmp_clientinfo->group_list, entry_node) {
		list_for_each_entry_safe(mb_pos, mb_n, &igmp_mbship_db->mbship_list, entry_node) {
			if (mb_pos->uni_vid != igmp_clientinfo->vid)
				continue;
			if (igmp_pkt_ip_cmp(&mb_pos->group_ip,&client_pos->group_ip))
				continue;
			if (igmp_pkt_ip_cmp(&mb_pos->client_ip,&igmp_clientinfo->client_ip))
				continue;
			if (igmp_pkt_ip_cmp(&mb_pos->src_ip,&client_pos->src_ip))
				continue;
//			if (memcmp(mb_pos->client_mac, igmp_clientinfo->client_mac,6))
//				continue;
                        if (mb_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
			
			list_del(&mb_pos->entry_node);
			free_safe(mb_pos);
		}
	}
	return 0;
}

void igmp_mbship_lw_v3_timeout_update_routine(void *unused)
{
        igmp_mbship_lw_v3_db_timeout_update(IGMPV3_QUERY_TIMEOUT_INTERVAL);
}

void igmp_mbship_timeout_update_routine(void *unused)
{
	
	if (omci_env_g.igmp_proxy_snooping) ageactiveroutes();
	if (omci_env_g.igmp_proxy_snooping == 1) return; // only snooping & proxy snooping does db timeout
	
	igmp_mbship_db_timeout_update(igmp_config_g.max_respon_time*100 + igmp_config_g.robustness*igmp_config_g.query_interval*1000);
}

int
igmp_mbship_db_timeout_update(long timer_interval)
{
	struct igmp_mbship_t *pos=NULL, *n=NULL;
	struct me_t *bridge_port_me=NULL; 
//	unsigned short logical_port_id=0; 
	unsigned short pbmp=0; 
	struct switch_mac_tab_entry_t mac_tab_entry; 
	unsigned int counter=0,index=0;
	struct me_t *mcast_port=NULL;

	long current_time = igmp_timer_get_cur_time();

	if (switch_hw_g.mac_ipmc_get_next==NULL) {
		dbprintf_igmp(LOG_ERR, "switch_hw_g.mac_ipmc_get_next==NULL\n");
		return -1;
	}

	list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
		if (current_time - pos->access_time > timer_interval) {
			bridge_port_me = meinfo_me_get_by_meid(47, pos->src_port_meid);
			mcast_port= igmp_omci_mcast_bridge_port_get(bridge_port_me);
			if (!mcast_port) {
				dbprintf_igmp(LOG_WARNING, "bridge port or veip port is null\n");
				continue;
			}

			if (pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;

                        igmp_omci_ipv4_active_group_remove(mcast_port, pos);
			igmp_omci_ipv6_active_group_remove(mcast_port, pos);
			list_del(&pos->entry_node);
			free_safe(pos);
		}
	}

	while (index<8192) {
		pbmp = 0;
		// range 0..2047, use 8192 for safety
		memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
		if (switch_hw_g.mac_ipmc_get_next(&index, &mac_tab_entry)!=1)
			break;
		list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
			if (igmp_pkt_ip_cmp(&mac_tab_entry.ipmcast.dst_ip, &pos->group_ip))
				continue;
			if (pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
			//if (*(unsigned int *)&mac_tab_entry.vid != *(unsigned int *)&pos->uni_vid )
			//	continue;
			if (pos->p2p_cfg == 0xff) {
				//logical_port_id = igmp_omci_get_logical_port_id_by_bridge_port_meid(pos->src_port_meid );
				pbmp |= 1 << pos->logical_port_id ;
			} else {
				pbmp |= 1 << pos->p2p_cfg ;
			}
		}

		if (pbmp == 0) {
			*(unsigned int *)&mac_tab_entry.ipmcast.src_ip = 0;
			igmp_switch_mcast_entry_del(&mac_tab_entry);
		} else {
			if (igmp_switch_mcast_entry_add(&mac_tab_entry) < 0)
				return -1;
			counter++;
		}
		index++;
	};
	return 0;
}

int
igmp_mbship_port_clear(struct igmp_mbship_db_t *igmp_mbship_db, unsigned short src_port_id)
{
	struct igmp_mbship_t *pos=NULL, *n=NULL;

	list_for_each_entry_safe(pos,n,&igmp_mbship_db->mbship_list, entry_node) {
		if (pos->src_port_meid != src_port_id)
			continue;
		if (pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
		struct me_t *bridge_port = meinfo_me_get_by_meid(47, src_port_id);
		igmp_omci_ipv4_active_group_remove(bridge_port, pos);
		struct igmp_group_t g;
		memset(&g, 0, sizeof(struct igmp_group_t));
		memcpy(&g.group_ip, &pos->group_ip, sizeof(union ipaddr));
		memcpy(&g.src_ip, &pos->src_ip, sizeof(union ipaddr));
		list_del(&pos->entry_node);
		free_safe(pos);
		igmp_switch_mac_table_client_update(NULL, igmp_mbship_db, &g);
	}
	return 0;
}

int 
igmp_mbship_client_already_exist (struct igmp_mbship_db_t * igmp_mbship_db,struct igmp_clientinfo_t *igmp_clientinfo) 
{
	struct igmp_mbship_t *mbship_pos=NULL, *mbship_n=NULL;
	struct igmp_group_t *client_pos=NULL, *client_n=NULL;
	list_for_each_entry_safe(mbship_pos,mbship_n,&igmp_mbship_db->mbship_list, entry_node) {
		if (mbship_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
		list_for_each_entry_safe(client_pos,client_n,&igmp_clientinfo->group_list, entry_node) {
			if (igmp_pkt_ip_cmp(&igmp_clientinfo->client_ip,&mbship_pos->client_ip) == 0 && igmp_pkt_ip_cmp(&client_pos->group_ip,&mbship_pos->group_ip) == 0 
			  && igmp_pkt_ip_cmp(&client_pos->src_ip,  &mbship_pos->src_ip) == 0 && (igmp_clientinfo->vid  ==  mbship_pos->uni_vid) && igmp_clientinfo->rx_desc.logical_port_id != mbship_pos->logical_port_id )
				return 1;
		}
	}
	return 0;
}

int
igmp_mbship_group_other_client_exist (struct igmp_mbship_db_t * igmp_mbship_db, struct igmp_group_t *igmp_group)
{

	struct igmp_mbship_t *mbship_pos=NULL, *mbship_n=NULL;

	list_for_each_entry_safe(mbship_pos,mbship_n,&igmp_mbship_db->mbship_list, entry_node) {
		if (mbship_pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE) continue;
		if (igmp_pkt_ip_cmp(&igmp_group->group_ip, &mbship_pos->group_ip) == 0)
			return 1;
	}
	return 0;
}

