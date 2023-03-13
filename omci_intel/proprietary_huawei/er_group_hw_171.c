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
 * File    : er_group_hw_171.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"

// 171 Extended_VLAN_tagging_operation_configuration_data

// 171@6
int
er_group_hw_ignore_vlan_drop_rule(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char treatment_tags_to_remove = 0;
	struct attr_table_header_t *table_header = NULL;	
	struct attr_table_entry_t *table_entry_pos;
	struct er_attr_group_instance_t *current_attr_inst=NULL;
	
	if(omci_env_g.olt_workaround_enable != ENV_OLT_WORKAROUND_HUAWEI)
		return 0;
	
	if(omci_env_g.olt_huawei_ignore_vlan_drop_rule == 0)
		return 0;
	
	if(attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:	
		if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[0].attr_value.ptr) == NULL) {
			dbprintf(LOG_ERR, "vlan tagging table is empty\n");
			return -1;
		}
		
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				treatment_tags_to_remove = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 64, 2);
				if(treatment_tags_to_remove==3) { // change 3 (drop) to 0 (bypass)
					util_bitmap_set_value(table_entry_pos->entry_data_ptr, 16*8, 64, 2, 0);
					meinfo_me_attr_set_table_entry(me, 6, table_entry_pos->entry_data_ptr, 0);
				}
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_ignore_vlan_drop_rule(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_ignore_vlan_drop_rule(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}
	return 0;
}
