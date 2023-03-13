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
 * File    : er_group_hw_47_281_268.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "er_group_hw.h"
#include "switch.h"
#include "batchtab.h"
#include "mcast.h"

// 47 MAC_bridge_port_configuration_data 
// 281 Multicast_GEM_interworking_termination_point 
// 268 GEM_port_network_CTP 

//47@2,281@6,268@1,268@3

int
er_group_hw_mac_gem_bridge_allocate_multicast_gem_iw_tp(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL){
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		// the streamid alloc/dealloc in now handled by batchtab_cb 'gemflow'
		batchtab_omci_update("gemflow");
		// whether to start igmp task
		batchtab_omci_update("mcastmode");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		// the streamid alloc/dealloc in now handled by batchtab_cb 'gemflow'
		batchtab_omci_update("gemflow");
		// whether to stop igmp task
		batchtab_omci_update("mcastmode");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//delete
		if (er_group_hw_mac_gem_bridge_allocate_multicast_gem_iw_tp(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}

		//add
		if (er_group_hw_mac_gem_bridge_allocate_multicast_gem_iw_tp(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
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
