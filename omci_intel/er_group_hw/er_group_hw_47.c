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
 * File    : er_group_hw_47_130.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "switch.h"
#include "batchtab.h"
#include "er_group_hw_util.h"

//@47 meid , 1,2,3,4,11,12,13
int 
er_group_hw_47_entity_set(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct switch_mac_bp_config_data_upd_data upd_data;
	unsigned short meid;
	
	if (attr_inst == NULL)
		return -1;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:		
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		
		meid = me->meid;//(unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
		memset(&upd_data,0x0,sizeof(struct switch_mac_bp_config_data_upd_data));
		upd_data.bridge_id_ptr          = (unsigned short) attr_inst->er_attr_instance[1].attr_value.data;
		upd_data.port_num               = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
		upd_data.tp_type                = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;
		upd_data.tp_ptr                 = (unsigned short) attr_inst->er_attr_instance[4].attr_value.data;
		upd_data.outbound_td_ptr        = (unsigned short) attr_inst->er_attr_instance[5].attr_value.data;
		upd_data.inbound_td_ptr         = (unsigned short) attr_inst->er_attr_instance[6].attr_value.data;
		upd_data.mac_learning_depth     = (unsigned char) attr_inst->er_attr_instance[7].attr_value.data;

		if (switch_hw_g.bridge_port_config_update != NULL) {			

			switch_hw_g.bridge_port_config_update(meid,&upd_data,0);
			
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		//meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
		meid = me->meid;
		memset(&upd_data,0x0,sizeof(struct switch_mac_bp_config_data_upd_data));
		upd_data.bridge_id_ptr          = (unsigned short) attr_inst->er_attr_instance[1].attr_value.data;
		upd_data.port_num               = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
		upd_data.tp_type                = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;
		upd_data.tp_ptr                 = (unsigned short) attr_inst->er_attr_instance[4].attr_value.data;
		upd_data.outbound_td_ptr        = (unsigned short) attr_inst->er_attr_instance[5].attr_value.data;
		upd_data.inbound_td_ptr         = (unsigned short) attr_inst->er_attr_instance[6].attr_value.data;
		upd_data.mac_learning_depth     = (unsigned char) attr_inst->er_attr_instance[7].attr_value.data;

		if (switch_hw_g.bridge_port_config_update != NULL) {			

			switch_hw_g.bridge_port_config_update(meid,&upd_data,1);
			
		}
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}


