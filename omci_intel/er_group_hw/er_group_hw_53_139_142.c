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
 * File    : er_group_hw_53_139_142.c
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
// 139 VoIP_voice_CTP
// 142 VoIP_media_profile

//53@254,142@1,142@3,142@4,142@5,142@6,142@7,142@8,142@9,142@10,142@11,142@12,142@13,142@14,142@15
int
er_group_hw_voip_media_profile(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	struct voip_hw_voip_media_profile_t vmp;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = attr_inst->er_attr_instance[0].attr_value.data;

	memset(&vmp, 0x00, sizeof(vmp));

	//fax mode
	vmp.fax_mode = (unsigned char)attr_inst->er_attr_instance[1].attr_value.data;

	//codec selection 1st order
	vmp.codec_selection_1st_order = (unsigned char)attr_inst->er_attr_instance[2].attr_value.data;

	//packet period selection 1st order
	vmp.packet_period_selection_1st_order = (unsigned char)attr_inst->er_attr_instance[3].attr_value.data;

	//silence suppression 1st order
	vmp.silence_suppression_1st_order = (unsigned char)attr_inst->er_attr_instance[4].attr_value.data;

	//codec selection 2nd order
	vmp.codec_selection_2nd_order = (unsigned char)attr_inst->er_attr_instance[5].attr_value.data;

	//packet period selection 2nd order
	vmp.packet_period_selection_2nd_order = (unsigned char)attr_inst->er_attr_instance[6].attr_value.data;

	//silence suppression 2nd order
	vmp.silence_suppression_2nd_order = (unsigned char)attr_inst->er_attr_instance[7].attr_value.data;

	//codec selection 3rd order
	vmp.codec_selection_3rd_order = (unsigned char)attr_inst->er_attr_instance[8].attr_value.data;

	//packet period selection 3rd order
	vmp.packet_period_selection_3rd_order = (unsigned char)attr_inst->er_attr_instance[9].attr_value.data;

	//silence suppression 3rd order
	vmp.silence_suppression_3rd_order = (unsigned char)attr_inst->er_attr_instance[10].attr_value.data;

	//codec selection 4th order
	vmp.codec_selection_4th_order = (unsigned char)attr_inst->er_attr_instance[11].attr_value.data;

	//packet period selection 4th order
	vmp.packet_period_selection_4th_order = (unsigned char)attr_inst->er_attr_instance[12].attr_value.data;

	//silence suppression 4th order
	vmp.silence_suppression_4th_order = (unsigned char)attr_inst->er_attr_instance[13].attr_value.data;

	//oob dtmf
	vmp.oob_dtmf = (unsigned char)attr_inst->er_attr_instance[14].attr_value.data;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_attr_group_util_get_attr_mask_by_classid(attr_inst, 142, attr_mask);
	case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:
		if ((ret = er_group_hw_voip_wrapper_voip_media_profile(VOIP_HW_ACTION_SET, port_id, attr_mask, &vmp)) < 0) {
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
		if (er_group_hw_voip_media_profile(ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&vmp, 0x00, sizeof(vmp));
		if ((ret = er_group_hw_voip_wrapper_voip_media_profile(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &vmp)) < 0) {
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
