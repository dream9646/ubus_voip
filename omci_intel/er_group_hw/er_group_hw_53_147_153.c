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
 * File    : er_group_hw_53_147_153.c
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
// 147 VoIP_feature_access_codes
// 153 SIP_user_data

//53@254,147@1,147@2,147@3,147@4,147@5,147@6,147@7,147@8,147@9,147@10
int
er_group_hw_voip_feature_access_codes(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	struct voip_hw_voip_feature_access_codes_t vfac;
	struct attr_value_t attr;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = attr_inst->er_attr_instance[0].attr_value.data;

	memset(&vfac, 0x00, sizeof(vfac));

	//cancel call waiting
	attr.ptr = vfac.cancel_call_waiting;
	meinfo_me_attr_copy_value(147, 1, &attr, &(attr_inst->er_attr_instance[1].attr_value));

	//call hold
	attr.ptr = vfac.call_hold;
	meinfo_me_attr_copy_value(147, 2, &attr, &(attr_inst->er_attr_instance[2].attr_value));

	//call park
	attr.ptr = vfac.call_park;
	meinfo_me_attr_copy_value(147, 3, &attr, &(attr_inst->er_attr_instance[3].attr_value));

	//caller id activate
	attr.ptr = vfac.caller_id_activate;
	meinfo_me_attr_copy_value(147, 4, &attr, &(attr_inst->er_attr_instance[4].attr_value));

	//caller id deactivate
	attr.ptr = vfac.caller_id_deactivate;
	meinfo_me_attr_copy_value(147, 5, &attr, &(attr_inst->er_attr_instance[5].attr_value));

	//do not disturb activation
	attr.ptr = vfac.do_not_disturb_activation;
	meinfo_me_attr_copy_value(147, 6, &attr, &(attr_inst->er_attr_instance[6].attr_value));

	//do not disturb deactivation
	attr.ptr = vfac.do_not_disturb_deactivation;
	meinfo_me_attr_copy_value(147, 7, &attr, &(attr_inst->er_attr_instance[7].attr_value));

	//do not disturb pin change
	attr.ptr = vfac.do_not_disturb_pin_change;
	meinfo_me_attr_copy_value(147, 8, &attr, &(attr_inst->er_attr_instance[8].attr_value));

	//emergency service number
	attr.ptr = vfac.emergency_service_number;
	meinfo_me_attr_copy_value(147, 9, &attr, &(attr_inst->er_attr_instance[9].attr_value));

	//intercom service
	attr.ptr = vfac.intercom_service;
	meinfo_me_attr_copy_value(147, 10, &attr, &(attr_inst->er_attr_instance[10].attr_value));

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_attr_group_util_get_attr_mask_by_classid(attr_inst, 147, attr_mask);
	case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:
		if ((ret = er_group_hw_voip_wrapper_voip_feature_access_codes(VOIP_HW_ACTION_SET, port_id, attr_mask, &vfac)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_voip_feature_access_codes(ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&vfac, 0x00, sizeof(vfac));
		if ((ret = er_group_hw_voip_wrapper_voip_feature_access_codes(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &vfac)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
		;
	}
	return 0;
}
