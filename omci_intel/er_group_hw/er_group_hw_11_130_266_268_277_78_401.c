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
 * File    : er_group_hw_11_130_266_268_277_78_401.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"

// 11 Physical_path_termination_point_Ethernet_UNI 
// 130 802.1p_mapper_service_profile 
// 266 GEM_interworking_termination_point 
// 268 GEM_port_network_CTP 
// 277 Priority_queue_G 
// 78 VLAN_tagging_operation_configuration_data
// 401 Downstream pseudo obj 

//11@254,130@2,130@3,130@4,130@5,130@6,130@7,130@8,130@9,266@6,268@1,268@3,277@255,78@1,78@2,78@3
int
er_group_hw_l2s_unipqmap_802_1p_tagop(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return er_group_hw_l2s_unipqmap_802_1p(action, attr_inst, me, attr_mask);
}
