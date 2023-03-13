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
 * File    : meinfo_hw_90.c
 *
 ******************************************************************/

#include <string.h>
#include <sys/time.h>
#include "meinfo_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "meinfo_hw_rf.h"
#include "meinfo_hw_anig.h"

// 9.13.2 Pptp_video_ANI

struct rf_opt_history_t rf_opt_history[60];
static int rf_opt_history_index = 0;
static float opt_rx_previous = 0xffff;
static float opt_rx_previous2 = 0xffff;

static int 
meinfo_hw_90_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	unsigned char value = 0;
    	struct rf_result_t rf_result;

	if (util_attr_mask_get_bit(attr_mask, 2) ||
	    util_attr_mask_get_bit(attr_mask, 8) ||
	    util_attr_mask_get_bit(attr_mask, 14)) {
		// get counters from hw
		if (meinfo_hw_rf_get_threshold_result(&rf_result)<0)
			return-1;
		
		if (util_attr_mask_get_bit(attr_mask, 2)) { // Operational_state
			attr.ptr=NULL;
			switch (rf_result.opt_en) {
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
		
		if (util_attr_mask_get_bit(attr_mask, 8)) { // Optical_signal_level
			if(rf_result.opt_rx != 0xffff) {
				if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0)
					value=rf_result.opt_rx/0.2;	// unit 0.2dbm
				else
					value=rf_result.opt_rx-30;	// unit dbw
			}
			attr.ptr=NULL;
			attr.data=(unsigned char)value;
			meinfo_me_attr_set(me, 8, &attr, 0);
		}
		
		if (util_attr_mask_get_bit(attr_mask, 14)) { // AGC_setting
			if(rf_result.agc != 0xffff)
				value=rf_result.agc;	
			attr.ptr=NULL;
			attr.data=(unsigned char)value;
			meinfo_me_attr_set(me, 14, &attr, 0);
		}
	}
	
	return 0;
}

static int
meinfo_hw_90_alarm_check_power(struct meinfo_t *miptr, unsigned char alarm_mask[], struct me_t *me, 
				struct rf_result_t *rf_result)
{
	int alarm_is_on=0;
	char rx_min, rx_max;
	float rx_min_val, rx_max_val;

	// calculate past 60 seconds statistics
	if(rf_result->opt_rx != 0xffff) {
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv, &tz);
		rf_opt_history[rf_opt_history_index%60].opt_rx = rf_result->opt_rx;
		rf_opt_history[(rf_opt_history_index++)%60].tv_sec = tv.tv_sec;
	}
	
	if (!util_alarm_mask_get_bit(miptr->alarm_mask, 0) &&
	    !util_alarm_mask_get_bit(miptr->alarm_mask, 1) &&
	    !util_alarm_mask_get_bit(miptr->alarm_mask, 2))
		return 0;

	// we sync the unit to dbm for all counters
	// alarm will be triggered only if (value>max || value<min) && max>=min
	// some hw may has wrong max/min, we don't trigger alarm for these device

	rx_min=(char)meinfo_util_me_attr_data(me, 15);
	rx_min_val=rx_min*(0.1);
	rx_max=(char)meinfo_util_me_attr_data(me, 16);	
	rx_max_val=rx_max*(0.1);
	
	dbprintf(LOG_INFO, "opt_rx: hw=%.3fdbm, min=%d(%.3fdbm), max=%d(%.3fdbm)\n", 
		rf_result->opt_rx, rx_min, rx_min_val, rx_max, rx_max_val);

	if ((rx_min!=0xff && rx_max!=0xff) && (rx_max_val < rx_min_val)) {
		dbprintf(LOG_WARNING, "alarm skipped! rx_max(%.3fdbm) < rx_min(%.3fdbm)?\n", rx_max_val, rx_min_val);
		util_alarm_mask_clear_bit(alarm_mask, 0);
		util_alarm_mask_clear_bit(alarm_mask, 1);
		util_alarm_mask_clear_bit(alarm_mask, 2);
	} else {
		if(rf_result->opt_en == 0) rf_result->opt_rx = 0; // clear alarm when opt_en is disabled
		if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			struct me_t *me_248 = meinfo_me_get_by_instance_num(248, 0);
			if(me_248) {
				char rx_low=(char)meinfo_util_me_attr_data(me_248, 6); // LowSigThreshold
				if (util_alarm_mask_get_bit(miptr->alarm_mask, 0)) {
					if (rf_result->opt_rx != 0xffff) {
						if(rf_result->opt_rx < rx_min_val) {
							dbprintf(LOG_WARNING, "alarm: rx(%.3fdbm) < rx_min(%.3fdbm)\n", rf_result->opt_rx, rx_min_val);
							util_alarm_mask_set_bit(alarm_mask, 0);
							alarm_is_on=1;
						} else {
							util_alarm_mask_clear_bit(alarm_mask, 0);
						}
					}
				}
				if (util_alarm_mask_get_bit(miptr->alarm_mask, 1)) {
					if (rf_result->opt_rx != 0xffff) {
						if(rf_result->opt_rx < rx_low && rf_result->opt_rx >= rx_min_val) {
							dbprintf(LOG_WARNING, "alarm: rx(%.3fdbm) < rx_low(%.3fdbm)\n", rf_result->opt_rx, rx_low);
							util_alarm_mask_set_bit(alarm_mask, 1);
							alarm_is_on=1;
						} else {
							util_alarm_mask_clear_bit(alarm_mask, 1);
						}
					}
				}
			} else {
				if (util_alarm_mask_get_bit(miptr->alarm_mask, 0)) {
					// avoid false alarm, report alarm when continuous 3 los
					if (rf_result->opt_rx == 0xffff && opt_rx_previous == 0xffff && opt_rx_previous2 == 0xffff) {
						dbprintf(LOG_WARNING, "alarm: los(%.3f)\n", rf_result->opt_rx);
						util_alarm_mask_set_bit(alarm_mask, 0);
						alarm_is_on=1;
					} else if (rf_result->opt_rx != 0xffff && opt_rx_previous != 0xffff && opt_rx_previous2 != 0xffff) {
						util_alarm_mask_clear_bit(alarm_mask, 0);
					}	
				}
				if (util_alarm_mask_get_bit(miptr->alarm_mask, 1)) {
					if (rf_result->opt_rx != 0xffff) {
						if(rf_result->opt_rx < rx_min_val) {
						dbprintf(LOG_WARNING, "alarm: rx(%.3fdbm) < rx_min(%.3fdbm)\n", rf_result->opt_rx, rx_min_val);
							util_alarm_mask_set_bit(alarm_mask, 1);
							alarm_is_on=1;
						} else {
							util_alarm_mask_clear_bit(alarm_mask, 1);
						}
					}
				}
			}
		} else {
			if (util_alarm_mask_get_bit(miptr->alarm_mask, 0)) {
				// avoid false alarm, report alarm when continuous 3 los
				if (rf_result->opt_rx == 0xffff && opt_rx_previous == 0xffff && opt_rx_previous2 == 0xffff) {
					dbprintf(LOG_WARNING, "alarm: los(%.3f)\n", rf_result->opt_rx);
					util_alarm_mask_set_bit(alarm_mask, 0);
					alarm_is_on=1;
				} else if (rf_result->opt_rx != 0xffff && opt_rx_previous != 0xffff && opt_rx_previous2 != 0xffff) {
					util_alarm_mask_clear_bit(alarm_mask, 0);
				}	
			}
			if (util_alarm_mask_get_bit(miptr->alarm_mask, 1)) {
				if (rf_result->opt_rx != 0xffff) {
					if(rf_result->opt_rx < rx_min_val) {
					dbprintf(LOG_WARNING, "alarm: rx(%.3fdbm) < rx_min(%.3fdbm)\n", rf_result->opt_rx, rx_min_val);
						util_alarm_mask_set_bit(alarm_mask, 1);
						alarm_is_on=1;
					} else {
						util_alarm_mask_clear_bit(alarm_mask, 1);
					}
				}
			}
		}
		if (util_alarm_mask_get_bit(miptr->alarm_mask, 2)) {
			if (rf_result->opt_rx != 0xffff) {
				if (rf_result->opt_rx > rx_max_val) {
					dbprintf(LOG_WARNING, "alarm: rx(%.3fdbm) > rx_max(%.3fdbm)\n", rf_result->opt_rx, rx_max_val);
					util_alarm_mask_set_bit(alarm_mask, 2);
					alarm_is_on=1;
				} else {
					util_alarm_mask_clear_bit(alarm_mask, 2);
				}
			}
		}
	}

	opt_rx_previous = rf_result->opt_rx;
	opt_rx_previous2 = opt_rx_previous;
	return alarm_is_on;
}

static int 
meinfo_hw_90_alarm(struct me_t *me, unsigned char alarm_mask[28])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	struct rf_result_t rf_result;
	
	if (meinfo_hw_util_get_ontstate() != 5)	// no alarm check for rf if not in O5
		return 0;
	
	// get counters from hw
	if (meinfo_hw_rf_get_threshold_result(&rf_result)<0)
   		return -1;

	// check rx optical level
	meinfo_hw_90_alarm_check_power(miptr, alarm_mask, me, &rf_result);
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_90 = {
	.get_hw			= meinfo_hw_90_get,
	.alarm_hw		= meinfo_hw_90_alarm,
};

