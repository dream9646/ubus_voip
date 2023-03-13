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
 * File    : er_group_hw_244.c
 *
 ******************************************************************/

#include "util.h"
#include "er_group.h"
#include "switch.h"

// 244 ONT_Self_Loop_Detect

//244@1,2,3,4,8
int
er_group_hw_self_loop_detect(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int loop_detection_mask = switch_get_uni_logical_portmask();
	unsigned char enable = attr_inst->er_attr_instance[0].attr_value.data;
	unsigned char check = attr_inst->er_attr_instance[1].attr_value.data;
	unsigned short period = attr_inst->er_attr_instance[2].attr_value.data;
	unsigned short timer = attr_inst->er_attr_instance[3].attr_value.data;
	unsigned char unblock = attr_inst->er_attr_instance[4].attr_value.data;

	if (attr_inst == NULL)
		return -1;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		// Set action (check-only or block)
		omci_env_g.rldp_enable = (check==1) ? 3 : 2;
		// Enable / Disable
		switch_loop_detect_set(enable, loop_detection_mask);
		// Set period
		if(period) switch_loop_detect_interval_set(period*1000, period*1000);
		// Set block time
		if(timer) switch_loop_block_time_set(timer);
		// Unblock, re-init rldp
		if(unblock) {
			switch_loop_detect_set(0, loop_detection_mask);
			switch_loop_detect_set(enable, loop_detection_mask);
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//del
		if (er_group_hw_self_loop_detect(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		if (er_group_hw_self_loop_detect(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
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
