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
 * File    : er_group_hw_11_47_78.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "omciutil_vlan.h"
#include "batchtab.h"

// 11 Physical_path_termination_point_Ethernet_UNI
// 47 MAC_bridge_port_configuration_data
// 78 VLAN_tagging_operation_configuration_data

//11@inst,47@2,78@1,78@2,78@3
int
er_group_hw_l2s_tags_create_add01_pptp_uni(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	case ER_ATTR_GROUP_HW_OP_DEL:
		if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ERICSSON))
		{
			batchtab_omci_update("ericsson_pvlan");
		}
		batchtab_omci_update("classf");
		batchtab_omci_update("tagging");
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}
	return 0;
}

