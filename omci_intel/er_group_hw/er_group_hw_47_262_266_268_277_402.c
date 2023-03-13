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
 * File    : er_group_hw_47_262_266_268_277_402.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"

// 47 MAC_bridge_port_configuration_data 
// 266 GEM_interworking_termination_point 
// 268 GEM_port_network_CTP 
// 277 Priority_queue_G 
// 402 Upstream pseudo obj 

//47@2,266@6,268@1,268@3,277@255
int
er_group_hw_mac_gem_bridge_allocate_gem_iw_tp_tm_rate_controlled(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	er_group_hw_mac_gem_bridge_allocate_gem_iw_tp(action, attr_inst, me, attr_mask);
	return 0;
}
