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
 * File    : er_group_hw_65333.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"

int
er_group_hw_onu_pon_limit_speed(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int pon_logical_portid;
	unsigned int pir_us, pir_ds;
	struct switch_rate_control_config_t rate_cfg;
	int ret_val;

	if (attr_inst == NULL)
		return -1;

	pon_logical_portid = switch_get_wan_logical_portid();

	if (switch_hw_g.rate_control_get == NULL ||
	    switch_hw_g.rate_control_set == NULL) {
		dbprintf(LOG_ERR, "rate_control_get/set not supported\n");
	    	return -1;
	}

	// KBit/Second -> Byte/Second
	pir_us = (unsigned int)attr_inst->er_attr_instance[0].attr_value.data * 1000 / 8;
	pir_ds = (unsigned int)attr_inst->er_attr_instance[1].attr_value.data * 1000 / 8;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
		// config pon uplink
		rate_cfg.dir = DIR_EGRESS;
		if ((ret_val = switch_hw_g.rate_control_get(pon_logical_portid, &rate_cfg)) != 0) {
			dbprintf(LOG_ERR, "fail to get pon exgress rate, err=%d\n", ret_val);
			return -1;
		}
		if (pir_us == 0 || action == ER_ATTR_GROUP_HW_OP_DEL) {	// rate limit disabled
			rate_cfg.enable = 0;
		} else	{
			rate_cfg.enable = 1;
			rate_cfg.rate = pir_us + 1000;	// granuity is 8kbps=1000byte/s, ensure limit won't become 0
		}
		if ((ret_val = switch_hw_g.rate_control_set(pon_logical_portid, rate_cfg)) != 0) {
			dbprintf(LOG_ERR, "fail to set pon egress rate, err=%d\n", ret_val);
			return -1;
		}

		// config pon downlink
		rate_cfg.dir = DIR_INGRESS;
		if ((ret_val = switch_hw_g.rate_control_get(pon_logical_portid, &rate_cfg)) != 0) {
			dbprintf(LOG_ERR, "fail to get pon ingress rate, err=%d\n", ret_val);
			return -1;
		}
		if (pir_ds == 0 || action == ER_ATTR_GROUP_HW_OP_DEL) {	// rate limit disabled
			rate_cfg.enable = 0;
		} else	{
			rate_cfg.enable = 1;
			rate_cfg.rate = pir_ds + 1000;	// granuity is 8kbps=1000byte/s, ensure limit won't become 0
		}
		if ((ret_val = switch_hw_g.rate_control_set(pon_logical_portid, rate_cfg)) != 0) {
			dbprintf(LOG_ERR, "fail to set pon ingress rate, err=%d\n", ret_val);
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
		//allocate
		if (er_group_hw_onu_pon_limit_speed(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}

	return 0;
}

