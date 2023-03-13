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
 * File    : er_group_hw_65415.c
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
#include "proprietary_fiberhome.h"

// 65415 WRI_IPT_config_data

//65415@1,65415@2
int
er_group_hw_wri_ipt_config_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	struct voip_hw_wri_ipt_config_data_t wicd;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	struct voip_hw_pos_data_entry_t *pd_entry;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	port_id = VOIP_HW_GLOBAL_PORT_DEF;

	memset(&wicd, 0x00, sizeof(wicd));

	wicd.item_number = (unsigned char) attr_inst->er_attr_instance[0].attr_value.data;

	//table
	INIT_LIST_HEAD(&wicd.ipt_data_list);
	if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[1].attr_value.ptr) != NULL) {
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				pd_entry = malloc_safe(sizeof(struct voip_hw_pos_data_entry_t));
				pd_entry->id = ((unsigned char *)table_entry_pos->entry_data_ptr)[0];
				memcpy(pd_entry->tel_num, &(((unsigned char *)table_entry_pos->entry_data_ptr)[1]), 20);
				pd_entry->pos_type = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*23, 168, 16);
				list_add_tail(&pd_entry->pos_data_node, &wicd.ipt_data_list);
			}
		}
	} else {
		dbprintf(LOG_ERR, "attr value was not table\n");
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((ret = er_group_hw_voip_wrapper_wri_ipt_config_data(VOIP_HW_ACTION_SET, port_id, attr_mask, &wicd)) < 0)
		{
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			voip_hw_wri_ipt_config_data_release_table(&wicd);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			voip_hw_wri_ipt_config_data_release_table(&wicd);
			return -1;
		}

		//add
		if (er_group_hw_wri_ipt_config_data(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			voip_hw_wri_ipt_config_data_release_table(&wicd);
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		voip_hw_wri_ipt_config_data_release_table(&wicd);
		memset(&wicd, 0x00, sizeof(wicd));
		INIT_LIST_HEAD(&wicd.ipt_data_list);
		if ((ret = er_group_hw_voip_wrapper_wri_ipt_config_data(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &wicd)) < 0)
		{
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}

	voip_hw_wri_ipt_config_data_release_table(&wicd);

	return 0;
}

