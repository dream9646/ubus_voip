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
 * File    : er_group_hw_47_130_266_268_277_402.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"
#include "batchtab.h"

// 47 MAC_bridge_port_configuration_data 
// 130 802.1p_mapper_service_profile 
// 266 GEM_interworking_termination_point 
// 268 GEM_port_network_CTP 
// 277 Priority_queue_G 
// 402 Upstream pseudo obj 

//47@2,130@2,130@3,130@4,130@5,130@6,130@7,130@8,130@9,266@6,268@1,268@3,277@255
int
er_group_hw_mac_gem_bridge_allocate_802_1p(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	case ER_ATTR_GROUP_HW_OP_DEL:
                // the 8021p configuration is moved into batchtab flow and classf
                batchtab_omci_update("gemflow");
                batchtab_omci_update("classf");
	        batchtab_omci_update("tagging");
		batchtab_omci_update("mcastmode");
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}
	return 0;
}
