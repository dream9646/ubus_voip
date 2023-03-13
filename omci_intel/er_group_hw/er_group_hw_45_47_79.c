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
 * File    : er_group_hw_45_47_79.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "me_related.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "switch.h"

// 45 MAC_bridge_service_profile
// 47 MAC_bridge_port_configuration_data
// 79 MAC_bridge_port_filter_preassign_table

// gps l2s bridging preassign-filter
//45@inst,47@2,79@1-10
int er_group_hw_l2s_preassign_filt(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	//unsigned char ibridge_id, ibridgeport_portid;
	int protocol_num ;
	int ret = 0;
	struct me_t *ibridgeport_me;
	struct er_attr_group_instance_t *current_attr_inst;
	struct switch_port_info_t port_info;
	
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	// attr_inst[1] is bridge port config data port number
	if ((ibridgeport_me=er_attr_group_util_attrinst2private_me(attr_inst, 1))==NULL) {
		dbprintf(LOG_ERR, "private bridge port me null?\n");
		return -1;
	}	
	//ibridgeport_portid=meinfo_util_me_attr_data(ibridgeport_me, 6);

	if (switch_get_port_info_by_me(ibridgeport_me, &port_info) < 0)
	{
		dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", ibridgeport_me->classid, ibridgeport_me->meid);
		return -1;
	}

	if (switch_hw_g.preassign_filter_set == NULL){
		dbprintf(LOG_ERR, "preassign_filter_set not supported\n");
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		// attr_inst[2-11] is protocol_type defined MAC_bridge_port_filter_table_data:
		// ipv4_mcast, ipv6_mcast, ipv4_bcast, rarp, ipx, netbeui, apple_talk, brg_mng, arp, pppoe_bcast,
		for (protocol_num=0; protocol_num<10; protocol_num++) {
			int filter_enabled = attr_inst->er_attr_instance[protocol_num+2].attr_value.data;
			
			if((ret = switch_hw_g.preassign_filter_set (port_info.logical_port_id, protocol_num, filter_enabled?1:0)) < 0) {
				dbprintf(LOG_ERR, "error, protocol num =%u, ret=%d\n", protocol_num, ret);
				return -1;
			}
		}		
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		for (protocol_num=0; protocol_num<10; protocol_num++) {
			if (switch_hw_g.protocol_filter_set == NULL || 
				(ret = switch_hw_g.protocol_filter_set(port_info.logical_port_id, protocol_num, 0) < 0)){
				dbprintf(LOG_ERR, "error, protocol num =%u, ret=%d\n", protocol_num, ret);
				return -1;
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if (er_group_hw_l2s_preassign_filt(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		//allocate
		if (er_group_hw_l2s_preassign_filt(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	}

	return 0;
}
