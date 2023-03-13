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
 * Module  : proprietary_calix
 * File    : er_group_hw_11_242.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "meinfo_hw_util.h"
#include "er_group.h"
#include "cli.h"
#include "hwresource.h"
#include "switch.h"

int
er_group_hw_VideoTxMode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{    
	// not supported
	return 0;
}

int
er_group_hw_IfCountReset_242(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char enable = attr_inst->er_attr_instance[0].attr_value.data;
	int ret = 0;

	switch (action) {
		case ER_ATTR_GROUP_HW_OP_ADD:
			dbprintf(LOG_DEBUG,"er_group_hw_IfCountReset_242 enable - %d\n", enable);
			if (!enable)
			{
				if( (ret=switch_hw_g.pm_refresh(!enable)) !=0 ) {
					dbprintf(LOG_ERR, "ret = %d\n", ret);
				}
				struct attr_value_t attr;
				attr.data = 0;
				meinfo_me_attr_set(me, 2, &attr, 0);
			}
			break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_IfCountReset_242(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}
	
	return 0;
}

int
er_group_hw_ForceOperMode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned short meid = attr_inst->er_attr_instance[0].attr_value.data;
	unsigned char mode = attr_inst->er_attr_instance[1].attr_value.data;
	struct switch_port_mac_config_t port_mac_config;
	struct switch_port_info_t sw_port_info;
	int ret = 0;
	struct me_t *meptr = NULL;
	
	if ((meptr = meinfo_me_get_by_meid(11, meid)) == NULL) {
		return -1;
	}
	
	// get value from HW depend on different switch configure
	memset( &port_mac_config, 0, sizeof(struct switch_port_mac_config_t));
	if (switch_get_port_info_by_me(meptr, &sw_port_info) == 0) {
		if ((switch_get_uni_logical_portmask() & (1<<sw_port_info.logical_port_id))) {
			if( (ret=switch_hw_g.port_config_get(sw_port_info.logical_port_id, &port_mac_config)) !=0 ) {
				dbprintf(LOG_ERR, "ret = %d\n", ret);
				return -1;
			}
		}
	} else {
		dbprintf(LOG_ERR, "Can't get hw information in switch\n");
		return -1;
	}
        
	switch (action) {
		case ER_ATTR_GROUP_HW_OP_ADD:
			dbprintf(LOG_WARNING,"er_group_hw_ForceOperMode port=%d mode=%d:%d\n", sw_port_info.logical_port_id, port_mac_config.force_mode, mode);
			if(port_mac_config.force_mode != mode) {
				port_mac_config.force_mode = mode;
				if( (ret=switch_hw_g.port_config_set(sw_port_info.logical_port_id, port_mac_config)) !=0 ) {
					dbprintf(LOG_ERR, "Can't upate force mode, ret = %d\n", ret);
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
			if (er_group_hw_ForceOperMode(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
				er_attr_group_util_release_attrinst(current_attr_inst);
				dbprintf(LOG_ERR, "fail to update for adding\n");
				return -1;
			}
			er_attr_group_util_release_attrinst(current_attr_inst);
			break;
		case ER_ATTR_GROUP_HW_OP_DEL:
			break;
		default:
			dbprintf(LOG_ERR,"Unknown operator\n");
	}
	
	return 0;
}

