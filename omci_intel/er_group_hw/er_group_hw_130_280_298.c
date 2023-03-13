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
 * File    : er_group_hw_130_280_298.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"
#include "er_group_hw.h"

// 46 MAC_bridge_configuration_data or 130 802.1p_mapper_service_profile
// in T-REC-G.988-201006-P change to 
// 45 MAC bridge service profile or 130 802.1p_mapper_service_profile
// 280 Traffic_descriptor
// 298 Dot1_rate_limiter
// storm_control_dup
//		0	1	2	3    4      5      6
// 45|46|130@meid, 280@meid, 280@1, 280@2, 298@1, 298@2, 298@3
// 45|46|130@meid, 280@meid, 280@1, 280@2, 298@1, 298@2, 298@4
// 45|46|130@meid, 280@meid, 280@1, 280@2, 298@1, 298@2, 298@5

int
er_group_hw_storm_control_dup_unknow_unicast(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return er_group_hw_port_storm_control(action, attr_inst, me, attr_mask, STORM_UNKNOWN_UNICAST);
}

int
er_group_hw_storm_control_dup_broadcast(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return er_group_hw_port_storm_control(action, attr_inst, me, attr_mask, STORM_BROADCAST);
}

int
er_group_hw_storm_control_dup_unknow_multicast(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return er_group_hw_port_storm_control(action, attr_inst, me, attr_mask, STORM_UNKNOWN_MULTICAST);
}
