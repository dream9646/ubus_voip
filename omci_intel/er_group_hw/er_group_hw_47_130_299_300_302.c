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
 * File    : er_group_hw_47_130_299_300_302.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"

//9.3.4 47 MAC_bridge_port_configuration_data
//9.3.10 130 802.1p_mapper_service_profile
//9.3.19 299 Dot1ag_maintenance_domain
//9.3.20 300 Dot1ag_maintenance_association
//9.3.22 302 Dot1ag_MEP

// 47@meid 299@meid,1,2,3,4,5,6 300@meid,2,3,4,5,6,7,8 302@meid,4,5,6,8,9,11
int 
er_group_hw_cfm_mapper_info(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	er_group_hw_cfm_bridge_info(action, attr_inst, me, attr_mask);
	return 0;
}
