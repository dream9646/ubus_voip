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
 * File    : er_group_hw_53_65426.c
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

// 65426 WRI_NGN_port_statics

//53@254,65426@2,65426@3,65426@4,65426@5,65426@6,65426@7,65426@8,65426@9,65426@10,65426@11,65426@12,65426@13,65426@14,65426@15
int
er_group_hw_wri_ngn_port_statics(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
#if 0 //do not set these values to voip, these values only for read
	unsigned short port_id;
	int ret;
	struct voip_hw_wri_ngn_port_statics_t wnps;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	port_id = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;

	memset(&wnps, 0x00, sizeof(wnps));

	wnps.received_rtp_packets = (unsigned int) attr_inst->er_attr_instance[1].attr_value.data;
	wnps.send_rtp_packets = (unsigned int) attr_inst->er_attr_instance[2].attr_value.data;
	wnps.received_rtp_bytes = (unsigned int) attr_inst->er_attr_instance[3].attr_value.data;
	wnps.send_rtp_bytes = (unsigned int) attr_inst->er_attr_instance[4].attr_value.data;
	wnps.rtp_jitter_buffer = (unsigned int) attr_inst->er_attr_instance[5].attr_value.data;

	wnps.latest_call_begin_time.year = util_bitmap_get_value(attr_inst->er_attr_instance[6].attr_value.ptr, 56, 0, 16);
	wnps.latest_call_begin_time.month = util_bitmap_get_value(attr_inst->er_attr_instance[6].attr_value.ptr, 56, 16, 8);
	wnps.latest_call_begin_time.day = util_bitmap_get_value(attr_inst->er_attr_instance[6].attr_value.ptr, 56, 24, 8);
	wnps.latest_call_begin_time.hour = util_bitmap_get_value(attr_inst->er_attr_instance[6].attr_value.ptr, 56, 32, 8);
	wnps.latest_call_begin_time.minute = util_bitmap_get_value(attr_inst->er_attr_instance[6].attr_value.ptr, 56, 40, 8);
	wnps.latest_call_begin_time.second = util_bitmap_get_value(attr_inst->er_attr_instance[6].attr_value.ptr, 56, 48, 8);

	wnps.latest_call_end_time.year = util_bitmap_get_value(attr_inst->er_attr_instance[7].attr_value.ptr, 56, 0, 16);
	wnps.latest_call_end_time.month = util_bitmap_get_value(attr_inst->er_attr_instance[7].attr_value.ptr, 56, 16, 8);
	wnps.latest_call_end_time.day = util_bitmap_get_value(attr_inst->er_attr_instance[7].attr_value.ptr, 56, 24, 8);
	wnps.latest_call_end_time.hour = util_bitmap_get_value(attr_inst->er_attr_instance[7].attr_value.ptr, 56, 32, 8);
	wnps.latest_call_end_time.minute = util_bitmap_get_value(attr_inst->er_attr_instance[7].attr_value.ptr, 56, 40, 8);
	wnps.latest_call_end_time.second = util_bitmap_get_value(attr_inst->er_attr_instance[7].attr_value.ptr, 56, 48, 8);

	wnps.last_call_duration = (unsigned int) attr_inst->er_attr_instance[8].attr_value.data;
	wnps.total_call_times = (unsigned int) attr_inst->er_attr_instance[9].attr_value.data;
	wnps.total_call_duration = (unsigned int) attr_inst->er_attr_instance[10].attr_value.data;
	wnps.upstream_rates = (unsigned int) attr_inst->er_attr_instance[11].attr_value.data;
	wnps.downstream_rates = (unsigned int) attr_inst->er_attr_instance[12].attr_value.data;
	wnps.avg_network_delay = (unsigned int) attr_inst->er_attr_instance[13].attr_value.data;
	wnps.packet_loss = (unsigned int) attr_inst->er_attr_instance[14].attr_value.data;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((ret = er_group_hw_voip_wrapper_wri_ngn_port_statics(VOIP_HW_ACTION_SET, port_id, attr_mask, &wnps)) < 0)
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
		if (er_group_hw_wri_ngn_port_statics(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&wnps, 0x00, sizeof(wnps));
		if ((ret = er_group_hw_voip_wrapper_wri_ngn_port_statics(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &wnps)) < 0)
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

