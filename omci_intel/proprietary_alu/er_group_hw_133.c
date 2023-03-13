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
 * File    : er_group_hw_133.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw_util.h"
#include "switch.h"
#include "proprietary_alu.h"

// 133 ONT_power_shedding

//133@1
int
er_group_hw_mac_limit_learning(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	int ret = 0;
	
	if (attr_inst == NULL)
		return -1;
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		// since me133 is created by ont, so we dont hook the ER_ATTR_GROUP_HW_OP_ADD & ER_ATTR_GROUP_HW_OP_DEL
		// ALU-CHT workaround for OLT 7342 to enable "mac limit learning" via ME133 attribute #1 "Restore_power_timer_reset_interval"
		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU &&
		    proprietary_alu_get_olt_version() == 0) {
			unsigned int global_depth = (unsigned short) (attr_inst->er_attr_instance[0].attr_value.data);
			dbprintf(LOG_ERR, "me133 0x%x: ont global_depth=%d, learn_ind=1\n", me->meid, global_depth);
			// port 255 means per ont instead of per port
			if (global_depth) {
				// mac depth >0, learn ind 1: mac limit enabled, add 3 more entries for eth0, pon0, pon0.4090
				ret = er_group_hw_util_mac_learning_depth_set(255, global_depth+3, 1);
			} else {
				// mac depth 0, learn ind 1: mac limit disabled
				ret = er_group_hw_util_mac_learning_depth_set(255, 0, 1);
			}
		}
		
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
		ret = -1;
	}
	return ret;
}
