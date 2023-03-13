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
 * File    : er_group_hw_65282.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw_util.h"
#include "hwresource.h"
#include "switch.h"

static unsigned char loop_detection_mask = 0;

int
er_group_hw_loopcheck_status(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char enable;
	struct me_t *status_me;

	if (attr_inst == NULL)
		return -1;

	enable = (unsigned char)attr_inst->er_attr_instance[0].attr_value.data;

	status_me = er_attr_group_util_attrinst2me(attr_inst, 0);
	if (status_me == NULL)
		return -1;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:

		if ( enable == 2 ) {
			loop_detection_mask |= (1 << (status_me->meid));
		} else {
			loop_detection_mask &= ~(1 << (status_me->meid));
		}

		if (loop_detection_mask) {
			if(switch_loop_detect_set(1, loop_detection_mask) < 0) {
				dbprintf(LOG_ERR, "switch_loop_detect_set error!\n");
				return -1;
			}

		} else {
			if(switch_loop_detect_set(0, loop_detection_mask) < 0) {
				dbprintf(LOG_ERR, "switch_loop_detect_set error!\n");
				return -1;
			}
		}
		break;

	case ER_ATTR_GROUP_HW_OP_DEL:

		loop_detection_mask &= ~(1 << (status_me->meid));

		if (loop_detection_mask) {
			if(switch_loop_detect_set(1, loop_detection_mask) < 0) {
				dbprintf(LOG_ERR, "switch_loop_detect_set error!\n");
				return -1;
			}

		} else {
			if(switch_loop_detect_set(0, loop_detection_mask) < 0) {
				dbprintf(LOG_ERR, "switch_loop_detect_set error!\n");
				return -1;
			}
		}
		break;

	case ER_ATTR_GROUP_HW_OP_UPDATE:

		if (er_group_hw_loopcheck_status(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		//allocate
		if (er_group_hw_loopcheck_status(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}

	return 0;

}

int
er_group_hw_loopcheck_interval(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int time_period;
	if (attr_inst == NULL)
		return -1;

	time_period = (unsigned int)attr_inst->er_attr_instance[0].attr_value.data;

	if (time_period < 10 )
		time_period = 10000;
	else if (time_period > 65)
		time_period = 65535;
	else
		time_period = time_period*1000;
		

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if( switch_loop_detect_interval_set(time_period, 2000) < 0) {
			dbprintf(LOG_ERR, "switch_loop_detect_interval_set error!\n");
			return -1;
		}
		break;

	case ER_ATTR_GROUP_HW_OP_DEL:
		if( switch_loop_detect_interval_set(0xffff, 2000) < 0 ) {
			dbprintf(LOG_ERR, "switch_loop_detect_interval_set error!\n");
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if (er_group_hw_loopcheck_interval(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		//allocate
		if (er_group_hw_loopcheck_interval(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}
