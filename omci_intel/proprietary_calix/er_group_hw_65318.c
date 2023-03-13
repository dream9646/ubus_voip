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
 * Module  : proprietary_calix
 * File    : er_group_hw_65318.c
 *
 ******************************************************************/
#include <string.h>
#include "util.h"
#include "er_group.h"
#include "meinfo_hw_poe.h"

// 65318@1,2
int
er_group_hw_max_power_budget(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	
	if (attr_inst == NULL)
	{
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(attr_inst->er_attr_instance[0].attr_value.data == 0) {
			// double-check Management_Capabilities to avoid it is mib default value
			extern int poe_mgt_cap;
			if (poe_mgt_cap == -1)
				meinfo_hw_poe_get_management_capability(&poe_mgt_cap);
			if (poe_mgt_cap == 0) {
				meinfo_hw_poe_set_power_budget(attr_inst->er_attr_instance[1].attr_value.data);
				if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
					// write poe power budget to default config because calix don't want to power off poe client during mib reset
					char str[8] = {0};
					memset(str, 0, sizeof(str));
					sprintf(str, "%d", (unsigned short) attr_inst->er_attr_instance[1].attr_value.data);
					meinfo_config_set_attr_by_str(me->classid, me->instance_num, 2, str);
				}
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_max_power_budget(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}

	return 0;
}
