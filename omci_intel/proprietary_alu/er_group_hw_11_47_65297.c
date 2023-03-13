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
 * File    : er_group_hw_11_47_65297.c
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
#include "batchtab.h"

// 11 Physical_path_termination_point_Ethernet_UNI
// 47 MAC_bridge_port_configuration_data
// 65297 UNI_Supplemental_1_V2

// 47@2,65297@1
int
er_group_hw_l2s_tags_create_add012_ds_supplemental_1_old(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *issue_bridge_port_me, *issue_me;
	
	if (attr_inst == NULL)
	{
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	//get bridge port me
	if ((issue_bridge_port_me = er_attr_group_util_attrinst2me(attr_inst, 0)) == NULL)
	{
		dbprintf(LOG_ERR, "could not get bridge port me, classid=%u, meid=%u\n", 
			attr_inst->er_attr_group_definition_ptr->er_attr[0].classid, attr_inst->er_attr_instance[0].meid);
		return -1;
	}

	if ((issue_me = er_attr_group_util_attrinst2me(attr_inst, 1)) == NULL)
	{
		dbprintf(LOG_ERR, "could not get issue me, classid=%u, meid=%u\n", 
			attr_inst->er_attr_group_definition_ptr->er_attr[1].classid, attr_inst->er_attr_instance[1].meid);
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	case ER_ATTR_GROUP_HW_OP_DEL:
	        // tagging table configuration is handled in batchtab classf
	        batchtab_omci_update("classf");
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}

	return 0;
}

