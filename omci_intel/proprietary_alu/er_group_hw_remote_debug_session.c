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
 * Module  : proprietary_alu
 * File    : er_group_hw_remote_debug_session.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "proprietary_alu_remote_debug_session.h"

// 65281 ONT_Aggregate_GEM_Port_PM

// 65281@meid
int
er_group_hw_remote_debug_session(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		proprietary_alu_set_gem_port(1);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		proprietary_alu_set_gem_port(0);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:		
		//delete
		if (er_group_hw_remote_debug_session(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0) {
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_remote_debug_session(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}

	return 0;
}

