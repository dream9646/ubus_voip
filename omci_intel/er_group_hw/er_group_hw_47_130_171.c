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
 * File    : er_group_hw_47_130_171.c
 *
 ******************************************************************/

#include <stdio.h>
#include <assert.h>

#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "omciutil_vlan.h"
#include "switch.h"

// 47 MAC_bridge_port_configuration_data
// 130 802.1p_mapper_service_profile
// 171 Extended_VLAN_tagging_operation_configuration_data

// 47@2, 171@8
int 
er_group_hw_l2s_dscp_vlan_tagging_rxtagging_map_802_1p(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	// this is to handle vtagging(171) bond on 8021p mapper(130) but not bport(47) directly
	return er_group_hw_l2s_dscp_vlan_tagging_rxtagging_map_bridge(action, attr_inst, me, attr_mask);
}

//47@2,171@6
int
er_group_hw_l2s_tags_create_add012_802_1p(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	// this is to handle vtagging(171) bond on 8021p mapper(130) but not bport(47) directly	
	return er_group_hw_l2s_tags_create_add012_bridge(action, attr_inst, me, attr_mask);
}

