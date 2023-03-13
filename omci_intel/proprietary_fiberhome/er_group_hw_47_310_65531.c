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
 * Module  : proprietary_fiberhome
 * File    : er_group_hw_47_310_65531.c
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

extern int er_group_hw_util_vlan_tagging_manager(int action, struct me_t *issue_bridge_port_me, struct me_t *issue_me);
        
// 47 MAC_bridge_port_configuration_data
// 310 Multicast_subscriber_config_info
// 65531 Extended_multicast_operations_profile

//47@2,310@6,310@7,65531@7,65531@8,65531@16
int
er_group_hw_l2s_tags_create_add01_multicast_65531(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
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
		//re-collect all tagging rules of all ports, and set to hw tagging table
		if (er_group_hw_util_vlan_tagging_manager(action, issue_bridge_port_me, issue_me) < 0)
		{
			dbprintf(LOG_ERR, "vlan tagging op error, action=%u, classid=%u, meid=%u\n", action, issue_me->classid, issue_me->meid);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}

	return 0;
}

