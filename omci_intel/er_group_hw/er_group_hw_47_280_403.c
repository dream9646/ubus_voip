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
 * File    : er_group_hw_47_280_403.c
 *
 ******************************************************************/

#include "util.h"
#include "er_group.h"
#include "switch.h"
#include "batchtab.h"

// 47 MAC_bridge_port_configuration_data
// 280 GEM_traffic_descriptor
// 403 Inbound pseudo obj

//47@3,280@1,280@2,280@3,280@4,280@5,280@6,280@7,280@8
int
er_group_hw_mac_traffic_control_td_inbound(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst=NULL;
	unsigned int tp_type = 0;
	
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}
	
	tp_type = attr_inst->er_attr_instance[0].attr_value.data;
	
	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(tp_type == 11) { // VEIP
			if(omci_env_g.bundle_rate_support) {
				if(switch_hw_g.rate_control_set) {
					unsigned int cir=0, pir=0;
					struct switch_rate_control_config_t rate_control_cfg = {
						.enable = 1,
						.dir = 1,
						.rate = 0x7FFFC00,    // bytes/s
						.has_ifg = 0,
						.fc_en = 1,
					};
					cir=attr_inst->er_attr_instance[1].attr_value.data;//CIR
					pir=attr_inst->er_attr_instance[2].attr_value.data;//PIR
					if( pir > cir )
						rate_control_cfg.rate=pir;
					else
						rate_control_cfg.rate=cir;
					if(!rate_control_cfg.rate) rate_control_cfg.enable = 0;
					switch_hw_g.rate_control_set(switch_get_wan_logical_portid(), rate_control_cfg);
				}
			}
		} else {
			batchtab_omci_update("td");
			batchtab_omci_update("tm");
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		if(tp_type == 11) { // VEIP
			if(omci_env_g.bundle_rate_support) {
				if(switch_hw_g.rate_control_set) {
					struct switch_rate_control_config_t rate_control_cfg = {
						.enable = 0,
						.dir = 1,
						.rate = 0x7FFFC00,    // bytes/s
						.has_ifg = 0,
						.fc_en = 0,
					};
					switch_hw_g.rate_control_set(switch_get_wan_logical_portid(), rate_control_cfg);
				}
			}
		} else {
			batchtab_omci_update("td");
			batchtab_omci_update("tm");
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if(tp_type == 11) { // VEIP
			//delete
			if (er_group_hw_mac_traffic_control_td_inbound(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
			{
				dbprintf(LOG_ERR, "fail to update for deleting\n");
				return -1;
			}
			//get current value 
			if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
			{
				dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
				return -1;
			}
			//add
			if (er_group_hw_mac_traffic_control_td_inbound(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
			{
				er_attr_group_util_release_attrinst(current_attr_inst);
				dbprintf(LOG_ERR, "fail to update for adding\n");
				return -1;
			}
			er_attr_group_util_release_attrinst(current_attr_inst);
		} else {
			batchtab_omci_update("td");
			batchtab_omci_update("tm");
		}
		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}

