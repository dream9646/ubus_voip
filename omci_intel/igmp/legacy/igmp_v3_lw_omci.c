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
 * File    : igmp_v3_lw_omci.c
 *
 ******************************************************************/
 
#include <string.h>
#include "list.h"
#include "util.h"
#include "switch.h"
#include "mcast_const.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "mcast.h"
#include "igmp_omci.h"
#include "mcast_snoop.h"
extern int layer2bridged;

#define IGMP_V3_LW_ENTITY_EXIST(o, msg, ...) if (!o) {dbprintf_igmp(LOG_IGMPV3_LW, msg, ##__VA_ARGS__);return IGMP_JOIN_DENIED_RELATED_ME_NOT_FOUND;}

static int
igmp_v3_lw_omci_add_ipv4_active_group_list(void *active_entry_data, struct me_t *subscriber_monitor_me) 
{
	if (active_entry_data==NULL) 
		return -1;

	if (subscriber_monitor_me==NULL) 
		return -2;		
	
	return meinfo_me_attr_set_table_entry(subscriber_monitor_me, 5, active_entry_data, 1);
}

int
igmp_v3_lw_omci_filter(struct igmp_clientinfo_t *igmp_clientinfo)
{
        struct me_t *bridge_port_me = igmp_clientinfo->rx_desc.bridge_port_me;
        IGMP_V3_LW_ENTITY_EXIST(bridge_port_me, "bridge port is NULL\n");     
        struct me_t *mcast_port = igmp_omci_mcast_bridge_port_get(bridge_port_me);
        IGMP_V3_LW_ENTITY_EXIST(mcast_port, "class=%d, meid=0x%x(%u) has no related igmp subscriber config me\n",
                                bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);
        struct me_t *subscriber_config_me=igmp_omci_get_subscriber_config_me(mcast_port);
        IGMP_V3_LW_ENTITY_EXIST(subscriber_config_me, "igmp subscriber config is NULL\n");  

        struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);
        struct me_t *profile_me = NULL;
	if(meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0) {
                // check multcast service package table
                profile_me = igmp_omci_get_mspt_op_profile_me( bridge_port_me, igmp_clientinfo->vid); 
		if (!profile_me) profile_me = igmp_omci_get_op_profile_me(bridge_port_me);
	} else 
		profile_me = igmp_omci_get_op_profile_me(bridge_port_me);
        
	if (!profile_me && omci_env_g.igmp_join_default_grant)
		return IGMP_JOIN_GRANTED_RELATED_ME_NOT_FOUND;

        IGMP_V3_LW_ENTITY_EXIST(profile_me, "profile me not found");
        
        // filter out records not permitted by static and dynamic ACL
        igmp_omci_rm_illeagal_group_in_client(mcast_port, igmp_clientinfo);       
        
        if (igmp_omci_max_allowed_limit_filter(igmp_clientinfo) < 0) {
                dbprintf_igmp(LOG_IGMPV3_LW, "IGMP JOIN BANDWIDTH OR GROUP OVERFLOW\n"); 
		return IGMP_JOIN_DENIED_ACL_FILTERED;
	}
	
	if (!igmp_omci_check_msgrate_is_allowed(bridge_port_me, igmp_clientinfo->vid))
	{
	       dbprintf_igmp(LOG_IGMPV3_LW,"igmp message rate override\n");
	       return IGMP_JOIN_DENIED_ACL_FILTERED;
        }
        
        igmp_omci_update_msgrate(bridge_port_me, igmp_clientinfo->vid);
	
        return 0;
}

int
igmp_v3_lw_omci_active_group_list_remove(struct igmp_mbship_t *h)
{
        struct me_t *bridge_port_me = meinfo_me_get_by_meid(47, h->src_port_meid);
        IGMP_V3_LW_ENTITY_EXIST(bridge_port_me, "bridge port is NULL\n");
        struct me_t *mcast_port = igmp_omci_mcast_bridge_port_get(bridge_port_me);
        IGMP_V3_LW_ENTITY_EXIST(mcast_port, "class=%d, meid=0x%x(%u) has no related igmp subscriber config me\n",
                                bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);                                      
        return igmp_omci_ipv4_active_group_remove(mcast_port, h);        
}

static int
igmp_v3_lw_omci_get_full_auth_acl_node_988(struct list_head *acl_list, struct igmp_mbship_t *h, struct igmp_omci_acl_node_t *acl_node)
{
	struct igmp_omci_acl_node_t *acl_list_entry;  
	
	list_for_each_entry(acl_list_entry, acl_list , entry_node) {
                unsigned int groupip = ntohl(h->group_ip.ipv4.s_addr);
                if (groupip >= acl_list_entry->dst_ip_start && groupip <= acl_list_entry->dst_ip_end) {
                        if (ntohl(h->client_ip.ipv4.s_addr) && acl_list_entry->source_ip) {	// cmp server ip only if both are not zero
                                if( ntohl(h->client_ip.ipv4.s_addr) != acl_list_entry->source_ip)
                                        continue;
			}
			if (acl_list_entry->preview_length == 0) {
			     *acl_node = *acl_list_entry;
			     return 0;
			}
	       }            
        }
	
	return -1;
}

static int
igmp_v3_lw_omci_get_acl_node( struct list_head *acl_list, struct igmp_mbship_t *h, struct igmp_omci_acl_node_t *acl_node)
{
        struct igmp_omci_acl_node_t *acl_list_entry=NULL;
        
        unsigned int groupip = ntohl(h->group_ip.ipv4.s_addr);
        
        list_for_each_entry(acl_list_entry, acl_list , entry_node) {
                if (ntohl(h->src_ip.ipv4.s_addr) && acl_list_entry->source_ip) {	// cmp server ip only if both are not zero
                        if( ntohl(h->src_ip.ipv4.s_addr) != acl_list_entry->source_ip)
			     continue;
		}
                
                if (( groupip >= acl_list_entry->dst_ip_start && groupip <= acl_list_entry->dst_ip_end )
		      || ( acl_list_entry->dst_ip_end == 0 && acl_list_entry->dst_ip_start == 0)) {
                        *acl_node = *acl_list_entry;
			INIT_LIST_NODE(&acl_node->entry_node);
			return 0;
		}
	}
	
	return -1;
}

static int
igmp_v3_lw_omci_acl_table_entry_find(struct me_t *bridge_port_me, struct igmp_mbship_t *h, struct igmp_omci_acl_node_t *acl_node)
{
	struct list_head dynamic_acl_list;
	struct list_head static_acl_list;
	struct attr_table_header_t *tab_header = NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	struct me_t *profile_me= NULL;
	int entry_num=0;


	INIT_LIST_HEAD(&dynamic_acl_list);
	INIT_LIST_HEAD(&static_acl_list);

        if( meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 ) {
		// check multcast service package table
		profile_me = igmp_omci_get_mspt_op_profile_me(bridge_port_me, h->uni_vid); 
		if (!profile_me) 
			profile_me = igmp_omci_get_op_profile_me(bridge_port_me);

		if (profile_me==NULL) {
			if (omci_env_g.igmp_join_default_grant)
				return IGMP_JOIN_GRANTED_RELATED_ME_NOT_FOUND;
			else
				return  IGMP_JOIN_DENIED_RELATED_ME_NOT_FOUND;
		}

		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 8);// static acl table
		if ((entry_num= igmp_omci_load_acl_table_988(&static_acl_list, tab_header)) <= 0)
			dbprintf_igmp(LOG_IGMPV3_LW, "static acl table is empty\n") ;
			
		if (igmp_v3_lw_omci_get_full_auth_acl_node_988(&static_acl_list, h, acl_node) == 0) {
			igmp_omci_unload_acl_table(&static_acl_list); 
			return IGMP_JOIN_STATIC_ACL_ENTRY_FOUND; // static acl entry is found	
		}

		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 7);// dynamic acl table
		if ((entry_num = igmp_omci_load_acl_table_988(&dynamic_acl_list, tab_header)) <= 0 )
			dbprintf_igmp(LOG_IGMPV3_LW, "dynamic acl table is empty\n") ;

		if( igmp_v3_lw_omci_get_full_auth_acl_node_988( &dynamic_acl_list, h, acl_node) == 0 ){
			igmp_omci_unload_acl_table(&dynamic_acl_list); 
			igmp_omci_unload_acl_table(&static_acl_list); 
			return IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND; // dynamic acl entry is found	
		}
	} else {
		profile_me = igmp_omci_get_op_profile_me(bridge_port_me);
               	struct me_t *subscriber_config_me=NULL;
                if (bridge_port_me != NULL) subscriber_config_me=igmp_omci_get_subscriber_config_me(bridge_port_me);

		// ME 310 attr2 can point to a ME309 not contained in attr 6. ME309 by attr 6 allows UNI vid matching prior to any vlan tag or tag control
		// we load mstp table first and check vid. if mstp happens to contain ME309 pointed by attr2, we set me310_attr2 to 1
		// and dont load it after mspt table. ME309 pointed by ME310 attr2 does not have UNI vid matching  
		int me310_attr2 = 0;
		
                struct attr_table_header_t *tab_header_mspt = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(subscriber_config_me , 6);
                if (tab_header_mspt != NULL)
                {
                        struct attr_table_entry_t *tab_entry;
                        list_for_each_entry(tab_entry, &tab_header_mspt->entry_list, entry_node) {
                                unsigned short vlan_id = (unsigned short)util_bitmap_get_value(tab_entry->entry_data_ptr, 20*8, 2*8, 16);
                                if (!layer2bridged)	// layer 2 bridged dont check vlan id - calix
				{
	                                if (vlan_id == 0xffff)  continue;       // undefined, ignore
	                                if (vlan_id == 4097)                    // apply to tagged messages but ignore vlan id
	                                {
	                                        if (h->uni_vid == 0) continue;
	                                }
	                        
	                                if (vlan_id == 4096)                    // apply to untagged messages
	                                {
	                                        if (h->uni_vid != 0) continue;        
	                                }
	                                
	                                if (vlan_id <= 4095)    // match vlan
	                                {
	                                        if (omci_env_g.igmp_proxy_snooping == 0 && h->uni_vid != vlan_id)    continue;
	                                }
                                }
                                
                                // get actual valid ME 309
                                struct me_t *mspt_me = meinfo_me_get_by_meid(309, util_bitmap_get_value(tab_entry->entry_data_ptr,20*8,10*8,16));
                                if (mspt_me->meid == profile_me->meid) me310_attr2 = 1;
                                
                                // static acl table
                                tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 8);
                                if ((entry_num = igmp_omci_load_acl_table(&static_acl_list, tab_header)) <= 0)
                                	dbprintf_igmp(LOG_WARNING, "static acl table is empty\n"); 
                                
                                // dynamic acl table
                                tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 7);
                                if ((entry_num = igmp_omci_load_acl_table(&dynamic_acl_list, tab_header)) <= 0)
                                	dbprintf_igmp(LOG_WARNING, "dynamic acl table is empty\n");
                                
				dbprintf_igmp(LOG_WARNING, "mspt entry vlan %d, profile pointer-%d\n", vlan_id, mspt_me->meid);                                                      
                        }                
                }
                
		// static acl table
		if (!me310_attr2)
		{
			tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 8);
			if ((entry_num= igmp_omci_load_acl_table(&static_acl_list, tab_header)) <= 0)
				dbprintf_igmp(LOG_WARNING, "static acl table is empty\n"); 
			// dynamic acl table
			tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 7);
			if ((entry_num = igmp_omci_load_acl_table(&dynamic_acl_list, tab_header)) <= 0)
				dbprintf_igmp(LOG_WARNING, "dynamic acl table is empty\n"); 
		}
		
                if( igmp_v3_lw_omci_get_acl_node( &static_acl_list, h, acl_node) == 0 ){
			igmp_omci_unload_acl_table(&static_acl_list); 
			return IGMP_JOIN_STATIC_ACL_ENTRY_FOUND; // static entry is found
		}
		
		if( igmp_v3_lw_omci_get_acl_node( &dynamic_acl_list, h, acl_node) == 0 ){
			igmp_omci_unload_acl_table(&static_acl_list); 
			igmp_omci_unload_acl_table(&dynamic_acl_list); 
			return IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND; // dynamic acl entry is found	
		}
	}

	igmp_omci_unload_acl_table(&static_acl_list); 
	igmp_omci_unload_acl_table(&dynamic_acl_list); 

	return IGMP_JOIN_FULL_AUTH_ACL_ENTRY_NOT_FOUND;
}

static int
igmp_v3_lw_omci_get_preview_acl_node_988( struct list_head *acl_list,  struct igmp_mbship_t *h, struct igmp_omci_acl_node_t *acl_node )
{
	struct igmp_omci_acl_node_t *acl_list_entry;  

	list_for_each_entry(acl_list_entry, acl_list , entry_node) {
                unsigned int groupip = ntohl(h->group_ip.ipv4.s_addr);
		if ( (groupip >= acl_list_entry->dst_ip_start && groupip <= acl_list_entry->dst_ip_end)
                        || ( acl_list_entry->dst_ip_start == 0 && acl_list_entry->dst_ip_end == 0)) {
                        if (ntohl(h->client_ip.ipv4.s_addr) && acl_list_entry->source_ip) {	// cmp server ip only if both are not zero
                                if( ntohl(h->client_ip.ipv4.s_addr) != acl_list_entry->source_ip)
				    continue;
			}
			if (acl_list_entry->preview_length > 0) {
                                *acl_node = *acl_list_entry;
                                return 0;
			}
		}                 
	}
    	
	return -1;
}

static int
igmp_v3_lw_omci_acl_table_preview_entry_find(struct me_t *bridge_port_me, struct igmp_mbship_t *h, struct igmp_omci_acl_node_t *acl_node)
{
	struct list_head dynamic_acl_list;
	struct attr_table_header_t *tab_header = NULL;
	struct me_t *profile_me=NULL;
	int entry_num=0;

	INIT_LIST_HEAD(&dynamic_acl_list);

	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		

	if( meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 ) {
		// check multcast service package table
		profile_me = igmp_omci_get_mspt_op_profile_me( bridge_port_me, h->uni_vid); 
		if (!profile_me) 
			profile_me = igmp_omci_get_op_profile_me(bridge_port_me);

		if (profile_me==NULL) {
			if (omci_env_g.igmp_join_default_grant)
				return IGMP_JOIN_GRANTED_RELATED_ME_NOT_FOUND;
			else
				return  IGMP_JOIN_DENIED_RELATED_ME_NOT_FOUND;
		}

		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 7);// dynamic acl table
		if ((entry_num = igmp_omci_load_acl_table_988(&dynamic_acl_list, tab_header)) <= 0 )
			dbprintf_igmp(LOG_IGMPV3_LW, "dynamic acl table is empty\n") ;

		if( igmp_v3_lw_omci_get_preview_acl_node_988(&dynamic_acl_list, h, acl_node) == 0 ){
			igmp_omci_unload_acl_table(&dynamic_acl_list); 
			return IGMP_JOIN_PREVIEW_ACL_ENTRY_FOUND; // dynamic acl entry is found	
		}

	}
	
	igmp_omci_unload_acl_table(&dynamic_acl_list); 

	return IGMP_JOIN_PREVIEW_ACL_ENTRY_NOT_FOUND;
}

static int
igmp_v3_lw_omci_get_allowed_preview_groups_table_node( struct list_head *apgt_list, struct igmp_mbship_t *h, struct igmp_omci_allowed_preview_node_t *apgt_node )
{
	struct igmp_omci_allowed_preview_node_t *apgt_list_entry;
        
        unsigned int groupip = ntohl(h->group_ip.ipv4.s_addr);
	
        list_for_each_entry(apgt_list_entry, apgt_list, entry_node) {
                if (h->uni_vid != 0xffff) {	// 0xffff means ignore vid_uni, special value used by 5vt
                        if (h->uni_vid != apgt_list_entry->vid_uni || (apgt_list_entry->vid_uni == 0 && h->uni_vid))
                                continue;
		}
	
		if ( apgt_list_entry->dst_ip == groupip ) {
                        *apgt_node = *apgt_list_entry;
                        return 0;
		}
        }

	return -1;
}

static int
igmp_v3_lw_omci_allowed_preview_group_table_entry_find(struct me_t *bridge_port_me, struct igmp_mbship_t *h, struct igmp_omci_allowed_preview_node_t *apgt_node)
{
	struct me_t *subscriber_config_me=igmp_omci_get_subscriber_config_me(bridge_port_me);
	struct list_head apgt_list;
	struct attr_table_header_t *tab_header = NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	int entry_num=0;

	INIT_LIST_HEAD(&apgt_list);

	if( meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 ) {
		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(subscriber_config_me, 7);// allowed preview groups table
		if ( (entry_num= igmp_omci_load_allowed_preview_groups_table(&apgt_list, tab_header)) <= 0 )
			dbprintf_igmp(LOG_IGMPV3_LW, "static acl table is empty\n"); 

		if( igmp_v3_lw_omci_get_allowed_preview_groups_table_node( &apgt_list, h, apgt_node ) == 0 ){
			igmp_omci_unload_apgt_table( &apgt_list );
			return IGMP_JOIN_APGT_ENTRY_FOUND; // allowed preview group table entry is found
		}
	}

	igmp_omci_unload_apgt_table( &apgt_list );
	return IGMP_JOIN_APGT_ENTRY_NOT_FOUND; // allowed preview group table entry is found
}

int
igmp_v3_lw_omci_active_group_list_add(struct igmp_mbship_t *h)
{
        struct me_t *bridge_port_me = meinfo_me_get_by_meid(47, h->src_port_meid);
        IGMP_V3_LW_ENTITY_EXIST(bridge_port_me, "bridge port is NULL\n");     
        struct me_t *mcast_port = igmp_omci_mcast_bridge_port_get(bridge_port_me);
        IGMP_V3_LW_ENTITY_EXIST(mcast_port, "class=%d, meid=0x%x(%u) has no related igmp subscriber config me\n",
                                bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);
                                
	struct me_t *subscriber_monitor_me = igmp_omci_get_subscriber_monitor_me(mcast_port);
	IGMP_V3_LW_ENTITY_EXIST(subscriber_monitor_me, "class=%d, meid=0x%x(%u) has no related igmp subscriber monitor me\n",
                                mcast_port->classid, mcast_port->meid, mcast_port->meid);

        struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);
        struct me_t *profile_me = NULL;     
	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0) {
		// check multcast service package table
		profile_me = igmp_omci_get_mspt_op_profile_me(mcast_port, h->uni_vid); 
		if (!profile_me) 
			profile_me = igmp_omci_get_op_profile_me(mcast_port);
	} else 
		profile_me = igmp_omci_get_op_profile_me(mcast_port);
        IGMP_V3_LW_ENTITY_EXIST(profile_me, "profile me not found");

	void *active_entry_data = NULL;
	void *found_entry_data = NULL;
	
	unsigned int imputed_group_bandwidth = 0;
	
	found_entry_data = igmp_omci_locate_ipv4_active_group_list_client(mcast_port, h->uni_vid,
                                        h->group_ip.ipv4, h->src_ip.ipv4, h->client_ip.ipv4,
                                        IGMP_ACTIVE_GROUP_LIST_CLIENT);

        if (!found_entry_data)
        {
                struct igmp_omci_acl_node_t acl_node, preview_acl_node;
                struct igmp_omci_allowed_preview_node_t  apgt_node;
                // get ACL group bandwidth
                if (igmp_v3_lw_omci_acl_table_entry_find(mcast_port, h, &acl_node) == IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND)
                {
                        imputed_group_bandwidth = acl_node.imputed_bw;
                        h->ani_vid = acl_node.vid_ani;
                }
                else if (igmp_v3_lw_omci_allowed_preview_group_table_entry_find(mcast_port, h, &apgt_node) == IGMP_JOIN_APGT_ENTRY_FOUND) 
			imputed_group_bandwidth = apgt_node.imputed_bw;
		else if (igmp_v3_lw_omci_acl_table_preview_entry_find(mcast_port, h, &preview_acl_node) == IGMP_JOIN_PREVIEW_ACL_ENTRY_FOUND) 
			imputed_group_bandwidth = preview_acl_node.imputed_bw;        
                
                active_entry_data = malloc_safe(24);
                if (h->uni_vid == 4096)
                        util_bitmap_set_value(active_entry_data, 24*8, 0, 2*8, 0);
                else
                        util_bitmap_set_value(active_entry_data, 24*8, 0, 2*8, h->uni_vid);
                
                util_bitmap_set_value(active_entry_data, 24*8, 0, 2*8, h->ani_vid);
		util_bitmap_set_value(active_entry_data, 24*8, 2*8, 4*8, ntohl(*(int *)&h->src_ip)); 
                util_bitmap_set_value(active_entry_data, 24*8, 6*8, 4*8, ntohl(*(int *)&h->group_ip));
                util_bitmap_set_value(active_entry_data, 24*8, 10*8, 4*8, imputed_group_bandwidth ); 
                util_bitmap_set_value(active_entry_data, 24*8, 14*8, 4*8, ntohl(*(int *)&h->client_ip)); 
                util_bitmap_set_value(active_entry_data, 24*8, 18*8, 4*8, h->access_time);
                igmp_v3_lw_omci_add_ipv4_active_group_list(active_entry_data, subscriber_monitor_me); 
                free_safe(active_entry_data);
        }
        else
        {
                util_bitmap_set_value(found_entry_data , 24*8, 18*8, 4*8, h->access_time);
        }
        
        struct attr_value_t attr={0};
	// inc accepted join counter
	attr.data = meinfo_util_me_attr_data(subscriber_monitor_me, 3) + 1;	
	meinfo_me_attr_set(subscriber_monitor_me, 3, &attr, 1);
//*	igmp_counter_successful_join_incr(h->uni_vid, bridge_port_me);
	
	// update current bandwidth usage with sum of imputed bandwidth, FAKED
	attr.data = igmp_omci_total_bandwidth_in_active_group_list(mcast_port, h->uni_vid, IGMP_PORT_SCOPE);
	meinfo_me_attr_set(subscriber_monitor_me, 2, &attr, 1);
                                
        return 0;        
}

          

