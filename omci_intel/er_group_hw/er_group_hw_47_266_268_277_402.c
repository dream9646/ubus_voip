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
 * File    : er_group_hw_47_266_268_277_402.c
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
// 266 GEM_interworking_termination_point 
// 268 GEM_port_network_CTP 
// 277 Priority_queue_G 
// 402 Upstream pseudo obj 

//47@2,266@6,268@1,268@3,277@255
int
er_group_hw_mac_gem_bridge_allocate_gem_iw_tp(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
                // the us bridge->gem->pq configuration is moved into batchtab tm/classf        
                batchtab_omci_update("tm");
                batchtab_omci_update("classf");
                batchtab_omci_update("tagging");
		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}