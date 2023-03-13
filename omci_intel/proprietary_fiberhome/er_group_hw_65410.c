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
 * File    : er_group_hw_65410.c
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

// 65410 WRI_RTP_resource_name

//65410@1,65410@2,65410@3,65410@4,65410@5
int
er_group_hw_wri_rtp_resource_name(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	struct voip_hw_wri_rtp_resource_name_t wrrn;
	struct attr_value_t attr;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	port_id = VOIP_HW_GLOBAL_PORT_DEF;

	memset(&wrrn, 0x00, sizeof(wrrn));

	attr.ptr = wrrn.rtp_name_fixed_part;
	meinfo_me_attr_copy_value(65410, 1, &attr, &(attr_inst->er_attr_instance[0].attr_value));
	wrrn.rtp_name_var_begin = (unsigned short) attr_inst->er_attr_instance[1].attr_value.data;
	wrrn.rtp_name_var_end = (unsigned short) attr_inst->er_attr_instance[2].attr_value.data;
	wrrn.rtp_name_var_step = (unsigned short) attr_inst->er_attr_instance[3].attr_value.data;
	wrrn.rtp_name_fixed_length = (unsigned short) attr_inst->er_attr_instance[4].attr_value.data;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((ret = er_group_hw_voip_wrapper_wri_rtp_resource_name(VOIP_HW_ACTION_SET, port_id, attr_mask, &wrrn)) < 0)
		{
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_wri_rtp_resource_name(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&wrrn, 0x00, sizeof(wrrn));
		if ((ret = er_group_hw_voip_wrapper_wri_rtp_resource_name(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &wrrn)) < 0)
		{
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}
