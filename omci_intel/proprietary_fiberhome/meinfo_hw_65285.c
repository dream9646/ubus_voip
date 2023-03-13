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
 * File    : meinfo_hw_65285.c
 *
 ******************************************************************/

#include <stdlib.h>

#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"

//classid 65285 Switch port packet control

static int 
meinfo_hw_65285_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	unsigned int portid;

	if (me == NULL) {
		dbprintf(LOG_ERR, "invalid me\n");
		return -1;
	}

	portid = me->meid; // meid is portid 
	if (switch_hw_g.rate_storm_get == NULL) {
		dbprintf(LOG_ERR, "not support\n");
		return -1;
	}		
	
	if (util_attr_mask_get_bit(attr_mask, 1) || util_attr_mask_get_bit(attr_mask, 2)) {
		unsigned int rate, ifg_include;
		unsigned char enable;
		switch_hw_g.rate_storm_get(portid, STORM_BROADCAST, &rate, &ifg_include);
		enable = (rate>0 && rate<0xFFFF8)? 1 : 0;
		rate = (enable) ? rate/125 : 0; // bytes/s -> Kbps
		attr.data = enable;
		meinfo_me_attr_set(me, 1, &attr, 0); // 1: broadcast_enable
		attr.data = (unsigned int)rate;
		meinfo_me_attr_set(me, 2, &attr, 0); // 2: broadcast_rate_limit		
	}

	if (util_attr_mask_get_bit(attr_mask, 3) || util_attr_mask_get_bit(attr_mask, 4)) {
		unsigned int rate, ifg_include;
		unsigned char enable;		
		switch_hw_g.rate_storm_get(portid, STORM_UNKNOWN_MULTICAST, &rate, &ifg_include);
		enable = (rate>0 && rate<0xFFFF8)? 1 : 0;
		rate = (enable) ? rate/125 : 0; // bytes/s -> Kbps
		attr.data = enable;
		meinfo_me_attr_set(me, 3, &attr, 0); // 3: multicast_enable
		attr.data = (unsigned int)rate;
		meinfo_me_attr_set(me, 4, &attr, 0); // 4: multicast_rate_limit		
	}

	if (util_attr_mask_get_bit(attr_mask, 5) || util_attr_mask_get_bit(attr_mask, 6)) {
		unsigned int rate, ifg_include;
		unsigned char enable;
		switch_hw_g.rate_storm_get(portid, STORM_UNKNOWN, &rate, &ifg_include);
		enable = (rate>0 && rate<0xFFFF8)? 1 : 0;
		rate = (enable) ? rate/125 : 0; // bytes/s -> Kbps
		attr.data = enable;
		meinfo_me_attr_set(me, 5, &attr, 0); // 5: unknown_enable
		attr.data = (unsigned int)rate;
		meinfo_me_attr_set(me, 6, &attr, 0); // 6: unknown_rate_limit		
	}
	
	return 0;		
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65285 = {
	.get_hw		= meinfo_hw_65285_get,
};

