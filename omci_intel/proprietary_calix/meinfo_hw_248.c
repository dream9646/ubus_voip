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
 * Module  : proprietary_calix
 * File    : meinfo_hw_248.c
 *
 ******************************************************************/

#include <sys/time.h>
#include "meinfo_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "meinfo_hw_rf.h"

// 9.99.5 VideoAniExtension

static int 
meinfo_hw_248_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
    	
	if (util_attr_mask_get_bit(attr_mask, 1)) { // RfReturnOperState
		attr.ptr=NULL;
		attr.data=1; // always return 1 because RF return isn't supported
		meinfo_me_attr_set(me, 1, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) { // AgcEnable
    		struct rf_result_t rf_result;
		// get counters from hw
		if (meinfo_hw_rf_get_threshold_result(&rf_result)<0)
			return-1;
		
		attr.ptr=NULL;
		attr.data=rf_result.agc_en ? 1 : 0;
		meinfo_me_attr_set(me, 4, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) { // RfPowerPeak
		struct timeval tv;
		struct timezone tz;
		unsigned char i, opt_max = 0, value = 0;
		gettimeofday(&tv, &tz);
		for(i=0; i<60; i++) {
			if(tv.tv_sec - rf_opt_history[i].tv_sec <= 60) {
				if(rf_opt_history[i].opt_rx >= opt_max)
					opt_max = rf_opt_history[i].opt_rx;
			}
		}
		value = (unsigned char) opt_max;
		attr.ptr=NULL;
		attr.data=(unsigned char)value;
		meinfo_me_attr_set(me, 7, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 8)) { // RfPowerAverage
		struct timeval tv;
		struct timezone tz;
		unsigned char i, num, opt_sum = 0, value = 0;
		gettimeofday(&tv, &tz);
		for(i=0; i<60; i++) {
			if(tv.tv_sec - rf_opt_history[i].tv_sec <= 60) {
				opt_sum += rf_opt_history[i].opt_rx;
				num++;
			}
		}
		if(num) value = (unsigned char) (opt_sum / num);
		attr.ptr=NULL;
		attr.data=(unsigned char)value;
		meinfo_me_attr_set(me, 8, &attr, 0);
	}
	
	return 0;
}

static int 
meinfo_hw_248_alarm(struct me_t *me, unsigned char alarm_mask[28])
{
	if (meinfo_hw_util_get_ontstate() != 5)	// no alarm check for rf if not in O5
		return 0;
	
	util_alarm_mask_clear_bit(alarm_mask, 0); // always do not return Laser-EOL alarm because RF return isn't supported
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_248 = {
	.get_hw			= meinfo_hw_248_get,
	.alarm_hw		= meinfo_hw_248_alarm,
};

