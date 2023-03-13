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

// 53 Physical_path_termination_point_POTS_UNI

//53@254,53@1,53@3,53@4,53@5,53@6,53@7,53@8,53@9,53@10,53@11
int
er_group_hw_physical_path_termination_point_pots_uni(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	struct voip_hw_physical_path_termination_point_pots_uni_t pptppu;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = attr_inst->er_attr_instance[0].attr_value.data;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		memset(&pptppu, 0x00, sizeof(pptppu));
		pptppu.administrative_state = (unsigned char) current_attr_inst->er_attr_instance[1].attr_value.data;
		pptppu.arc = (unsigned char) current_attr_inst->er_attr_instance[2].attr_value.data;
		pptppu.arc_interval = (unsigned char) current_attr_inst->er_attr_instance[3].attr_value.data;
		pptppu.impedance = (unsigned char) current_attr_inst->er_attr_instance[4].attr_value.data;
		pptppu.transmission_path = (unsigned char) current_attr_inst->er_attr_instance[5].attr_value.data;
		pptppu.rx_gain = (char) current_attr_inst->er_attr_instance[6].attr_value.data;
		pptppu.tx_gain = (char) current_attr_inst->er_attr_instance[7].attr_value.data;
		pptppu.operational_state = (unsigned char) current_attr_inst->er_attr_instance[8].attr_value.data;
		pptppu.hook_state = (unsigned char) current_attr_inst->er_attr_instance[9].attr_value.data;
		pptppu.pots_holdover_time = (unsigned short) current_attr_inst->er_attr_instance[10].attr_value.data;
		if ((ret = er_group_hw_voip_wrapper_physical_path_termination_point_pots_uni(VOIP_HW_ACTION_SET, port_id, attr_mask, &pptppu)) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&pptppu, 0x00, sizeof(pptppu));
		if ((ret = er_group_hw_voip_wrapper_physical_path_termination_point_pots_uni(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &pptppu)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

