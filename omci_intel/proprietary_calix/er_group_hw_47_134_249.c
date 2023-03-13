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
 * File    : er_group_hw_47_134_249.c
 *
 ******************************************************************/

#include "proprietary_calix.h"

// 47 MAC_bridge_port_configuration_data
// 134 IP_host_config_data
// 249 ExtL2VlanTagConfigData

//47@2,249@2,3,5
int
er_group_hw_l2s_l2_tags_create_add012_ip_host(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	// this is to handle vtagging(249) bond on ip host(134) but not bport(47) directly	
	return er_group_hw_l2s_l2_tags_create_add012_bridge(action, attr_inst, me, attr_mask);
}

