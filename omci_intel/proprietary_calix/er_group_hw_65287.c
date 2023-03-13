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
 * File    : er_group_hw_65287.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "cpuport.h"
#include "cfm_core.h"
#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_switch.h"
#include "er_group_hw.h"
#include "omciutil_misc.h"
#include "gpon_sw.h"
//9.99.25 65287 OamCfg

// 65287@1,2,3,4,5,6,7,8,9,10,11,12,13,14 [0-13]
int 
er_group_hw_oam_config(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	cfm_config_t *cfm_config = NULL, *cfm_config_safe = NULL;

	if (omci_env_g.cfm_enable == 0) {
		dbprintf(LOG_INFO,"cfm_enable==0\n");
		return 0;
	}
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}
	
	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		// Fill chassis_id as GPON serial_number
		if(cfm_task_loop_g && gpon_hw_g.onu_serial_number_get) {
			char vendor_id[4];
			unsigned int serial_number = 0;
			vendor_id[0] = vendor_id[1] = vendor_id[2] = vendor_id[3] = '?';
			gpon_hw_g.onu_serial_number_get(vendor_id, &serial_number);
			strncpy(cfm_chassis_g.chassis_id, vendor_id, sizeof(vendor_id));
			sprintf(cfm_chassis_g.chassis_id+sizeof(vendor_id), "%08X", serial_number);
			cfm_chassis_g.chassis_id_subtype = CHASSIS_ID_SUBTYPE_LOCAL;
			switch ((unsigned char) attr_inst->er_attr_instance[1].attr_value.data) {
				case 1: // chassis
				case 3: // both
					cfm_chassis_g.chassis_id_length = 12;
					break;
				default: // none, management
					cfm_chassis_g.chassis_id_length = 0;
					break;
			}
		}
		if(cfm_task_loop_g && !list_empty(&cfm_config_list)) {
			list_for_each_entry_safe(cfm_config, cfm_config_safe, &cfm_config_list, node) {
				switch ((unsigned char) attr_inst->er_attr_instance[1].attr_value.data) {
					case 0:
						cfm_config->md_sender_id_permission = MA_SENDER_ID_PERMISSION_NONE;
						cfm_config->ma_sender_id_permission = MA_SENDER_ID_PERMISSION_NONE;
						break;
					case 1:
						cfm_config->md_sender_id_permission = MA_SENDER_ID_PERMISSION_CHASSIS;
						cfm_config->ma_sender_id_permission = MA_SENDER_ID_PERMISSION_CHASSIS;
						break;
					case 2:
						cfm_config->md_sender_id_permission = MA_SENDER_ID_PERMISSION_MANAGE;
						cfm_config->ma_sender_id_permission = MA_SENDER_ID_PERMISSION_MANAGE;
						break;
					case 3:
						cfm_config->md_sender_id_permission = MA_SENDER_ID_PERMISSION_CHASSIS_MANAGE;
						cfm_config->ma_sender_id_permission = MA_SENDER_ID_PERMISSION_CHASSIS_MANAGE;
						break;
				}
				cfm_config->admin_state = (unsigned char) attr_inst->er_attr_instance[0].attr_value.data ? 0 : 1;
				cfm_config->payload_format = ((unsigned char) cfm_config->ma_sender_id_permission<=1) ? 0 : 1;
				cfm_config->send_cc_senderid = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
				cfm_config->send_cc_port_status = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;
				cfm_config->send_cc_interface_status = (unsigned char) attr_inst->er_attr_instance[4].attr_value.data;
				cfm_config->send_cc_data_status = (unsigned char) attr_inst->er_attr_instance[5].attr_value.data;
				cfm_config->send_lb_senderid = (unsigned char) attr_inst->er_attr_instance[6].attr_value.data;
				cfm_config->send_lb_port_status = (unsigned char) attr_inst->er_attr_instance[7].attr_value.data;
				cfm_config->send_lb_interface_status = (unsigned char) attr_inst->er_attr_instance[8].attr_value.data;
				cfm_config->send_lb_data_status = (unsigned char) attr_inst->er_attr_instance[9].attr_value.data;
				cfm_config->send_lt_senderid = (unsigned char) attr_inst->er_attr_instance[10].attr_value.data;
				cfm_config->send_lt_port_status = (unsigned char) attr_inst->er_attr_instance[11].attr_value.data;
				cfm_config->send_lt_interface_status = (unsigned char) attr_inst->er_attr_instance[12].attr_value.data;
				cfm_config->send_lt_data_status = (unsigned char) attr_inst->er_attr_instance[13].attr_value.data;
			}
		}
		break;
		
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
		
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_oam_config(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			//er_attr_group_util_release_attrinst(attr_inst);
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			//return -1;
		}
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_oam_config(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;

	default:
       		dbprintf(LOG_ERR,"Unknown operator\n");
		break;
	}
	return 0;
}
