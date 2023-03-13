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
 * File    : er_group_hw_47_130_266_268_277_280_402.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"

// 268 GEM_port_network_CTP
// 280 GEM_traffic_descriptor
// 402 Upstream pseudo obj

//268@1,268@5,280@1,280@2,280@3,280@4,280@5,280@6,280@7,280@8
int
er_group_hw_mac_traffic_control_td_us_gem_802_1p(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	er_group_hw_mac_traffic_control_td_us_gem_iwtp(action, attr_inst, me, attr_mask);
	return 0;
}
