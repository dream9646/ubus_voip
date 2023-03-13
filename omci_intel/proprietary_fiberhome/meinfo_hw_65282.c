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
 * File    : meinfo_hw_65282.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "er_group.h"
#include "util.h"
#include "hwresource.h"
#include "switch.h"


static int 
meinfo_hw_65282_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	unsigned int enable = 0;
	unsigned int period = 0, period_loop = 0;
	unsigned int portmask= 0;

	if (me == NULL) {
		dbprintf(LOG_ERR, "invalid me\n");
		return -1;
	}

	if (util_attr_mask_get_bit(attr_mask, 1)) {	// attr1: loop check status

		//todo need to change
		if (switch_hw_g.rldp_enable_get == NULL
				|| switch_hw_g.rldp_portmask_get == NULL) 
			return -1;
		switch_hw_g.rldp_enable_get(&enable);
		switch_hw_g.rldp_portmask_get(&portmask);
		
		if ( enable == 1 && ( portmask & (1 << me->meid) ))
			attr.data = 2;
		else
			attr.data = 1;

		attr.ptr=NULL;
		meinfo_me_attr_set(me, 1, &attr, 0);	// check_avc==0 means none update avc_change_mask
	}

	if (util_attr_mask_get_bit(attr_mask, 2)) {	// attr1: Designated_bridge_root_cost_port
		if (switch_hw_g.rldp_time_period_get == NULL) 
			return -1;
		switch_hw_g.rldp_time_period_get(&period, &period_loop);

		attr.data=period/1000;
		attr.ptr=NULL;
		meinfo_me_attr_set(me, 2, &attr, 0);	// check_avc==0 means none update avc_change_mask
	}

	return 0;
}


struct meinfo_hardware_t meinfo_hw_fiberhome_65282 = {
	.get_hw		= meinfo_hw_65282_get
};
