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
 * File    : er_group_hw_45_47.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"
#include "er_group_hw.h"
#include "er_group_hw_util.h"
#include "batchtab.h"
#include "proprietary_alu.h"

//9.3.1 45 MAC_bridge_service_profile
//9.3.4 47 MAC_bridge_port_configuration_data

//45@3(Port_bridging_ind), 47@2(Port_num)
// get port members(me47) of the specific bridge service profile(me45)

// gps l2s bridging localbridge
// 45@3, 47@2
int 
er_group_hw_l2s_bridging_localbridging(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	// private me wont be updated by olt, no need to do er_group update
	if (me && me->classid == 47 && me->is_private)
		return 0;

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	        batchtab_omci_update("isolation_set");
		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

// gps l2s mac-table assign
// 45@inst, 47@1, 47@2
int 
er_group_hw_l2s_mac_table_assign(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}
	// FIXME, KEVIN
	// 2510 doesn't support virtual bridge, so we don't need to do bridge assignment for per bridge port
	return 0;
}

// 0   1   2
//45@2,8, 47@inst
int
er_group_hw_l2s_mac_table_flood(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char learning_ind, unknown_mac_address_discard;
	struct er_attr_group_instance_t *current_attr_inst;
	struct switch_port_info_t port_info;
	struct me_t *ibridgeport_me;
	int ret = 0;

	if (attr_inst == NULL)
		return -1;
		
	learning_ind=attr_inst->er_attr_instance[0].attr_value.data;
	unknown_mac_address_discard=attr_inst->er_attr_instance[1].attr_value.data;

	// private me wont be updated by olt, no need to do er_group update
	if (me && me->classid == 47 && me->is_private)
		return 0;

	if ((ibridgeport_me=er_attr_group_util_attrinst2private_me(attr_inst, 2)) == NULL) {	//need 47's port
	       	dbprintf(LOG_ERR,"private bridge port me null?\n");
		return -1;
	}

	//get port info
	if (switch_get_port_info_by_me(ibridgeport_me, &port_info) < 0)
	{
		dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", ibridgeport_me->classid, ibridgeport_me->meid);
		return -1;
	}

	if ( port_info.port_type != 1 ) {	// 0:cpu(veip), 1:uni, 2:wan, 3:iphost, 4:ds bcast
		return 0;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0) &&
		    (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU)) {
			// Always disable unknown MAC discard for ALU OLT
			;
		} else {
			if (switch_hw_g.mac_flood_enable_set) {
				if((ret=switch_hw_g.mac_flood_enable_set(port_info.logical_port_id, !unknown_mac_address_discard, learning_ind)) !=0 ) {
					dbprintf(LOG_ERR, "switch_hw_g.mac_flood_enable_set error=%d\n", ret);
					return -1;
				}
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_l2s_mac_table_flood(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
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

static int
mac_learning_depth_by_me_port_info(struct me_t *bport_me, struct switch_port_info_t *port_info, int port_depth, int global_depth, int learning_ind)
{
	unsigned int portid;
	
	if (port_info->port_type == 0) {	// veip
		struct meinfo_t *miptr47 = meinfo_util_miptr(47);
		struct me_t *meptr47;
		unsigned short veip_bridge_meid = meinfo_util_me_attr_data(bport_me, 1);
		list_for_each_entry(meptr47, &miptr47->me_instance_list, instance_node) {
			unsigned short tp_classid;
			struct switch_port_info_t port_info2;
			// find pptp_uni related bport me in the same veip bridge (these me are created by batchtab autouni)
			// then applied veip port_depth to those found pptp_uni port
			if (meptr47->is_private && meptr47->attr[1].value.data == veip_bridge_meid) {
				tp_classid = miptr47->callback.get_pointer_classid_cb(meptr47, 4);
				if (tp_classid == 11 && switch_get_port_info_by_me(meptr47, &port_info2) ==0) {
					portid = port_info2.logical_port_id;
					dbprintf(LOG_ERR, "veip bport 0x%x -> uni bport 0x%x: port=%d, port_depth=%d, global_depth=%d, learning_ind=%d\n",
						bport_me->meid, meptr47->meid, portid, port_depth, global_depth, learning_ind);
					er_group_hw_util_mac_learning_depth_set2(portid, port_depth, global_depth, learning_ind);
				}
			}
		}
	} else if (port_info->port_type == 1) {	// uni
		portid = port_info->logical_port_id;
		dbprintf(LOG_ERR, "uni bport 0x%x: port=%d, port_depth=%d, global_depth=%d, learning_ind=%d\n",
			bport_me->meid, portid, port_depth, global_depth, learning_ind);
		return er_group_hw_util_mac_learning_depth_set2(portid, port_depth, global_depth, learning_ind);
	}	

	return 0;	// port_type ignored
}

// 0   1   2
//45@2,9, 47@13
int
er_group_mac_learning_depth_by_port(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *bport_me;
	struct switch_port_info_t port_info;
	unsigned port_depth, global_depth, learning_ind;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	// do nothing for ALU OLT, which doesn't support me45 learning_ind, mac_depth, me47 mac_depth
	// ALU 7432 uses me133 (ONT_power_shedding) attr1 for ont mac_depth to control the maclimiting function
	if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0) &&
	    (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU))
	   	return 0;

	//get port info
	if ((bport_me = er_attr_group_util_attrinst2me(attr_inst, 2)) == NULL) {	//need 47's port
	       	dbprintf(LOG_ERR,"bridge port me null?\n");
		return -1;
	}
	// private bport wont be updated by olt, no need to do er_group update
	if (bport_me->is_private)
		return 0;

	if (switch_get_port_info_by_me(bport_me, &port_info) < 0) {
		dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", bport_me->classid, bport_me->meid);
		return -1;
	}
	if (port_info.port_type > 1)		// 0:cpu(veip), 1:uni, 2:wan, 3:iphost, 4:ds bcast
		return 0;

	// learning_ind only appear in me45, mac_learning_depth appears on me45 & me47
	learning_ind=attr_inst->er_attr_instance[0].attr_value.data;
	global_depth = attr_inst->er_attr_instance[1].attr_value.data;	// use 45@9 MAC_learning_depth instead
	port_depth = attr_inst->er_attr_instance[2].attr_value.data;	// 47@13 MAC_learning_depth	

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		return mac_learning_depth_by_me_port_info(bport_me, &port_info, port_depth, global_depth, learning_ind);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_mac_learning_depth_by_port(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;	
	case ER_ATTR_GROUP_HW_OP_DEL:
		// set port_depth=0, global_depth=0, learning_ind=1 for DEL
		return mac_learning_depth_by_me_port_info(bport_me, &port_info, 0, 0, 1);
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	
	batchtab_omci_update("hardbridge");
	return 0;
}

//45@inst,47@2
int
er_group_hw_l2s_tags_create_add012_bridge_port_add(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *issue_bridge_port_me;
	
	if (attr_inst == NULL)
	{
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	// private me wont be updated by olt, no need to do er_group update
	if (me && me->classid == 47 && me->is_private)
		return 0;

	//get bridge port me
	if ((issue_bridge_port_me = er_attr_group_util_attrinst2me(attr_inst, 1)) == NULL)
	{
		dbprintf(LOG_ERR, "could not get bridge port me, classid=%u, meid=%u\n", 
			attr_inst->er_attr_group_definition_ptr->er_attr[1].classid, attr_inst->er_attr_instance[1].meid);
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	case ER_ATTR_GROUP_HW_OP_DEL:
	        // as some proprietary me is related to all uni port implicitly, 
	        // however, the bridge port might be changed after the proprietary is set
	        // so we trigger tagging/filtering collect in classf on bport change
	        batchtab_omci_update("classf");
	        batchtab_omci_update("tagging");
	        batchtab_omci_update("calix_apas");
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}

	return 0;
}
