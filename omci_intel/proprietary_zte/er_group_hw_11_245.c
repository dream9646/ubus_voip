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
 * Module  : proprietary_zte
 * File    : er_group_hw_11_245.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"

static int
er_group_hw_uni_rate(int enable, int port_id, int dir, unsigned int cir)
{
	if(switch_hw_g.rate_control_set) {
		struct switch_rate_control_config_t rate_control_cfg = {
			.enable = enable,
			.dir = dir,       // 0 means ingress, 1 means egress
			.rate = (enable) ? cir*125 : 0x7FFFC00,    // kbps->bytes/s
			.has_ifg = (enable) ? 1 : 0,
			.fc_en = 1,
		};
		switch_hw_g.rate_control_set(port_id, rate_control_cfg);
	}
	
	return 0;
}

//245@meid,245@1,245@6
int
er_group_hw_uni_traffic_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	struct me_t *meptr = NULL, *iethuni_me = NULL;
	unsigned char dir;
	unsigned short meid, port_id;
	unsigned int cir = 0;
	
	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}
	
	dir = ((unsigned short) attr_inst->er_attr_instance[0].attr_value.data) >> 15;
	meid = (unsigned short) attr_inst->er_attr_instance[1].attr_value.data;
	if ((meptr = meinfo_me_get_by_meid(11, meid)) == NULL) {
		dbprintf(LOG_ERR, "could not get bridge port me, classid=11, meid=%u\n", meid);
		return -1;
	}
	iethuni_me = hwresource_public2private(meptr);
	port_id = meinfo_util_me_attr_data(iethuni_me, 4);
	cir = attr_inst->er_attr_instance[2].attr_value.data;
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_group_hw_uni_rate(1, port_id, dir, cir);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		er_group_hw_uni_rate(0, port_id, dir, cir);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_uni_traffic_limit(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0) {
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_uni_traffic_limit(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}
	return 0;
}
