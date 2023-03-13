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
 * Module  : proprietary_tellion
 * File    : er_group_hw_240.c
 *
 ******************************************************************/

#include "util.h"
#include "util_run.h"
#include "er_group.h"

// 240 Private_ONT_System_Management

//240@1,2,3
int
er_group_hw_self_rogue_detect(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned short disable = attr_inst->er_attr_instance[0].attr_value.data;
	//unsigned short interval = attr_inst->er_attr_instance[1].attr_value.data; // polling interval, not support yet
	//unsigned char count = attr_inst->er_attr_instance[2].attr_value.data; // polling count, not support yet

	if (attr_inst == NULL)
		return -1;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (disable != 0xffff) {
			char cmd[64];
			sprintf(cmd, "sleep %d; /etc/init.d/bosa.sh tx_dis", disable);
			util_run_by_thread(cmd, 0);
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//del
		if (er_group_hw_self_rogue_detect(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_self_rogue_detect(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
		;
	}

	return 0;
}
