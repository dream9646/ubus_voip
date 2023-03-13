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
 * File    : er_group_hw_53.c
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
#include "switch.h"

// 83 Physical_path_termination_point_LCT_UNI

//83
int
er_group_hw_physical_path_termination_point_lct_uni(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL)
		return -1;


	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (switch_hw_g.pptp_lct_uni_create != NULL) {
			switch_hw_g.pptp_lct_uni_create(me->meid);
		}

		if (switch_hw_g.pptp_lct_uni_update != NULL) {
			switch_hw_g.pptp_lct_uni_update(me->meid, (unsigned short) attr_inst->er_attr_instance[1].attr_value.data);
		}

		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if (switch_hw_g.pptp_lct_uni_update != NULL) {
			switch_hw_g.pptp_lct_uni_update(me->meid, (unsigned short) attr_inst->er_attr_instance[1].attr_value.data);
		}

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		if (switch_hw_g.pptp_lct_uni_destory != NULL) {
			switch_hw_g.pptp_lct_uni_destory(me->meid);
		}
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

