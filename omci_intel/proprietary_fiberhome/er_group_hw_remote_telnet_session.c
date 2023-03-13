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
 * File    : er_group_hw_remote_telnet_session.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "proprietary_fiberhome_remote_telnet_session.h"

// 131 OLT_G

// 131@4
int
er_group_hw_remote_telnet_session(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	struct timeval tv;

	if (attr_inst == NULL)
		return -1;

	tv.tv_sec = (unsigned long)util_bitmap_get_value(attr_inst->er_attr_instance[0].attr_value.ptr, 112, 32, 48);
	tv.tv_usec = (unsigned long)(util_bitmap_get_value(attr_inst->er_attr_instance[0].attr_value.ptr, 112, 80, 32)/1000);
	
	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (tv.tv_sec) proprietary_fiberhome_set_gem_port(1);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		if (!tv.tv_sec) proprietary_fiberhome_set_gem_port(0);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:		
		//delete
		if (er_group_hw_remote_telnet_session(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0) {
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_remote_telnet_session(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}

	return 0;
}

