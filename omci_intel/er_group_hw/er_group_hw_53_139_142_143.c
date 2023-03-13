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
 * File    : er_group_hw_53_139_142_143.c
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
#include "omcimain.h"
#include "batchtab.h"

// 53 Physical_path_termination_point_POTS_UNI
// 139 VoIP_voice_CTP
// 142 VoIP_media_profile
// 143 RTP_profile_data

//53@254,142@15,143@1,143@2,143@3,143@4,143@5,143@6,143@7
int
er_group_hw_rtp_profile_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	unsigned char dtmf_events;
	unsigned char MAYBE_UNUSED oob_dtmf;
	struct voip_hw_rtp_profile_data_t rpd;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = (unsigned short)attr_inst->er_attr_instance[0].attr_value.data;

	memset(&rpd, 0x00, sizeof(rpd));
	//ME142, attr15: OOB_DTMF
	oob_dtmf = (unsigned char)attr_inst->er_attr_instance[1].attr_value.data;

	//local port min
	rpd.local_port_min = (unsigned short)attr_inst->er_attr_instance[2].attr_value.data;

	//local port max
	rpd.local_port_max = (unsigned short)attr_inst->er_attr_instance[3].attr_value.data;

	//dscp mark
	rpd.dscp_mark = (unsigned char)attr_inst->er_attr_instance[4].attr_value.data;

	//piggyback events
	rpd.piggyback_events = (unsigned char)attr_inst->er_attr_instance[5].attr_value.data;

	//tone events
	rpd.tone_events = (unsigned char)attr_inst->er_attr_instance[6].attr_value.data;

	//dtmf events
	dtmf_events = (unsigned char)attr_inst->er_attr_instance[7].attr_value.data;

	if ((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_CALIX)) {
		//test with OLT found dtmf_events is always 0, but maybe other project will use this method
		rpd.dtmf_events = dtmf_events;
	} else {
		rpd.dtmf_events = dtmf_events;
	}

	//cas events
	rpd.cas_events = (unsigned char)attr_inst->er_attr_instance[8].attr_value.data;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_attr_group_util_get_attr_mask_by_classid(attr_inst, 143, attr_mask);
	case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:
		if ((ret = er_group_hw_voip_wrapper_rtp_profile_data(VOIP_HW_ACTION_SET, port_id, attr_mask, &rpd)) < 0) {
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
		if (er_group_hw_rtp_profile_data(ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&rpd, 0x00, sizeof(rpd));
		if ((ret = er_group_hw_voip_wrapper_rtp_profile_data(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &rpd)) < 0) {
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
