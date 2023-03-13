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
 * File    : er_group_hw_53_141.c
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
// 141 VoIP_line_status

//53@254,141@1,141@2,141@3,141@4,141@5,141@6,141@7
int
er_group_hw_voip_line_status(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	struct voip_hw_voip_line_status_t vls;
	struct attr_value_t attr;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = attr_inst->er_attr_instance[0].attr_value.data;

	memset(&vls, 0x00, sizeof(vls));

	//voip codec used
	vls.voip_codec_used = (unsigned short) attr_inst->er_attr_instance[1].attr_value.data;

	//voip voice server status
	vls.voip_voice_server_status = (unsigned short) attr_inst->er_attr_instance[2].attr_value.data;

	//voip port session type
	vls.voip_port_session_type = (unsigned short) attr_inst->er_attr_instance[3].attr_value.data;

	//voip call 1 packet period
	vls.voip_call_1_packet_period = (unsigned short) attr_inst->er_attr_instance[4].attr_value.data;

	//voip call 2 packet period
	vls.voip_call_2_packet_period = (unsigned short) attr_inst->er_attr_instance[5].attr_value.data;

	//voip call 1 dest addr
	attr.ptr = vls.voip_call_1_dest_addr;
	meinfo_me_attr_copy_value(141, 6, &attr, &(attr_inst->er_attr_instance[6].attr_value));

	//voip call 2 dest addr
	attr.ptr = vls.voip_call_2_dest_addr;
	meinfo_me_attr_copy_value(141, 7, &attr, &(attr_inst->er_attr_instance[7].attr_value));

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_attr_group_util_get_attr_mask_by_classid(attr_inst, 141, attr_mask);
	case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:
		if ((ret = er_group_hw_voip_wrapper_voip_line_status(VOIP_HW_ACTION_SET, port_id, attr_mask, &vls)) < 0) {
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
		if (er_group_hw_voip_line_status(ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&vls, 0x00, sizeof(vls));
		if ((ret = er_group_hw_voip_wrapper_voip_line_status(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &vls)) < 0) {
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
