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
 * File    : er_group_hw_53_145_153.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "voip_hw.h"
#include "er_group_hw.h"
#include "er_group_hw_util.h"
#include "batchtab.h"

// 53 Physical_path_termination_point_POTS_UNI
// 145 Network_dial_plan_table
// 153 SIP_user_data

//53@254,145@1,145@2,145@3,145@4,145@5,145@6
int
er_group_hw_network_dial_plan_table(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	struct voip_hw_network_dial_plan_table_t ndpt;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	struct voip_hw_dial_plan_entry_t *dp_entry;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = attr_inst->er_attr_instance[0].attr_value.data;

	memset(&ndpt, 0x00, sizeof(ndpt));

	//dial plan number
	ndpt.dial_plan_number = (unsigned short) attr_inst->er_attr_instance[1].attr_value.data;

	//dial plan table max size
	ndpt.dial_plan_table_max_size = (unsigned short) attr_inst->er_attr_instance[2].attr_value.data;

	//critical dial timeout
	ndpt.critical_dial_timeout = (unsigned short) attr_inst->er_attr_instance[3].attr_value.data;

	//partial dial timeout
	ndpt.partial_dial_timeout = (unsigned short) attr_inst->er_attr_instance[4].attr_value.data;

	//dial plan format
	ndpt.dial_plan_format = (unsigned char) attr_inst->er_attr_instance[5].attr_value.data;

	//table
	INIT_LIST_HEAD(&ndpt.dial_plan_list);
	if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[6].attr_value.ptr) != NULL) {
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node) {
			if (table_entry_pos->entry_data_ptr != NULL) {
				dp_entry = malloc_safe(sizeof(struct voip_hw_dial_plan_entry_t));
				dp_entry->id = ((unsigned char *)table_entry_pos->entry_data_ptr)[0];
				dp_entry->action = ((unsigned char *)table_entry_pos->entry_data_ptr)[1];
				memcpy(dp_entry->token, &(((unsigned char *)table_entry_pos->entry_data_ptr)[2]), 28);
				list_add_tail(&dp_entry->dial_plan_node, &ndpt.dial_plan_list);
			}
		}
	} else {
		dbprintf(LOG_ERR, "attr value was not table\n");
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_attr_group_util_get_attr_mask_by_classid(attr_inst, 145, attr_mask);
	case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:
		if ((ret = er_group_hw_voip_wrapper_network_dial_plan_table(VOIP_HW_ACTION_SET, port_id, attr_mask, &ndpt)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			voip_hw_network_dial_plan_table_release_table(&ndpt);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			voip_hw_network_dial_plan_table_release_table(&ndpt);
			return -1;
		}
		//add
		if (er_group_hw_network_dial_plan_table(ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			voip_hw_network_dial_plan_table_release_table(&ndpt);
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		voip_hw_network_dial_plan_table_release_table(&ndpt);
		memset(&ndpt, 0x00, sizeof(ndpt));
		INIT_LIST_HEAD(&ndpt.dial_plan_list);
		if ((ret = er_group_hw_voip_wrapper_network_dial_plan_table(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &ndpt)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
		;
	}

	voip_hw_network_dial_plan_table_release_table(&ndpt);

	return 0;
}
