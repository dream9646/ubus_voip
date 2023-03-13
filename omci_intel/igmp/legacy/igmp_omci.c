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
 * File    : igmp_omci.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <errno.h>
#include <time.h>

#include "list.h"
#include "util.h"
#include "meinfo.h"
#include "hwresource.h"
#include "er_group.h"
#include "me_related.h"
#include "notify_chain.h"
#include "mcast_const.h"
#include "mcast.h"
#include "cpuport.h"
#include "switch.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "igmp_omci.h" 
#include "mcast_switch.h"
#include "mcast_timer.h"
#include "fwk_timer.h"
#include "proprietary_alu.h"

unsigned short cli_mcast_op_profile_meid;
unsigned char p2p_cfg_g=0xff;
extern int layer2bridged;

#define DEFAULT_ROBUSTNESS     2

int igmp_omci_update_upstream_rate(int rate)
{
        // read multicast operation profile & set rate limit of all ports under it
	struct meinfo_t *miptr= meinfo_util_miptr(310);
        struct me_t *me=NULL;
        unsigned char found = 0;		
	
	if (miptr==NULL)
	{
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 310);
		return -1;
	}

	struct attr_table_entry_t *table_entry_pos;
	struct attr_table_header_t *table_header = NULL;
	struct attrinfo_table_t *attrinfo_table_ptr = NULL;
                
	// traverse all subscriber_config_info me
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {

		table_header = (struct attr_table_header_t *) me->attr[6].value.ptr; 
		attrinfo_table_ptr = meinfo_util_aitab_ptr(310,6);

		// check multicast service package table
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				//get Extended_multicast_operations_profile_pointer
				unsigned short ext_pointer = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*attrinfo_table_ptr->entry_byte_size, 80, 16);

				if (ext_pointer)
				{
					found = 1;
					break;
				}
			}
		}
		
		if (found == 0 && meinfo_util_me_attr_data(me, 2))
		{
                        found = 1;		
			break;
		}
	}
	
        if (!found)     return -1;      // no multicast profile found
        
	struct me_t *meptr_47 = NULL; 
	miptr = meinfo_util_miptr(47);
	list_for_each_entry(meptr_47, &miptr->me_instance_list, instance_node) {
                if (meptr_47->meid == me->meid)
                {
                        struct me_t *iport_me = hwresource_public2private(meptr_47);
                        if (iport_me == NULL)
                                dbprintf(LOG_ERR, "iport_me == NULL\n");
                        else
                                igmp_switch_ratelimit_set(meinfo_util_me_attr_data(iport_me, 5), rate);
                }
	}        
        
        return 0; 
}

struct me_t *
igmp_omci_mcast_bridge_port_get(struct me_t *bridge_port_me)
{
	struct me_t *mcast_port = NULL;
	if (!bridge_port_me)
	{
		dbprintf_igmp(LOG_WARNING, "bridge port ME is NULL.\n");
		return NULL;
	}

	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);
	struct me_t *meptr = NULL;
	struct attr_table_header_t *table_header = NULL;
	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0)
	{
		// For G.988, check ME310 service_package_table
		meptr = meinfo_me_get_by_meid(310, bridge_port_me->meid);
		if (meptr) table_header = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(meptr, 6);
	}
	else
	{
		// For G984.4, check ME309 dynamic_acl_table
		meptr = meinfo_me_get_by_meid(309, bridge_port_me->meid);
		if (meptr) table_header = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(meptr, 7);
	}
	
	if (meptr && ((table_header && !list_empty(&table_header->entry_list)) || meinfo_me_get_by_meid(309, meinfo_util_me_attr_data(meptr, 2))))
	{
		mcast_port = bridge_port_me;
	}
	else
	{
		struct me_t *meptr_47 = NULL; 
		struct meinfo_t *miptr = meinfo_util_miptr(47);
		list_for_each_entry(meptr_47, &miptr->me_instance_list, instance_node) {
			if (meinfo_util_me_attr_data(meptr_47, 3) == 11)
			{ // VEIP
				if (meinfo_me_get_by_meid(310, meptr_47->meid))
				{
					mcast_port = meptr_47;
					break;
				}
			}
		}
	}
	
	if (mcast_port == NULL)	mcast_port = bridge_port_me;
	
	return mcast_port;
}
///////////////////////////////////////////////////////////////////////////

// 310 Multicast_subscriber_config_info (abbrev: subscriber_config_me)
struct me_t *
igmp_omci_get_subscriber_config_me(struct me_t *bridge_port_me)
{
	struct me_t *me;

	struct meinfo_t *miptr= meinfo_util_miptr(310);		
	if (miptr==NULL)
	{
		dbprintf_igmp(LOG_ERR, "class=%d, null miptr?\n", 310);
		return NULL;
	}
			
	if (bridge_port_me==NULL) return NULL;

	// traverse all subscriber_config_info me
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		char me_type=meinfo_util_me_attr_data(me, 1);
		if (me_type==0)
		{// meid same as bridge port
			if (me->meid==bridge_port_me->meid)
				return me;
		}
		else if (me_type==1)
		{	// meid same as  802.1p mapper service
			struct me_t *me_8021p=meinfo_me_get_by_meid(130, me->meid);		
			if (me_related(me_8021p, bridge_port_me))
				return me;
		}
		else
		{
			dbprintf_igmp(LOG_ERR, "class=%d, meid=0x%x(%u) attr1: invalid type %u\n", me->classid, me->meid, me->meid, me_type);
			return NULL;
		}		
	}
	
	return NULL;	
}


// 311 Multicast_subscriber_config_minitor (abbrev: subscriber_monitor_me)
struct me_t *
igmp_omci_get_subscriber_monitor_me(struct me_t *bridge_port_me)
{
	struct me_t *me;

	struct meinfo_t *miptr= meinfo_util_miptr(311);		
	if (miptr==NULL)
	{
		dbprintf_igmp(LOG_ERR, "class=%d, null miptr?\n", 311);
		return NULL;
	}
			
	if (bridge_port_me==NULL)
		return NULL;

	// traverse all subscriber_monitor me
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		char me_type=meinfo_util_me_attr_data(me, 1);
		if (me_type==0)
		{// meid same as bridge port
			if (me->meid==bridge_port_me->meid)
				return me;
		}
		else if (me_type==1)
		{	// meid same as  802.1p mapper service
			struct me_t *me_8021p=meinfo_me_get_by_meid(130, me->meid);		
			if (me_related(me_8021p, bridge_port_me))
				return me;
		}
		else
		{
			dbprintf_igmp(LOG_ERR, "class=%d, meid=0x%x(%u) attr1: invalid type %u\n", me->classid, me->meid, me->meid, me_type);
				return NULL;
		}		
	}
	
	return NULL;	
}

// 309 multicast_operations_profile (abbrev: op_profile_me)
struct me_t *
igmp_omci_get_op_profile_me(struct me_t *bridge_port_me)
{
	struct me_t *subscriber_config_me=NULL;
	unsigned short mcast_op_profile_meid;

	if (bridge_port_me != NULL) subscriber_config_me=igmp_omci_get_subscriber_config_me(bridge_port_me);

	if (bridge_port_me == NULL || subscriber_config_me == NULL)
	{
		struct meinfo_t *miptr= meinfo_util_miptr(309);		
		struct me_t *me;
		if (miptr==NULL)
		{
			dbprintf_igmp(LOG_ERR, "class=%d, null miptr?\n", 309);
			return NULL;
		}
				
		// traverse multicast_profile_info me
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			if (me)
				return me;
			else
				return NULL;
		}
	}

	mcast_op_profile_meid=meinfo_util_me_attr_data(subscriber_config_me, 2);
	cli_mcast_op_profile_meid = mcast_op_profile_meid;

	return meinfo_me_get_by_meid(309, mcast_op_profile_meid);
}

// check if multicast service package table is empty(null)
static int
igmp_omci_is_service_pkg_table_empty(struct me_t *mcast_subscribe_config_me)
{
	struct attr_table_header_t *tab_header;
	struct attr_table_entry_t *tab_entry;

	if (mcast_subscribe_config_me == NULL) return 1;
				
	//multicast service package table  
	tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mcast_subscribe_config_me, 6);
	
	if (tab_header==NULL) return 1;

	list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
		if (tab_entry->entry_data_ptr)	// at least one entry found in table 
			return 0;
	}
	
	return 1;
}

// return  multicast service package table entry
static void *
igmp_omci_get_mspt_entry_data(struct me_t *mcast_subscribe_config_me, unsigned short uni_vid)
{
	struct attr_table_header_t *tab_header;

	if (mcast_subscribe_config_me == NULL) return NULL;
			
	//multicast service package table  
	tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mcast_subscribe_config_me , 6);

	if (tab_header == NULL) return NULL;

	struct attr_table_entry_t *tab_entry;
	unsigned short vlan_id;

	list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
	
		if (tab_entry->entry_data_ptr == NULL) continue;
			
		if (!omci_env_g.igmp_proxy_snooping )
		{
			vlan_id = (unsigned short)util_bitmap_get_value(tab_entry->entry_data_ptr, 20*8, 2*8, 16);

			if ((vlan_id == uni_vid) || (vlan_id ==4097 && uni_vid != 4096) || (vlan_id == 4096 && uni_vid == 0))
			{
				// return the whole entry
				return tab_entry;
			}
		}
		else
			return tab_entry;
	}
	
	return NULL;
}

//me 310 Multicast_subscriber_config_info (abbrev: subscriber_config_me)
struct me_t *
igmp_omci_get_mspt_op_profile_me(struct me_t *bridge_port_me, int uni_vid )
{

	struct me_t *subscriber_config_me = NULL ;
	struct attr_table_entry_t *tab_entry;
 
	if (bridge_port_me != NULL) subscriber_config_me=igmp_omci_get_subscriber_config_me(bridge_port_me);

	if (bridge_port_me == NULL || subscriber_config_me == NULL)
	{
		struct meinfo_t *miptr= meinfo_util_miptr(309);		
		struct me_t *me;
		
		if (miptr == NULL)
		{
			dbprintf_igmp(LOG_ERR, "class=%d, null miptr?\n", 309);
			return NULL;
		}
				
		// traverse multicast_profile_info me
		list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
			if (me)
				return me;
			else
				return NULL;
		}
	}

	tab_entry = igmp_omci_get_mspt_entry_data (subscriber_config_me, uni_vid); 
	if (tab_entry) return meinfo_me_get_by_meid(309, util_bitmap_get_value(tab_entry->entry_data_ptr,20*8,10*8,16));
	dbprintf_igmp(LOG_WARNING, "mspt_op_profile_me == NULL\n");
	
	return NULL;
}

///////////////////////////////////////////////////////////////////////////

// load dynamic/static acl table from multicast operation profile (classid 309)
int
igmp_omci_load_acl_table(struct list_head *acl_list, struct attr_table_header_t *tab_header)
{
	struct attr_table_entry_t *tab_entry;
	int total=0;

	if (acl_list == NULL || tab_header == NULL)
		return -1;
			
	list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) 
	{
		struct igmp_omci_acl_node_t *acl_entry;

		if (tab_entry->entry_data_ptr == NULL) continue;
			
		acl_entry=(struct igmp_omci_acl_node_t*)malloc_safe(sizeof(struct igmp_omci_acl_node_t));
		INIT_LIST_NODE(&acl_entry->entry_node);
		
		acl_entry->gemport = (unsigned short) util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 2*8, 16);
		acl_entry->vid_ani = (unsigned short) util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 4*8, 16);
		acl_entry->source_ip = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 6*8, 32);
		acl_entry->dst_ip_start = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 10*8, 32);
		acl_entry->dst_ip_end = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 14*8, 32);
		acl_entry->imputed_bw = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 18*8, 32);
		
		list_add(&acl_entry->entry_node, acl_list);
		total++;
	}
	return total;
}

int
igmp_omci_unload_apgt_table(struct list_head *apgt_list )
{

	struct igmp_omci_acl_node_t *apgt_entry, *n_apgt_entry;

	list_for_each_entry_safe(apgt_entry, n_apgt_entry, apgt_list, entry_node) {
		list_del(&apgt_entry->entry_node);
		free_safe(apgt_entry);
	}
	
	return 0;
}
int
igmp_omci_unload_acl_table(struct list_head *acl_list)
{

	struct igmp_omci_acl_node_t *acl_entry, *n_acl_entry;

	list_for_each_entry_safe(acl_entry, n_acl_entry, acl_list, entry_node) {
		list_del(&acl_entry->entry_node);
		free_safe(acl_entry);
	}
	
	return 0;
}

// load dynamic/static acl table from multicast operation profile (classid 309)
int
igmp_omci_load_acl_table_988(struct list_head *acl_list, struct attr_table_header_t *tab_header)
{
	struct attr_table_entry_t *tab_entry = NULL;
	int total = 0;
	
	if (acl_list == NULL || tab_header == NULL) return -1;
	
	list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) 
	{
		unsigned char set_ctrl, row_part, is_found = 0;
		unsigned short row_id;
		struct igmp_omci_acl_node_t *acl_node=NULL, *n=NULL;
		
		if (tab_entry->entry_data_ptr == NULL)
			continue;
		
		set_ctrl=util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 0, 2);
		if (set_ctrl != 1) continue;
		
		row_part=util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 2, 3);
		row_id=util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 6, 10);
		
		list_for_each_entry_safe(acl_node, n, acl_list , entry_node) {
			if(acl_node->row_id == row_id)
			{
				is_found = 1;
				break;
			}
		}
		
		if (!is_found)
		{
			acl_node = (struct igmp_omci_acl_node_t*)malloc_safe(sizeof(struct igmp_omci_acl_node_t));	// alloc new entry
			INIT_LIST_NODE(&acl_node->entry_node);
			acl_node->row_id = row_id;
			list_add(&acl_node->entry_node, acl_list);	// add cur entry to acl list
			total++;
		}
		
		switch(row_part)
		{
		case 1:
			acl_node->source_ipv6.s6_addr32[0] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 2*8, 32);
			acl_node->source_ipv6.s6_addr32[1] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 6*8, 32);
			acl_node->source_ipv6.s6_addr32[2] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 10*8, 32);
			acl_node->source_ipv6.s6_addr32[3] = 0;
			acl_node->preview_length = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 14*8, 16);
			acl_node->preview_repeat_time = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 16*8, 16);
			acl_node->preview_repeat_count = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 18*8, 16);
			acl_node->preview_reset_time = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 20*8, 16);
			break;
		case 2:
			acl_node->dst_ipv6.s6_addr32[0] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 2*8, 32);
			acl_node->dst_ipv6.s6_addr32[1] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 6*8, 32);
			acl_node->dst_ipv6.s6_addr32[2] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 10*8, 32);
			acl_node->dst_ipv6.s6_addr32[3] = 0;
			break;
		case 0:
		default:
			acl_node->gemport = (unsigned short) util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 2*8, 16);
			acl_node->vid_ani = (unsigned short) util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 4*8, 16);
			acl_node->source_ip = (unsigned short) util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 6*8, 32);
			acl_node->dst_ip_start = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 10*8, 32);
			acl_node->dst_ip_end = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 14*8, 32);
			acl_node->imputed_bw = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 18*8, 32);
			break;
		}
	}
	
	return total;
}

// load allowed preview group table from preview table in subscriber config me (classid 310)
int
igmp_omci_load_allowed_preview_groups_table(struct list_head *acl_list, struct attr_table_header_t *tab_header)
{
	struct attr_table_entry_t *tab_entry = NULL;
	int total = 0;
	
	if (acl_list == NULL || tab_header == NULL) return -1;
	
	list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) 
	{
		unsigned char set_ctrl, row_part, is_found = 0;
		unsigned short row_id;
		struct igmp_omci_allowed_preview_node_t *apgt_node=NULL, *n=NULL;
		
		if (tab_entry->entry_data_ptr == NULL) continue;
		
		set_ctrl=util_bitmap_get_value(tab_entry->entry_data_ptr, 22*8, 0, 2);
		if (set_ctrl != 1) continue;
		
		row_part=util_bitmap_get_value(tab_entry->entry_data_ptr, 22*8, 2, 3);
		row_id=util_bitmap_get_value(tab_entry->entry_data_ptr, 22*8, 6, 10);
		
		list_for_each_entry_safe(apgt_node, n, acl_list , entry_node) {
			if(apgt_node->row_id == row_id)
			{
				is_found = 1;
				break;
			}
		}
		
		if (!is_found)
		{
			apgt_node = (struct igmp_omci_allowed_preview_node_t*)malloc_safe(sizeof(struct igmp_omci_allowed_preview_node_t));	// alloc new entry
			INIT_LIST_NODE(&apgt_node->entry_node);
			apgt_node->row_id = row_id;
			list_add(&apgt_node->entry_node, acl_list);	// add cur entry to acl list
			total++;
		}
		
		switch (row_part)
		{
		case 1:
			apgt_node->dst_ip = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 22*8, 14*8, 32);
			apgt_node->dst_ipv6.s6_addr32[0] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 2*8, 32);
			apgt_node->dst_ipv6.s6_addr32[1] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 6*8, 32);
			apgt_node->dst_ipv6.s6_addr32[2] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 10*8, 32);
			apgt_node->dst_ipv6.s6_addr32[3] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 14*8, 32);
			apgt_node->duration = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 22*8, 18*8, 16);	// minutes
			break;
		case 0:
		default:
			apgt_node->source_ip = (unsigned int) util_bitmap_get_value(tab_entry->entry_data_ptr, 22*8, 14*8, 32);
			apgt_node->source_ipv6.s6_addr32[0] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 2*8, 32);
			apgt_node->source_ipv6.s6_addr32[1] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 6*8, 32);
			apgt_node->source_ipv6.s6_addr32[2] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 10*8, 32);
			apgt_node->source_ipv6.s6_addr32[3] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 14*8, 32);
			apgt_node->vid_ani = (unsigned short) util_bitmap_get_value(tab_entry->entry_data_ptr, 22*8, 18*8, 16);
			apgt_node->vid_uni = (unsigned short) util_bitmap_get_value(tab_entry->entry_data_ptr, 22*8, 20*8, 16);
			break;
		}
	}
	
	return total;
}

// get acl node from acl list of allowed preview table
static int
igmp_omci_get_allowed_preview_groups_table_node( struct list_head *apgt_list , struct igmp_group_t *igmp_group, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_omci_allowed_preview_node_t *apgt_node )
{
	struct igmp_omci_allowed_preview_node_t *apgt_list_entry;  
	
	if (igmp_clientinfo->ethertype == ETH_P_IPV6)
	{
		unsigned int i, group_ipv6[4];
		
		memcpy(group_ipv6, igmp_group->group_ip.ipv6.s6_addr, sizeof(group_ipv6));
		for(i=0;i<4;i++) group_ipv6[i] = htonl(group_ipv6[i]);
		
		list_for_each_entry(apgt_list_entry, apgt_list , entry_node) {
	
			if (igmp_clientinfo->vid != 0xffff)	// 0xffff means ignore vid_uni, special value used by 5vt 
			{
				if (igmp_clientinfo->vid != apgt_list_entry->vid_uni || (apgt_list_entry->vid_uni == 0 && igmp_clientinfo->vid))
					continue;
			}
			
			if(memcmp(apgt_list_entry->dst_ipv6.s6_addr, group_ipv6, 16) == 0)
			{
				*apgt_node = *apgt_list_entry;
				return 0;
			}
		}
	}
	else
	{
		unsigned int groupip = ntohl(igmp_group->group_ip.ipv4.s_addr);
	
		list_for_each_entry(apgt_list_entry, apgt_list , entry_node) {
	
			if (igmp_clientinfo->vid != 0xffff) // 0xffff means ignore vid_uni, special value used by 5vt
			{	
				if (igmp_clientinfo->vid != apgt_list_entry->vid_uni || (apgt_list_entry->vid_uni == 0 && igmp_clientinfo->vid))
					continue;
			}
	
			if (apgt_list_entry->dst_ip == groupip)
			{
				*apgt_node = *apgt_list_entry;
				return 0;
			}
		}
	}

	return -1;
}

// get acl node from acl list of mcast op profile
static int
igmp_omci_get_acl_node( struct list_head *acl_list, struct igmp_group_t *igmp_group, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_omci_acl_node_t *acl_node)
{
	struct igmp_omci_acl_node_t *acl_list_entry=NULL;

	if (igmp_clientinfo->ethertype == ETH_P_IPV6)
	{
		unsigned int i, group_ipv6[4], src_ipv6[4], client_ipv6[4], zeroip[4];
		
		memcpy(group_ipv6, igmp_group->group_ip.ipv6.s6_addr, sizeof(group_ipv6));
		for(i=0;i<4;i++) group_ipv6[i] = htonl(group_ipv6[i]);
		memcpy(src_ipv6, igmp_group->src_ip.ipv6.s6_addr, sizeof(src_ipv6));
		for(i=0;i<4;i++) src_ipv6[i] = htonl(src_ipv6[i]);
		memcpy(client_ipv6, igmp_clientinfo->client_ip.ipv6.s6_addr, sizeof(client_ipv6));
		for(i=0;i<4;i++) client_ipv6[i] = htonl(client_ipv6[i]);
		memset(zeroip, 0, sizeof(zeroip));
		
		list_for_each_entry(acl_list_entry, acl_list , entry_node) {
			if ((memcmp(igmp_group->src_ip.ipv6.s6_addr, zeroip, 16) != 0) && (memcmp(acl_list_entry->source_ipv6.s6_addr, zeroip, 12) != 0))
			{	// cmp server ip only if both are not zero
				if(memcmp(src_ipv6, acl_list_entry->source_ipv6.s6_addr, sizeof(src_ipv6)) != 0)
					continue;
			}
			
			if ((memcmp(group_ipv6, acl_list_entry->dst_ipv6.s6_addr, 12) == 0) || (memcmp(acl_list_entry->dst_ipv6.s6_addr, zeroip, 12) == 0))
			{
				*acl_node = *acl_list_entry;
				INIT_LIST_NODE(&acl_node->entry_node);
				return 0;
			}
		}
	}
	else
	{
		unsigned int groupip = ntohl(igmp_group->group_ip.ipv4.s_addr);
		
		list_for_each_entry(acl_list_entry, acl_list , entry_node) {
			
			if (ntohl(igmp_group->src_ip.ipv4.s_addr) && acl_list_entry->source_ip) // cmp server ip only if both are not zero
			{	
				if(ntohl(igmp_group->src_ip.ipv4.s_addr) != acl_list_entry->source_ip)
					continue;
			}
	
			if ((groupip >= acl_list_entry->dst_ip_start && groupip <= acl_list_entry->dst_ip_end) || ( acl_list_entry->dst_ip_end == 0 && acl_list_entry->dst_ip_start == 0))
			{
				*acl_node = *acl_list_entry;
				INIT_LIST_NODE(&acl_node->entry_node);
				return 0;
			}
		}
	}
	
	return -1;
}

static unsigned int IPV4_ROW_PART_00[] = { 0x00, 0x00, 0x00 };
static unsigned int IPV4_ROW_PART_FF[] = { 0x00, 0x00, 0xFFFF };

// get acl node from acl list of ext mcast op profile
static int
igmp_omci_get_full_auth_acl_node_988( struct list_head *acl_list,  struct igmp_group_t *igmp_group, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_omci_acl_node_t *acl_node )
{
	struct igmp_omci_acl_node_t *acl_list_entry;  
	
	list_for_each_entry(acl_list_entry, acl_list , entry_node) {
	        if (memcmp(IPV4_ROW_PART_00, acl_list_entry->dst_ipv6.s6_addr, 12) == 0 || memcmp(IPV4_ROW_PART_FF, acl_list_entry->dst_ipv6.s6_addr, 12) == 0)
	        {
	            if (igmp_clientinfo->ethertype == ETH_P_IPV6) continue;  // this acl is in IPv4
	            
	            unsigned int groupip = ntohl(igmp_group->group_ip.ipv4.s_addr);
				if (groupip  >=  acl_list_entry->dst_ip_start && groupip <= acl_list_entry->dst_ip_end)
				{
					if (ntohl(igmp_clientinfo->client_ip.ipv4.s_addr) && acl_list_entry->source_ip)
					{	// cmp server ip only if both are not zero
						if(ntohl(igmp_clientinfo->client_ip.ipv4.s_addr) != acl_list_entry->source_ip)
							continue;
					}
					if (acl_list_entry->preview_length == 0) {
						*acl_node = *acl_list_entry;
						return 0;
					}
				}            
	        }
	        else
	        {
			if (igmp_clientinfo->ethertype != ETH_P_IPV6) continue;     // this acl is in IPv6
			
			unsigned int i, group_ipv6[4], client_ipv6[4];
			memcpy(group_ipv6, igmp_group->group_ip.ipv6.s6_addr, sizeof(group_ipv6));
			for(i=0;i<4;i++) group_ipv6[i] = htonl(group_ipv6[i]);
			memcpy(client_ipv6, igmp_clientinfo->client_ip.ipv6.s6_addr, sizeof(client_ipv6));
			for(i=0;i<4;i++) client_ipv6[i] = htonl(client_ipv6[i]);
			if (memcmp(acl_list_entry->dst_ipv6.s6_addr, group_ipv6, 12)) continue; // prefix is different
			
			if (group_ipv6[3]  >=  acl_list_entry->dst_ip_start && group_ipv6[3] <= acl_list_entry->dst_ip_end)
	    		{
				if(acl_list_entry->source_ip)   // source ip is defined
				{
					if (memcmp(acl_list_entry->source_ipv6.s6_addr, client_ipv6, 12) || acl_list_entry->source_ip != client_ipv6[3])
	                        		continue;    
				}
				
				if (acl_list_entry->preview_length == 0)
				{
					*acl_node = *acl_list_entry;
					return 0;
				}
			}
	        }     
	}
	
	return -1;
}


static int
igmp_omci_get_preview_acl_node_988( struct list_head *acl_list,  struct igmp_group_t *igmp_group, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_omci_acl_node_t *acl_node )
{
	struct igmp_omci_acl_node_t *acl_list_entry;  

	list_for_each_entry(acl_list_entry, acl_list , entry_node) {
	        if (memcmp(IPV4_ROW_PART_00, acl_list_entry->dst_ipv6.s6_addr, 12) == 0 || memcmp(IPV4_ROW_PART_FF, acl_list_entry->dst_ipv6.s6_addr, 12) == 0)
	        {
			if (igmp_clientinfo->ethertype == ETH_P_IPV6) continue;  // this acl is in IPv4
			
			unsigned int groupip = ntohl(igmp_group->group_ip.ipv4.s_addr);
			if ((groupip  >=  acl_list_entry->dst_ip_start && groupip <= acl_list_entry->dst_ip_end) || ( acl_list_entry->dst_ip_start == 0 && acl_list_entry->dst_ip_end == 0))
			{
					if (ntohl(igmp_clientinfo->client_ip.ipv4.s_addr) && acl_list_entry->source_ip) // cmp server ip only if both are not zero
					{	
						if(ntohl(igmp_clientinfo->client_ip.ipv4.s_addr) != acl_list_entry->source_ip)
							continue;
					}
					
					if (acl_list_entry->preview_length > 0)
					{
						*acl_node = *acl_list_entry;
						return 0;
					}
			}            
	        }
	        else
	        {
			if (igmp_clientinfo->ethertype != ETH_P_IPV6) continue;     // this acl is in IPv6
			
			unsigned int i, group_ipv6[4], client_ipv6[4];
			memcpy(group_ipv6, igmp_group->group_ip.ipv6.s6_addr, sizeof(group_ipv6));
			for(i=0;i<4;i++) group_ipv6[i] = htonl(group_ipv6[i]);
			memcpy(client_ipv6, igmp_clientinfo->client_ip.ipv6.s6_addr, sizeof(client_ipv6));
			for(i=0;i<4;i++) client_ipv6[i] = htonl(client_ipv6[i]);
			if (memcmp(acl_list_entry->dst_ipv6.s6_addr, group_ipv6, 12)) continue; // prefix is different
			
			if ((group_ipv6[3]  >=  acl_list_entry->dst_ip_start && group_ipv6[3] <= acl_list_entry->dst_ip_end) || ( acl_list_entry->dst_ip_start == 0 && acl_list_entry->dst_ip_end == 0))
	    		{
				if(acl_list_entry->source_ip)   // source ip is defined
				{
					if (memcmp(acl_list_entry->source_ipv6.s6_addr, client_ipv6, 12) || acl_list_entry->source_ip != client_ipv6[3])
					continue;    
	              		}
				
				if (acl_list_entry->preview_length == 0) 
				{
	                		*acl_node = *acl_list_entry;
					return 0;
				}
			}
	        }      
	}
    	
	return -1;
}

static int
igmp_omci_add_ipv4_active_group_list(void *active_entry_data, struct me_t *subscriber_monitor_me) 
{
	if (active_entry_data==NULL) return -1;

	if (subscriber_monitor_me==NULL) return -2;		
	
	return meinfo_me_attr_set_table_entry(subscriber_monitor_me, 5, active_entry_data, 1);
}

static int
igmp_omci_add_ipv6_active_group_list(void *active_entry_data, struct me_t *subscriber_monitor_me) 
{
	if (active_entry_data==NULL) return -1;

	if (subscriber_monitor_me==NULL) return -2;		
	
	return meinfo_me_attr_set_table_entry(subscriber_monitor_me, 6, active_entry_data, 1);
}

int
igmp_omci_del_ipv4_active_group_list(void *active_entry_data, struct me_t *subscriber_monitor_me) 
{
	struct attr_table_entry_t *found_table_entry;

	if (active_entry_data==NULL) return -1;
	if (subscriber_monitor_me==NULL) return -2;		
	found_table_entry=meinfo_me_attr_locate_table_entry(subscriber_monitor_me, 5, active_entry_data);	
	if (found_table_entry) return meinfo_me_attr_delete_table_entry(subscriber_monitor_me, 5, found_table_entry, 1);
	
	return 0;
}

int
igmp_omci_del_ipv6_active_group_list(void *active_entry_data, struct me_t *subscriber_monitor_me) 
{
	struct attr_table_entry_t *found_table_entry;

	if (active_entry_data==NULL) return -1;
	if (subscriber_monitor_me==NULL) return -2;		
	found_table_entry=meinfo_me_attr_locate_table_entry(subscriber_monitor_me, 6, active_entry_data);	
	if (found_table_entry) return meinfo_me_attr_delete_table_entry(subscriber_monitor_me, 6, found_table_entry, 1);

	return 0;
}


// if mode == IGMP_ACTIVE_GROUP_LIST_GROUP search the group in the active list
// if mode == IGMP_ACTIVE_GROUP_LIST_CLIENT search the client in the active list
void *
igmp_omci_locate_ipv4_active_group_list_client(struct me_t *bridge_port_me, unsigned short client_vid, struct in_addr group_ip, struct in_addr src_ip, struct in_addr client_ip,unsigned char mode)
{
	struct me_t *subscriber_monitor_me= igmp_omci_get_subscriber_monitor_me(bridge_port_me);
	struct get_attr_table_context_t table_context;

	if (subscriber_monitor_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "bridge port meid=0x%x(%u) related subscriber monitor me == NULL\n", bridge_port_me->meid, bridge_port_me->meid);
		return NULL;
	}

	void *active_entry_data = malloc_safe(24);
	int entry_count = 0;
	struct attr_table_entry_t *found_table_entry=NULL;
	if ((entry_count = meinfo_me_attr_get_table_entry_count(subscriber_monitor_me, 5)) > 0)
	{
		if (meinfo_me_attr_get_table_entry(subscriber_monitor_me, 5, &table_context, active_entry_data) == MEINFO_RW_OK)
		{
			do
			{
				if (mode == IGMP_ACTIVE_GROUP_LIST_CLIENT)
				{
					if (memcmp(&client_ip , active_entry_data+14 ,sizeof(struct in_addr))) 
						continue;
				}

				if (memcmp(&group_ip, active_entry_data+6, sizeof(struct in_addr)))
					continue;		
				//if ( client_vid != 4096 && util_bitmap_get_value(active_entry_data, 24*8, 0, 16) != 0)
				if (client_vid != 4097)
				{
					if (client_vid == 4096 && ntohs(*(unsigned short *)active_entry_data) != 0)
						continue;
					else if (client_vid != ntohs(*(unsigned short *)active_entry_data)) 
						continue;
				}


				if (memcmp(&src_ip, active_entry_data+2,sizeof(struct in_addr)) != 0)
					continue;
					
				found_table_entry=meinfo_me_attr_locate_table_entry(subscriber_monitor_me, 5, active_entry_data);	
				if (found_table_entry)
				{
					free_safe(active_entry_data);
					return found_table_entry->entry_data_ptr;
				}
			} while (meinfo_me_attr_get_table_entry_next(subscriber_monitor_me, 5, &table_context, active_entry_data) == MEINFO_RW_OK);
		}
	}
	free_safe(active_entry_data);
	
	return NULL;
}

void *
igmp_omci_locate_ipv6_active_group_list_client(struct me_t *bridge_port_me, unsigned short client_vid, struct in6_addr group_ipv6, struct in6_addr src_ipv6, struct in6_addr client_ipv6,unsigned char mode)
{
	struct me_t *subscriber_monitor_me= igmp_omci_get_subscriber_monitor_me(bridge_port_me);
	struct get_attr_table_context_t table_context;

	if (subscriber_monitor_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "bridge port meid=0x%x(%u) related subscriber monitor me == NULL \n", bridge_port_me->meid, bridge_port_me->meid );
		return NULL;
	}

	void *active_entry_data = malloc_safe(58);
	int entry_count = 0;
	struct attr_table_entry_t *found_table_entry=NULL;
	if ((entry_count = meinfo_me_attr_get_table_entry_count(subscriber_monitor_me, 6)) > 0) {
		if (meinfo_me_attr_get_table_entry(subscriber_monitor_me, 6, &table_context, active_entry_data) == MEINFO_RW_OK)
		{
			do
			{
				if (mode == IGMP_ACTIVE_GROUP_LIST_CLIENT )
				{
					if (memcmp(&client_ipv6 , active_entry_data+38 ,sizeof(struct in6_addr))) 
						continue;
				}
				if (memcmp(&group_ipv6, active_entry_data+18, sizeof(struct in6_addr)))
					continue;
				if (client_vid == 4096 && ntohs(*(unsigned short *)active_entry_data) != 0)
					continue;
				else if ( client_vid != ntohs(*(unsigned short *)active_entry_data)) 
					continue;

				if (memcmp(&src_ipv6, active_entry_data+2,sizeof(struct in6_addr))!=0)
					continue;
				found_table_entry=meinfo_me_attr_locate_table_entry(subscriber_monitor_me, 6, active_entry_data);	
				if (found_table_entry)
				{
					free_safe(active_entry_data);
					return found_table_entry->entry_data_ptr;
				}
			} while (meinfo_me_attr_get_table_entry_next(subscriber_monitor_me, 6, &table_context, active_entry_data) == MEINFO_RW_OK);
		}
	}
	free_safe(active_entry_data);
	
	return NULL;
}

static struct igmp_omci_group_t *
igmp_omci_locate_group_entry(struct list_head *active_group_list, struct igmp_omci_group_t *igmp_omci_group, struct igmp_clientinfo_t *igmp_clientinfo)
{

	struct igmp_omci_group_t  *pos=NULL;
	list_for_each_entry( pos, active_group_list, entry_node ) {
		if (memcmp(&pos->uni_vid, &igmp_omci_group->uni_vid, sizeof(struct in_addr)))
			continue;
		if (memcmp(&pos->igmp_group.group_ip, &igmp_omci_group->igmp_group.group_ip, sizeof(struct in_addr)))
			continue;
		if (memcmp(&pos->igmp_group.src_ip, &igmp_omci_group->igmp_group.src_ip, sizeof(struct in_addr)))
			continue;

		return pos;
	}
	return NULL;
}

static int
igmp_omci_groups_in_active_group_list(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo, struct list_head *active_group_list,unsigned scope)
{

	void *active_entry_data=NULL;
	int total_groups =0, entry_count = 0;
	struct get_attr_table_context_t table_context;
	struct me_t  *subscriber_monitor_me=igmp_omci_get_subscriber_monitor_me(bridge_port_me);
	
	if (subscriber_monitor_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "subscriber monitor me is NULL\n" );
		return -1;
	}

	INIT_LIST_HEAD(active_group_list);

	if (igmp_clientinfo->ethertype == ETH_P_IPV6)
	{
		active_entry_data=malloc_safe(58);
		if ((entry_count = meinfo_me_attr_get_table_entry_count(subscriber_monitor_me, 6)) > 0)
		{
			if (meinfo_me_attr_get_table_entry(subscriber_monitor_me, 6, &table_context, active_entry_data) == MEINFO_RW_OK)
			{
				do
				{
					if (scope == IGMP_VID_SCOPE)
					{
						if (igmp_clientinfo->vid == util_bitmap_get_value( active_entry_data, 58*8, 0, 2*8))
						{
							struct igmp_omci_group_t igmp_omci_client_group;
							memset(&igmp_omci_client_group,0,sizeof(struct igmp_omci_group_t));
							igmp_omci_client_group.igmp_group.src_ip.ipv6 = *(struct in6_addr *)(active_entry_data+2);
							igmp_omci_client_group.igmp_group.group_ip.ipv6 = *(struct in6_addr *)(active_entry_data+18);
							igmp_omci_client_group.uni_vid = igmp_clientinfo->vid;
							 
							if (igmp_omci_locate_group_entry(active_group_list, &igmp_omci_client_group, igmp_clientinfo) == NULL )
							{
								struct igmp_omci_group_t *igmp_omci_group=NULL;
								igmp_omci_group = malloc_safe(sizeof(struct igmp_omci_group_t ));
								memcpy(igmp_omci_group, &igmp_omci_client_group, sizeof(struct igmp_omci_group_t ));
								igmp_omci_group->bandwidth = util_bitmap_get_value(active_entry_data, 58*8, 34*8, 4*8); 
								INIT_LIST_NODE( &igmp_omci_group->entry_node );
								list_add( &igmp_omci_group->entry_node, active_group_list);
								total_groups++;
							}
						}
					}
					else if (scope == IGMP_PORT_SCOPE)
					{
						struct igmp_omci_group_t igmp_omci_client_group;
						memset(&igmp_omci_client_group,0,sizeof(struct igmp_omci_group_t));
						igmp_omci_client_group.igmp_group.src_ip.ipv6 = *(struct in6_addr *)(active_entry_data+2);
						igmp_omci_client_group.igmp_group.group_ip.ipv6 = *(struct in6_addr *)(active_entry_data+18);
						
						if (igmp_omci_locate_group_entry(active_group_list, &igmp_omci_client_group, igmp_clientinfo)== NULL )
						{
							struct igmp_omci_group_t *igmp_omci_group=NULL;
							igmp_omci_group=malloc_safe(sizeof(struct igmp_omci_group_t ));
							memcpy(igmp_omci_group, &igmp_omci_client_group, sizeof(struct igmp_omci_group_t ));
							igmp_omci_group->bandwidth = util_bitmap_get_value(active_entry_data, 58*8, 34*8, 4*8); 
							INIT_LIST_NODE( &igmp_omci_group->entry_node );
							list_add(&igmp_omci_group->entry_node,active_group_list);
							total_groups++;
						}
					}
				} while (meinfo_me_attr_get_table_entry_next(subscriber_monitor_me, 6, &table_context, active_entry_data) == MEINFO_RW_OK);
			}
		}
		free_safe(active_entry_data);
	}
	else
	{
		active_entry_data=malloc_safe(24);
		if ((entry_count = meinfo_me_attr_get_table_entry_count(subscriber_monitor_me, 5)) > 0) {
	
			if (meinfo_me_attr_get_table_entry(subscriber_monitor_me, 5, &table_context, active_entry_data) == MEINFO_RW_OK)
			{
				do {
					if (scope == IGMP_VID_SCOPE)
					{
						if (igmp_clientinfo->vid == util_bitmap_get_value( active_entry_data, 24*8, 0, 2*8))
						{
							struct igmp_omci_group_t igmp_omci_client_group;
							memset(&igmp_omci_client_group,0,sizeof(struct igmp_omci_group_t));
							igmp_omci_client_group.igmp_group.src_ip.ipv4 = *(struct in_addr *)(active_entry_data+2);
							igmp_omci_client_group.igmp_group.group_ip.ipv4 = *(struct in_addr *)(active_entry_data+6);
							igmp_omci_client_group.uni_vid = igmp_clientinfo->vid; 
							if (igmp_omci_locate_group_entry(active_group_list, &igmp_omci_client_group, igmp_clientinfo) == NULL)
							{
								struct igmp_omci_group_t *igmp_omci_group=NULL;
								igmp_omci_group = malloc_safe(sizeof(struct igmp_omci_group_t ));
								memcpy(igmp_omci_group, &igmp_omci_client_group, sizeof(struct igmp_omci_group_t ));
								igmp_omci_group->bandwidth = util_bitmap_get_value(active_entry_data, 24*8, 10*8, 4*8); 
								INIT_LIST_NODE( &igmp_omci_group->entry_node );
								list_add( &igmp_omci_group->entry_node, active_group_list);
								total_groups++;
							}
						}
					}
					else if (scope == IGMP_PORT_SCOPE)
					{
						struct igmp_omci_group_t igmp_omci_client_group;
						memset(&igmp_omci_client_group,0,sizeof(struct igmp_omci_group_t));
						igmp_omci_client_group.igmp_group.src_ip.ipv4 = *(struct in_addr *)(active_entry_data+2);
						igmp_omci_client_group.igmp_group.group_ip.ipv4 = *(struct in_addr *)(active_entry_data+6);
						if (igmp_omci_locate_group_entry(active_group_list, &igmp_omci_client_group, igmp_clientinfo)== NULL)
						{
							struct igmp_omci_group_t *igmp_omci_group=NULL;
							igmp_omci_group=malloc_safe(sizeof(struct igmp_omci_group_t ));
							memcpy(igmp_omci_group, &igmp_omci_client_group, sizeof(struct igmp_omci_group_t ));
							igmp_omci_group->bandwidth = util_bitmap_get_value(active_entry_data, 24*8, 10*8, 4*8); 
							INIT_LIST_NODE( &igmp_omci_group->entry_node );
							list_add(&igmp_omci_group->entry_node,active_group_list);
							total_groups++;
						}
					}
				} while (meinfo_me_attr_get_table_entry_next(subscriber_monitor_me, 5, &table_context, active_entry_data) == MEINFO_RW_OK);
			}
		}
		free_safe(active_entry_data);
	}
	
	return total_groups;
}

static int
igmp_omci_free_groups_in_active_group_list(struct list_head *active_group_list)
{
	struct igmp_omci_group_t *pos=NULL,*n=NULL;
	list_for_each_entry_safe( pos, n, active_group_list, entry_node ) {
		list_del(&pos->entry_node);
		free_safe(pos);
	}
	
	return 0;
}

// if mode == IGMP_VID_SCOPE search groups in the vid of the the active list
// if mode == IGMP_PORT_SCOPE search groups the port of the active list
int
igmp_omci_total_bandwidth_in_active_group_list( struct me_t *bridge_port, int vid, unsigned char mode)
{
	//int current_group=0;
	LIST_HEAD(active_group_list);
	//current_group = igmp_omci_groups_in_active_group_list( bridge_port, vid, &active_group_list, mode);

	unsigned int current_bandwidth=0;
	struct igmp_omci_group_t *active_pos=NULL,*active_n = NULL;

	list_for_each_entry_safe( active_pos, active_n, &active_group_list, entry_node) {
		current_bandwidth += active_pos->bandwidth;
	}	
	igmp_omci_free_groups_in_active_group_list(&active_group_list);
	
	return current_bandwidth;
}

int
igmp_omci_check_is_in_static_acl(union ipaddr *match_ip)
{
	struct list_head static_acl_list;
	struct attr_table_header_t *tab_header = NULL;
	struct me_t *me;
       
	INIT_LIST_HEAD(&static_acl_list);
    
	me =  igmp_omci_get_op_profile_me(NULL);
	if (me == NULL) return 0;
    
	tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 8);
	if (igmp_omci_load_acl_table(&static_acl_list, tab_header) > 0)
	{
		struct igmp_omci_acl_node_t *acl_list_entry=NULL;

		list_for_each_entry(acl_list_entry, &static_acl_list , entry_node) {
			if (match_ip->ipv4.s_addr >= acl_list_entry->dst_ip_start && match_ip->ipv4.s_addr <= acl_list_entry->dst_ip_end)
				return 1;
	              
		}        
		igmp_omci_unload_acl_table(&static_acl_list);
	}
	
	return 0;
}

int
igmp_omci_rm_illeagal_group_in_client(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo)
{
	
	int entry_num = 0;
	struct list_head dynamic_acl_list;
	struct list_head static_acl_list;
	struct attr_table_header_t *tab_header = NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	struct igmp_omci_acl_node_t  acl_node;
	struct igmp_omci_allowed_preview_node_t apgt_node;

	if (igmp_clientinfo == NULL || list_empty(&igmp_clientinfo->group_list)) return 0;

	INIT_LIST_HEAD(&dynamic_acl_list);
	INIT_LIST_HEAD(&static_acl_list);

	struct me_t *subscriber_config_me=NULL,  *profile_me=NULL;
	if (bridge_port_me != NULL) subscriber_config_me=igmp_omci_get_subscriber_config_me(bridge_port_me);

	struct igmp_group_t *pos=NULL,*n=NULL;
	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 && igmp_omci_is_service_pkg_table_empty(subscriber_config_me) == 0)
	{
		struct list_head apgt_list;
		INIT_LIST_HEAD(&apgt_list);
		
		// check multcast service package table
		profile_me = igmp_omci_get_mspt_op_profile_me( bridge_port_me, igmp_clientinfo->vid ); 

		if (profile_me==NULL)
		{
			if (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)
				return 0;
			else
				return -1;
		}

		// static acl table
		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 8);
		if ((entry_num= igmp_omci_load_acl_table_988(&static_acl_list, tab_header)) <= 0) dbprintf_igmp(LOG_WARNING, "static acl table is empty\n") ;
		
		// dynamic acl table
		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 7);
		if ((entry_num = igmp_omci_load_acl_table_988(&dynamic_acl_list, tab_header)) <= 0) dbprintf_igmp(LOG_WARNING, "dynamic acl table is empty\n") ;
		
		// allowed preview groups table
		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(subscriber_config_me, 7);
		if ((entry_num= igmp_omci_load_allowed_preview_groups_table(&apgt_list, tab_header)) <= 0) dbprintf_igmp(LOG_WARNING, "allowed preview groups table is empty\n"); 

		//No group match acl or apg table, clear all group
		if (list_empty(&static_acl_list) && list_empty(&dynamic_acl_list) && list_empty(&apgt_list))
		{
			dbprintf_igmp(LOG_WARNING, "all tables are empty\n");
			igmp_omci_unload_acl_table(&static_acl_list);
			igmp_omci_unload_acl_table(&dynamic_acl_list);
			igmp_omci_unload_apgt_table(&apgt_list);

			if (omci_env_g.igmp_join_default_grant)
				return 0;

			list_for_each_entry_safe( pos, n, &igmp_clientinfo->group_list, entry_node) {
				list_del(&pos->entry_node);
				if(pos) free_safe(pos);
			}

			return 0;
		}

		// remove client group that is not in dynamic ACL
		list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {

			if (igmp_omci_get_full_auth_acl_node_988(&static_acl_list, pos, igmp_clientinfo, &acl_node) == 0)
				continue;
			if (igmp_omci_get_full_auth_acl_node_988(&dynamic_acl_list, pos, igmp_clientinfo, &acl_node) == 0)
			{
				pos->allowed_type = FULL_ACL_AUTH;
				continue;
			}

			if (igmp_omci_get_preview_acl_node_988(&dynamic_acl_list, pos, igmp_clientinfo, &acl_node) == 0)
			{

				struct igmp_mbship_preview_acl_t *preview_client=NULL;
				preview_client = igmp_mbship_search_preview_acl_list( &igmp_mbship_db_g, igmp_clientinfo, pos);
				if (preview_client == NULL)
				{
					pos->allowed_type = PREVIEW_ACL_AUTH;
					pos->preview_length = acl_node.preview_length;
					pos->preview_repeat_time = acl_node.preview_repeat_time;
					pos->preview_repeat_count = acl_node.preview_repeat_count;
					pos->preview_reset_time = acl_node.preview_reset_time;
					continue;
				}
				else
				{
					// client group preview count < acl group preview  count 
					// and preview time length between 2 preview > acl repeat time
					if (preview_client->repeat_count < acl_node.preview_repeat_count && (igmp_timer_get_cur_time() - preview_client->last_join_time) >  acl_node.preview_repeat_time)
					{
						pos->allowed_type = PREVIEW_ACL_AUTH;
						pos->preview_length = acl_node.preview_length;
						pos->preview_repeat_time = acl_node.preview_repeat_time;
						pos->preview_repeat_count = acl_node.preview_repeat_count;
						pos->preview_reset_time = acl_node.preview_reset_time;
						continue;
					}
				}
			}

			if(igmp_omci_get_allowed_preview_groups_table_node(&apgt_list, pos, igmp_clientinfo, &apgt_node)==0)
			{
				pos->allowed_type = ALLOWED_PREVIEW_GROUP;
				pos->preview_length = apgt_node.duration;

				/* TODO: what's the purpose of these? memory-leak ??? */
#if 0
				struct igmp_mbship_apgt_t *apgt_client = NULL;
				apgt_client = igmp_mbship_search_apgt_list( &igmp_mbship_db_g, igmp_clientinfo, pos);
				if (apgt_client == NULL)
				{
					apgt_client = malloc_safe(sizeof(struct igmp_mbship_apgt_t) );
					INIT_LIST_NODE(&apgt_client->entry_node);
			//		struct igmp_mbship_apgt_t *igmp_mbship_apgt = malloc_safe(sizeof(struct igmp_mbship_apgt_t));
					memcpy( &apgt_client->group_ip, &pos->group_ip, sizeof(struct in_addr) );
					memcpy( &apgt_client->group_ip.ipv6, &pos->group_ip.ipv6, sizeof(struct in6_addr) );
					memcpy( &apgt_client->src_ip, &pos->src_ip, sizeof(struct in_addr) );
					memcpy( &apgt_client->src_ip.ipv6, &pos->src_ip.ipv6, sizeof(struct in6_addr) );
					apgt_client->uni_vid = igmp_clientinfo->vid;
				}
#endif
				continue;
			}
			list_del(&pos->entry_node);
			if (pos) free_safe(pos);
			igmp_clientinfo->group_num--;
		}
		igmp_omci_unload_apgt_table(&apgt_list);
	}
	else
	{
		profile_me = igmp_omci_get_op_profile_me(bridge_port_me);

		if (profile_me==NULL)
		{
			if (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)
				return 0;
			else
				return -1;
		}
	
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
	                                        if (igmp_clientinfo->vid == 0) continue;
	                                }
	                        
	                                if (vlan_id == 4096)                    // apply to untagged messages
	                                {
	                                        if (igmp_clientinfo->vid != 0) continue;        
	                                }
	                                
	                                if (vlan_id <= 4095)    // match vlan
	                                {
	                                        if (omci_env_g.igmp_proxy_snooping == 0 && igmp_clientinfo->vid != vlan_id)    continue;
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

		//No group match acl or apg table, clear all group
		if (list_empty(&static_acl_list) && list_empty(&dynamic_acl_list))
		{
			dbprintf_igmp(LOG_WARNING, "acl tables are empty\n"); 
			igmp_omci_unload_acl_table(&static_acl_list); 
			igmp_omci_unload_acl_table(&dynamic_acl_list); 
			if (omci_env_g.igmp_join_default_grant) return 0;
			list_for_each_entry_safe( pos, n, &igmp_clientinfo->group_list, entry_node) {
				list_del(&pos->entry_node);
				if(pos) free_safe(pos);
			}
			return 0;
		}

		list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {
			if (igmp_omci_get_acl_node( &static_acl_list,  pos, igmp_clientinfo, &acl_node) == 0)
				continue;
			if (igmp_omci_get_acl_node( &dynamic_acl_list,  pos, igmp_clientinfo, &acl_node) == 0)
				continue;
			list_del(&pos->entry_node);
			if(pos) free_safe(pos);
			igmp_clientinfo->group_num--;
		}
	}
	
	igmp_omci_unload_acl_table(&static_acl_list); 
	igmp_omci_unload_acl_table(&dynamic_acl_list); 

	return 0;
}

int
igmp_omci_rm_all_active_group_list_group()
{
	struct get_attr_table_context_t table_context;
	void *active_entry_data;
	int entry_count=0;

        struct me_t *me;

	struct meinfo_t *miptr= meinfo_util_miptr(311);		
	if (miptr==NULL)
	{
		dbprintf_igmp(LOG_ERR, "class=%d, null miptr?\n", 311);
		return -1;
	}

	// traverse all subscriber_monitor me
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
                // iterate the active list table to collect groups
                active_entry_data=malloc_safe(24);
                while ((entry_count = meinfo_me_attr_get_table_entry_count(me, 5)) > 0)
		{
                        if (meinfo_me_attr_get_table_entry(me, 5, &table_context, active_entry_data) == MEINFO_RW_OK)
                                igmp_omci_del_ipv4_active_group_list(active_entry_data, me);
                }
                free_safe(active_entry_data);		
	}
        	
	return 0;
}

int
igmp_omci_rm_active_group_list_group_in_client(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct me_t *subscriber_monitor_me;
	struct get_attr_table_context_t table_context;
	void *active_entry_data;
	int entry_count=0;
	int group_num=igmp_clientinfo->group_num;

	if (igmp_clientinfo == NULL || list_empty(&igmp_clientinfo->group_list))
		return 0;

	subscriber_monitor_me=igmp_omci_get_subscriber_monitor_me(bridge_port_me);
	if (subscriber_monitor_me==NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp subscriber monitor profile\n", bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);
		return 0;
	}
	
	// iterate the active list table to collect groups
	if(igmp_clientinfo->ethertype == ETH_P_IPV6)
	{
		active_entry_data=malloc_safe(58);
		if ((entry_count = meinfo_me_attr_get_table_entry_count(subscriber_monitor_me, 6)) > 0)
		{
			if (meinfo_me_attr_get_table_entry(subscriber_monitor_me, 6, &table_context, active_entry_data) == MEINFO_RW_OK)
			{
				do {
					if (!((util_bitmap_get_value(active_entry_data, 58*8, 0, 16) == 0 && igmp_clientinfo->vid == 4096) || ( igmp_clientinfo->vid == util_bitmap_get_value(active_entry_data, 58*8, 0, 16))))
						continue;
						
					if (memcmp(&igmp_clientinfo->client_ip.ipv6 , active_entry_data+38 ,sizeof(struct in6_addr)))
						continue;
					
					struct igmp_group_t *pos,*n;
					list_for_each_entry_safe( pos, n, &igmp_clientinfo->group_list, entry_node ) {
						if ( memcmp(&pos->group_ip.ipv6, active_entry_data+18,sizeof(struct in6_addr)))
							continue;
						igmp_mbship_client_access_time_update( &igmp_mbship_db_g, igmp_clientinfo, pos);
						if (igmp_clientinfo->igmp_msg_type == MLD_V2_LISTENER_REPORT)
							igmp_pkt_remove_mld_v2_group(igmp_clientinfo,igmp_clientinfo->vid,pos->group_ip.ipv6,pos->src_ip.ipv6); 
						igmp_clientinfo->group_num--;
						list_del(&pos->entry_node);
						if (pos) free_safe(pos);
					}
				} while (meinfo_me_attr_get_table_entry_next(subscriber_monitor_me, 6, &table_context, active_entry_data) == MEINFO_RW_OK);
			}
		}
		free_safe(active_entry_data);
	}
	else
	{
		active_entry_data=malloc_safe(24);
		if ((entry_count = meinfo_me_attr_get_table_entry_count(subscriber_monitor_me, 5)) > 0) 
		{
			if (meinfo_me_attr_get_table_entry(subscriber_monitor_me, 5, &table_context, active_entry_data) == MEINFO_RW_OK)
			{
				do
				{
					if (!((util_bitmap_get_value(active_entry_data, 24*8, 0, 16) == 0 && igmp_clientinfo->vid == 4096) || (igmp_clientinfo->vid == util_bitmap_get_value(active_entry_data, 24*8, 0, 16))))
						continue;
						
					if (memcmp(&igmp_clientinfo->client_ip , active_entry_data+14 ,sizeof(struct in_addr)))
						continue;
	
					struct igmp_group_t *pos,*n;
					list_for_each_entry_safe( pos, n, &igmp_clientinfo->group_list, entry_node ) {
						if (memcmp(&pos->group_ip, active_entry_data+6,sizeof(struct in_addr)))
							continue;
						igmp_mbship_client_access_time_update( &igmp_mbship_db_g, igmp_clientinfo, pos);
						if (igmp_clientinfo->igmp_msg_type == IGMP_V3_MEMBERSHIP_REPORT)
							igmp_pkt_remove_igmp_v3_group(igmp_clientinfo,igmp_clientinfo->vid,pos->group_ip.ipv4,pos->src_ip.ipv4); 
						igmp_clientinfo->group_num--;
						list_del(&pos->entry_node);
						if (pos) free_safe(pos);
					}
				} while (meinfo_me_attr_get_table_entry_next(subscriber_monitor_me, 5, &table_context, active_entry_data) == MEINFO_RW_OK);
			}
		}
		free_safe(active_entry_data);
	}

	if (group_num > igmp_clientinfo->group_num) return 1;
	
	return 0;
}

static int
igmp_omci_acl_table_preview_entry_find(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo, 
		struct igmp_group_t *igmp_group, struct igmp_omci_acl_node_t *acl_node)
{
	struct list_head dynamic_acl_list;
	struct attr_table_header_t *tab_header = NULL;
	struct me_t *profile_me=NULL;
	int entry_num=0;

	INIT_LIST_HEAD(&dynamic_acl_list);

	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		

	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0) 
	{
		// check multcast service package table
		profile_me = igmp_omci_get_mspt_op_profile_me( bridge_port_me, igmp_clientinfo->vid); 
		if (!profile_me) profile_me = igmp_omci_get_op_profile_me(bridge_port_me);

		if (profile_me==NULL)
		{
			if (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)
				return IGMP_JOIN_GRANTED_RELATED_ME_NOT_FOUND;
			else
				return  IGMP_JOIN_DENIED_RELATED_ME_NOT_FOUND;
		}

		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 7);// dynamic acl table
		if ((entry_num = igmp_omci_load_acl_table_988(&dynamic_acl_list, tab_header)) <= 0)
			dbprintf_igmp(LOG_WARNING, "dynamic acl table is empty\n") ;

		if (igmp_omci_get_preview_acl_node_988( &dynamic_acl_list, igmp_group, igmp_clientinfo, acl_node) == 0)
		{
			igmp_omci_unload_acl_table(&dynamic_acl_list); 
			return IGMP_JOIN_PREVIEW_ACL_ENTRY_FOUND; // dynamic acl entry is found	
		}

	}
	igmp_omci_unload_acl_table(&dynamic_acl_list); 

	return IGMP_JOIN_PREVIEW_ACL_ENTRY_NOT_FOUND;
}

static int igmp_omci_mspt_acl_table_entry_find(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group, struct igmp_omci_acl_node_t *acl_node)
{
	struct list_head dynamic_acl_list;
	struct list_head static_acl_list;
	struct attr_table_header_t *tab_header = NULL;
        struct attr_table_entry_t *tab_entry = NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	int entry_num=0;
        struct me_t *me310;
        int ret = IGMP_JOIN_FULL_AUTH_ACL_ENTRY_NOT_FOUND;

	INIT_LIST_HEAD(&dynamic_acl_list);
	INIT_LIST_HEAD(&static_acl_list);
        
        if (bridge_port_me != NULL) me310 = igmp_omci_get_subscriber_config_me(bridge_port_me);
        if (bridge_port_me == NULL || me310 == NULL)
        {
		struct meinfo_t *miptr= meinfo_util_miptr(309);
		if (miptr==NULL)
		{
			dbprintf_igmp(LOG_ERR, "no associated me 309/310 error\n", 309);
			return ret;
		}
        }
        
        // load me309 pointed by field 2 first
        struct attr_table_header_t *tab_header_mspt = NULL;
	struct me_t *mspt_me = meinfo_me_get_by_meid(309,  meinfo_util_me_attr_data(me310, 2));
        if(mspt_me) {
	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0)
	{
                // static acl table
                tab_header_mspt = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 8);
                if ((entry_num= igmp_omci_load_acl_table_988(&static_acl_list, tab_header_mspt)) <= 0)
                dbprintf_igmp(LOG_WARNING, "me309-%d static acl table is empty\n", mspt_me->meid); 
                // dynamic acl table
                tab_header_mspt = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 7);
                if ((entry_num = igmp_omci_load_acl_table_988(&dynamic_acl_list, tab_header_mspt)) <= 0)
                dbprintf_igmp(LOG_WARNING, "me309-%d dynamic acl table is empty\n", mspt_me->meid);                  
        }
        else
        {
                // static acl table
                tab_header_mspt = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 8);
                if ((entry_num= igmp_omci_load_acl_table(&static_acl_list, tab_header_mspt)) <= 0)
                dbprintf_igmp(LOG_WARNING, "me309-%d static acl table is empty\n", mspt_me->meid); 
                // dynamic acl table
                tab_header_mspt = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 7);
                if ((entry_num = igmp_omci_load_acl_table(&dynamic_acl_list, tab_header_mspt)) <= 0)
                dbprintf_igmp(LOG_WARNING, "me309-%d dynamic acl table is empty\n", mspt_me->meid);     
        }   
        }
        //multicast service package table  
	tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me310 , 6);
	if (tab_header == NULL)
	{
                dbprintf_igmp(LOG_WARNING, "associated me 310 defines no additional 309");
                goto ACL_NODE_MATCHING;
        }

	list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
	        unsigned short vid_uni = 0;
                
		if (tab_entry->entry_data_ptr == NULL) continue;
		
                vid_uni = (unsigned short)util_bitmap_get_value(tab_entry->entry_data_ptr, 20*8, 2*8, 16);
                if (!layer2bridged)
                {
	                if (vid_uni == 0xffff)  continue;       // undefined, ignore
	                if (vid_uni == 4097)                    // apply to tagged messages but ignore vlan id
	                {
	                        if (igmp_clientinfo->vid == 0) continue;
	                }
	                        
	                if (vid_uni == 4096)                    // apply to untagged messages
	                {
	                        if (igmp_clientinfo->vid != 0) continue;        
	                }
	                                
	                if (vid_uni <= 4095)    // match vlan
	                {
	                        if (igmp_clientinfo->vid != vid_uni)    continue;
	                }
                }
                
                // get actual valid ME 309
                int me309_id = util_bitmap_get_value(tab_entry->entry_data_ptr,20*8,10*8,16);
                if (me309_id == meinfo_util_me_attr_data(me310, 2)) continue;   // already loaded
                mspt_me = meinfo_me_get_by_meid(309, me309_id);

                if( meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 )
		{
                        // static acl table
                        tab_header_mspt = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 8);
                        if ( (entry_num= igmp_omci_load_acl_table_988(&static_acl_list, tab_header_mspt)) <= 0 )
                        dbprintf_igmp(LOG_WARNING, "me309-%d static acl table is empty\n", mspt_me->meid); 
                        // dynamic acl table
                        tab_header_mspt = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 7);
                        if ( (entry_num = igmp_omci_load_acl_table_988(&dynamic_acl_list, tab_header_mspt)) <= 0 )
                        dbprintf_igmp(LOG_WARNING, "me309-%d dynamic acl table is empty\n", mspt_me->meid);                  
                }
                else
                {
                        // static acl table
                        tab_header_mspt = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 8);
                        if ( (entry_num= igmp_omci_load_acl_table(&static_acl_list, tab_header_mspt)) <= 0 )
                        dbprintf_igmp(LOG_WARNING, "me309-%d static acl table is empty\n", mspt_me->meid); 
                        // dynamic acl table
                        tab_header_mspt = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mspt_me, 7);
                        if ( (entry_num = igmp_omci_load_acl_table(&dynamic_acl_list, tab_header_mspt)) <= 0 )
                        dbprintf_igmp(LOG_WARNING, "me309-%d dynamic acl table is empty\n", mspt_me->meid);     
                }                         	
	}
        
ACL_NODE_MATCHING:
        if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0)
	{
		if (igmp_omci_get_full_auth_acl_node_988( &static_acl_list, igmp_group, igmp_clientinfo, acl_node) == 0)
		{
			ret = IGMP_JOIN_STATIC_ACL_ENTRY_FOUND; // static acl entry is found
                        goto ACL_NODE_CLEANUP;
		}
                	
		if (igmp_omci_get_full_auth_acl_node_988( &dynamic_acl_list, igmp_group, igmp_clientinfo, acl_node) == 0)
		{
			ret = IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND; // dynamic acl entry is found
                        dbprintf_igmp(LOG_WARNING, "hit dynamic entry 988 vid_ani - %d\n", acl_node->vid_ani);	
		}		        
        }
        else
        {
		if (igmp_omci_get_acl_node( &static_acl_list,  igmp_group, igmp_clientinfo, acl_node) == 0)
		{
			ret = IGMP_JOIN_STATIC_ACL_ENTRY_FOUND; // static entry is found
                        goto ACL_NODE_CLEANUP;
		}
                
		if (igmp_omci_get_acl_node( &dynamic_acl_list, igmp_group, igmp_clientinfo,acl_node) == 0)
		{
			ret = IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND; // dynamic acl entry is found
                        dbprintf_igmp(LOG_WARNING, "hit dynamic entry vid_ani - %d\n", acl_node->vid_ani);	
		}
	}

ACL_NODE_CLEANUP:                                            
        igmp_omci_unload_acl_table(&static_acl_list); 
	igmp_omci_unload_acl_table(&dynamic_acl_list); 

	return ret;
}
/*
static int
igmp_omci_acl_table_entry_find(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo, 
		struct igmp_group_t *igmp_group, struct igmp_omci_acl_node_t *acl_node)
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
		profile_me = igmp_omci_get_mspt_op_profile_me( bridge_port_me, igmp_clientinfo->vid ); 
		if (!profile_me) 
			profile_me = igmp_omci_get_op_profile_me(bridge_port_me);

		if (profile_me==NULL) {
			if (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)
				return IGMP_JOIN_GRANTED_RELATED_ME_NOT_FOUND;
			else
				return  IGMP_JOIN_DENIED_RELATED_ME_NOT_FOUND;
		}

		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 8);// static acl table
		if ( (entry_num= igmp_omci_load_acl_table_988(&static_acl_list, tab_header)) <= 0 )
			dbprintf_igmp(LOG_WARNING, "static acl table is empty\n") ;

		if( igmp_omci_get_full_auth_acl_node_988( &static_acl_list, igmp_group, 
					igmp_clientinfo, acl_node) == 0 ) {
			igmp_omci_unload_acl_table(&static_acl_list); 
			return IGMP_JOIN_STATIC_ACL_ENTRY_FOUND; // static acl entry is found	
		}

		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 7);// dynamic acl table
		if ( (entry_num = igmp_omci_load_acl_table_988(&dynamic_acl_list, tab_header)) <= 0 )
			dbprintf_igmp(LOG_WARNING, "dynamic acl table is empty\n") ;

		if( igmp_omci_get_full_auth_acl_node_988( &dynamic_acl_list, igmp_group,
				       igmp_clientinfo, acl_node) == 0 ){
			igmp_omci_unload_acl_table(&dynamic_acl_list); 
			igmp_omci_unload_acl_table(&static_acl_list); 
			return IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND; // dynamic acl entry is found	
		}

	} else {
		profile_me = igmp_omci_get_op_profile_me(bridge_port_me);

		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 8);// static acl table
		if ( (entry_num= igmp_omci_load_acl_table(&static_acl_list, tab_header)) <= 0 )
			dbprintf_igmp(LOG_WARNING, "static acl table is empty\n"); 

		if( igmp_omci_get_acl_node( &static_acl_list,  igmp_group, igmp_clientinfo, acl_node) == 0 ){
			igmp_omci_unload_acl_table(&static_acl_list); 
			return IGMP_JOIN_STATIC_ACL_ENTRY_FOUND; // static entry is found
		}

		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(profile_me, 7);// dynamic acl table
		if ( (entry_num = igmp_omci_load_acl_table(&dynamic_acl_list, tab_header)) <= 0 )
			dbprintf_igmp(LOG_WARNING, "dynamic acl table is empty\n"); 

		if( igmp_omci_get_acl_node( &dynamic_acl_list, igmp_group, igmp_clientinfo,acl_node) == 0 ){
			igmp_omci_unload_acl_table(&static_acl_list); 
			igmp_omci_unload_acl_table(&dynamic_acl_list); 
			return IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND; // dynamic acl entry is found	
		}
	}

	igmp_omci_unload_acl_table(&static_acl_list); 
	igmp_omci_unload_acl_table(&dynamic_acl_list); 

	return IGMP_JOIN_FULL_AUTH_ACL_ENTRY_NOT_FOUND;
}
*/


//if item == 0 means max simutaneous group 
//if item == 1 means max multicast bandwidth
static int
igmp_omci_subscriber_max_limit( struct igmp_clientinfo_t *igmp_clientinfo, unsigned char item, unsigned char scope, unsigned int *limit)
{
	struct me_t *bridge_port_me=NULL;
	struct me_t *subscriber_config_me=NULL;
	struct attr_table_entry_t *tab_entry=NULL;

	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	bridge_port_me= igmp_omci_mcast_bridge_port_get(igmp_clientinfo->rx_desc.bridge_port_me);
	if (bridge_port_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "bridge port me or VEIP port is NULL.\n");
		return -1;
	}

	subscriber_config_me = igmp_omci_get_subscriber_config_me(bridge_port_me);
	if (subscriber_config_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp subscriber config me\n", bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);
		return -1;
	}

	int tag_num = igmp_pkt_tag_num_get(igmp_clientinfo->pktinfo);
	if (scope == IGMP_VID_SCOPE && meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0)
	{
		if (tag_num == 0)
		{
			tab_entry = igmp_omci_get_mspt_entry_data(subscriber_config_me, 4096);
			if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) && (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) &&
			     (proprietary_alu_get_olt_version() == 0))
			{
				struct me_t *meptr = meinfo_me_get_by_meid(65302, 0);
				// For ALU 7342 OLT, check ethertype workaround mspt
				if(meptr && (meinfo_util_me_attr_data(meptr, 1) == 1))
				{
					if (!tab_entry) tab_entry = igmp_omci_get_mspt_entry_data(subscriber_config_me, 4081);
				}
			}
		} else 
			tab_entry = igmp_omci_get_mspt_entry_data(subscriber_config_me, igmp_clientinfo->vid );

		// check multcast service package table
		if (tab_entry == NULL)
		{
			dbprintf_igmp(LOG_WARNING, "the assigned mspt entry not found!\n", subscriber_config_me->classid,subscriber_config_me->meid, subscriber_config_me->meid);
			return 1;
		}
		else
		{
			if (item == 0)
			{
				*limit = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 20*8, 4*8, 2*8);
				return 0;
			}
			if (item == 1)
			{
				*limit = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 20*8, 6*8, 4*8);
				return 0;
			}
		}

	}
	
	if (scope == IGMP_PORT_SCOPE)
	{
		if (item == 0)
		{
			*limit =  (unsigned int)meinfo_util_me_attr_data(subscriber_config_me, 3);		
			return 0;
		}
		if (item == 1)
		{
			*limit =  (unsigned int)meinfo_util_me_attr_data(subscriber_config_me, 4);		
			return 0;
		}
	}
	dbprintf_igmp(LOG_WARNING, "cannot find matched item in subscriber config me\n" );
	
	return 1;
}

static int
igmp_omci_max_allowed_groups( struct igmp_clientinfo_t *igmp_clientinfo , unsigned char mode , unsigned int *max)
{
	return igmp_omci_subscriber_max_limit(igmp_clientinfo , 0, mode, max); 
}

static int
igmp_omci_max_allowed_bandwidth( struct igmp_clientinfo_t *igmp_clientinfo, unsigned char mode, unsigned int *max)
{
	return igmp_omci_subscriber_max_limit(igmp_clientinfo , 1, mode, max); 
}


static int
igmp_omci_allowed_preview_group_table_entry_find(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group, struct igmp_omci_allowed_preview_node_t *apgt_node)
{
	struct me_t *subscriber_config_me=igmp_omci_get_subscriber_config_me(bridge_port_me);
	struct list_head apgt_list;
	struct attr_table_header_t *tab_header = NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	int entry_num=0;
	
	INIT_LIST_HEAD(&apgt_list);

	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 )
	{
		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(subscriber_config_me, 7);// allowed preview groups table
		if ((entry_num= igmp_omci_load_allowed_preview_groups_table(&apgt_list, tab_header)) <= 0)
			dbprintf_igmp(LOG_WARNING, "static acl table is empty\n"); 

		if (igmp_omci_get_allowed_preview_groups_table_node(&apgt_list, igmp_group, igmp_clientinfo, apgt_node ) == 0)
		{
			igmp_omci_unload_apgt_table(&apgt_list);
			return IGMP_JOIN_APGT_ENTRY_FOUND; // allowed preview group table entry is found
		}
	}
	igmp_omci_unload_apgt_table( &apgt_list );
	
	return IGMP_JOIN_APGT_ENTRY_NOT_FOUND; // allowed preview group table entry is found
}



int
igmp_omci_max_allowed_limit_filter(struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct me_t *bridge_port_me=NULL;
	struct me_t *subscriber_config_me=NULL;
	struct me_t *subscriber_monitor_me=NULL;
	//struct me_t *profile_me=NULL;
	struct me_t *mspt_profile_me=NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	unsigned int max_bandwidth=0;
	unsigned int max_group=0;

	unsigned int current_group_per_port=0;
	unsigned int current_group_per_vid=0;
	unsigned int current_bandwidth_per_port=0;
	unsigned int current_bandwidth_per_vid=0;
	unsigned int imputed_group_bandwidth=0; 

	if (igmp_clientinfo == NULL || list_empty(&igmp_clientinfo->group_list)) return 0;

	bridge_port_me =  igmp_omci_mcast_bridge_port_get(igmp_clientinfo->rx_desc.bridge_port_me);

	if (bridge_port_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "bridge port or VEIP port is NULL.\n");
		return -1;
	}

	subscriber_config_me = igmp_omci_get_subscriber_config_me(bridge_port_me);
	if (subscriber_config_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp subscriber config me\n", bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);
		return -1;
	}

	subscriber_monitor_me = igmp_omci_get_subscriber_monitor_me(bridge_port_me);
	if (subscriber_monitor_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp subscriber monitor me\n", bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);
		return 0;
	}

	//profile_me = igmp_omci_get_op_profile_me( bridge_port_me );
	mspt_profile_me = igmp_omci_get_mspt_op_profile_me( bridge_port_me, igmp_clientinfo->vid);

	struct list_head active_group_list;
	INIT_LIST_HEAD(&active_group_list);

	//extended 988 config ( per vid config)
	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 && igmp_omci_is_service_pkg_table_empty(subscriber_config_me) == 0)
	{

		if (mspt_profile_me == NULL ) return -1;

		current_group_per_vid = igmp_omci_groups_in_active_group_list(bridge_port_me,igmp_clientinfo, &active_group_list, IGMP_VID_SCOPE);
		struct igmp_omci_group_t *active_pos=NULL,*active_n = NULL;
		list_for_each_entry_safe( active_pos, active_n, &active_group_list, entry_node) {
			current_bandwidth_per_vid += active_pos->bandwidth;
		}

		igmp_omci_max_allowed_groups( igmp_clientinfo, IGMP_VID_SCOPE, &max_group);
		igmp_omci_max_allowed_bandwidth( igmp_clientinfo, IGMP_VID_SCOPE, &max_bandwidth);

		struct igmp_group_t *pos,*n;
		list_for_each_entry_safe( pos, n, &igmp_clientinfo->group_list, entry_node ) {
			struct igmp_omci_acl_node_t  acl_node;
			struct igmp_omci_allowed_preview_node_t apgt_node;
			struct igmp_omci_acl_node_t  preview_acl_node;

			if (igmp_omci_mspt_acl_table_entry_find(bridge_port_me, igmp_clientinfo, pos, &acl_node) == IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND)
				imputed_group_bandwidth = acl_node.imputed_bw;
			else if (igmp_omci_allowed_preview_group_table_entry_find( bridge_port_me, igmp_clientinfo, pos, &apgt_node) == IGMP_JOIN_APGT_ENTRY_FOUND) 
				imputed_group_bandwidth = apgt_node.imputed_bw;
			else if (igmp_omci_acl_table_preview_entry_find(bridge_port_me, igmp_clientinfo, pos, &preview_acl_node) == IGMP_JOIN_PREVIEW_ACL_ENTRY_FOUND) 
				imputed_group_bandwidth = preview_acl_node.imputed_bw;
								
			if (igmp_clientinfo->ethertype == ETH_P_IPV6)
			{
				if (igmp_omci_locate_ipv6_active_group_list_client(bridge_port_me, acl_node.vid_ani, pos->group_ip.ipv6, pos->src_ip.ipv6, igmp_clientinfo->client_ip.ipv6 ,IGMP_ACTIVE_GROUP_LIST_GROUP))
					continue;
			}
			else
			{
				if (igmp_omci_locate_ipv4_active_group_list_client(bridge_port_me, acl_node.vid_ani, pos->group_ip.ipv4, pos->src_ip.ipv4, igmp_clientinfo->client_ip.ipv4 ,IGMP_ACTIVE_GROUP_LIST_GROUP))
					continue;
			}

			//check group overflow
			if (max_group != 0 && current_group_per_vid +1 >  max_group)
			{
				dbprintf_igmp(LOG_WARNING, "group overflow at vid=%d class=%d meid=0x%x(%u)\n", igmp_clientinfo->vid,bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid); 
				igmp_clientinfo->group_num -= 1;
				list_del(&pos->entry_node);
				if(pos) free_safe(pos);
				continue;
			}

			//check bandwidth overflow
			if (max_bandwidth != 0)
			{				
				if (current_bandwidth_per_vid + imputed_group_bandwidth  > max_bandwidth)
				{
					dbprintf_igmp(LOG_WARNING, "bandwidth overflow at vid=%d class=%d meid=0x%x(%u)\n", igmp_clientinfo->vid,bridge_port_me->classid, bridge_port_me->meid,bridge_port_me->meid); 
					// inc bandwidth exceed join counter
					if (subscriber_monitor_me)
					{
						struct attr_value_t attr={0};
						attr.data = meinfo_util_me_attr_data(subscriber_monitor_me, 4)+1;	
						meinfo_me_attr_set(subscriber_monitor_me, 4, &attr, 1);
						if (meinfo_util_me_attr_data(subscriber_config_me, 5))
						{
							igmp_clientinfo->group_num -= 1;
							list_del(&pos->entry_node);
							if(pos) free_safe(pos);
							continue;
						}
					}
				}
			}
		}
		igmp_omci_free_groups_in_active_group_list(&active_group_list);
		
		return 0;
	}
	else
	{
		current_group_per_port = igmp_omci_groups_in_active_group_list(bridge_port_me ,igmp_clientinfo, &active_group_list, IGMP_PORT_SCOPE);
		struct igmp_omci_group_t *active_pos=NULL,*active_n = NULL;
		list_for_each_entry_safe( active_pos, active_n, &active_group_list, entry_node) {
			current_bandwidth_per_port += active_pos->bandwidth;
		}

		igmp_omci_max_allowed_groups( igmp_clientinfo, IGMP_PORT_SCOPE, &max_group);
		igmp_omci_max_allowed_bandwidth( igmp_clientinfo, IGMP_PORT_SCOPE, &max_bandwidth);

		struct igmp_group_t *pos,*n;
		list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {
			struct igmp_omci_acl_node_t  acl_node;
			struct igmp_omci_acl_node_t  preview_acl_node;
			struct igmp_omci_allowed_preview_node_t apgt_node;
                                
			if (igmp_omci_mspt_acl_table_entry_find(bridge_port_me, igmp_clientinfo, pos, &acl_node) == IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND)
				imputed_group_bandwidth = acl_node.imputed_bw;
			else if (igmp_omci_allowed_preview_group_table_entry_find( bridge_port_me, igmp_clientinfo, pos,  &apgt_node) == IGMP_JOIN_APGT_ENTRY_FOUND) 
				imputed_group_bandwidth = apgt_node.imputed_bw;
			else if (igmp_omci_acl_table_preview_entry_find(bridge_port_me,igmp_clientinfo, pos, &preview_acl_node) == IGMP_JOIN_PREVIEW_ACL_ENTRY_FOUND) 
				imputed_group_bandwidth = preview_acl_node.imputed_bw;
					
			if (igmp_clientinfo->ethertype == ETH_P_IPV6)
			{
				if (igmp_omci_locate_ipv6_active_group_list_client(bridge_port_me,acl_node.vid_ani, pos->group_ip.ipv6, pos->src_ip.ipv6, igmp_clientinfo->client_ip.ipv6 ,IGMP_ACTIVE_GROUP_LIST_GROUP))
					continue;
			}
			else
			{
				if (igmp_omci_locate_ipv4_active_group_list_client(bridge_port_me,acl_node.vid_ani, pos->group_ip.ipv4, pos->src_ip.ipv4, igmp_clientinfo->client_ip.ipv4 ,IGMP_ACTIVE_GROUP_LIST_GROUP))
					continue;
			}
			//check group overflow
			if (max_group != 0 && current_group_per_port +1 >  max_group)
			{
				dbprintf_igmp(LOG_WARNING, "group overflow at bridge port class=%d meid=0x%x(%u)\n", bridge_port_me->classid, bridge_port_me->meid,bridge_port_me->meid); 
				igmp_clientinfo->group_num -= 1;
                                list_del(&pos->entry_node);
				if(pos) free_safe(pos);
				continue;
			}

			//check bandwidth overflow
			if (max_bandwidth != 0) //is_bandwidth_enforcement
			{
				if (current_bandwidth_per_port + imputed_group_bandwidth  > max_bandwidth)
				{
					dbprintf_igmp(LOG_WARNING, "bandwidth overflow at bridge port class=%d meid=0x%x(%u)\n", bridge_port_me->classid, bridge_port_me->meid,bridge_port_me->meid); 
					// inc bandwidth exceed join counter
					if(subscriber_monitor_me)
					{
						struct attr_value_t attr={0};
						attr.data = meinfo_util_me_attr_data(subscriber_monitor_me, 4)+1;
						meinfo_me_attr_set(subscriber_monitor_me, 4, &attr, 1);
						if (meinfo_util_me_attr_data(subscriber_config_me, 5))
						{
							igmp_clientinfo->group_num -= 1;
							list_del(&pos->entry_node);
							if(pos) free_safe(pos);
							continue;
						}
					}
				}
			}
		}
		igmp_omci_free_groups_in_active_group_list(&active_group_list);
		return 0;
	}
	
	return -1;
}




//////////////////////////////////////////////////////////////////////////

// api for igmpd /////////////////////////////////////////////////////////
int
igmp_omci_check_join_is_allowed(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo)
{
	//struct me_t *subscriber_monitor_me=igmp_omci_get_subscriber_monitor_me(bridge_port_me);
	struct me_t *subscriber_config_me=NULL;
	struct me_t *profile_me=NULL;
	struct me_t *mcast_port=NULL;

	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		

	if (!omci_env_g.igmp_mcast_me_support) return IGMP_JOIN_GRANTED;
	if (igmp_clientinfo == NULL || list_empty(&igmp_clientinfo->group_list)) return IGMP_JOIN_DENIED_ACL_FILTERED;

	mcast_port = igmp_omci_mcast_bridge_port_get(bridge_port_me);
	if (!mcast_port) dbprintf_igmp(LOG_WARNING, "bridge port or VEIP port is NULL\n");
	
	if (mcast_port) subscriber_config_me = igmp_omci_get_subscriber_config_me(mcast_port);
	if (subscriber_config_me==NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp subscriber config me\n", bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);
		if (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)
			return IGMP_JOIN_GRANTED_RELATED_ME_NOT_FOUND;
		else
			return IGMP_JOIN_DENIED_ACL_FILTERED;
	}

	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0)
	{
		// check multcast service package table
		profile_me = igmp_omci_get_mspt_op_profile_me( bridge_port_me, igmp_clientinfo->vid); 
		if (!profile_me) profile_me = igmp_omci_get_op_profile_me(bridge_port_me);
	} else 
		profile_me = igmp_omci_get_op_profile_me(bridge_port_me);


	if (profile_me == NULL)
	{
		if (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)
			return IGMP_JOIN_GRANTED_RELATED_ME_NOT_FOUND;
		else
			return IGMP_JOIN_DENIED_ACL_FILTERED;
	}
	
	if (igmp_mbship_client_already_exist(&igmp_mbship_db_g, igmp_clientinfo))
	{
		dbprintf_igmp(LOG_WARNING, "IGMP_JOIN_DENIED_CLIENT_ALREADY_EXIST\n"); 
		return IGMP_JOIN_DENIED_ACL_FILTERED;	// the client has ever joined 
	}	
	
	igmp_omci_rm_illeagal_group_in_client(mcast_port, igmp_clientinfo);	// actual static & dynamic SACL checl

	int ret = 0;
	ret = igmp_omci_rm_active_group_list_group_in_client(mcast_port, igmp_clientinfo);
	if (list_empty(&igmp_clientinfo->group_list))
	{
		if (ret == 1)
		{
			dbprintf_igmp(LOG_WARNING, "IGMP JOIN ALREADY BY SAME CLIENT\n"); 
			return IGMP_JOIN_ALREADY_BY_SAME_CLIENT;	// the client has ever joined this group
		}
		dbprintf_igmp(LOG_WARNING, "IGMP JOIN FITLERED\n");
		return IGMP_JOIN_DENIED_ACL_FILTERED;
	}

	if (igmp_omci_max_allowed_limit_filter(igmp_clientinfo) < 0)
	{
		dbprintf_igmp(LOG_WARNING, "IGMP JOIN BANDWIDTH OR GROUP OVERFLOW\n"); 
		return IGMP_JOIN_DENIED_ACL_FILTERED;
	}

	if (list_empty(&igmp_clientinfo->group_list) || igmp_clientinfo->group_num == 0)
	{
		dbprintf_igmp(LOG_WARNING, "IGMP JOIN DENIED ACL FILTERED\n"); 
		return IGMP_JOIN_DENIED_ACL_FILTERED;
	}
	
	return IGMP_JOIN_GRANTED;		// igmpd should add membership, call igmp_omci_update_join() later
}

int
igmp_omci_update_join(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct me_t *subscriber_monitor_me=NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	struct me_t *profile_me=NULL;
	struct me_t *mcast_port=NULL;

	struct attr_value_t attr={0};
	struct igmp_omci_acl_node_t acl_node,preview_acl_node;
	struct igmp_omci_allowed_preview_node_t  apgt_node;

	if (igmp_clientinfo == NULL || list_empty(&igmp_clientinfo->group_list)) return 0;

	mcast_port = igmp_omci_mcast_bridge_port_get(bridge_port_me);
	if (!mcast_port) dbprintf_igmp(LOG_WARNING, "bridge port or VEIP port is NULL\n");

	subscriber_monitor_me=igmp_omci_get_subscriber_monitor_me(mcast_port);
	if (subscriber_monitor_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp subscriber monitor me\n", mcast_port->classid, mcast_port->meid, mcast_port->meid);
		return 0;
	}

	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0)
	{
		// check multcast service package table
		profile_me = igmp_omci_get_mspt_op_profile_me( mcast_port, igmp_clientinfo->vid); 
		if (!profile_me) profile_me = igmp_omci_get_op_profile_me(mcast_port);
	}
	else 
		profile_me = igmp_omci_get_op_profile_me(mcast_port);

	if (profile_me == NULL)
	{
		if (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)
			return IGMP_JOIN_GRANTED_RELATED_ME_NOT_FOUND;
		else
			return  IGMP_JOIN_DENIED_RELATED_ME_NOT_FOUND;
	}


	//add group to active group list if the group not exist
	void *active_entry_data=NULL;
	void *found_entry_data=NULL;
	struct igmp_group_t *pos=NULL,*n=NULL;
	unsigned int imputed_group_bandwidth=0, current_multicast_bandwidth=0;

	list_for_each_entry_safe( pos, n, &igmp_clientinfo->group_list, entry_node) {
		if (igmp_clientinfo->ethertype == ETH_P_IPV6)
		{
			if ((pos->group_ip.ipv6.s6_addr32[0]!=0 || pos->group_ip.ipv6.s6_addr32[1]!=0 || pos->group_ip.ipv6.s6_addr32[2]!=0 || pos->group_ip.ipv6.s6_addr32[3]!=0) && (found_entry_data == NULL))
			{
				found_entry_data=igmp_omci_locate_ipv6_active_group_list_client(mcast_port,igmp_clientinfo->vid,pos->group_ip.ipv6,pos->src_ip.ipv6,igmp_clientinfo->client_ip.ipv6, IGMP_ACTIVE_GROUP_LIST_CLIENT);
				
				// get ACL group bandwidth
				if (igmp_omci_mspt_acl_table_entry_find(mcast_port, igmp_clientinfo, pos, &acl_node) == IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND)
					imputed_group_bandwidth = acl_node.imputed_bw;
				else if (igmp_omci_allowed_preview_group_table_entry_find( mcast_port, igmp_clientinfo, pos,  &apgt_node) == IGMP_JOIN_APGT_ENTRY_FOUND) 
					imputed_group_bandwidth = apgt_node.imputed_bw;
				else if (igmp_omci_acl_table_preview_entry_find(mcast_port, igmp_clientinfo, pos, &preview_acl_node) == IGMP_JOIN_PREVIEW_ACL_ENTRY_FOUND) 
					imputed_group_bandwidth = preview_acl_node.imputed_bw;
	
				active_entry_data=malloc_safe(58);
				
				if (igmp_clientinfo->vid == 4096) 
					util_bitmap_set_value(active_entry_data, 58*8, 0, 2*8, 0); 
				else
					util_bitmap_set_value(active_entry_data, 58*8, 0, 2*8, igmp_clientinfo->vid); 
				
				util_bitmap_set_value(active_entry_data, 58*8, 2*8, 4*8, ntohl(*(int *)&pos->src_ip.ipv6.s6_addr32[0])); 
				util_bitmap_set_value(active_entry_data, 58*8, 6*8, 4*8, ntohl(*(int *)&pos->src_ip.ipv6.s6_addr32[1])); 
				util_bitmap_set_value(active_entry_data, 58*8, 10*8, 4*8, ntohl(*(int *)&pos->src_ip.ipv6.s6_addr32[2])); 
				util_bitmap_set_value(active_entry_data, 58*8, 14*8, 4*8, ntohl(*(int *)&pos->src_ip.ipv6.s6_addr32[3])); 
				util_bitmap_set_value(active_entry_data, 58*8, 18*8, 4*8, ntohl(*(int *)&pos->group_ip.ipv6.s6_addr32[0])); 
				util_bitmap_set_value(active_entry_data, 58*8, 22*8, 4*8, ntohl(*(int *)&pos->group_ip.ipv6.s6_addr32[1])); 
				util_bitmap_set_value(active_entry_data, 58*8, 26*8, 4*8, ntohl(*(int *)&pos->group_ip.ipv6.s6_addr32[2])); 
				util_bitmap_set_value(active_entry_data, 58*8, 30*8, 4*8, ntohl(*(int *)&pos->group_ip.ipv6.s6_addr32[3]));
				util_bitmap_set_value(active_entry_data, 58*8, 34*8, 4*8, imputed_group_bandwidth ); 
				util_bitmap_set_value(active_entry_data, 58*8, 38*8, 4*8, ntohl(*(int *)&igmp_clientinfo->client_ip.ipv6.s6_addr32[0])); 
				util_bitmap_set_value(active_entry_data, 58*8, 42*8, 4*8, ntohl(*(int *)&igmp_clientinfo->client_ip.ipv6.s6_addr32[1])); 
				util_bitmap_set_value(active_entry_data, 58*8, 46*8, 4*8, ntohl(*(int *)&igmp_clientinfo->client_ip.ipv6.s6_addr32[2])); 
				util_bitmap_set_value(active_entry_data, 58*8, 50*8, 4*8, ntohl(*(int *)&igmp_clientinfo->client_ip.ipv6.s6_addr32[3]));
				util_bitmap_set_value(active_entry_data, 58*8, 54*8, 4*8, igmp_timer_get_cur_time()); 
				
				igmp_omci_add_ipv6_active_group_list(active_entry_data, subscriber_monitor_me); 
				free_safe(active_entry_data);
			}
			else
			{
				util_bitmap_set_value(found_entry_data , 58*8, 54*8, 4*8, igmp_timer_get_cur_time()); 
			}
		}
		else
		{
			if ((pos->group_ip.ipv4.s_addr!=0) && (found_entry_data == NULL))
			{
				found_entry_data=igmp_omci_locate_ipv4_active_group_list_client(mcast_port,igmp_clientinfo->vid,pos->group_ip.ipv4,pos->src_ip.ipv4,igmp_clientinfo->client_ip.ipv4, IGMP_ACTIVE_GROUP_LIST_CLIENT);

				active_entry_data=malloc_safe(24);
				
				//if (igmp_clientinfo->vid == 4096) 
					util_bitmap_set_value(active_entry_data, 24*8, 0, 2*8, 0); 
				//else
				//	util_bitmap_set_value(active_entry_data, 24*8, 0, 2*8, igmp_clientinfo->vid);
                                        				
				// get ACL group bandwidth
				if (igmp_omci_mspt_acl_table_entry_find(mcast_port, igmp_clientinfo, pos, &acl_node) == IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND)
				{
					imputed_group_bandwidth = acl_node.imputed_bw;
					// calix suggests ME311 vid implies ANI vid not UNI
					util_bitmap_set_value(active_entry_data, 24*8, 0, 2*8, acl_node.vid_ani);
					igmp_clientinfo->ani_vid = acl_node.vid_ani;
				}
				else if (igmp_omci_allowed_preview_group_table_entry_find( mcast_port, igmp_clientinfo, pos,  &apgt_node) == IGMP_JOIN_APGT_ENTRY_FOUND) 
					imputed_group_bandwidth = apgt_node.imputed_bw;
				else if (igmp_omci_acl_table_preview_entry_find(mcast_port, igmp_clientinfo, pos, &preview_acl_node) == IGMP_JOIN_PREVIEW_ACL_ENTRY_FOUND) 
					imputed_group_bandwidth = preview_acl_node.imputed_bw; 
				
				util_bitmap_set_value(active_entry_data, 24*8, 2*8, 4*8, ntohl(*(int *)&pos->src_ip)); 
				util_bitmap_set_value(active_entry_data, 24*8, 6*8, 4*8, ntohl(*(int *)&pos->group_ip)); 
				util_bitmap_set_value(active_entry_data, 24*8, 10*8, 4*8, imputed_group_bandwidth ); 
				util_bitmap_set_value(active_entry_data, 24*8, 14*8, 4*8, ntohl(*(int *)&igmp_clientinfo->client_ip)); 
				util_bitmap_set_value(active_entry_data, 24*8, 18*8, 4*8, igmp_timer_get_cur_time()); 
				
				igmp_omci_add_ipv4_active_group_list(active_entry_data, subscriber_monitor_me); 
				free_safe(active_entry_data);
			}
			else
			{
				util_bitmap_set_value(found_entry_data , 24*8, 18*8, 4*8, igmp_timer_get_cur_time()); 
			}
		}
		current_multicast_bandwidth += imputed_group_bandwidth;
	}


	// inc accepted join counter
	attr.data = meinfo_util_me_attr_data(subscriber_monitor_me, 3)+1;	
	meinfo_me_attr_set(subscriber_monitor_me, 3, &attr, 1);
	
	// update current bandwidth usage with sum of imputed bandwidth, FAKED
	//attr.data = igmp_omci_total_bandwidth_in_active_group_list( mcast_port, igmp_clientinfo->vid,IGMP_PORT_SCOPE);
	attr.data = current_multicast_bandwidth;
	meinfo_me_attr_set(subscriber_monitor_me, 2, &attr, 1);
	
	return 0;
}

int
igmp_omci_update_leave(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo)
{
	struct me_t *subscriber_monitor_me = NULL;
	void *active_entry_data = NULL;
	void *found_entry_data = NULL;
	struct attr_value_t attr = {0};
	int ani_vid = 0;
	struct me_t *mcast_port;
	
	mcast_port= igmp_omci_mcast_bridge_port_get(bridge_port_me);
	if (!mcast_port) dbprintf_igmp(LOG_WARNING, "bridge port or veip port is null\n");

	subscriber_monitor_me = igmp_omci_get_subscriber_monitor_me(mcast_port);
	if (subscriber_monitor_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp subscriber monitor me\n", mcast_port->classid, mcast_port->meid, mcast_port->meid);
		return (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)?0:-1;
	}

	if (igmp_clientinfo == NULL || list_empty(&igmp_clientinfo->group_list)) return 0;
	
	struct igmp_group_t *pos=NULL,*n=NULL;
	unsigned int current_multicast_bandwidth = 0;
	list_for_each_entry_safe(pos, n, &igmp_clientinfo->group_list, entry_node) {
		struct igmp_omci_acl_node_t acl_node;
		if (igmp_omci_mspt_acl_table_entry_find(mcast_port, igmp_clientinfo, pos, &acl_node) == IGMP_JOIN_FULL_AUTH_ACL_ENTRY_FOUND)
			ani_vid = acl_node.vid_ani;
		if (igmp_clientinfo->ethertype == ETH_P_IPV6)
		{
			if ((found_entry_data = igmp_omci_locate_ipv6_active_group_list_client(mcast_port, ani_vid, pos->group_ip.ipv6,pos->src_ip.ipv6,igmp_clientinfo->client_ip.ipv6, IGMP_ACTIVE_GROUP_LIST_CLIENT)) == NULL) 
				continue;
			active_entry_data=malloc_safe(58);
			memcpy(active_entry_data, found_entry_data, 58);
			igmp_omci_del_ipv6_active_group_list(active_entry_data, subscriber_monitor_me);
			free_safe(active_entry_data);
		}
		else
		{
			if ((found_entry_data=igmp_omci_locate_ipv4_active_group_list_client(mcast_port, ani_vid, pos->group_ip.ipv4, pos->src_ip.ipv4, igmp_clientinfo->client_ip.ipv4, IGMP_ACTIVE_GROUP_LIST_CLIENT)) == NULL) 
			{
				ani_vid = 4097; // do wildcard search to detect possible multicast profile change
				if ((found_entry_data=igmp_omci_locate_ipv4_active_group_list_client(mcast_port, ani_vid, pos->group_ip.ipv4, pos->src_ip.ipv4, igmp_clientinfo->client_ip.ipv4, IGMP_ACTIVE_GROUP_LIST_CLIENT)) == NULL)
					continue;
			}
			active_entry_data=malloc_safe(24);
			memcpy(active_entry_data, found_entry_data, 24);
			igmp_omci_del_ipv4_active_group_list(active_entry_data, subscriber_monitor_me);
			free_safe(active_entry_data);
		}

		current_multicast_bandwidth += acl_node.imputed_bw;
	}

	// update current bandwidth usage with sum of imputed bandwidth, FAKED
	//attr.data = igmp_omci_total_bandwidth_in_active_group_list( mcast_port, igmp_clientinfo->vid, IGMP_PORT_SCOPE ) ;
	meinfo_me_attr_get(subscriber_monitor_me, 2, &attr);	
	attr.data -= current_multicast_bandwidth;
	meinfo_me_attr_set(subscriber_monitor_me, 2, &attr, 1);	
	
	return 0;
}

int
igmp_omci_ipv4_active_group_remove( struct me_t *bridge_port_me, struct igmp_mbship_t *igmp_mbship)
{
	struct me_t *subscriber_monitor_me=igmp_omci_get_subscriber_monitor_me(bridge_port_me);
	void *active_entry_data;
	void *found_entry_data;
	struct attr_value_t attr={0};
	unsigned short uni_vid =0;


	if (bridge_port_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "bridge port me ==  NULL\n" );
		return -1;
	}

	if (subscriber_monitor_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp subscriber monitor me\n", bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);
		return (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)?0:-1;
	}

	if (igmp_mbship->uni_vid == 4096)
		uni_vid  = 0;
	else
		uni_vid  = igmp_mbship->ani_vid;
                
	if ((found_entry_data = igmp_omci_locate_ipv4_active_group_list_client(bridge_port_me,uni_vid ,igmp_mbship->group_ip.ipv4,igmp_mbship->src_ip.ipv4,igmp_mbship->client_ip.ipv4, IGMP_ACTIVE_GROUP_LIST_CLIENT)) == NULL) 
		return 0;

	active_entry_data=malloc_safe(24);
	memcpy(active_entry_data, found_entry_data, 24);
	
	igmp_omci_del_ipv4_active_group_list(active_entry_data, subscriber_monitor_me);
	free_safe(active_entry_data);

	// update current bandwidth usage with sum of imputed bandwidth, FAKED
	attr.data = igmp_omci_total_bandwidth_in_active_group_list( bridge_port_me, igmp_mbship->uni_vid, IGMP_PORT_SCOPE ) ;
	meinfo_me_attr_set(subscriber_monitor_me, 2, &attr, 1);	
	
	return 0;
}

int
igmp_omci_ipv6_active_group_remove(struct me_t *bridge_port_me, struct igmp_mbship_t *igmp_mbship)
{
	struct me_t *subscriber_monitor_me=igmp_omci_get_subscriber_monitor_me(bridge_port_me);
	void *active_entry_data;
	void *found_entry_data;
	struct attr_value_t attr={0};
	unsigned short uni_vid =0;


	if (bridge_port_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "bridge port me ==  NULL\n" );
		return -1;
	}

	if (subscriber_monitor_me == NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp subscriber monitor me\n", bridge_port_me->classid, bridge_port_me->meid, bridge_port_me->meid);
		return (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)?0:-1;
	}

	if (igmp_mbship->uni_vid == 4096)
		uni_vid = 0;
	else
		uni_vid = igmp_mbship->uni_vid;

	if ((found_entry_data = igmp_omci_locate_ipv6_active_group_list_client(bridge_port_me,uni_vid ,igmp_mbship->group_ip.ipv6,igmp_mbship->src_ip.ipv6,igmp_mbship->client_ip.ipv6, IGMP_ACTIVE_GROUP_LIST_CLIENT)) == NULL) 
		return 0;

	active_entry_data=malloc_safe(58);
	memcpy(active_entry_data, found_entry_data, 58);
	
	igmp_omci_del_ipv6_active_group_list(active_entry_data, subscriber_monitor_me);
	free_safe(active_entry_data);

	// update current bandwidth usage with sum of imputed bandwidth, FAKED
	attr.data = igmp_omci_total_bandwidth_in_active_group_list(bridge_port_me, igmp_mbship->uni_vid, IGMP_PORT_SCOPE) ;
	meinfo_me_attr_set(subscriber_monitor_me, 2, &attr, 1);	
	
	return 0;
}

static LIST_HEAD(bridge_port_rate_stat);

static int 
igmp_omci_get_bridge_id(struct me_t *bridge_port_me)
{
	struct me_t *bridge_me, *ibridge_me;	
	unsigned char ibridge_id;

	if (bridge_port_me==NULL) return -1;
	bridge_me=meinfo_me_get_by_meid(45, meinfo_util_me_attr_data(bridge_port_me, 1));
	if (bridge_me==NULL) return -2;		
	ibridge_me=hwresource_public2private(bridge_me);	
	if (ibridge_me==NULL)return -3;		

	ibridge_id=meinfo_util_me_attr_data(ibridge_me, 4);	//attr 4 = bridge id		
	return (ibridge_id);
}

int
igmp_omci_check_msgrate_is_allowed(struct me_t *bridge_port_me,int vid_uni )
{
	int msgrate_max_allowed;
	int bridge_id;
	struct me_t *subscriber_config_me=NULL;
	struct me_t *profile_me=NULL;
	struct me_t *mspt_profile_me=NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	struct me_t *mcast_port=NULL;
	
	mcast_port= igmp_omci_mcast_bridge_port_get(bridge_port_me);
	if (!mcast_port) dbprintf_igmp(LOG_WARNING, "bridge port or veip port is null\n");

	subscriber_config_me = igmp_omci_get_subscriber_config_me(mcast_port);

	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0)
	{
		// check multcast service package table
		mspt_profile_me = igmp_omci_get_mspt_op_profile_me( mcast_port, vid_uni ); 
		if (!mspt_profile_me) profile_me = igmp_omci_get_op_profile_me(mcast_port);
	}
	else
	{
		profile_me = igmp_omci_get_op_profile_me(mcast_port);
	}

	if (profile_me==NULL && mspt_profile_me==NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp op profile me\n", mcast_port->classid, mcast_port->meid, mcast_port->meid);
		return (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)?1:0;
	}

	if (mspt_profile_me == NULL)
		msgrate_max_allowed=meinfo_util_me_attr_data(profile_me, 6);
	else
		msgrate_max_allowed=meinfo_util_me_attr_data(mspt_profile_me, 6);
 	//max igmp message rate check
	if (msgrate_max_allowed == 0) msgrate_max_allowed = 0xffffffff;

	bridge_id=igmp_omci_get_bridge_id(mcast_port);
	//dbprintf_igmp(LOG_ERR, "bridge_id:%d,mcast_port->instance_num:%d\n",bridge_id,mcast_port->instance_num); 
	if (bridge_id>2 || mcast_port->instance_num >= 16)
	{
		dbprintf_igmp(LOG_WARNING, "invalid bridge_id or instance_num (bridge_id:%d,mcast_port->instance_num:%d)\n", bridge_id,mcast_port->instance_num); 
		return 0;
	}
	
	if (list_empty(&bridge_port_rate_stat))
	{
		struct igmp_omci_msgrate_stat_t *stat=NULL;
		if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 && igmp_omci_is_service_pkg_table_empty(subscriber_config_me) ==0)
		{
			stat= malloc_safe(sizeof(struct igmp_omci_msgrate_stat_t));
			stat->bridge_id = bridge_id;
			if ( omci_env_g.port2port_enable )
				stat->bridge_port_instance  = p2p_cfg_g  ;
			else
				stat->bridge_port_instance = mcast_port->instance_num;
			stat->vid = vid_uni;
			INIT_LIST_NODE(&stat->entry_node);
			list_add( &stat->entry_node, &bridge_port_rate_stat);
		}

		stat= malloc_safe(sizeof(struct igmp_omci_msgrate_stat_t));
		stat->bridge_id = bridge_id;
		if ( omci_env_g.port2port_enable )
			stat->bridge_port_instance  = p2p_cfg_g  ;
		else
			stat->bridge_port_instance = mcast_port->instance_num;
		stat->vid = 4097;
		INIT_LIST_NODE(&stat->entry_node);
		list_add( &stat->entry_node, &bridge_port_rate_stat);
		
		return 1;
	}
	else
	{
		int now=time(0);		
		struct igmp_omci_msgrate_stat_t *pos=NULL, *n=NULL, *msg_stat=NULL;
		list_for_each_entry_safe( pos, n, &bridge_port_rate_stat, entry_node ) {
			if (pos->bridge_id != bridge_id)
				continue;
			if (omci_env_g.port2port_enable)
			{
				if (pos->bridge_port_instance  != p2p_cfg_g ) 
					continue;
			}
			else
			{
				if (pos->bridge_port_instance != mcast_port->instance_num)
					continue;
			}
			
			if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 && igmp_omci_is_service_pkg_table_empty(subscriber_config_me) ==0)
			{
				if (mspt_profile_me != NULL && pos->vid == vid_uni)
				{
					msg_stat = pos;
					if (now!=msg_stat->time || msg_stat->msg_total+1 <= msgrate_max_allowed)
						return 1;
				}
				else if (mspt_profile_me == NULL)
				{
					return 0;
				}
			}
			else if (meinfo_util_me_attr_data(omci_ver, 2) < 0xa0)
			{
				if (profile_me != NULL && pos->vid == 4097)
				{
					msg_stat = pos;
					if (now!=msg_stat->time || msg_stat->msg_total+1 <= msgrate_max_allowed)
						return 1;
				}
			}
		}

		if (msg_stat == NULL)
		{
			if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 && igmp_omci_is_service_pkg_table_empty(subscriber_config_me) ==0)
			{
				struct igmp_omci_msgrate_stat_t *stat= malloc_safe(sizeof(struct igmp_omci_msgrate_stat_t));
				stat->bridge_id = bridge_id;
				if (omci_env_g.port2port_enable)
					stat->bridge_port_instance  = p2p_cfg_g;
				else
					stat->bridge_port_instance = mcast_port->instance_num;
				stat->vid = vid_uni;
				INIT_LIST_NODE(&stat->entry_node);
				list_add( &stat->entry_node, &bridge_port_rate_stat);
			}
			return 1;
		}
	}
	
	return 0;
}

int
igmp_omci_update_msgrate(struct me_t *bridge_port_me, unsigned int vid_uni)
{
	struct me_t *profile_me=NULL;
	struct me_t *mspt_profile_me=NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		
	int bridge_id=-1;
	struct me_t *subscriber_config_me = NULL;
	struct me_t *mcast_port=NULL;
	
	mcast_port= igmp_omci_mcast_bridge_port_get(bridge_port_me);
	if (!mcast_port) dbprintf_igmp(LOG_WARNING, "bridge port or veip port is null\n");


	bridge_id=igmp_omci_get_bridge_id(mcast_port);
	subscriber_config_me = igmp_omci_get_subscriber_config_me(mcast_port);

	if( meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 )
	{
		// check multcast service package table
		mspt_profile_me = igmp_omci_get_mspt_op_profile_me(mcast_port, vid_uni); 
		if (!mspt_profile_me) profile_me = igmp_omci_get_op_profile_me(mcast_port);
	}
	else
	{
		profile_me = igmp_omci_get_op_profile_me(mcast_port);
	}

	if (profile_me==NULL && mspt_profile_me==NULL)
	{
		dbprintf_igmp(LOG_WARNING, "class=%d, meid=0x%x(%u) has no related igmp op profile me\n", mcast_port->classid, mcast_port->meid, mcast_port->meid);
		return (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)?1:0;
	}

	if (bridge_id>=0 && bridge_id<2 && mcast_port->instance_num < 16)
	{	
		struct igmp_omci_msgrate_stat_t *pos=NULL, *n=NULL ;
		list_for_each_entry_safe( pos, n, &bridge_port_rate_stat, entry_node ) {
			if (pos->bridge_id != bridge_id)
				continue;
			if (pos->bridge_port_instance != mcast_port->instance_num )
				continue;
			if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0 && igmp_omci_is_service_pkg_table_empty(subscriber_config_me) ==0)
			{
				if (mspt_profile_me != NULL && pos->vid == vid_uni) {
					int now=time(0);		
					if (now==pos->time)
					{
						pos->msg_total++;
					}
					else
					{
						pos->time=now;
						pos->msg_total=1;
					}
					return pos->msg_total;
				}
				else if (mspt_profile_me == NULL)
				{
					return 0;
				}
			}
			else
			{
				int now=time(0);		
				if (now==pos->time)
				{
					pos->msg_total++;
				}
				else
				{
					pos->time=now;
					pos->msg_total=1;
				}
				return pos->msg_total;
			}
		}
	}	
	return 0;
}

int
igmp_omci_read_igmp_profile_me(struct igmp_config_t *igmp_config, struct me_t *bridge_port_me, int vid_uni)
{
	//read IGMP profile from ME
	struct me_t *profile_me = NULL  ;
	unsigned char igmp_ver;
	struct igmp_config_t me_config;
	struct me_t *config_bridge_port=NULL;

	config_bridge_port = igmp_omci_mcast_bridge_port_get(bridge_port_me);
	if (!config_bridge_port && vid_uni != -1)
	{
		dbprintf_igmp(LOG_ERR, "config_bridge_port == NULL\n");
		return -1;
	}
	else if (vid_uni == -1)	// init
	{
		struct me_t *meptr_47 = NULL; 
		struct meinfo_t *miptr = meinfo_util_miptr(47);
		list_for_each_entry(meptr_47, &miptr->me_instance_list, instance_node) {
			if (meinfo_util_me_attr_data(meptr_47, 3) == 11)
			{ // VEIP
				if (meinfo_me_get_by_meid(310, meptr_47->meid))
				{
					config_bridge_port = meptr_47;
					vid_uni = 0;
					break;
				}
			}
		}
		if (!config_bridge_port) return -1;
	}

	igmp_old_config_g = igmp_config_g;
		
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);		

	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0)
	{
		// check multcast service package table
		profile_me = igmp_omci_get_mspt_op_profile_me(config_bridge_port, vid_uni); 
		if (!profile_me) profile_me = igmp_omci_get_op_profile_me(config_bridge_port);
	}
	else
	{
		profile_me = igmp_omci_get_op_profile_me(config_bridge_port);
	}

	if (!profile_me) return -1;
	
	if (meinfo_util_me_attr_data(profile_me, 1))
	{
		igmp_ver = meinfo_util_me_attr_data(profile_me, 1);
		switch (igmp_ver)
		{
		case 1:
			igmp_config->igmp_version = IGMP_V1_BIT;
			break;
		case 2:
			igmp_config->igmp_version = IGMP_V2_BIT;
			break;
		case 3:
			igmp_config->igmp_version = IGMP_V3_BIT;
			break;
		case 16:
			igmp_config->igmp_version = MLD_V1_BIT;
			break;
		case 17:
			igmp_config->igmp_version = MLD_V2_BIT;
			break;
		default:
			dbprintf_igmp(LOG_ERR, "invalid IGMP/MLD version\n");
			return -1;
		}
	}
		
	igmp_config->igmp_function = meinfo_util_me_attr_data(profile_me, 2);
	if (igmp_config->igmp_function > 2) igmp_config->igmp_function = 0;

	igmp_config->immediate_leave = meinfo_util_me_attr_data(profile_me, 3);
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
		igmp_config->immediate_leave = 1;	// always write to 1 in case calix
	igmp_config->upstream_igmp_tci = (unsigned short) util_bitmap_get_value(meinfo_util_me_attr_ptr(profile_me, 4), 2*8, 0, 16);
	igmp_config->upstream_igmp_tag_ctl = meinfo_util_me_attr_data(profile_me, 5);
	igmp_config->upstream_igmp_rate = meinfo_util_me_attr_data(profile_me, 6);
	
	{
		unsigned char robustness = meinfo_util_me_attr_data(profile_me, 10);
		if (robustness == 0)	// set robustness to DEFAULT_ROBUSTNESS if value from ME309 is 0
			robustness = DEFAULT_ROBUSTNESS;
		if (igmp_config->robustness != robustness)
		{		
			dbprintf_igmp(LOG_WARNING, "change igmp config robustness from %u to %u (classid=%u, meid=%u, attr%u)\n", igmp_config->robustness, robustness, profile_me->classid, profile_me->meid, 10);
			igmp_config->robustness = robustness;
		}
	}

	if (meinfo_util_me_attr_ptr(profile_me, 11))
	{
		struct attr_value_t attr;
		attr.data = 0;
		attr.ptr = meinfo_util_me_attr_ptr(profile_me, 11);
		memset(igmp_config->querier_ip, 0, 16);
		strcpy(igmp_config->querier_ip, inet_ntoa(*(struct in_addr *) (attr.ptr)));
	}
	
	me_config.query_interval = meinfo_util_me_attr_data(profile_me, 12) ;
	if (me_config.query_interval > 0) igmp_config->query_interval = me_config.query_interval; 

	me_config.max_respon_time = meinfo_util_me_attr_data(profile_me, 13) ;
	if (me_config.max_respon_time > 0) igmp_config->max_respon_time = me_config.max_respon_time ; 

	igmp_config->last_member_query_interval = meinfo_util_me_attr_data(profile_me, 14) ; 

	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0)
	{
		igmp_config->downstream_igmp_tag_ctl = (unsigned char) util_bitmap_get_value(meinfo_util_me_attr_ptr(profile_me, 16), 3*8, 0, 8);
		igmp_config->downstream_igmp_tci = (unsigned short) util_bitmap_get_value(meinfo_util_me_attr_ptr(profile_me, 16), 3*8, 8, 16);
	}
	
        if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
        {
        	// calix uses smaller omci version but still support donwstream tag control
		igmp_config->downstream_igmp_tag_ctl = (unsigned char) util_bitmap_get_value(meinfo_util_me_attr_ptr(profile_me, 16), 3*8, 0, 8);
		igmp_config->downstream_igmp_tci = (unsigned short) util_bitmap_get_value(meinfo_util_me_attr_ptr(profile_me, 16), 3*8, 8, 16);
		
		struct meinfo_t *me65317_miptr = meinfo_util_miptr(65317);
		short mode = 0;
		if (me65317_miptr)
		{
			struct me_t *me65317 = NULL;
			list_for_each_entry(me65317, &me65317_miptr->me_instance_list, instance_node)
			{ 
				mode = meinfo_util_me_attr_data(me65317, 1);
				break;
			}
		}
		// use external value if it's external mdoe
		if (mode)
		{
			igmp_config->igmp_version = igmp_old_config_g.igmp_version;
			igmp_config->immediate_leave = igmp_old_config_g.immediate_leave;
			igmp_config->query_interval = igmp_old_config_g.query_interval;
			igmp_config->max_respon_time = igmp_old_config_g.max_respon_time;
			igmp_config->robustness = igmp_old_config_g.robustness;
			igmp_config->last_member_query_interval = igmp_old_config_g.last_member_query_interval;
		}
		else
		{
			// native mode, use omci igmp max stream value
			struct me_t * subscriber_config_me = igmp_omci_get_subscriber_config_me(config_bridge_port);
			if (subscriber_config_me)
			{			
				igmp_config->group_limit = (unsigned int)meinfo_util_me_attr_data(subscriber_config_me, 3);
				dbprintf_igmp(LOG_WARNING, "native mode override group limit to %d of me %d\n", igmp_config->group_limit, subscriber_config_me->meid);
			}
		}	
        
	}
		
	return 0;
}

int
igmp_omci_clear_all_mcast_subscriber_monitor_me(void)
{
	struct me_t *me;
	struct attr_table_header_t *tab_header;
	struct attr_value_t attr={0};

	struct meinfo_t *miptr= meinfo_util_miptr(311);		
	if (miptr==NULL)
	{
		dbprintf_igmp(LOG_ERR, "class=%d, null miptr?\n", 311);
		return -1;
	}		

	// traverse all subscriber_monitor me
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		attr.data = 0;
		meinfo_me_attr_set(me, 2, &attr, 0);
		meinfo_me_attr_set(me, 3, &attr, 0);
		meinfo_me_attr_set(me, 4, &attr, 0);
		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 5);// ipv4 active acl table	
		meinfo_me_attr_release_table_all_entries(tab_header);
		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 6);// ipv6 active acl table	
		meinfo_me_attr_release_table_all_entries(tab_header);
	}
	
	return 0;
}

int
igmp_omci_get_logical_port_id_by_bridge_port_meid(unsigned short port_meid)
{
	struct me_t *bridge_port_me = NULL,*ibridge_port_me = NULL;
	if ((bridge_port_me = meinfo_me_get_by_meid(47,port_meid)) == NULL)
	{
		dbprintf_igmp(LOG_ERR, "No such bridge port meid = %d(0x%x)\n",port_meid,port_meid );
		return -1;
	}
	
	if ((ibridge_port_me = hwresource_public2private(bridge_port_me)) == NULL)
	{
		dbprintf_igmp(LOG_ERR, "No such private bridge port meid = %d(0x%x)\n",port_meid,port_meid );
		return -1;
	}
	
	return meinfo_util_me_attr_data(ibridge_port_me,6);
}

int igmp_omci_remove_bridge_port_rate_list()
{
	struct igmp_omci_msgrate_stat_t *pos=NULL, *n=NULL;
	
	list_for_each_entry_safe(pos, n, &bridge_port_rate_stat, entry_node) {
		list_del(&pos->entry_node);
		free_safe(pos);
	}
	return 0;
}


/* This function was copied and was modified from
 * igmp_omci_rm_illeagal_group_in_client(). it tried to find out if the mbship
 * conflicted the rules in ME-309 and ME-310
 */
static int
__igmp_omci_check_mbship(struct igmp_mbship_t *igmp_mbship)
{
	int entry_num = 0;
	struct list_head dynamic_acl_list;
	struct list_head static_acl_list;
	struct attr_table_header_t *tab_header = NULL;
	struct me_t *omci_ver=meinfo_me_get_by_meid(257, 0);
	struct me_t *subscriber_config_me = NULL;
	struct me_t *profile_me = NULL;
	struct me_t *bridge_port_me = NULL;
	struct igmp_omci_acl_node_t  acl_node;
	struct igmp_omci_allowed_preview_node_t apgt_node;
	int ret = 0;

	/* TODO: Ugly work-around, remove it later */
	struct igmp_clientinfo_t igmp_clientinfo;
	struct igmp_group_t igmp_group;
	igmp_clientinfo.vid = igmp_mbship->uni_vid;
	igmp_clientinfo.ethertype = (IS_IPV6_ADDR(igmp_mbship->group_ip) ?
				     ETH_P_IPV6 : ETH_P_IP);
	igmp_pkt_ip_cpy(&igmp_clientinfo.client_ip, &igmp_mbship->client_ip);
	igmp_pkt_ip_cpy(&igmp_group.group_ip, &igmp_mbship->group_ip);
#if 0
	/* do it need the ntohl or htonl conversion? */
	if (igmp_clientinfo.ethertype == ETH_P_IP) {
		igmp_clientinfo.client_ip.ipv4.s_addr = htonl(igmp_mbship->client_ip.ipv4.s_addr);
	} else {
		unsigned int i, *client_ipv6;
		client_ipv6 = (unsigned int *)&igmp_clientinfo.client_ip;
		for(i = 0; i < 4; ++i)
			client_ipv6[i] = htonl(client_ipv6[i]);
	}
#endif
	/* Ugly work-around */

	INIT_LIST_HEAD(&dynamic_acl_list);
	INIT_LIST_HEAD(&static_acl_list);

	bridge_port_me = meinfo_me_get_by_meid(47, igmp_mbship->src_port_meid);
	subscriber_config_me = igmp_omci_get_subscriber_config_me(bridge_port_me);

	if (meinfo_util_me_attr_data(omci_ver, 2) >= 0xa0
		&& igmp_omci_is_service_pkg_table_empty(subscriber_config_me) == 0) {

		struct list_head apgt_list;
		INIT_LIST_HEAD(&apgt_list);

		profile_me = igmp_omci_get_mspt_op_profile_me(bridge_port_me,
							igmp_clientinfo.vid);
		if (profile_me==NULL) {
			if (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)
				return 0;
			else
				return 1;
		}

		// static acl table
		tab_header = meinfo_util_me_attr_ptr(profile_me, 8);
		if ((entry_num = igmp_omci_load_acl_table_988(&static_acl_list,
							tab_header)) <= 0)
			dbprintf_igmp(LOG_WARNING, "static acl table is empty\n");

		// dynamic acl table
		tab_header = meinfo_util_me_attr_ptr(profile_me, 7);
		if ((entry_num = igmp_omci_load_acl_table_988(&dynamic_acl_list, tab_header)) <= 0)
			dbprintf_igmp(LOG_WARNING, "dynamic acl table is empty\n");

		// allowed preview groups table
		tab_header = meinfo_util_me_attr_ptr(subscriber_config_me, 7);
		if ((entry_num = igmp_omci_load_allowed_preview_groups_table(&apgt_list, tab_header)) <= 0)
			dbprintf_igmp(LOG_WARNING, "allowed preview groups table is empty\n");

		if (list_empty(&static_acl_list)
				&& list_empty(&dynamic_acl_list)
				&& list_empty(&apgt_list)) {
			dbprintf_igmp(LOG_WARNING, "all tables are empty\n");
			goto __NOTFOUND1;
		}

		if (igmp_omci_get_full_auth_acl_node_988(&static_acl_list,
					&igmp_group, &igmp_clientinfo,
					&acl_node) == 0)
			goto __FOUND1;

		if (igmp_omci_get_full_auth_acl_node_988(&dynamic_acl_list,
					&igmp_group, &igmp_clientinfo,
					&acl_node) == 0)
			goto __FOUND1;

		if (igmp_omci_get_preview_acl_node_988(&dynamic_acl_list,
					&igmp_group, &igmp_clientinfo,
					&acl_node) == 0) {
			struct igmp_mbship_preview_acl_t *preview_client = NULL;
			preview_client = igmp_mbship_search_preview_acl_list(
						&igmp_mbship_db_g,
						&igmp_clientinfo, &igmp_group);
			if (preview_client == NULL)
				goto __FOUND1;
			// client group preview count < acl group preview  count
			// and preview time length between 2 preview > acl repeat time
			if (preview_client->repeat_count < acl_node.preview_repeat_count
				&& (igmp_timer_get_cur_time() - preview_client->last_join_time) >  acl_node.preview_repeat_time)
				goto __FOUND1;
		}

		if(igmp_omci_get_allowed_preview_groups_table_node(&apgt_list,
					&igmp_group, &igmp_clientinfo,
					&apgt_node) == 0)
			goto __FOUND1;
__NOTFOUND1:
		ret = 1;
__FOUND1:
		igmp_omci_unload_apgt_table(&apgt_list);
	} else {
		profile_me = igmp_omci_get_op_profile_me(bridge_port_me);
		if (profile_me == NULL) {
			if (!omci_env_g.igmp_mcast_me_support || omci_env_g.igmp_join_default_grant)
				return 0;
			else
				return 1;
		}

		// ME 310 attr2 can point to a ME309 not contained in attr 6. ME309 by attr 6 allows UNI vid matching prior to any vlan tag or tag control
		// we load mstp table first and check vid. if mstp happens to contain ME309 pointed by attr2, we set me310_attr2 to 1
		// and dont load it after mspt table. ME309 pointed by ME310 attr2 does not have UNI vid matching  
		int me310_attr2 = 0;

		struct attr_table_header_t *tab_header_mspt;
		tab_header_mspt = meinfo_util_me_attr_ptr(subscriber_config_me , 6);
		if (tab_header_mspt != NULL) {
			struct attr_table_entry_t *tab_entry;
			list_for_each_entry(tab_entry, &tab_header_mspt->entry_list, entry_node) {
				unsigned short vlan_id = (unsigned short)util_bitmap_get_value(tab_entry->entry_data_ptr, 20*8, 2*8, 16);
				// layer 2 bridged dont check vlan id - calix
				if (!layer2bridged) {
					// undefined, ignore
					if (vlan_id == 0xffff)
						continue;
					// apply to tagged messages but ignore vlan id
					if (vlan_id == 4097) {
						if (igmp_clientinfo.vid == 0)
							continue;
					}
					// apply to untagged messages
					if (vlan_id == 4096) {
						if (igmp_clientinfo.vid != 0)
							continue;
					}
					// match vlan
					if (vlan_id <= 4095) {
						if (omci_env_g.igmp_proxy_snooping == 0
							&& igmp_clientinfo.vid != vlan_id)
							continue;
					}
				}

				// get actual valid ME 309
				struct me_t *mspt_me = meinfo_me_get_by_meid(309, util_bitmap_get_value(tab_entry->entry_data_ptr,20*8,10*8,16));
				if (mspt_me->meid == profile_me->meid)
					me310_attr2 = 1;

				// static acl table
				tab_header = meinfo_util_me_attr_ptr(mspt_me, 8);
				if ((entry_num = igmp_omci_load_acl_table(&static_acl_list, tab_header)) <= 0)
					dbprintf_igmp(LOG_WARNING, "static acl table is empty\n");

				// dynamic acl table
				tab_header = meinfo_util_me_attr_ptr(mspt_me, 7);
				if ((entry_num = igmp_omci_load_acl_table(&dynamic_acl_list, tab_header)) <= 0)
					dbprintf_igmp(LOG_WARNING, "dynamic acl table is empty\n");

				dbprintf_igmp(LOG_WARNING, "mspt entry vlan %d, profile pointer-%d\n", vlan_id, mspt_me->meid);
			}
		}

		// static acl table
		if (!me310_attr2) {
			tab_header = meinfo_util_me_attr_ptr(profile_me, 8);
			if ((entry_num= igmp_omci_load_acl_table(&static_acl_list, tab_header)) <= 0)
				dbprintf_igmp(LOG_WARNING, "static acl table is empty\n"); 
			// dynamic acl table
			tab_header = meinfo_util_me_attr_ptr(profile_me, 7);
			if ((entry_num = igmp_omci_load_acl_table(&dynamic_acl_list, tab_header)) <= 0)
				dbprintf_igmp(LOG_WARNING, "dynamic acl table is empty\n"); 
		}

		//No group match acl or apg table, clear all group
		if (list_empty(&static_acl_list) && list_empty(&dynamic_acl_list)) {
			dbprintf_igmp(LOG_WARNING, "acl tables are empty\n");
			if (omci_env_g.igmp_join_default_grant)
				goto __FOUND2;
			goto __NOTFOUND2;
		}

		if (igmp_omci_get_acl_node(&static_acl_list, &igmp_group, &igmp_clientinfo, &acl_node) == 0)
			goto __FOUND2;
		if (igmp_omci_get_acl_node(&dynamic_acl_list, &igmp_group, &igmp_clientinfo, &acl_node) == 0)
			goto __FOUND2;
__NOTFOUND2:
		ret = 1;
__FOUND2:
		/* Do nothing */
		;
	}

	igmp_omci_unload_acl_table(&static_acl_list);
	igmp_omci_unload_acl_table(&dynamic_acl_list);

	return ret;
}

/* TODO: another ugly work-around, should be remove or revamp the calling process */
void acceptleavemessage(uint32_t src, uint32_t group);
int
igmp_omci_check_mbships(void)
{
	struct igmp_mbship_t *pos, *n;
	list_for_each_entry_safe(pos, n, &igmp_mbship_db_g.mbship_list, entry_node) {
		if (pos->lw_igmpv3_signature == IGMP_V3_LW_SIGNATURE)
			continue;
		if (__igmp_omci_check_mbship(pos) == 0)
			continue;
		struct me_t *bridge_port = meinfo_me_get_by_meid(47, pos->src_port_meid);
		igmp_omci_ipv4_active_group_remove(bridge_port, pos);
		if (omci_env_g.igmp_proxy_snooping)
			acceptleavemessage(pos->client_ip.ipv4.s_addr, pos->group_ip.ipv4.s_addr);
		struct igmp_group_t g;
		memset(&g, 0, sizeof(struct igmp_group_t));
		memcpy(&g.group_ip, &pos->group_ip, sizeof(union ipaddr));
		memcpy(&g.src_ip, &pos->src_ip, sizeof(union ipaddr));
		list_del(&pos->entry_node);
		free_safe(pos);
		igmp_switch_mac_table_client_update(NULL, &igmp_mbship_db_g, &g);
	}

	return 0;
}
