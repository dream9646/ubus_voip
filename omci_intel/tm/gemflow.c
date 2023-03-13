/******************************************************************
 *
 * Copyright (C) 2013 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : map gem portid to flowid
 * Module  : tm
 * File    : gemflow.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <math.h>

#include "util.h"
#include "me_related.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "switch.h"
#include "omciutil_misc.h"
#include "proprietary_alu.h"
#include "tm.h"

// me access routines //////////////////////////////////////////////////////////////////////
static int
is_gem_related_to_mcast_iwtp(struct me_t *me_gem)
{
	struct meinfo_t *miptr_mcast = meinfo_util_miptr(281);	// mcast iwtp
	struct me_t *me_mcast;

	if (!me_gem)
		return 0;
	list_for_each_entry(me_mcast, &miptr_mcast->me_instance_list, instance_node) {
		if (me_related(me_mcast, me_gem)) {
			dbprintf_bat(LOG_NOTICE, "me_gem:0x%x is related to mcast_iwtp:0x%x\n", me_gem->meid, me_mcast->meid);
			return 1;
		}
	}
	return 0;
}

static int
gemlist_to_unique_gemlist(int gemport_count, struct me_t *gem_list[8], struct me_t *unique_gem_list[8])
{
	int unique_gemport_count=0;
	int i, j;

	//collect unique_gem_list
	for (j=0; j<8; j++)
		unique_gem_list[j] = NULL;

	for (i=0; i < gemport_count; i++) {
		int is_gem_exist=0;
		for (j=0; j < unique_gemport_count; j++) {
			if (gem_list[i] == unique_gem_list[j]) {
				is_gem_exist=1;
				break;
			}
		}
		if (!is_gem_exist) {
			unique_gem_list[unique_gemport_count]=gem_list[i];
			unique_gemport_count++;
		}
	}
	return unique_gemport_count;
}

// routines for gemflow_map maintaince //////////////////////////////////////////////////////

static int
_gemflow_map_alloc_mask_group(struct batchtab_cb_gemflow_t *g, int pos, int mask_group_size)
{
	int i;
	// check if entries in this "mask_group" are free
	for (i=pos; i < pos+mask_group_size; i++) {
		if ((i == 0 && omci_env_g.exclude_flowid_0) || i<0 || i >= GPON_FLOW_ID_TOTAL ||
		    g->gemflow_map[i].is_used ||
		    g->gemflow_map[i].is_masked) {
		    	return 0;	// non-free entry detected in "mask_group" 
		}
	}

	// mark entries in this group as masked
	for (i=pos; i< pos+mask_group_size; i++)
		g->gemflow_map[i].is_masked=1;
	return 1;
}

// return: start flowid
static int
_gemflow_map_alloc(struct batchtab_cb_gemflow_t *g, int count, int start_flow, int is_descending)
{
	int pos, init_pos, mask_group_size;

	switch(count) {
		case 1:	mask_group_size=1; break;
		case 2:	mask_group_size=2; break;
		case 3:
		case 4:	mask_group_size=4; break;
		default:mask_group_size=8; break;
	}

	// 1. a mask_group is group of continuous entries which could be masked by 2^n, this is useful for calssf to save rule entries
	// 2. an entry is considered free only if is_used==0 && is_masked==0
	//    when is_used==0 && is_masked==1, which means the gemflow is converd by a existing mask_group

	if (!is_descending) {
		init_pos = start_flow;
		if (init_pos % mask_group_size != 0)
			init_pos = (init_pos/mask_group_size +1) *mask_group_size;
		for (pos=init_pos; pos+mask_group_size < GPON_FLOW_ID_TOTAL; pos+=mask_group_size) {
			if (_gemflow_map_alloc_mask_group(g, pos, mask_group_size)) {
				dbprintf_bat(LOG_NOTICE, "count=%d, mask_group_size=%d, start=%d, end=%d, is_descending=%d, ret pos=%d\n", 
					count, mask_group_size, start_flow, GPON_FLOW_ID_TOTAL-1, is_descending, pos);
				return pos;
			}
		}
	} else {
		init_pos = GPON_FLOW_ID_TOTAL-1;
		if (init_pos % mask_group_size != 0)
			init_pos = (init_pos/mask_group_size) *mask_group_size;
		for (pos=init_pos; pos>=0; pos-=mask_group_size) {
			if (_gemflow_map_alloc_mask_group(g, pos, mask_group_size)) {
				dbprintf_bat(LOG_NOTICE, "count=%d, mask_group_size=%d, start=%d, end=%d, is_descending=%d, ret pos=%d\n", 
					count, mask_group_size, start_flow, GPON_FLOW_ID_TOTAL-1, is_descending, pos);
				return pos;
			}
		}
	}
#if 0
	if (omci_env_g.debug_level_bat>= LOG_NOTICE) {
		int i;
		for (i=0; i < GPON_FLOW_ID_TOTAL; i++) {
			dbprintf_bat(LOG_NOTICE, "gemflow_map[%d] used=%d, masked=%d, pre_alloc=%d, phy_qid=%d\n", 
				i, g->gemflow_map[i].is_used, g->gemflow_map[i].is_masked, g->gemflow_map[i].is_pre_alloc, g->gemflow_map[i].phy_queueid);
		}
	}
#endif
	dbprintf_bat(LOG_NOTICE, "count=%d, mask_group_size=%d, start=%d, end=%d, is_descending=%d, ret pos=%d\n", 
		count, mask_group_size, start_flow, GPON_FLOW_ID_TOTAL-1, is_descending, -1);
	return -1;	// not found
}

static int
gemflow_map_alloc(struct batchtab_cb_gemflow_t *g, int count)
{
	return _gemflow_map_alloc(g, count, 0, 0);	// alloc in ascending order
}

int
gemflow_map_set_by_me_gem(struct gemflow_map_t *fl, struct me_t *me_gem, int is_yellow)
{
	int direction=meinfo_util_me_attr_data(me_gem, 3);
	unsigned short gem_portid=meinfo_util_me_attr_data(me_gem, 1);

	//dbprintf_bat(LOG_ERR, "debug, me_gem=(%d)0x%X, start_flowid=%d, unique_gemport_count=%d\n",
	//	me_gem->meid, me_gem->meid, start_flowid, unique_gemport_count);

	if (direction == 1) {// upstream gemflow
		int tcont_id, phy_queueid, is_policy_wrr, hw_weight;
		if (tm_get_tcont_id_phy_queue_info_by_me_pq_gem(NULL, me_gem, is_yellow, 
						&tcont_id, &phy_queueid, &is_policy_wrr, &hw_weight) <=0) {
			dbprintf_bat(LOG_NOTICE, "Can't find related pq for me_gem:0x%x\n", me_gem->meid);
			return -1;
		}
		// us attr
		fl->phy_queueid=phy_queueid;
		fl->is_yellow=is_yellow;
		// ds attr
		fl->mcast_enable=0;
		fl->aes_enable=0;

	} else if (direction == 2) {// downstream gemflow
		// us attr
		fl->phy_queueid=-1;
		fl->is_yellow=is_yellow;
		// ds attr
		fl->mcast_enable=is_gem_related_to_mcast_iwtp(me_gem);
		fl->aes_enable=meinfo_util_me_attr_data(me_gem, 8); //Encryption_state

	} else if (direction == 3) {// bi-direction gemflow
		int tcont_id, phy_queueid, is_policy_wrr, hw_weight;
		if (tm_get_tcont_id_phy_queue_info_by_me_pq_gem(NULL, me_gem, is_yellow, 
						&tcont_id, &phy_queueid, &is_policy_wrr, &hw_weight) <=0) {
			dbprintf_bat(LOG_NOTICE, "Can't find related pq for me_gem:0x%x\n", me_gem->meid);
			return -1;
		}
		// us attr
		fl->phy_queueid=phy_queueid;
		fl->is_yellow=is_yellow;
		// ds attr
		fl->mcast_enable=is_gem_related_to_mcast_iwtp(me_gem);
		fl->aes_enable=meinfo_util_me_attr_data(me_gem, 8); //Encryption_state

	} else {
		dbprintf_bat(LOG_ERR, "direction=%d, error, should never happen\n", direction);
		return -2;
	}

	fl->gem_portid=gem_portid;
	fl->direction=direction;
	fl->is_used=1;
	fl->is_pre_alloc=0;
	fl->me_gem=me_gem;

	///////////////////////for intel gemupdate data
#if 0	
	fl->meid = (unsigned short)meinfo_util_me_attr_data(me_gem, 0);
	fl->gem_data.gem_port_id = gem_portid;
	fl->gem_data.tcont_ptr   = (unsigned short)meinfo_util_me_attr_data(me_gem, 2);
	fl->gem_data.direction = (unsigned char)meinfo_util_me_attr_data(me_gem, 3);;
	fl->gem_data.us_priority_queue_ptr   = (unsigned short)meinfo_util_me_attr_data(me_gem, 4);
	fl->gem_data.us_priority_queue_drop_precedence_color_marking   = 0;
	fl->gem_data.us_traffic_descriptor_egress_color_marking   = 0;
	fl->gem_data.us_traffic_descriptor_profile_ptr   = (unsigned short)meinfo_util_me_attr_data(me_gem, 5);
	fl->gem_data.ds_priority_queue_ptr   = (unsigned short)meinfo_util_me_attr_data(me_gem, 7);
	fl->gem_data.ds_traffic_descriptor_profile_ptr   = (unsigned short)meinfo_util_me_attr_data(me_gem, 9);
	fl->gem_data.mcc_enable   = 1;
	fl->gem_data.encryption_key_ring   = (unsigned char)meinfo_util_me_attr_data(me_gem, 10);
#endif	
	return gem_portid;
}
static int
gemflow_map_collect_8021p_iwtp(struct batchtab_cb_gemflow_t *g,struct me_t *bridge_port_me)
{
	unsigned short related_classid=meinfo_me_attr_pointer_classid(bridge_port_me, 4);
	unsigned short related_pointer=meinfo_util_me_attr_data(bridge_port_me, 4);
	struct me_t *me=meinfo_me_get_by_meid(related_classid, related_pointer);
	struct me_t *gal_me;
	struct attr_value_t attr_dscp2pbit_bitmap={0, NULL};
	int i,idx=0,idx_iwtp=0,k;
	struct meinfo_t *miptr_br_port;
	struct me_t *me_bridge_port;
	struct me_t *ctp_me;
	int bfound = 0;
	unsigned char tp_type;
	unsigned short  tp_ptr,iwtp_brid=0;
	struct me_t *me_iwtp;
	
	if (me == NULL)
		return -1;	// no related iwtp/mapper

	if (related_classid == 130) {	// mapper

		bfound  = 0 ;
		for(idx = 0;idx<GPON_FLOW_8021P_MAP_TOTAL;idx++){
			if(g->gem_8021p_map[idx].is_used == 0)
				continue;
			if(g->gem_8021p_map[idx].meid ==me->meid){
				bfound =1 ;
				break;
			}
		}

		if(bfound)
			return 0;
		
		for(idx = 0;idx<GPON_FLOW_8021P_MAP_TOTAL;idx++){
			if(g->gem_8021p_map[idx].is_used == 0)
				break;
		}

		if(idx >= GPON_FLOW_8021P_MAP_TOTAL ){
			dbprintf_bat(LOG_ERR, "no valid gem_8021p %d\n", idx);
			return -1;
		}

		g->gem_8021p_map[idx].is_used = 1;
		g->gem_8021p_map[idx].meid = me->meid;//meinfo_util_me_attr_data(me, 0);
		g->gem_8021p_map[idx].update_data.tp_pointer = meinfo_util_me_attr_data(me, 1);
		g->gem_8021p_map[idx].update_data.unmarked_frame_opt = meinfo_util_me_attr_data(me, 10);
		meinfo_me_attr_get(me, 11, &attr_dscp2pbit_bitmap); //ptr
		memcpy(&g->gem_8021p_map[idx].dscp2pbit[0],attr_dscp2pbit_bitmap.ptr,24);		
		g->gem_8021p_map[idx].update_data.dscp_prio_map = &g->gem_8021p_map[idx].dscp2pbit[0];
		meinfo_me_attr_clear_value(130, 11, &attr_dscp2pbit_bitmap);
		g->gem_8021p_map[idx].update_data.default_priority_marking = meinfo_util_me_attr_data(me, 12);
		g->gem_8021p_map[idx].update_data.tp_pointer_type = meinfo_util_me_attr_data(me, 13);	
		g->gem_8021p_map[idx].update_data.gem_port_id = &g->gem_8021p_map[idx].gem_port_id[0];
		
		//g->gem_8021p_map[idx].brport_data.me_id =  (unsigned short)meinfo_util_me_attr_data(bridge_port_me, 0);
		//g->gem_8021p_map[idx].brport_data.bridge_me_id =  (unsigned short)meinfo_util_me_attr_data(bridge_port_me, 1);		
		//g->gem_8021p_map[idx].brport_data.tp_type =  (unsigned char)meinfo_util_me_attr_data(bridge_port_me, 3);
		//g->gem_8021p_map[idx].brport_data.tp_ptr =  (unsigned short)meinfo_util_me_attr_data(bridge_port_me, 4);
		miptr_br_port = meinfo_util_miptr(47);
		g->gem_8021p_map[idx].pmapper.me_id = g->gem_8021p_map[idx].meid;
		g->gem_8021p_map[idx].pmapper.count = 0;
		
		list_for_each_entry(me_bridge_port, &miptr_br_port->me_instance_list, instance_node) {
			//unsigned char tp_type;
			//unsigned short  tp_ptr;
			tp_type = meinfo_util_me_attr_data(me_bridge_port, 3);

			if(tp_type != 3)
				continue;

			tp_ptr =meinfo_util_me_attr_data(me_bridge_port, 4);

			if(tp_ptr != g->gem_8021p_map[idx].pmapper.me_id)
				continue;

			if(g->gem_8021p_map[idx].pmapper.count>=8)
				continue;

			g->gem_8021p_map[idx].pmapper.bridge_me_id[g->gem_8021p_map[idx].pmapper.count] =meinfo_util_me_attr_data(me_bridge_port, 1);
			
			g->gem_8021p_map[idx].pmapper.bridge_port_me_id[g->gem_8021p_map[idx].pmapper.count] = me_bridge_port->meid;
			
			g->gem_8021p_map[idx].pmapper.count++;
			
		}

		iwtp_brid =  g->gem_8021p_map[idx].pmapper.bridge_me_id[0];
		for (k = 0; k < g->gem_8021p_map[idx].pmapper.count; k++) {
			
			g->gem_8021p_map[idx].pmapper.gembridge[k].me_id = g->gem_8021p_map[idx].pmapper.bridge_me_id[k];
			g->gem_8021p_map[idx].pmapper.gembridge[k].count = 0;

			miptr_br_port = meinfo_util_miptr(47);

			list_for_each_entry(me_bridge_port, &miptr_br_port->me_instance_list, instance_node) {
				
				unsigned short br_id = meinfo_util_me_attr_data(me_bridge_port, 1);
				
				if(br_id != g->gem_8021p_map[idx].pmapper.gembridge[k].me_id)
					continue;
				if(g->gem_8021p_map[idx].pmapper.gembridge[k].count  < 16){
					g->gem_8021p_map[idx].pmapper.gembridge[k].port[g->gem_8021p_map[idx].pmapper.gembridge[k].count].me_id =
						me_bridge_port->meid;

					g->gem_8021p_map[idx].pmapper.gembridge[k].port[g->gem_8021p_map[idx].pmapper.gembridge[k].count].tp_type = 
						meinfo_util_me_attr_data(me_bridge_port, 3);

					g->gem_8021p_map[idx].pmapper.gembridge[k].port[g->gem_8021p_map[idx].pmapper.gembridge[k].count].tp_ptr = 
						meinfo_util_me_attr_data(me_bridge_port, 4);	
					g->gem_8021p_map[idx].pmapper.gembridge[k].count++;
						
				}
				
			}
			
		}

		for (i=0; i<8; i++) {
			
			unsigned short meid_iwtp=meinfo_util_me_attr_data(me, i+2); // iwtp meid is in mapper attr 2..9
			g->gem_8021p_map[idx].gem_port_id[i] = meid_iwtp;

			//dbprintf_bat(LOG_ERR, "meid_iwtp %d\n", meid_iwtp);
			me_iwtp=meinfo_me_get_by_meid(266, meid_iwtp);	//9.2.4 GEM interworking termination point
			
			
			if (me_iwtp) {
				bfound = 0;
				//dbprintf_bat(LOG_ERR, "111meid_iwtp meid=%d\n",  me_iwtp->meid);
				for(idx_iwtp=0;idx_iwtp<GPON_FLOW_ID_TOTAL;idx_iwtp++){
					
					if(g->iwtp_map[idx_iwtp].is_used == 0)
						continue;
					if(g->iwtp_map[idx_iwtp].meid == me_iwtp->meid){
						bfound  =1;
						break;
					}
				}
				//dbprintf_bat(LOG_ERR, "222meid_iwtp meid=%d\n",  me_iwtp->meid);
				if(bfound)
					continue;
				
				for(idx_iwtp=0;idx_iwtp<GPON_FLOW_ID_TOTAL;idx_iwtp++){
					if(g->iwtp_map[idx_iwtp].is_used == 0)
						break;
				}

				if(idx_iwtp >=GPON_FLOW_ID_TOTAL ){
					dbprintf_bat(LOG_ERR, "no valid gem_IWTP %d\n", idx_iwtp);
					continue;
				}
				
				g->iwtp_map[idx_iwtp].is_used = 1;
				//dbprintf_bat(LOG_ERR, "333meid_iwtp meid=%d\n",  meinfo_util_me_attr_data(me_iwtp, 0));
				g->iwtp_map[idx_iwtp].meid = me_iwtp->meid;
				g->iwtp_map[idx_iwtp].ctp_meid = meinfo_util_me_attr_data(me_iwtp, 1);
				g->iwtp_map[idx_iwtp].update_data.interworking_option = meinfo_util_me_attr_data(me_iwtp, 2);
				g->iwtp_map[idx_iwtp].update_data.service_profile_pointer = meinfo_util_me_attr_data(me_iwtp, 3);
				g->iwtp_map[idx_iwtp].update_data.interworking_tp_pointer = meinfo_util_me_attr_data(me_iwtp, 4);
				g->iwtp_map[idx_iwtp].update_data.gal_loopback_configuration = meinfo_util_me_attr_data(me_iwtp, 5);

				//dbprintf_bat(LOG_ERR, "444meid_iwtp meid=%d-%d-%d\n",idx_iwtp, g->iwtp_map[idx_iwtp].meid,g->iwtp_map[idx_iwtp].ctp_meid);
				//dbprintf_bat(LOG_ERR, "11111 gem_IWTP id=%d-%d-%d-%d-%d-%d-%d\n", idx_iwtp,meinfo_util_me_attr_data(me_iwtp, 0),g->iwtp_map[idx_iwtp].ctp_meid
				//	             ,g->iwtp_map[idx_iwtp].update_data.interworking_option
				//	             ,g->iwtp_map[idx_iwtp].update_data.service_profile_pointer,g->iwtp_map[idx_iwtp].update_data.interworking_tp_pointer
				//	             ,g->iwtp_map[idx_iwtp].update_data.gal_loopback_configuration);

				gal_me = meinfo_me_get_by_meid(272,meinfo_util_me_attr_data(me_iwtp, 7));
				if(gal_me)
					g->iwtp_map[idx_iwtp].update_data.gal_payload_len = meinfo_util_me_attr_data(gal_me, 1);
				else{
					g->iwtp_map[idx_iwtp].update_data.gal_payload_len = 0;
					dbprintf_bat(LOG_ERR, "IWTP gal_payload_len ZERO(%d)\n", g->iwtp_map[idx_iwtp].meid);
				}

				ctp_me = meinfo_me_get_by_meid(268,g->iwtp_map[idx_iwtp].ctp_meid);

				if(ctp_me)
					g->iwtp_map[idx_iwtp].update_data.gem_port_id = meinfo_util_me_attr_data(ctp_me, 1);
				else{
					g->iwtp_map[idx_iwtp].update_data.gem_port_id = 0;
					dbprintf_bat(LOG_ERR, "IWTP gem_port_id ZERO(%d)\n", g->iwtp_map[idx_iwtp].meid);
				}
				miptr_br_port = meinfo_util_miptr(47);
			
				g->iwtp_map[idx_iwtp].update_data.bridge_data.count = 0;
				
				list_for_each_entry(me_bridge_port, &miptr_br_port->me_instance_list, instance_node) {
					
					tp_type = meinfo_util_me_attr_data(me_bridge_port, 3);
					
					tp_ptr = meinfo_util_me_attr_data(me_bridge_port, 4);

					if(meinfo_util_me_attr_data(me_bridge_port, 1) != iwtp_brid)
						continue;
					
					g->iwtp_map[idx_iwtp].update_data.bridge_data.me_id = meinfo_util_me_attr_data(me_bridge_port, 1);
					
					g->iwtp_map[idx_iwtp].update_data.bridge_data.port[g->iwtp_map[idx_iwtp].update_data.bridge_data.count].me_id 
						= me_bridge_port->meid;
					
					g->iwtp_map[idx_iwtp].update_data.bridge_data.port[g->iwtp_map[idx_iwtp].update_data.bridge_data.count].tp_type 
						= tp_type;
					g->iwtp_map[idx_iwtp].update_data.bridge_data.port[g->iwtp_map[idx_iwtp].update_data.bridge_data.count].tp_ptr 
						= tp_ptr;
					g->iwtp_map[idx_iwtp].update_data.bridge_data.count++;
					
				}
				
			}
		}
	
	// 266: 9.2.4 GEM interworking termination point, 281: 9.2.5 Multicast GEM interworking termination point
	} else if (related_classid == 266 || related_classid == 281) {
		if(related_classid == 266){
			
			for(idx_iwtp=0;idx_iwtp<GPON_FLOW_ID_TOTAL;idx_iwtp++){
				if(g->iwtp_map[idx_iwtp].is_used == 0)
					break;
			}

			if(idx_iwtp >=GPON_FLOW_ID_TOTAL ){
				dbprintf_bat(LOG_ERR, "no valid gem_IWTP %d\n", idx_iwtp);
				return -1;
			}

			g->iwtp_map[idx_iwtp].is_used = 1;
			g->iwtp_map[idx_iwtp].meid =me->meid;
			g->iwtp_map[idx_iwtp].ctp_meid = meinfo_util_me_attr_data(me, 1);
			g->iwtp_map[idx_iwtp].update_data.interworking_option = meinfo_util_me_attr_data(me, 2);
			g->iwtp_map[idx_iwtp].update_data.service_profile_pointer = meinfo_util_me_attr_data(me, 3);
			g->iwtp_map[idx_iwtp].update_data.interworking_tp_pointer = meinfo_util_me_attr_data(me, 4);
			g->iwtp_map[idx_iwtp].update_data.gal_loopback_configuration = meinfo_util_me_attr_data(me, 8);

			gal_me = meinfo_me_get_by_meid(272,meinfo_util_me_attr_data(me, 7));
			if(gal_me)
				g->iwtp_map[idx_iwtp].update_data.gal_payload_len =meinfo_util_me_attr_data(gal_me, 1);
			else{
				g->iwtp_map[idx_iwtp].update_data.gal_payload_len = 0;
				dbprintf_bat(LOG_ERR, "IWTP gal_payload_len ZERO(%d)\n", g->iwtp_map[idx_iwtp].meid);
			}

			ctp_me = meinfo_me_get_by_meid(268,g->iwtp_map[idx_iwtp].ctp_meid);

			if(ctp_me)
				g->iwtp_map[idx_iwtp].update_data.gem_port_id = meinfo_util_me_attr_data(ctp_me, 1);
			else{
				g->iwtp_map[idx_iwtp].update_data.gem_port_id = 0;
				dbprintf_bat(LOG_ERR, "IWTP gem_port_id ZERO(%d)\n", g->iwtp_map[idx_iwtp].meid);
			}

			if(g->iwtp_map[idx_iwtp].update_data.interworking_option == 1){
				miptr_br_port = meinfo_util_miptr(47);
			
				g->iwtp_map[idx_iwtp].update_data.bridge_data.count = 0;
				
				list_for_each_entry(me_bridge_port, &miptr_br_port->me_instance_list, instance_node) {
					unsigned char tp_type;
					unsigned short  tp_ptr;
					tp_type = meinfo_util_me_attr_data(me_bridge_port, 3);

					tp_ptr = meinfo_util_me_attr_data(me_bridge_port, 4);

					if(meinfo_util_me_attr_data(me_bridge_port, 1) != g->iwtp_map[idx_iwtp].update_data.service_profile_pointer)
						continue;
					
					g->iwtp_map[idx_iwtp].update_data.bridge_data.me_id = meinfo_util_me_attr_data(me_bridge_port, 1);
					
					g->iwtp_map[idx_iwtp].update_data.bridge_data.port[g->iwtp_map[idx_iwtp].update_data.bridge_data.count].me_id 
						= me_bridge_port->meid;
					
					g->iwtp_map[idx_iwtp].update_data.bridge_data.port[g->iwtp_map[idx_iwtp].update_data.bridge_data.count].tp_type 
						= tp_type;
					g->iwtp_map[idx_iwtp].update_data.bridge_data.port[g->iwtp_map[idx_iwtp].update_data.bridge_data.count].tp_ptr 
						= tp_ptr;
					g->iwtp_map[idx_iwtp].update_data.bridge_data.count++;
					
				}	
			}

		}
	
		if(related_classid == 281){
			
			for(idx_iwtp=0;idx_iwtp<GPON_FLOW_ID_TOTAL;idx_iwtp++){
				if(g->mcast_iwtp_map[idx_iwtp].is_used == 0)
					break;
			}

			g->mcast_iwtp_map[idx_iwtp].is_used = 1;
			g->mcast_iwtp_map[idx_iwtp].meid =me->meid;
			g->mcast_iwtp_map[idx_iwtp].ctp_meid = meinfo_util_me_attr_data(me, 1);
		}
	
	}
	return 0;

}

static int
gemflow_map_collect(struct batchtab_cb_gemflow_t *g, struct me_t *gem_list[8], int gemport_count)
{
	struct me_t *new_gem_list[8];
	int new_gemport_count;
	int start_flowid, flowid, list_i;

	{// copy unallocated me_gem frin gem_list[] to new_gem_list[]
		int i, j, is_gemport_exist;
		memset(new_gem_list, 0, sizeof(new_gem_list));
		new_gemport_count=0;
		for (i=0; i < gemport_count; i++) {
			unsigned short gem_portid=meinfo_util_me_attr_data(gem_list[i], 1);
			is_gemport_exist=0;
			for (j=0; j < GPON_FLOW_ID_TOTAL; j++) {
				if (gem_portid == g->gemflow_map[j].gem_portid) {
					is_gemport_exist=1;
					break;
				}
			}
			if (!is_gemport_exist) {
				new_gem_list[new_gemport_count]=gem_list[i];
				new_gemport_count++;
			}
		}
	}
	if (new_gemport_count == 0) {
		return 0;	// no new gem portid need to map to flowid
	}

	//allocate a group of free flow id (green)
	if ((start_flowid=gemflow_map_alloc(g, new_gemport_count)) < 0) {
		dbprintf_bat(LOG_ERR, "Error, allocate flowid fail, new_gemport_count=%d\n", new_gemport_count);
		return -1;
	}
	for (flowid=start_flowid, list_i=0; flowid < start_flowid+new_gemport_count; flowid++, list_i++) {
		gemflow_map_set_by_me_gem(&g->gemflow_map[flowid], new_gem_list[list_i], 0);
	}
	//allocate a group of free flow id (yellow)
	if (omci_env_g.tm_pq_gem_qos_method==1) {
		if ((start_flowid=gemflow_map_alloc(g, new_gemport_count)) < 0) {
			dbprintf_bat(LOG_ERR, "Error, allocate flowid fail, new_gemport_count=%d\n", new_gemport_count);
			return -1;
		}
		for (flowid=start_flowid, list_i=0; flowid < start_flowid+new_gemport_count; flowid++, list_i++) {
			gemflow_map_set_by_me_gem(&g->gemflow_map[flowid], new_gem_list[list_i], 1);
		}
	}

	return 0;
}

// routine for batchtab gemflow cb generator //////////////////////////////////////////////////////

// find all gemports belonging to each wan port through 8021p mapper or gem iwtp me
// set flowid to power of 2, 4 or 8 to generator gemport flowid mapping
int
gemflow_generator(struct batchtab_cb_gemflow_t *g)
{
	struct gpon_tm_usflow_config_t usflow_config;
	struct gpon_tm_dsflow_config_t dsflow_config;
	struct meinfo_t *miptr_br_port=meinfo_util_miptr(47);
	struct me_t *me_bridge_port, *gem_list[8], *unique_gem_list[8];
	struct switch_port_info_t port_info;
	int gemport_count, unique_gemport_count, ret, flowid;
	unsigned short gem_portid;

	if (g == NULL) {
		dbprintf_bat(LOG_NOTICE, "Warning, g is NULL\n");
		return -1;
	}

	if (batchtab_table_data_get("tm") == NULL) {
		dbprintf_bat(LOG_ERR, "batchtab_cb_tm resource still not ready, please try again\n");
		return -1;
	} else {
		batchtab_table_data_put("tm");	//checkonly, flow generator should wait for "tm" ready
	}

	{ // omci flow is pre-allocated outside gemflow bat, so we mark it in gemflow map in the begining
		// get omci GEM port id
		if ((ret = gpon_hw_g.tm_usflow_get(GPON_OMCI_FLOW_ID, &usflow_config)) != 0) {
			dbprintf_bat(LOG_ERR, "gpon_hw_g.tm_usflow_get GPON_OMCI_FLOW_ID err %d\n", ret);
			return -1;	// omci flow is temporarily not available, skip gemflow batchtab
		}
		if ((ret = gpon_hw_g.tm_dsflow_get(GPON_OMCI_FLOW_ID, &dsflow_config)) != 0) {
			dbprintf_bat(LOG_ERR, "gpon_hw_g.tm_dsflow_get GPON_OMCI_FLOW_ID err %d\n", ret);
			return -1;	// omci flow is temporarily not available, skip gemflow batchtab
		}

		flowid=GPON_OMCI_FLOW_ID;
		g->gemflow_map[flowid].gem_portid=usflow_config.gemport;
		g->gemflow_map[flowid].direction=3;	//bi-direction;
		g->gemflow_map[flowid].is_used=1;
		g->gemflow_map[flowid].is_masked=1;
		g->gemflow_map[flowid].me_gem=NULL;
		g->gemflow_map[flowid].is_yellow=0;
		g->gemflow_map[flowid].is_pre_alloc=1;
		g->gemflow_map[flowid].phy_queueid=GPON_OMCI_PQ_ID;	//phy_queueid;
		g->gemflow_map[flowid].mcast_enable=dsflow_config.mcast_enable;
		g->gemflow_map[flowid].aes_enable=dsflow_config.aes_enable;
	}
	// ssh debug channel flow is pre-allocated outside gemflow bat, so we mark it in gemflow map in the begining
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0) {
		// get omci GEM port id
		if ((ret = gpon_hw_g.tm_usflow_get(GPON_OMCI_FLOW_ID, &usflow_config)) != 0) {
			dbprintf_bat(LOG_ERR, "gpon_hw_g.tm_usflow_get GPON_OMCI_FLOW_ID err %d\n", ret);
			return -1;	// omci flow is temporarily not available, skip gemflow batchtab
		}
		if ((proprietary_alu_get_olt_version() == 0) && (proprietary_alu_get_olt_release() < 0x04090000))
			gem_portid= 4026 + usflow_config.gemport; // FTTU-based
		else
			gem_portid= 2044 - usflow_config.gemport; // ISAM-based

		// get ssh dsflow attrs
		gpon_hw_g.tm_dsflow_get(GPON_SSHD_FLOW_ID, &dsflow_config);
		if ((ret = gpon_hw_g.tm_dsflow_get(GPON_SSHD_FLOW_ID, &dsflow_config)) != 0)
			dbprintf_bat(LOG_ERR, "gpon_hw_g.tm_dsflow_get GPON_SSHD_FLOW_ID err %d\n", ret);

		flowid=GPON_SSHD_FLOW_ID;
		g->gemflow_map[flowid].gem_portid=gem_portid;
		g->gemflow_map[flowid].direction=3;	//bi-direction;
		g->gemflow_map[flowid].is_used=1;
		g->gemflow_map[flowid].is_masked=1;
		g->gemflow_map[flowid].me_gem=NULL;
		g->gemflow_map[flowid].is_yellow=0;
		g->gemflow_map[flowid].is_pre_alloc=1;
		g->gemflow_map[flowid].phy_queueid=GPON_OMCI_PQ_ID;	//phy_queueid;
		g->gemflow_map[flowid].mcast_enable=dsflow_config.mcast_enable;	//mcast_enable; //related to 281
		g->gemflow_map[flowid].aes_enable=dsflow_config.aes_enable;	//meinfo_util_me_attr_data(me_gem, 8); //Encryption_state
	}

	ret = 0;
	list_for_each_entry(me_bridge_port, &miptr_br_port->me_instance_list, instance_node) {
		switch_get_port_info_by_me(me_bridge_port, &port_info);
		if (port_info.port_type != ENV_BRIDGE_PORT_TYPE_WAN &&
			port_info.port_type != ENV_BRIDGE_PORT_TYPE_DS_BCAST)
			continue;
		gemflow_map_collect_8021p_iwtp(g,me_bridge_port);
		if ((gemport_count=omciutil_misc_get_gem_me_list_by_bport_me(me_bridge_port, gem_list)) > 0) {
			unique_gemport_count = gemlist_to_unique_gemlist(gemport_count, gem_list, unique_gem_list);
			//after below function call is_masked will be set
			if (gemflow_map_collect(g, unique_gem_list, unique_gemport_count) < 0) {
				dbprintf_bat(LOG_ERR, "gemflow_map_collect error\n");
				ret=-1;
				continue;
			}
		}
	}

	return ret;
}
#if !defined(CONFIG_SDK_PRX126) && !defined(CONFIG_SDK_PRX120) && !defined(CONFIG_SDK_PRX321)
static int
gemflow_map_cmp(struct gemflow_map_t *fl1, struct gemflow_map_t *fl2)
{
	if (fl1->gem_portid == fl2->gem_portid &&
	    fl1->phy_queueid == fl2->phy_queueid &&
	    fl1->direction == fl2->direction &&
	    fl1->aes_enable == fl2->aes_enable &&
	    fl1->mcast_enable == fl2->mcast_enable &&
	    fl1->is_yellow == fl2->is_yellow )
	    	return 0;
	return -1;
}
#endif
// routine for batchtab gemflow cb sync_to_hardware //////////////////////////////////////////////////////

static struct gemflow_hwsync_state_t gemflow_hwsync_state[GPON_FLOW_ID_TOTAL];

char *
gemflow_hwsync_state_str(int state)
{
	switch (state)  {
		case STATE_LAST_USE:		return "last_use";
		case STATE_UNUSED:		return "unused";
		case STATE_CURRENT_USE:		return "curret_use";
		case STATE_DATA_THE_SAME:	return "same";
	}
	return "?";
}
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
static int
gemflow_hwsync_state_clear(struct batchtab_cb_gemflow_t *g)
{
	int i;

	if (g == NULL) {
		dbprintf_bat(LOG_NOTICE, "Warning, g is NULL\n");
		return -1;
	}

	for (i=0; i < GPON_FLOW_ID_TOTAL; i++) {
		struct gemflow_map_t *fl = &g->gemflow_map[i];
		if (omci_env_g.debug_level_bat>= LOG_NOTICE) {
			if (g->gemflow_map[i].is_used || g->gemflow_map[i].is_masked || g->gemflow_map[i].is_pre_alloc)
				dbprintf_bat(LOG_NOTICE, "gemflow_map[%d] used=%d, masked=%d, pre_alloc=%d, phy_qid=%d\n", i, 
					g->gemflow_map[i].is_used, g->gemflow_map[i].is_masked, g->gemflow_map[i].is_pre_alloc, 
					g->gemflow_map[i].phy_queueid);
		}
		
		if (!g->gemflow_map[i].is_used)
			continue;
		// do not clear pre allocated or omci pq related gemflow
		if (g->gemflow_map[i].is_used) {
			if (g->gemflow_map[i].is_pre_alloc )
				continue;
		}
		// we intensionally not only delete flow created by gemflow.c
		// we delete all hw flow entries except the omci, ssh or pre allocated flows in case there is any garbage config left in gemflow table
		if(fl->direction == 1 || fl->direction == 3){
			if (gpon_hw_g.tm_usflow_del != NULL && (gpon_hw_g.tm_usflow_del(fl->meid, &fl->gem_data)) != 0) {
				dbprintf_bat(LOG_ERR, "del usflow %d failed, tm_usflow_del() gemid=%d\n", fl->meid, fl->gem_data.gem_port_id);
				continue;
			}
		}
		/*set downstream*/
		if(fl->direction == 2){
			if (gpon_hw_g.tm_dsflow_del != NULL && (gpon_hw_g.tm_dsflow_del(fl->meid, &fl->gem_data)) != 0) {
				dbprintf_bat(LOG_ERR, "del usflow %d failed, tm_dsflow_del() gemid=%d\n", fl->meid, fl->gem_data.gem_port_id);
				continue;
			}
		}
	}
	return 0;
}

int
gemflow_sync_to_hardware(struct batchtab_cb_gemflow_t *g)
{
	int flowid, found = 0 ,k=0,i=0;
	struct me_t *me_gem,*me_ts,*me_tq;
	unsigned short tm_ptr_us = 0x0000,priority_queue_ptr_ds = 0xFFFF;
	struct switch_mac_bp_config_data_upd_data upd_data;
	struct me_t *dot1p47me;
	struct meinfo_t *miptr;
	
	if (g == NULL) {
		dbprintf_bat(LOG_NOTICE, "Warning, g is NULL\n");
		return -1;
	}

#if 0
	if (batchtab_o5_sequence_is_changed("gemflow")) {
		dbprintf_bat(LOG_WARNING, "gemflow_sync_to_hardware and clean cache!\n");
		gemflow_hwsync_state_clear(g);
	} else {
		dbprintf_bat(LOG_WARNING, "gemflow_sync_to_hardware\n");
	}
#endif	

	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		struct gemflow_map_t *fl = &g->gemflow_map[flowid];
		
		if (!fl->is_used)
			continue;
		found =0;
		tm_ptr_us = 0x0000;
		priority_queue_ptr_ds = 0xFFFF;
		me_gem = fl->me_gem;
		if(fl->direction == 3){
			miptr=meinfo_util_miptr(278);
			fl->gem_data.tcont_ptr   = meinfo_util_me_attr_data(me_gem, 2);
			list_for_each_entry(me_ts, &miptr->me_instance_list, instance_node) {
	
				if(fl->gem_data.tcont_ptr  == meinfo_util_me_attr_data(me_ts,1)){
					found =1;
					break;
				}
			}

			if(!found){
				dbprintf_bat(LOG_WARNING, "gemflow_sync_to_hardware gem map ts fail(%d)\n",fl->gem_data.tcont_ptr);
				continue;
			}
			found = 0;
			miptr=meinfo_util_miptr(277);
			list_for_each_entry(me_tq, &miptr->me_instance_list, instance_node) {
	
				if(me_ts->meid  == meinfo_util_me_attr_data(me_tq,7)){
					found =1;
					break;
				}
			}
			if(!found){
				dbprintf_bat(LOG_WARNING, "gemflow_sync_to_hardware gem map ts-pq fail(%d)\n",me_ts->meid);
				continue;
			}

			tm_ptr_us = me_tq->meid;
		}else{
			tm_ptr_us =meinfo_util_me_attr_data(me_gem, 4);
		}

		miptr=meinfo_util_miptr(277);
		list_for_each_entry(me_tq, &miptr->me_instance_list, instance_node) {

			if(me_tq->meid  == meinfo_util_me_attr_data(me_gem,7)){
				priority_queue_ptr_ds = meinfo_util_me_attr_data(me_gem,7);
				break;
			}
		}
	
		fl->meid = me_gem->meid;
		fl->gem_data.gem_port_id = meinfo_util_me_attr_data(me_gem, 1);
		fl->gem_data.tcont_ptr   =meinfo_util_me_attr_data(me_gem, 2);
		fl->gem_data.direction = meinfo_util_me_attr_data(me_gem, 3);;
		fl->gem_data.us_priority_queue_ptr   = tm_ptr_us;
		fl->gem_data.us_priority_queue_drop_precedence_color_marking   = 0;
		fl->gem_data.us_traffic_descriptor_egress_color_marking   = 0;
		fl->gem_data.us_traffic_descriptor_profile_ptr   = meinfo_util_me_attr_data(me_gem, 5);
		fl->gem_data.ds_priority_queue_ptr   = priority_queue_ptr_ds;
		fl->gem_data.ds_traffic_descriptor_profile_ptr   = meinfo_util_me_attr_data(me_gem, 9);
		fl->gem_data.mcc_enable   = 1;
		fl->gem_data.encryption_key_ring   = meinfo_util_me_attr_data(me_gem, 10);
		/*set upstream*/
		if(fl->direction == 1 || fl->direction == 3){
			if (gpon_hw_g.tm_usflow_set !=NULL && (gpon_hw_g.tm_usflow_set(fl->meid, &fl->gem_data)) != 0) {
				dbprintf_bat(LOG_ERR, "del usflow %d failed, tm_usflow_set() gemid=%d\n", fl->meid, fl->gem_data.gem_port_id);
				continue;
			}
		}
		/*set downstream*/
		if(fl->direction == 2){
			if (gpon_hw_g.tm_dsflow_set !=NULL && (gpon_hw_g.tm_dsflow_set(fl->meid, &fl->gem_data)) != 0) {
				dbprintf_bat(LOG_ERR, "del usflow %d failed, tm_dsflow_set() gemid=%d\n", fl->meid, fl->gem_data.gem_port_id);
				continue;
			}
		}
	}


	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		struct gemflow_iwtp_map_t *iwtp_map = &g->iwtp_map[flowid];
		if (!iwtp_map->is_used)
			continue;
		if (gpon_hw_g.tm_flow_iwtp_set !=NULL && (gpon_hw_g.tm_flow_iwtp_set(iwtp_map->meid, &iwtp_map->update_data,1)) != 0) {
			dbprintf_bat(LOG_ERR, "tm_flow_iwtp_set %d failed \n",iwtp_map->meid);
			continue;
		}
	}

	for (flowid=0; flowid < GPON_FLOW_8021P_MAP_TOTAL; flowid++) {

		struct gemflow_8021p_map_t *gem_8021p_map = &g->gem_8021p_map[flowid];

		if (!gem_8021p_map->is_used)
			continue;

		if (gpon_hw_g.tm_flow_8021p_set !=NULL && (gpon_hw_g.tm_flow_8021p_set(gem_8021p_map->meid, &gem_8021p_map->update_data,1)) != 0) {
			dbprintf_bat(LOG_ERR, "tm_flow_8021p_set %d failed \n",gem_8021p_map->meid);
			continue;
		}

		for (k = 0; k < gem_8021p_map->pmapper.count; k++) {

			for(i=0;i<gem_8021p_map->pmapper.gembridge[k].count;i++){


				switch (gem_8021p_map->pmapper.gembridge[k].port[i].tp_type) {
					case 3:

						dot1p47me=meinfo_me_get_by_meid(47, gem_8021p_map->pmapper.gembridge[k].port[i].me_id);
						if(dot1p47me == NULL){
							dbprintf_bat(LOG_ERR, "bridge_port_connect %d-%d failed \n",gem_8021p_map->pmapper.gembridge[k].me_id,gem_8021p_map->pmapper.gembridge[k].port[i].me_id);
							continue;
						}
						memset(&upd_data,0x0,sizeof(struct switch_mac_bp_config_data_upd_data));
						upd_data.bridge_id_ptr          = meinfo_util_me_attr_data(dot1p47me,1);
						upd_data.port_num               = meinfo_util_me_attr_data(dot1p47me,2);
						upd_data.tp_type                = meinfo_util_me_attr_data(dot1p47me,3);
						upd_data.tp_ptr                 = meinfo_util_me_attr_data(dot1p47me,4);
						upd_data.outbound_td_ptr        = meinfo_util_me_attr_data(dot1p47me,5);
						upd_data.inbound_td_ptr         = meinfo_util_me_attr_data(dot1p47me,6);
						upd_data.mac_learning_depth     = meinfo_util_me_attr_data(dot1p47me,7);

						if (switch_hw_g.bridge_port_config_update != NULL) {

							switch_hw_g.bridge_port_config_update(dot1p47me->meid,&upd_data,0);

						}

					case 1:
					case 7:
					case 11:
						//dbprintf_bat(LOG_ERR, "bridge_port_connect %d-%d failed \n",gem_8021p_map->pmapper.gembridge[k].me_id,gem_8021p_map->pmapper.gembridge[k].port[i].me_id);
						if (switch_hw_g.bridge_port_connect != NULL
		   					 && switch_hw_g.bridge_port_connect(gem_8021p_map->pmapper.gembridge[k].me_id , gem_8021p_map->pmapper.gembridge[k].port[i].me_id
		                                       ,gem_8021p_map->pmapper.gembridge[k].port[i].tp_type,gem_8021p_map->pmapper.gembridge[k].port[i].tp_ptr)!=0) {
		        				dbprintf_bat(LOG_ERR, "bridge_port_connect %d-%d failed \n",gem_8021p_map->pmapper.gembridge[k].me_id,gem_8021p_map->pmapper.gembridge[k].port[i].me_id);
							continue;

						}
					break;

				}
			}

		}

	}

/////multicast iwtp
	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		struct gemflow_mcast_iwtp_map_t *mciwtp_map = &g->mcast_iwtp_map[flowid];
		if (!mciwtp_map->is_used)
			continue;
		if (gpon_hw_g.tm_flow_mcastiwtp_set !=NULL && (gpon_hw_g.tm_flow_mcastiwtp_set(mciwtp_map->meid, mciwtp_map->ctp_meid,1)) != 0) {
			dbprintf_bat(LOG_ERR, "tm_flow_mcastiwtp_set %d failed \n",mciwtp_map->meid);
			continue;
		}
		//dot1p47me=meinfo_me_get_by_meid(47, gem_8021p_map->pmapper.gembridge[k].port[i].me_id);
		miptr=meinfo_util_miptr(47);
		list_for_each_entry(dot1p47me, &miptr->me_instance_list, instance_node) {

			if(meinfo_util_me_attr_data(dot1p47me,3)==6 &&  mciwtp_map->meid  == meinfo_util_me_attr_data(dot1p47me,4)){
				memset(&upd_data,0x0,sizeof(struct switch_mac_bp_config_data_upd_data));
				upd_data.bridge_id_ptr		= meinfo_util_me_attr_data(dot1p47me,1);
				upd_data.port_num		= meinfo_util_me_attr_data(dot1p47me,2);
				upd_data.tp_type		= meinfo_util_me_attr_data(dot1p47me,3);
				upd_data.tp_ptr 		= meinfo_util_me_attr_data(dot1p47me,4);
				upd_data.outbound_td_ptr	= meinfo_util_me_attr_data(dot1p47me,5);
				upd_data.inbound_td_ptr 	= meinfo_util_me_attr_data(dot1p47me,6);
				upd_data.mac_learning_depth	= meinfo_util_me_attr_data(dot1p47me,7);

				if (switch_hw_g.bridge_port_config_update != NULL) {

					switch_hw_g.bridge_port_config_update(dot1p47me->meid,&upd_data,0);

				}
				break;
			}
		}
	}
	return 0;
}


int
gemflow_sync_to_hardware_del(struct batchtab_cb_gemflow_t *g)
{
	int flowid;


	for (flowid=0; flowid < GPON_FLOW_8021P_MAP_TOTAL; flowid++)
	{

		struct gemflow_8021p_map_t *gem_8021p_map = &g->gem_8021p_map[flowid];

		if (!gem_8021p_map->is_used)
		{
			continue;
		}

		if (gpon_hw_g.tm_flow_8021p_set !=NULL && (gpon_hw_g.tm_flow_8021p_set(gem_8021p_map->meid, &gem_8021p_map->update_data, 0)) != 0) {
			dbprintf_bat(LOG_ERR, "tm_flow_8021p_set %d failed \n",gem_8021p_map->meid);
			continue;
		}
	}

	/////multicast iwtp
	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		struct gemflow_mcast_iwtp_map_t *mciwtp_map = &g->mcast_iwtp_map[flowid];
		if (!mciwtp_map->is_used)
			continue;
		if (gpon_hw_g.tm_flow_mcastiwtp_set !=NULL && (gpon_hw_g.tm_flow_mcastiwtp_set(mciwtp_map->meid, mciwtp_map->ctp_meid,0)) != 0) {
			dbprintf_bat(LOG_ERR, "tm_flow_mcastiwtp_set %d failed \n",mciwtp_map->meid);
			continue;
		}
	}

	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		struct gemflow_iwtp_map_t *iwtp_map = &g->iwtp_map[flowid];
		if (!iwtp_map->is_used)
			continue;

		if (gpon_hw_g.tm_flow_iwtp_set !=NULL && (gpon_hw_g.tm_flow_iwtp_set(iwtp_map->meid, &iwtp_map->update_data,0)) != 0) {
			dbprintf_bat(LOG_ERR, "tm_flow_iwtp_set %d failed \n",iwtp_map->meid);
			continue;
		}
	}

	gemflow_hwsync_state_clear(g);


	return 0;
}

#else
static int
gemflow_hwsync_state_clear(struct batchtab_cb_gemflow_t *g)
{
	int i, ret;
	struct gpon_tm_usflow_config_t usflow_config;
	struct gpon_tm_dsflow_config_t dsflow_config;

	if (g == NULL) {
		dbprintf_bat(LOG_NOTICE, "Warning, g is NULL\n");
		return -1;
	}

	for (i=0; i < GPON_FLOW_ID_TOTAL; i++) {
		if (omci_env_g.debug_level_bat>= LOG_NOTICE) {
			if (g->gemflow_map[i].is_used || g->gemflow_map[i].is_masked || g->gemflow_map[i].is_pre_alloc)
				dbprintf_bat(LOG_NOTICE, "gemflow_map[%d] used=%d, masked=%d, pre_alloc=%d, phy_qid=%d\n", i, 
					g->gemflow_map[i].is_used, g->gemflow_map[i].is_masked, g->gemflow_map[i].is_pre_alloc, 
					g->gemflow_map[i].phy_queueid);
		}

		// do not clear pre allocated or omci pq related gemflow
		if (g->gemflow_map[i].is_used) {
			if (g->gemflow_map[i].is_pre_alloc || g->gemflow_map[i].phy_queueid == GPON_OMCI_PQ_ID)
				continue;
		}
		// we intensionally not only delete flow created by gemflow.c
		// we delete all hw flow entries except the omci, ssh or pre allocated flows in case there is any garbage config left in gemflow table
		//if (gemflow_hwsync_state[i].us_state == STATE_CURRENT_USE) {
			if (gpon_hw_g.tm_usflow_get(i, &usflow_config)==0 && usflow_config.enable) {
				usflow_config.enable = 0;
				if ((ret=gpon_hw_g.tm_usflow_set(i, &usflow_config)) != 0)
					dbprintf_bat(LOG_ERR, "del usflow %d failed, tm_usflow_set() ret=%d\n", i, ret);
				else
					dbprintf_bat(LOG_NOTICE, "del usflow %d ok\n", i, ret);
			}
			if (gpon_hw_g.tm_dsflow_get(i, &dsflow_config)==0 && dsflow_config.enable) {
				dsflow_config.enable = 0;
				if ((ret=gpon_hw_g.tm_dsflow_set(i, &dsflow_config)) != 0)
					dbprintf_bat(LOG_ERR, "del dsflow %d failed, tm_dsflow_set() ret=%d\n", i, ret);
				else
					dbprintf_bat(LOG_NOTICE, "del dsflow %d ok\n", i, ret);
			}
		//}

		memset(&gemflow_hwsync_state[i], 0, sizeof(struct gemflow_hwsync_state_t));
	}
	return 0;
}

int
gemflow_sync_to_hardware(struct batchtab_cb_gemflow_t *g)
{



	int flowid, ret=0;
	struct gpon_tm_usflow_config_t usflow_config;
	struct gpon_tm_dsflow_config_t dsflow_config;

	if (g == NULL) {
		dbprintf_bat(LOG_NOTICE, "Warning, g is NULL\n");
		return -1;
	}

	if (batchtab_o5_sequence_is_changed("gemflow")) {
		dbprintf_bat(LOG_WARNING, "gemflow_sync_to_hardware and clean cache!\n");
		gemflow_hwsync_state_clear(g);
	} else {
		dbprintf_bat(LOG_WARNING, "gemflow_sync_to_hardware\n");
	}


	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		if (gemflow_hwsync_state[flowid].us_state != STATE_UNUSED &&
		    gemflow_hwsync_state[flowid].ds_state != STATE_UNUSED) 
			dbprintf_bat(LOG_NOTICE, "step0 hwsync_state[%d] us=%s, ds=%s\n", flowid, 
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].us_state), 
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].ds_state));
	}

	// Change disable process and make the minimal side effect
	//step 1. each time regenerator change STATE_CURRENT_USE to STATE_LAST_USE
	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		if (gemflow_hwsync_state[flowid].us_state==STATE_LAST_USE ||
			gemflow_hwsync_state[flowid].ds_state==STATE_LAST_USE) {
			dbprintf_bat(LOG_ERR, "Error, please check gemflow_hwsync_state because this is impossible to happen\n");
			ret=-1;
		}
		if (gemflow_hwsync_state[flowid].us_state==STATE_CURRENT_USE)
			gemflow_hwsync_state[flowid].us_state=STATE_LAST_USE;
		if (gemflow_hwsync_state[flowid].ds_state==STATE_CURRENT_USE)
			gemflow_hwsync_state[flowid].ds_state=STATE_LAST_USE;

		if (gemflow_hwsync_state[flowid].us_state != STATE_UNUSED &&
		    gemflow_hwsync_state[flowid].ds_state != STATE_UNUSED) 
			dbprintf_bat(LOG_NOTICE, "step1 hwsync_state[%d] us=%s, ds=%s\n", flowid, 
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].us_state), 
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].ds_state));
	}

	//step 2. find the flow whose attr is the same and change state to STATE_DATA_THE_SAME
	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		struct gemflow_map_t *fl = &g->gemflow_map[flowid];
		if (!fl->is_used)
			continue;

		//check upstream
		if (fl->direction == 1) {//UNI->ANI(up)
			//check if flow has the same attr and change state to STATE_DATA_THE_SAME
			if (gemflow_hwsync_state[flowid].us_state == STATE_LAST_USE) {
				if (gemflow_map_cmp(&gemflow_hwsync_state[flowid].us_fl, fl)==0) {
					gemflow_hwsync_state[flowid].us_state = STATE_DATA_THE_SAME;
				} else {
					struct gemflow_map_t *fl0 = &gemflow_hwsync_state[flowid].us_fl;
					dbprintf_bat(LOG_NOTICE, "hw_us[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d !=\n"
								 "flmap[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d\n",
						flowid, fl0->gem_portid, fl0->phy_queueid, fl0->direction, fl0->aes_enable, fl0->mcast_enable, fl0->is_yellow,
						flowid, fl->gem_portid, fl->phy_queueid, fl->direction, fl->aes_enable, fl->mcast_enable, fl->is_yellow);
				}
			}
		}
		//check downstream
		if (fl->direction == 2) {//ANI->UNI(down)
			//check if flow has the same attr and change state to STATE_DATA_THE_SAME
			if (gemflow_hwsync_state[flowid].ds_state == STATE_LAST_USE) {
				if (gemflow_map_cmp(&gemflow_hwsync_state[flowid].ds_fl, fl)==0) {
					gemflow_hwsync_state[flowid].ds_state = STATE_DATA_THE_SAME;
				} else {
					struct gemflow_map_t *fl0 = &gemflow_hwsync_state[flowid].ds_fl;
					dbprintf_bat(LOG_NOTICE, "hw_ds[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d !=\n"
								 "flmap[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d\n",
						flowid, fl0->gem_portid, fl0->phy_queueid, fl0->direction, fl0->aes_enable, fl0->mcast_enable, fl0->is_yellow,
						flowid, fl->gem_portid, fl->phy_queueid, fl->direction, fl->aes_enable, fl->mcast_enable, fl->is_yellow);
				}
			}
		}

		//check bidirection
		if (fl->direction == 3) {//Bidirection
			//check if flow has the same attr and change state to STATE_DATA_THE_SAME
			if (gemflow_hwsync_state[flowid].us_state == STATE_LAST_USE && 
			    gemflow_hwsync_state[flowid].ds_state == STATE_LAST_USE) {
				if (gemflow_map_cmp(&gemflow_hwsync_state[flowid].us_fl, fl)==0 && 
				    gemflow_map_cmp(&gemflow_hwsync_state[flowid].ds_fl, fl)==0) {
					gemflow_hwsync_state[flowid].us_state = STATE_DATA_THE_SAME;
					gemflow_hwsync_state[flowid].ds_state = STATE_DATA_THE_SAME;
				} else {
					struct gemflow_map_t *flu = &gemflow_hwsync_state[flowid].us_fl;
					struct gemflow_map_t *fld = &gemflow_hwsync_state[flowid].ds_fl;
					dbprintf_bat(LOG_NOTICE, "hw_us[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d !=\n"
								 "hw_ds[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d !=\n"
								 "flmap[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d\n",
						flowid, flu->gem_portid, flu->phy_queueid, flu->direction, flu->aes_enable, flu->mcast_enable, flu->is_yellow,
						flowid, fld->gem_portid, fld->phy_queueid, fld->direction, fld->aes_enable, fld->mcast_enable, fld->is_yellow,
						flowid, fl->gem_portid, fl->phy_queueid, fl->direction, fl->aes_enable, fl->mcast_enable, fl->is_yellow);
				}
			}
		}

		if (gemflow_hwsync_state[flowid].us_state != STATE_UNUSED &&
		    gemflow_hwsync_state[flowid].ds_state != STATE_UNUSED) 
			dbprintf_bat(LOG_NOTICE, "step2 hwsync_state[%d] us=%s, ds=%s\n", flowid, 
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].us_state), 
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].ds_state));
	}

	//step 3. delete the state which is STATE_LAST_USE(it means flow is used last time, but not the same this time) and change it to STATE_UNUSED
	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		struct gemflow_map_t *fl = &g->gemflow_map[flowid];
		int is_to_del = 0, ret2;

		if (gemflow_hwsync_state[flowid].us_state==STATE_LAST_USE ||
		    gemflow_hwsync_state[flowid].ds_state==STATE_LAST_USE)
			is_to_del = 1;

		if (gemflow_hwsync_state[flowid].us_state==STATE_LAST_USE) {
			if (!fl->is_pre_alloc) {
				usflow_config.enable=0;
				if ((ret2=gpon_hw_g.tm_usflow_set(flowid, &usflow_config)) != 0) {
					dbprintf_bat(LOG_ERR, "del usflow %d failed, tm_usflow_set() ret=%d\n", flowid, ret2);
					ret=-1;
				}
			}
			memset(&gemflow_hwsync_state[flowid].us_fl, 0, sizeof(struct gemflow_map_t));
			gemflow_hwsync_state[flowid].us_state=STATE_UNUSED;
		}
		if (gemflow_hwsync_state[flowid].ds_state==STATE_LAST_USE) {
			if (!fl->is_pre_alloc) {
				dsflow_config.enable=0;
				if ((ret2=gpon_hw_g.tm_dsflow_set(flowid, &dsflow_config)) !=0) {
					dbprintf_bat(LOG_ERR, "del dsflow %d failed, tm_dsflow_set() ret=%d\n", flowid, ret2);
					ret=-1;
				}
			}
			memset(&gemflow_hwsync_state[flowid].ds_fl, 0, sizeof(struct gemflow_map_t));
			gemflow_hwsync_state[flowid].ds_state=STATE_UNUSED;
		}
		if (is_to_del)
			dbprintf_bat(LOG_NOTICE, "step3 hwsync_state[%d] us=%s, ds=%s\n", flowid,
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].us_state), 
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].ds_state));
	}

	//step 4. set data and change STATE_DATA_THE_SAME to STATE_CURRENT_USE
	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		struct gemflow_map_t *fl = &g->gemflow_map[flowid];
		int ret2;

		if (fl->is_used != 1 )
			continue;

		//dbprintf_bat(LOG_WARNING, "flowid=%d gem_portid=%d\n",flowid, fl->gem_portid);

		if (fl->direction == 1 || fl->direction == 3) {// UNI->ANI(up) or Bidirection
			if (gemflow_hwsync_state[flowid].us_state == STATE_DATA_THE_SAME) {
				if (!fl->is_pre_alloc) {
					// rtk tcont/pq behavior workaround: 
					// when removing pq from tcont, the pq related flow will be changed as flow 127 to ensure no traffic comes into the pq 
					// so we need assign flow to phy_queueid again for existing flows
					if ((ret2=gpon_hw_g.tm_usflow2pq_set(flowid, fl->phy_queueid))!=0) {
						dbprintf_bat(LOG_ERR, "reassign usflow %d to phy_queue %d failed, tm_usflow2pq_set() ret=%d\n", 
							flowid, fl->phy_queueid, ret2);
						ret=-1;
						continue;
					}
				}
				// the same flow, only change state
				gemflow_hwsync_state[flowid].us_state=STATE_CURRENT_USE;

			} else if (gemflow_hwsync_state[flowid].us_state == STATE_UNUSED) {
				if (!fl->is_pre_alloc) {
					//enable gem to flow
					memset(&usflow_config, 0, sizeof(usflow_config));
					usflow_config.gemport=fl->gem_portid;
					usflow_config.flow_type=GPON_TM_FLOW_TYPE_ETH;
					usflow_config.enable=1;
					if ((ret2=gpon_hw_g.tm_usflow_set(flowid, &usflow_config)) != 0) {
						dbprintf_bat(LOG_ERR, "add usflow %d failed, tm_usflow_set() ret=%d\n", flowid, ret2);
						ret=-1;
						continue;
					}
					//set flow to phy_queueid
					if ((ret2=gpon_hw_g.tm_usflow2pq_set(flowid, fl->phy_queueid))!=0) {
						dbprintf_bat(LOG_ERR, "assign usflow %d to phy_queue %d failed, tm_usflow2pq_set() ret=%d\n", 
							flowid, fl->phy_queueid, ret2);
						ret=-1;
						continue;
					}
				}
				dbprintf_bat(LOG_NOTICE, "hwsync_us[%d] <- gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d\n",
						flowid, fl->gem_portid, fl->phy_queueid, fl->direction, fl->aes_enable, fl->mcast_enable, fl->is_yellow);
				// copy fl attr
				gemflow_hwsync_state[flowid].us_fl = *fl;
				gemflow_hwsync_state[flowid].us_state=STATE_CURRENT_USE;
			} else {
				dbprintf_bat(LOG_ERR, "Error this part should never happen!!!\n");
			}
		}

		if (fl->direction == 2 || fl->direction == 3) {//ANI->UNI(down) or Bidirection
			if (gemflow_hwsync_state[flowid].ds_state == STATE_DATA_THE_SAME) {
				//the same flow, only change state
				gemflow_hwsync_state[flowid].ds_state=STATE_CURRENT_USE;

			} else if (gemflow_hwsync_state[flowid].ds_state == STATE_UNUSED) {
				if (!fl->is_pre_alloc) {
					//enable gem to flow
					memset(&dsflow_config, 0, sizeof(dsflow_config));
					dsflow_config.gemport=fl->gem_portid;
					dsflow_config.flow_type=GPON_TM_FLOW_TYPE_ETH;
					dsflow_config.aes_enable=fl->aes_enable;
					dsflow_config.mcast_enable=fl->mcast_enable;
					dsflow_config.enable=1;
					if (fl->is_yellow !=1) {
						// for ds flow, only alloc hw flowid for green flow
						// ps: while a specific gemport id could be mapped in multiple hw usflows
						//     a specific gemport id could be mapped in only one hw dsflow
						if ((ret2=gpon_hw_g.tm_dsflow_set(flowid, &dsflow_config)) != 0) {
							dbprintf_bat(LOG_ERR, "add dsflow %d failed, tm_dsflow_set() ret=%d\n", flowid, ret2);
							ret=-1;
							continue;
						}
					}
				}
				dbprintf_bat(LOG_NOTICE, "hwsync_ds[%d] <- gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d\n",
						flowid, fl->gem_portid, fl->phy_queueid, fl->direction, fl->aes_enable, fl->mcast_enable, fl->is_yellow);
				// copy fl attr
				gemflow_hwsync_state[flowid].ds_fl = *fl;
				gemflow_hwsync_state[flowid].ds_state=STATE_CURRENT_USE;

			} else {
				dbprintf_bat(LOG_ERR, "Error this part should never happen!!!\n");
			}
		}

		if (gemflow_hwsync_state[flowid].us_state != STATE_UNUSED &&
		    gemflow_hwsync_state[flowid].ds_state != STATE_UNUSED) 
			dbprintf_bat(LOG_NOTICE, "step4 hwsync_state[%d] us=%s, ds=%s\n", flowid, 
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].us_state), 
				gemflow_hwsync_state_str(gemflow_hwsync_state[flowid].ds_state));
	}
	return ret;
	
}
#endif

// routine for batchtab gemflow cb dump //////////////////////////////////////////////////////

int
gemflow_dump(int fd, struct batchtab_cb_gemflow_t *g)
{
	static char *direction_str[4] = { "?", "us", "ds", "bi-dir" };
	int direction, flowid;

	if (g == NULL) {
		dbprintf_bat(LOG_NOTICE, "Warning, g is NULL\n");
		return -1;
	}

	for (direction = 1; direction <=3; direction++) {
		for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
			struct gemflow_map_t *fl = &g->gemflow_map[flowid];
			if ((fl->is_used==1 || fl->is_masked==1) && fl->direction == direction) {
				// for yellow dsflow, it will appear in gemflow_map with a ds flowid assigned, 
				// but this flowid won't be allocated from hw ds flowid table
				// so we dont show yellow dsflow in gemflow_map
				if (fl->direction == 2 && fl->is_yellow)
					continue;

				util_fdprintf(fd, "portid=%d(0x%x), %s", fl->gem_portid, fl->gem_portid, direction_str[fl->direction%4]);

				if (fl->direction == 1 || fl->direction == 3) { // upstream, bi-direction
					if (omci_env_g.tm_pq_gem_qos_method == 1)
						util_fdprintf(fd, ", is_yellow=%d", fl->is_yellow);
				}
				if (fl->direction == 2 || fl->direction == 3) { // downstream, bi-direction
					util_fdprintf(fd, ", mcast=%d, aes=%d", fl->mcast_enable, fl->aes_enable);
				}

				if (fl->direction == 1 || fl->direction == 3) { // upstream
					util_fdprintf(fd, " --> flowid=%d, phy_queueid=%d, used=%d, masked=%d", 
						flowid, fl->phy_queueid, fl->is_used, fl->is_masked);
				} else {
					util_fdprintf(fd, " --> flowid=%d, used=%d, masked=%d", flowid, fl->is_used, fl->is_masked);
				}

				if (fl->is_pre_alloc) {
					if (flowid == GPON_OMCI_FLOW_ID)
						util_fdprintf(fd, " (omci, pre-allocated)");
					else if (flowid == GPON_SSHD_FLOW_ID)
						util_fdprintf(fd, " (ssh, pre-allocated)");
				} else {
					if (fl->me_gem)
						util_fdprintf(fd, " (gem meid=0x%x)", fl->me_gem->meid);
				}
				util_fdprintf(fd, "\n");
			}
		}
	}

	if (omci_env_g.debug_level_bat>= LOG_NOTICE) {
		int i;
		util_fdprintf(fd, "\n");
		for (i=0; i < GPON_FLOW_ID_TOTAL; i++) {
			static struct gemflow_map_t fl_zero;
			struct gemflow_map_t *flu = &gemflow_hwsync_state[i].us_fl;
			struct gemflow_map_t *fld = &gemflow_hwsync_state[i].ds_fl;
			struct gemflow_map_t *fl = &g->gemflow_map[i];
			if ( g->gemflow_map[i].is_used || g->gemflow_map[i].is_masked || g->gemflow_map[i].is_pre_alloc ||
				memcmp(&fl_zero, fl, sizeof(fl_zero)) != 0 ||
				memcmp(&fl_zero, flu, sizeof(fl_zero)) != 0 ||
				memcmp(&fl_zero, fld, sizeof(fl_zero)) != 0 ) {
				util_fdprintf(fd, "gemflow_map[%d] used=%d, masked=%d, pre_alloc=%d, phy_qid=%d\n", 
					i, g->gemflow_map[i].is_used, g->gemflow_map[i].is_masked, g->gemflow_map[i].is_pre_alloc, g->gemflow_map[i].phy_queueid);
				util_fdprintf(fd, "gemflow_hwsync_state[%d] us=%s, ds=%s\n",
					i, gemflow_hwsync_state_str(gemflow_hwsync_state[i].us_state), gemflow_hwsync_state_str(gemflow_hwsync_state[i].ds_state));
				util_fdprintf(fd, "flmap[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d\n"
						  "hw_us[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d\n"
						  "hw_ds[%d]:gemport=%d,qid=%d,dir=%d,aes=%d,mcast=%d,is_y=%d\n",
					i, fl->gem_portid, fl->phy_queueid, fl->direction, fl->aes_enable, fl->mcast_enable, fl->is_yellow,
					i, flu->gem_portid, flu->phy_queueid, flu->direction, flu->aes_enable, flu->mcast_enable, flu->is_yellow,
					i, fld->gem_portid, fld->phy_queueid, fld->direction, fld->aes_enable, fld->mcast_enable, fld->is_yellow);
			}
		}
	}

	return 0;
}

// routine for batchtab gemflow cb release //////////////////////////////////////////////////////

int
gemflow_release(struct batchtab_cb_gemflow_t *g)
{
	int flowid;

	if (g == NULL) {
		dbprintf_bat(LOG_NOTICE, "Warning, g is NULL\n");
		return -1;
	}

	dbprintf_bat(LOG_WARNING, "gemflow_release\n");
	gemflow_sync_to_hardware_del(g);

	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		memset(&g->gemflow_map[flowid], 0, sizeof(g->gemflow_map[flowid]));
	}

	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		memset(&g->iwtp_map[flowid], 0, sizeof(g->iwtp_map[flowid]));
	}
	
	for (flowid=0; flowid < GPON_FLOW_ID_TOTAL; flowid++) {
		memset(&g->mcast_iwtp_map[flowid], 0, sizeof(g->mcast_iwtp_map[flowid]));
	}
	
	for (flowid=0; flowid < GPON_FLOW_8021P_MAP_TOTAL; flowid++) {
		memset(&g->gem_8021p_map[flowid], 0, sizeof(g->gem_8021p_map[flowid]));
	}
	return 0;
}

// routines for external function ////////////////////////////////////////////////////
int
gem_flowid_get_by_gem_me(struct me_t *me_gem, char is_yellow)
{
	int pos, found=-1;
	struct batchtab_cb_gemflow_t *g;

	g=(struct batchtab_cb_gemflow_t *)batchtab_table_data_get("gemflow");
	if (g == NULL) {
		dbprintf_bat(LOG_NOTICE, "Warning, g is NULL\n");
		return -1;
	}

	//0:normal, 1:green, yellow, 2:each gem port has it own pq(todo)
	//in normal, only support green, no yellow

	if (omci_env_g.tm_pq_gem_qos_method!=1) {
		is_yellow=0;
	}

	for (pos=0; pos < GPON_FLOW_ID_TOTAL; pos++) {
		if (g->gemflow_map[pos].is_used == 1 &&
			g->gemflow_map[pos].me_gem == me_gem &&
			g->gemflow_map[pos].is_yellow == is_yellow) {
			found=pos;
			break;
		}
	}
	batchtab_table_data_put("gemflow");
	return found;
}

int
gem_flowid_get_by_gem_portid(unsigned short gem_portid, char is_yellow)
{
	int pos, found=-1;
	struct batchtab_cb_gemflow_t *g;

	g=(struct batchtab_cb_gemflow_t *)batchtab_table_data_get("gemflow");
	if (g == NULL) {
		dbprintf_bat(LOG_WARNING, "Warning, g is NULL\n");
		return -1;
	}

	//0:normal, 1:green, yellow, 2:each gem port has it own pq(todo)
	//in normal, only support green, no yellow
	if (omci_env_g.tm_pq_gem_qos_method!=1) {
		is_yellow=0;
	}

	for (pos=0; pos < GPON_FLOW_ID_TOTAL; pos++) {
		if (g->gemflow_map[pos].is_used == 1 &&
			g->gemflow_map[pos].gem_portid == gem_portid &&
			g->gemflow_map[pos].is_yellow == is_yellow) {
			found=pos;
			break;
		}
	}
	batchtab_table_data_put("gemflow");
	return found;
}
