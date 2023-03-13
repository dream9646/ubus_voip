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
 * File    : meinfo_hw_82.c
 *
 ******************************************************************/

#include "util.h"
#include "meinfo_hw_util.h"
#include "meinfo_hw_rf.h"

// 9.13.1 Pptp_video_UNI

static float rf_tx_previous = 0xffff;
static float rf_tx_previous2 = 0xffff;

static int 
meinfo_hw_82_get(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		struct attr_value_t attr;
	    	struct rf_result_t rf_result;
    		
		// get counters from hw
		if (meinfo_hw_rf_get_threshold_result(&rf_result)<0)
			return-1;
		
		attr.ptr=NULL;
		switch (rf_result.rf_en) {
			case 0:
				attr.data=1;
				meinfo_me_attr_set(me, 2, &attr, 1);
				break;
			case 1:
				attr.data=0;
				meinfo_me_attr_set(me, 2, &attr, 1);
				break;
			default:
				// do nothing
				break;
		}
	}
	
	return 0;
}

static int
meinfo_hw_82_alarm_check_power(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me, 
				struct rf_result_t *rf_result)
{
	int alarm_is_on=0;

	if (!util_alarm_mask_get_bit(miptr->alarm_mask, 0))
		return 0;
	else {
		if (rf_result->rf_en == 0) rf_result->rf_tx = 0; // clear alarm when rf_en is disabled
		// avoid false alarm, report alarm when continuous 3 los
		if (rf_result->rf_tx == 0xffff && rf_tx_previous == 0xffff && rf_tx_previous2 == 0xffff) {
			dbprintf(LOG_WARNING, "alarm: los(%.3f)\n", rf_result->rf_tx);
			util_alarm_mask_set_bit(alarm_mask, 0);
			alarm_is_on=1;
		} else if (rf_result->rf_tx != 0xffff && rf_tx_previous != 0xffff && rf_tx_previous2 != 0xffff) {
			util_alarm_mask_clear_bit(alarm_mask, 0);
		}	
	}

	rf_tx_previous = rf_result->rf_tx;
	rf_tx_previous2 = rf_tx_previous;
	return alarm_is_on;
}

static int 
meinfo_hw_82_alarm(struct me_t *me, unsigned char alarm_mask[28])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	struct rf_result_t rf_result;
	
	if (meinfo_hw_util_get_ontstate() != 5)	// no alarm check for rf if not in O5
		return 0;
	
	// get counters from hw
	if (meinfo_hw_rf_get_threshold_result(&rf_result)<0)
   		return -1;

	// check rx optical level
	meinfo_hw_82_alarm_check_power(miptr, alarm_mask, me, &rf_result);
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_82 = {
	.get_hw			= meinfo_hw_82_get,
	.alarm_hw		= meinfo_hw_82_alarm,
};

