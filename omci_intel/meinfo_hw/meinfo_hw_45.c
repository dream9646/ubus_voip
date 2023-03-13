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
 * Module  : meinfo_hw
 * File    : meinfo_hw_46.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"

/*
class id: 45 
name: MAC_bridge_service_profile
section: 9.3.1
*/

static int 
meinfo_hw_45_get(struct me_t *me, unsigned char attr_mask[2])
{

	if (util_attr_mask_get_bit(attr_mask, 10)) {	// attr10: Dynamic filtering age time, aka: the mactab entry age
		unsigned int time;
		if (switch_hw_g.mac_aging_set && switch_hw_g.mac_aging_get(&time) >=0) {
			struct attr_value_t attr;
			attr.data = time;
			attr.ptr = 0;
			meinfo_me_attr_set(me, 10, &attr, 0);	// check_avc==0 means none update avc_change_mask
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_45 = {
	.get_hw		= meinfo_hw_45_get,
};
