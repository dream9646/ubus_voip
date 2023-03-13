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
 * File    : er_group_hw_65285.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "switch.h"

static int
er_group_hw_get_logical_portid_by_meid(unsigned short meid, unsigned int *portid) 
{
	// FIXME, KEVIN
	// the meid for 65285 is 0..3 which happens to be the logical_port_id of uni0..3
	*portid = meid;
	return 0;
}


int
er_group_hw_storm_rate_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2], unsigned char storm_type)
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int portid, rate, ifg_include;
	unsigned char enable;

	if (attr_inst == NULL)
		return -1;

	if (er_group_hw_get_logical_portid_by_meid(me->meid, &portid) != 0)
		return -1;

	if (switch_hw_g.rate_storm_set == NULL) {
		dbprintf(LOG_ERR, "not support\n");
		return -1;
	}

	enable = (unsigned char)attr_inst->er_attr_instance[0].attr_value.data;
	rate = (unsigned int)attr_inst->er_attr_instance[1].attr_value.data;
	ifg_include = 0;

	if (enable) { // enable
		if ((rate < 8)||(rate > 1048568)) { // Kbps (range: 1~131071 KBbs)
			dbprintf(LOG_ERR, "rate is out of range\n");
			return -1;
		}	
	}
	else
		rate = 0;  // disable

	// set to hw by portid and storm_type
	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		switch_hw_g.rate_storm_set(portid, storm_type, rate*125, ifg_include); // Kbps->bytes/s
		break;		
	case ER_ATTR_GROUP_HW_OP_UPDATE:		
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_storm_rate_limit(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask, storm_type) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		switch_hw_g.rate_storm_set(portid, storm_type, rate*125, ifg_include); // Kbps->bytes/s
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}

int
er_group_hw_broadcast_rate_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]) {
	return er_group_hw_storm_rate_limit(action, attr_inst, me, attr_mask, STORM_BROADCAST);
}

int
er_group_hw_multicast_rate_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]) {
	return er_group_hw_storm_rate_limit(action, attr_inst, me, attr_mask, STORM_UNKNOWN_MULTICAST);
}

int
er_group_hw_unknown_rate_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]) {
	int ret1, ret2;
	ret1=er_group_hw_storm_rate_limit(action, attr_inst, me, attr_mask, STORM_UNKNOWN_UNICAST);
	ret2=er_group_hw_storm_rate_limit(action, attr_inst, me, attr_mask, STORM_UNKNOWN_MULTICAST);

	if(ret1 != 0) {
		dbprintf(LOG_ERR,"er_group_hw_storm_rate_limit set STORM_UNKNOWN_UNICAST error\n");
		return ret1;
	}
	if(ret2 != 0) {
		dbprintf(LOG_ERR,"er_group_hw_storm_rate_limit set STORM_UNKNOWN_MULTICAST error\n");
		return ret2;
	}
	return 0;
}
