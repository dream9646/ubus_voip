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
 * File    : er_group_hw_157.c
 *
 ******************************************************************/

#include <string.h>
#include "util.h"
#include "er_group.h"
#include "batchtab.h"
#include "proprietary_tellion.h"

// 157 Large_string

//157@7
int
er_group_hw_nat_mode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	char *mode_str = attr_inst->er_attr_instance[0].attr_value.ptr; // bridge or nat

	if (attr_inst == NULL)
		return -1;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if((strncmp(mode_str, "nat", 3)==0) || (strncmp(mode_str, "bridge", 6)==0)) {
			proprietary_tellion_set_nat_mode((strncmp(mode_str, "nat", 3)==0) ? 1 : 0);
			batchtab_omci_update("classf");
			batchtab_omci_update("wanif");
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//del
		if (er_group_hw_nat_mode(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		if (er_group_hw_nat_mode(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
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
