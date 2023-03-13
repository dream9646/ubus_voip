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
 * File    : er_group_hw_11_290.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "switch.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "er_group_hw_util.h"

// 11 Physical_path_termination_point_Ethernet_UNI
// 290 Dot1X_port_extension_package


//gps l2s bridging dot1x 
//11@meid, 290@meid, 290@1

int
er_group_hw_dot1x(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char method;
	struct me_t *bridge_port_me;
	struct switch_port_info_t port_info;
	int ret;

	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	//check if 11's meid == 290's meid

	if (attr_inst->er_attr_instance[0].attr_value.data != attr_inst->er_attr_instance[1].attr_value.data)
	{
		dbprintf(LOG_ERR, "meid not the same, 11:%u, 290:%u\n",
			attr_inst->er_attr_instance[0].attr_value.data,
			attr_inst->er_attr_instance[1].attr_value.data);
		return 0;
	}

	method=attr_inst->er_attr_instance[2].attr_value.data;
	bridge_port_me=er_attr_group_util_attrinst2me(attr_inst, 0);

	if (bridge_port_me==NULL)
		return -1;
	if (switch_get_port_info_by_me(bridge_port_me, &port_info) < 0) {
		if (bridge_port_me!=NULL)
			dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", bridge_port_me->classid, bridge_port_me->meid);
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		//dot1x disable
		if (method == 0) {
			if (switch_hw_g.dot1x_mode_set == NULL || 
			    (ret = switch_hw_g.dot1x_mode_set(port_info.logical_port_id, 0) < 0)) {
				dbprintf(LOG_ERR, "dot1x_mode_set 0 error, port=%d, ret=%d\n", port_info.logical_port_id, ret);
				return -1;
			}
			return 0;
		}

		//dot1x enable

		// clear mac entry for this dot1x port
		if (switch_hw_g.mac_table_flush)
			switch_hw_g.mac_table_flush(1<<port_info.logical_port_id, 0);
		// enable dot1x on port
		if (switch_hw_g.dot1x_mode_set == NULL || 
		    (ret = switch_hw_g.dot1x_mode_set(port_info.logical_port_id, 1) < 0)) {
			dbprintf(LOG_ERR, "dot1x_mode_set 1 error, port=%d, ret=%d\n", port_info.logical_port_id, ret);
			return -1;
		}

		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_dot1x(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		if (er_group_hw_dot1x(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		//gps l2s bridging dot1x
		//dot1x disable
		if (switch_hw_g.dot1x_mode_set == NULL || 
		    (ret = switch_hw_g.dot1x_mode_set(port_info.logical_port_id, 0) < 0)) {
			dbprintf(LOG_ERR, "dot1x_mode_set 0 error, port=%d, ret=%d\n", port_info.logical_port_id, ret);
			return -1;
		}
		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//11@meid, 290@meid, 290@5

int
er_group_hw_dot1x_dir(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char dir;
	struct me_t *bridge_port_me;
	struct switch_port_info_t port_info;
	int ret;

	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	//check if 11's meid == 290's meid

	if (attr_inst->er_attr_instance[0].attr_value.data != attr_inst->er_attr_instance[1].attr_value.data)
	{
		dbprintf(LOG_ERR, "meid not the same, 11:%u, 290:%u\n",
			attr_inst->er_attr_instance[0].attr_value.data,
			attr_inst->er_attr_instance[1].attr_value.data);
		return 0;
	}

	dir=attr_inst->er_attr_instance[2].attr_value.data;
	bridge_port_me=er_attr_group_util_attrinst2me(attr_inst, 0);

	if (bridge_port_me==NULL)
		return -1;
	if (switch_get_port_info_by_me(bridge_port_me, &port_info) < 0) {
		if (bridge_port_me!=NULL)
			dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", bridge_port_me->classid, bridge_port_me->meid);
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
		//dot1x dir set
		if (switch_hw_g.dot1x_dir_set == NULL || 
		    (ret = switch_hw_g.dot1x_dir_set(port_info.logical_port_id, dir) < 0)) {
			dbprintf(LOG_ERR, "dot1x_dir_set %d error, port=%d, ret=%d\n", dir, port_info.logical_port_id, ret);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_dot1x_dir(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		if (er_group_hw_dot1x_dir(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}
