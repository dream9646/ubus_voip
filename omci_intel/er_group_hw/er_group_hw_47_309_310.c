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
 * Module  : er_group_hw
 * File    : er_group_hw_47_309_310.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "omciutil_vlan.h"
#include "switch.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "mcast_const.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "mcast_switch.h"

// 47 MAC_bridge_port_configuration_data
// 309 Multicast_operations_profile
// 310 Multicast_subscriber_config_info

//47@2,309@16,310@254
//47@2,309@4,309@5,309@7,309@8,309@16,310@6,310@7
int
er_group_hw_l2s_tags_create_add01_multicast_309(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *issue_bridge_port_me, *issue_me;
	extern int igmpproxytask_loop_g;
	
	if (attr_inst == NULL)
	{
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	//get bridge port me
	if ((issue_bridge_port_me = er_attr_group_util_attrinst2me(attr_inst, 0)) == NULL)
	{
		dbprintf(LOG_ERR, "could not get bridge port me, classid=%u, meid=%u\n", 
			attr_inst->er_attr_group_definition_ptr->er_attr[0].classid, attr_inst->er_attr_instance[0].meid);
		return -1;
	}

	if ((issue_me = er_attr_group_util_attrinst2me(attr_inst, 6)) == NULL)
	{
		dbprintf(LOG_ERR, "could not get issue me, classid=%u, meid=%u\n", 
			attr_inst->er_attr_group_definition_ptr->er_attr[6].classid, attr_inst->er_attr_instance[6].meid);
		return -1;
	}

	if(igmpproxytask_loop_g) {
		if ( action == ER_ATTR_GROUP_HW_OP_DEL && !list_empty( &igmp_mbship_db_g.mbship_list )) {
			igmp_mbship_port_clear(&igmp_mbship_db_g , issue_bridge_port_me->meid);
		}
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	case ER_ATTR_GROUP_HW_OP_DEL:
		if (me->classid == 309 || me->classid == 310)
			batchtab_cb_wanif_me_update(me, attr_mask);
		if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
		   (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU)) {
			batchtab_omci_update("alu_ds_mcastvlan");
		}
		batchtab_omci_update("classf");
		batchtab_omci_update("wanif");
		batchtab_omci_update("calix_apas");
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}

	return 0;
}

static int
er_group_hw_uni_port_index_from_mcast_me(struct me_t *mcast_me)
{
	struct me_t *me=NULL;
	struct me_t *bridge_port_me=NULL;
	struct meinfo_t *miptr= meinfo_util_miptr(310);		
	unsigned char found = 0;
	if (miptr==NULL) {
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

				if (mcast_me->meid == ext_pointer){
					bridge_port_me = meinfo_me_get_by_meid(47, me->meid);
					found = 1;
					break;
				}
			}
		}
		if ( found == 0 && meinfo_util_me_attr_data(me, 2) == mcast_me->meid ) {
			bridge_port_me = meinfo_me_get_by_meid(47, me->meid);		
			break;
		}
	}
	if (bridge_port_me == NULL){
		dbprintf(LOG_ERR, "bridge_port_me == NULL\n");
		return -1;
	}
	struct me_t *iport_me = hwresource_public2private( bridge_port_me );
	if (iport_me == NULL ){
		dbprintf(LOG_ERR, "iport_me == NULL\n");
		return -1;
	}
	return meinfo_util_me_attr_data( iport_me, 5 );
}

//309@6 310
int
er_group_hw_igmp_upstream_rate_set(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{

//	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
//	unsigned short inst;
	struct er_attr_group_instance_t *current_attr_inst=NULL;
	unsigned int upstream_rate=0;
	int port_idx=-1;
	struct me_t *mcast_me=NULL;

	if (attr_inst == NULL){
		dbprintf(LOG_ERR, "attr_inst == NULL\n");
		return -1;
	}

	upstream_rate=attr_inst->er_attr_instance[0].attr_value.data;
	if (upstream_rate == 0 || upstream_rate > 250 )
		upstream_rate = 250;

	mcast_me = er_attr_group_util_attrinst2me(attr_inst, 0);
	if (mcast_me == NULL)
		return -1;
        
	if (mcast_me && (port_idx = er_group_hw_uni_port_index_from_mcast_me(mcast_me)) < 0) {
		dbprintf(LOG_ERR, "related port of mcast profile(meid=%d) error\n",mcast_me->meid);
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (omci_env_g.port2port_enable ) {
			if (port_idx == 0) //otherwise, do notthing
			{
				igmp_switch_ratelimit_set(0, upstream_rate);
				igmp_switch_ratelimit_set(1, upstream_rate);
				igmp_switch_ratelimit_set(2, upstream_rate);
				igmp_switch_ratelimit_set(3, upstream_rate);
			}
		}else{
			igmp_switch_ratelimit_set(port_idx, upstream_rate);
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		if (mcast_me && (port_idx = er_group_hw_uni_port_index_from_mcast_me(mcast_me)) < 0) {
			dbprintf(LOG_ERR, "related port of mcast profile(meid=%d) error\n",mcast_me->meid);
			er_attr_group_util_release_attrinst(current_attr_inst);
			return -1;
		}

		upstream_rate=current_attr_inst->er_attr_instance[0].attr_value.data;
		if (upstream_rate == 0 || upstream_rate > 250 )
			upstream_rate = 250;

		if (!access("/proc/igmpmon", F_OK)) {
			if (omci_env_g.port2port_enable ) {
				if (port_idx == 0) //otherwise, do notthing
				{
					igmp_switch_ratelimit_set(0, upstream_rate);
					igmp_switch_ratelimit_set(1, upstream_rate);
					igmp_switch_ratelimit_set(2, upstream_rate);
					igmp_switch_ratelimit_set(3, upstream_rate);
				}
			}else{
				igmp_switch_ratelimit_set(port_idx, upstream_rate);
			}
		} else {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR,"/proc/igmpmon not found?\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//310@3
int
er_group_hw_igmp_max_membership(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{

//	return er_group_hw_defence(action, attr_inst, me, attr_mask, defense_attr);
//	unsigned short inst;
	struct er_attr_group_instance_t *current_attr_inst=NULL;
	FILE *fp;
	unsigned short max_membership=0;

	if (attr_inst == NULL){
		dbprintf(LOG_ERR, "attr_inst == NULL\n");
		return -1;
	}

	max_membership=attr_inst->er_attr_instance[0].attr_value.data;
	if ( max_membership == 0 )
		max_membership = 256;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((fp=fopen("/proc/sys/net/ipv4/igmp_max_memberships", "w")) !=NULL) {
			if (max_membership < 4) {
				dbprintf(LOG_ERR, "\x1b[31m""assigned max_membership %d is too small!!""\x1b[0m""\n", max_membership);
				max_membership = 20;
			}
			fprintf(fp, "%d\n", max_membership );
			fflush(fp);
			rewind(fp);
			fclose(fp);
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		max_membership=current_attr_inst->er_attr_instance[0].attr_value.data;
		if ( max_membership == 0 )
			max_membership = 256;

		if ((fp=fopen("/proc/sys/net/ipv4/igmp_max_memberships", "w")) !=NULL) {
			if (max_membership < 4) {
				dbprintf(LOG_ERR, "\x1b[31m""assigned max_membership %d is too small!!""\x1b[0m""\n", max_membership);
				max_membership = 20;
			}
			fprintf(fp, "%d\n", max_membership );
			fflush(fp);
			rewind(fp);
			fclose(fp);
		} else {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR,"/proc/sys/net/ipv4/igmp_max_memberships not found?\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//310@4
int
er_group_hw_max_multicast_bandwidth_set(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL){
		dbprintf(LOG_ERR, "attr_inst == NULL\n");
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
		batchtab_omci_update("mcastmode");	// mcastmode cares only the me 310 existance(not attr value) for wanif igmp_enable override
		batchtab_omci_update("mcastbw");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		batchtab_omci_update("mcastbw");
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator： %d\n", action);
		return -1;
	}
	return 0;
}

int
er_group_hw_mcast_conf_set(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL){
		dbprintf(LOG_ERR, "attr_inst == NULL\n");
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
		batchtab_omci_update("mcastconf");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		batchtab_omci_update("mcastconf");
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator： %d\n", action);
		return -1;
	}
	return 0;
}

static int
er_group_hw_mip_add(struct switch_mac_tab_entry_t *entry_da, unsigned int switch_port_bitmap)
{
	struct switch_mac_tab_entry_t mac_tab_entry;
	
	memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
	*(unsigned int*)&mac_tab_entry.ipmcast.dst_ip = *(unsigned int *)&entry_da->ipmcast.dst_ip;
	mac_tab_entry.vid = entry_da->vid;

	if((switch_hw_g.ipmcastaddr_get != NULL) &&
	   (switch_hw_g.ipmcastaddr_add != NULL)) {
		if(switch_hw_g.ipmcastaddr_get(&mac_tab_entry) == 0) {
			mac_tab_entry.port_bitmap |= switch_port_bitmap;
			switch_hw_g.ipmcastaddr_add (&mac_tab_entry);
		} else {
			if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0 && 
			   omci_env_g.port2port_enable)
				mac_tab_entry.port_bitmap = switch_get_uni_logical_portmask();
			else
				mac_tab_entry.port_bitmap = switch_port_bitmap;
			switch_hw_g.ipmcastaddr_add (&mac_tab_entry);
		}
	}

	return 0;
}

static int
er_group_hw_mip_del(struct switch_mac_tab_entry_t *entry_da, unsigned char switch_port_bitmap)
{
	struct switch_mac_tab_entry_t mac_tab_entry;
	
	memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
	*(unsigned int *)&mac_tab_entry.ipmcast.dst_ip = *(unsigned int *)&entry_da->ipmcast.dst_ip;
	mac_tab_entry.vid = entry_da->vid;
	
	if((switch_hw_g.ipmcastaddr_get != NULL) &&
	   (switch_hw_g.ipmcastaddr_del != NULL) &&
	   (switch_hw_g.ipmcastaddr_add != NULL)) {
		if(switch_hw_g.ipmcastaddr_get (&mac_tab_entry) == 0) {
			if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0 && 
			   omci_env_g.port2port_enable)
				mac_tab_entry.port_bitmap &= ~switch_get_uni_logical_portmask();
			else
				mac_tab_entry.port_bitmap &= ~switch_port_bitmap;
			if(mac_tab_entry.port_bitmap & switch_get_uni_logical_portmask())
				switch_hw_g.ipmcastaddr_add(&mac_tab_entry);
			else
				switch_hw_g.ipmcastaddr_del(&mac_tab_entry);
		}
	}

	return 0;
}

// static void er_group_hw_static_acl_entry_to_ip(struct attr_table_entry_t *tab_entry, unsigned int *addr)
// {
//     memset(addr, 0, 4);
//     
// 	addr[0] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 2*8, 32);
// 	addr[1] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 6*8, 32);
// 	addr[2] = (unsigned int)util_bitmap_get_value(tab_entry->entry_data_ptr, 24*8, 10*8, 32);
//     
//     if (addr[0] == 0x00 && addr[1] == 0x00)
//	 if (addr[2] == 0x00 || addr[2] == 0xffff)
//	     addr[3] = 0xffffffff;    // entry is in IPv4     */
// }

//47@meid 309@8
int
er_group_hw_static_acl_table(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short meid;
	unsigned int switch_port_bitmap ;
	struct me_t *meptr, *ibridgeport_me = NULL;
	struct er_attr_group_instance_t *current_attr_inst = NULL;
	struct attr_table_header_t *table_header = NULL;
	struct attr_table_entry_t *table_entry_pos = NULL;
	struct switch_mac_tab_entry_t mac_tab_entry;
	unsigned int dst[4] = {0};
	
	if (attr_inst == NULL)
	{
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}
	
	if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[1].attr_value.ptr) == NULL) {
		dbprintf(LOG_ERR, "static acl table is empty\n");
		return -1;
	}
	
	memset(&mac_tab_entry, 0, sizeof(struct switch_mac_tab_entry_t));
	
	meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
	if ((meptr = meinfo_me_get_by_meid(47, meid)) == NULL) {
		dbprintf(LOG_ERR, "could not get bridge port me, classid=47, meid=%u\n", meid);
		return -1;
	}
	
	ibridgeport_me = hwresource_public2private(meptr);
	if (ibridgeport_me && 
	   (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) {// Occupied
	  	if (meinfo_util_me_attr_data(ibridgeport_me, 4) == 1)  { // Bridge_port_type == UNI
	  		if (omci_env_g.port2port_enable)
		  		switch_port_bitmap = switch_get_uni_logical_portmask() | switch_get_wifi_logical_portmask();
			else
				switch_port_bitmap = (1 << meinfo_util_me_attr_data(ibridgeport_me, 5)); // Bridge_port_type_index
		} else if (meinfo_util_me_attr_data(ibridgeport_me, 4) == 0) { // Bridge_port_type == VEIP, for veip & all uni
			switch_port_bitmap = switch_get_uni_logical_portmask() | switch_get_wifi_logical_portmask() | (1<<switch_get_cpu_logical_portid(0));
		} else {
			return -1;
		}
	} else
		return -1;
		
	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				unsigned int dst_ip_start = (unsigned int)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 10*8, 32);
				unsigned int dst_ip_end = (unsigned int)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 14*8, 32);
				unsigned int offset = dst_ip_end - dst_ip_start;
				
				//er_group_hw_static_acl_entry_to_ip(table_entry_pos, &dst[0]);
				
				if (offset > 255 && dst[3] == 0xffffffff)
					offset = 255;
				mac_tab_entry.vid = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 4*8, 16);
				
				do {
					//if (dst[3] == 0xffffffff)
						*(unsigned int *)&mac_tab_entry.ipmcast.dst_ip = *(unsigned int *)&dst_ip_start ;
					//else {
						//TODO: 12 bytes prefix resides dst[0-2], last 4 byte dst_ip_start
						//memcpy ((unsigned int *)&mac_tab_entry.ipmcast.dst_ip, &dst[0], 3);
						//((unsigned int *)&mac_tab_entry.ipmcast.dst_ip)[3] = dst_ip_start;
					//}		     					   
					er_group_hw_mip_add(&mac_tab_entry,  switch_port_bitmap);
					dst_ip_start++;
					if (offset) offset--;
				}while(offset);

			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				unsigned int dst_ip_start = (unsigned int)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 10*8, 32);
				unsigned int dst_ip_end = (unsigned int)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 14*8, 32);
				unsigned int offset = dst_ip_end - dst_ip_start;
				
				//er_group_hw_static_acl_entry_to_ip(table_entry_pos, &dst[0]);
				
				mac_tab_entry.vid = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 24*8, 4*8, 16);
				do {
					//if (dst[3] == 0xffffffff) {
						*(unsigned int *)&mac_tab_entry.ipmcast.dst_ip = *(unsigned int *)&dst_ip_start ;
					//} else {
						//TODO: 12 bytes prefix resides dst[0-2], last 4 byte dst_ip_start
						//memcpy ((unsigned int *)&mac_tab_entry.ipmcast.dst_ip, &dst[0], 3);
						//((unsigned int *)&mac_tab_entry.ipmcast.dst_ip)[3] = dst_ip_start;
					//}
					er_group_hw_mip_del(&mac_tab_entry,  switch_port_bitmap);
					dst_ip_start++;
					if (offset) offset--;
				}while(offset);
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_static_acl_table(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}

		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_static_acl_table(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}

	return 0;
}
