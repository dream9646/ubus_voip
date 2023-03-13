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
 * Module  : proprietary_alu
 * File    : meinfo_hw_65304.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"

static int 
meinfo_hw_65304_get(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 9)) {	// Current_Time
		struct attr_value_t attr;
		struct timeval tv;
		
		// get current time
		gettimeofday(&tv, NULL);
		attr.data = tv.tv_sec;
		meinfo_me_attr_set(me, 9, &attr, 0);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 10)) {	// Timezone_Offset
		struct attr_value_t attr;
		struct timeval tv;
		struct timezone tz;
		
		// get current timezone
		gettimeofday(&tv, &tz);
		attr.data = tz.tz_minuteswest;
		meinfo_me_attr_set(me, 10, &attr, 0);
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_alu_65304 = {
	.get_hw		= meinfo_hw_65304_get
};


