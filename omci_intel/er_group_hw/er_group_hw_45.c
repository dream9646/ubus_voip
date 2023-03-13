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
 * File    : er_group_hw_45.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "er_group_hw_util.h"
#include "hwresource.h"
#include "switch.h"
#include "batchtab.h"
#include "batchtab_cb.h"

// 45 MAC_bridge_service_profile

//45@10
int
er_group_hw_l2s_mac_aging_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int time, uni_port;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		time=(unsigned short)attr_inst->er_attr_instance[0].attr_value.data;		
		if (switch_hw_g.mac_aging_set!=NULL) {
			for (uni_port=0; uni_port<3; uni_port++)
					switch_hw_g.mac_aging_enable_set(uni_port, SWITCH_ENABLED);
			

			if (time) {
				if (switch_hw_g.mac_aging_set(time)<0)
					return -1;
				util_write_file("/proc/mac_table_aging", "aging_time %d\n", time);
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
		if (er_group_hw_l2s_mac_aging_time(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}
#if defined CONFIG_SDK_PRX126 || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
int er_group_hw_l2s_bridge_inf(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int meid = 0;
	//struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (switch_hw_g.bridge_inf_create != NULL) {
			
			meid=(unsigned short)attr_inst->er_attr_instance[0].attr_value.data;
			switch_hw_g.bridge_inf_create(meid);
			
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		meid=(unsigned short)attr_inst->er_attr_instance[0].attr_value.data;
		dbprintf(LOG_ERR, "er_group_hw_l2s_bridge_inf update meid(%d)\n",meid);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		if (switch_hw_g.bridge_inf_del != NULL) {
			
			meid=(unsigned short)attr_inst->er_attr_instance[0].attr_value.data;
			switch_hw_g.bridge_inf_del(meid);
			
		}
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}
#endif
