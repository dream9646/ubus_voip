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
 * File    : er_group_hw_65425.c
 *
 ******************************************************************/

#include <stdio.h>

#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "voip_hw.h"
#include "er_group_hw.h"
#include "proprietary_fiberhome.h"

// 65425 WRI_digit_map_match_mode

//65425@1,65425@2,65425@3,65425@4,65425@5,65425@6,65425@7,65425@8,65425@9,65425@10,65425@11,65425@12,65425@13,65425@14,65425@15,65425@16
int
er_group_hw_wri_ngn_resource_info(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
#if 0 //do not set these values to voip, these values only for read
	unsigned short port_id;
	int ret;
	struct voip_hw_wri_ngn_resource_info_t wnri;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	port_id = VOIP_HW_GLOBAL_PORT_DEF;

	memset(&wnri, 0x00, sizeof(wnri));

	wnri.received_ngn_req_packets = (unsigned int) attr_inst->er_attr_instance[0].attr_value.data;
	wnri.send_ngn_req_packets = (unsigned int) attr_inst->er_attr_instance[1].attr_value.data;
	wnri.received_ngn_resp_packets = (unsigned int) attr_inst->er_attr_instance[2].attr_value.data;
	wnri.send_ngn_resp_packets = (unsigned int) attr_inst->er_attr_instance[3].attr_value.data;
	wnri.received_rtp_packets = (unsigned int) attr_inst->er_attr_instance[4].attr_value.data;
	wnri.send_rtp_packets = (unsigned int) attr_inst->er_attr_instance[5].attr_value.data;
	wnri.received_rtp_bytes = (unsigned int) attr_inst->er_attr_instance[6].attr_value.data;
	wnri.send_rtp_bytes = (unsigned int) attr_inst->er_attr_instance[7].attr_value.data;
	wnri.packet_loss = (unsigned int) attr_inst->er_attr_instance[8].attr_value.data;
	wnri.network_avg_delay = (unsigned int) attr_inst->er_attr_instance[9].attr_value.data;
	wnri.rtp_avg_jitter_buffer = (unsigned int) attr_inst->er_attr_instance[10].attr_value.data;
	wnri.bandwidth_usage = (unsigned int) attr_inst->er_attr_instance[11].attr_value.data;
	wnri.lost_sig_packets = (unsigned int) attr_inst->er_attr_instance[12].attr_value.data;
	wnri.retransmit_packets = (unsigned int) attr_inst->er_attr_instance[13].attr_value.data;
	wnri.wrong_sig_packets = (unsigned int) attr_inst->er_attr_instance[14].attr_value.data;
	wnri.unknown_sig_packets = (unsigned int) attr_inst->er_attr_instance[15].attr_value.data;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((ret = er_group_hw_voip_wrapper_wri_ngn_resource_info(VOIP_HW_ACTION_SET, port_id, attr_mask, &wnri)) < 0)
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
		if (er_group_hw_wri_ngn_resource_info(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&wnri, 0x00, sizeof(wnri));
		if ((ret = er_group_hw_voip_wrapper_wri_ngn_resource_info(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &wnri)) < 0)
		{
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
#endif

	return 0;
}

