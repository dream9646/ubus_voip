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
 * File    : er_group_hw_47_329_65305.c
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
#include "proprietary_alu.h"

// 47 MAC_bridge_port_configuration_data
// 329 Virtual_Ethernet_interface_point
// 65305 Downstream_VLAN_Tag_Egress_Behavior_Supplemental_1

// 47@2,65305@1,65305@2
int
er_group_hw_l2s_tags_create_add012_veip_ds_supplemental_1(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	er_group_hw_l2s_tags_create_add012_ds_supplemental_1(action, attr_inst, me, attr_mask);
	return 0;
}

