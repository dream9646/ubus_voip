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
 * File    : er_group_hw_65307.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"

// 9.99.22 65307 ONU_switchport_config 

// 65307@10, 65307@11, 65307@12, 65307@13
int
er_group_hw_switchport_upstream_rate_control(int action, struct
					     er_attr_group_instance_t
					     *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char rate_control_enable;
	//unsigned int cir;
	unsigned int pir;
	unsigned char logical_port_id;

	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	// FIXME, KEVIN, alu 65307 meid is uni port id
	logical_port_id = attr_inst->er_attr_instance[0].meid;
	rate_control_enable = attr_inst->er_attr_instance[0].attr_value.data;
	//cir = attr_inst->er_attr_instance[1].attr_value.data*125;	// kbits/s -> byte/s
	//pir = attr_inst->er_attr_instance[2].attr_value.data*125;	//workaround for FH
	pir = attr_inst->er_attr_instance[1].attr_value.data*125;	//workaround for FH

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
		if (action==ER_ATTR_GROUP_HW_OP_DEL)
			rate_control_enable=0;					
		if (switch_hw_g.rate_control_set) {
			struct switch_rate_control_config_t rate_control_cfg = { 
				.enable=rate_control_enable,
				.dir = 0, 	// 0 means uni ingress
				.rate = pir, 	// bytes/s
				.has_ifg = 0, 
				.fc_en = 1,
			};
			if (switch_hw_g.rate_control_set(logical_port_id, rate_control_cfg) <0)  {
				dbprintf(LOG_ERR, "fail to set ext bridge port inbound td\n");
				return -1;
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_switchport_upstream_rate_control(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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

// 65307@14, 65307@15, 65307@16
int
er_group_hw_switchport_downstream_rate_control(int action, struct
					     er_attr_group_instance_t
					     *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char rate_control_enable;
	//unsigned int cir;
	unsigned int pir;
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char logical_port_id;

	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	// FIXME, KEVIN, alu 65307 meid is uni port id
	logical_port_id = attr_inst->er_attr_instance[0].meid;
	rate_control_enable = attr_inst->er_attr_instance[0].attr_value.data;
	//cir = attr_inst->er_attr_instance[1].attr_value.data*125;	// kbits/s -> byte/s
	//pir = attr_inst->er_attr_instance[2].attr_value.data*125;	//workaround for FH
	pir = attr_inst->er_attr_instance[1].attr_value.data*125;	//workaround for FH

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
		if (action==ER_ATTR_GROUP_HW_OP_DEL)
			rate_control_enable=0;	
		if (switch_hw_g.rate_control_set) {
			struct switch_rate_control_config_t rate_control_cfg = { 
				.enable=rate_control_enable,
				.dir = 0, 	// 1 means uni egress
				.rate = pir, 	// bytes/s
				.has_ifg = 0, 
				.fc_en = 1,
			};
			if (switch_hw_g.rate_control_set(logical_port_id, rate_control_cfg) <0)  {
				dbprintf(LOG_ERR, "fail to set ext bridge port inbound td\n");
				return -1;
			}
		}				
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_switchport_downstream_rate_control(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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
