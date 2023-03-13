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
 * File    : er_group_hw_247.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"
#include "gpon_sw.h"

// 247 ONT3_G

//247@7, 247@8
int
er_group_hw_loopdetect_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int interval_check = attr_inst->er_attr_instance[0].attr_value.data;
	unsigned int interval_loop = attr_inst->er_attr_instance[1].attr_value.data;

	if (attr_inst == NULL)
		return -1;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (switch_hw_g.rldp_time_period_set)
			switch_hw_g.rldp_time_period_set(interval_check*1000, interval_loop*1000);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_loopdetect_time(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		if (switch_hw_g.rldp_time_period_set)
			switch_hw_g.rldp_time_period_set(2000, 2000);
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}

	return 0;
}

//247@9
int
er_group_hw_onu_password(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	char password[GPON_PASSWORD_LEN];

	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}
	
	switch (action) {
	case ER_ATTR_GROUP_HW_OP_DEL:
		// Do nothing
		break;
	case ER_ATTR_GROUP_HW_OP_ADD:
		// First time to execute, read from HW then write into MIB
		if (gpon_hw_g.onu_password_get)
			gpon_hw_g.onu_password_get(password);
		else
			memcpy(password, "0000000000", GPON_PASSWORD_LEN);	// shoudn't happen
		memcpy(attr_inst->er_attr_instance[0].attr_value.ptr, password, GPON_PASSWORD_LEN);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		// Only write into HW while there is different with previous value
		if(strcmp((char *) current_attr_inst->er_attr_instance[0].attr_value.ptr, (char *) attr_inst->er_attr_instance[0].attr_value.ptr) != 0){
			char cmd[512];
			char *s = cmd;
			int i;

			// set password in runtime mem
			if (gpon_hw_g.onu_password_set)
				gpon_hw_g.onu_password_set((char*) current_attr_inst->er_attr_instance[0].attr_value.ptr);

			// FIXME, KEVIN
			// set onu passwd in uboot env
			s+=sprintf(s, "fw_setenv onu_password ");
			for (i=0; i<GPON_PASSWORD_LEN; i++) {
				if (i>0) s+=sprintf(s, ",");
				s+=sprintf(s, "0x%02x", ((char*) current_attr_inst->er_attr_instance[0].attr_value.ptr)[i]);
			}
			util_run_by_system(cmd);
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}

	return 0;
}
