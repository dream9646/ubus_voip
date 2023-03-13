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
 * File    : meinfo_cb_65348.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "util.h"
#include "pmtask.h"

// 9.99.26 omci_me_pm_window_time_g_private 655348

static int 
meinfo_cb_65348_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	struct tm pm_start_time, pm_end_time;

	pmtask_get_history_time(&pm_start_time, &pm_end_time);

	if (util_attr_mask_get_bit(attr_mask, 1)) {		// 1: pm_start_year
		attr.data = 1900 + pm_start_time.tm_year;
		meinfo_me_attr_set(me, 1, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {		// 2: pm_start_month
		attr.data = 1 + pm_start_time.tm_mon;
		meinfo_me_attr_set(me, 2, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {		// 3: pm_start_day
		attr.data = pm_start_time.tm_mday;
		meinfo_me_attr_set(me, 3, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) {		// 4: pm_start_hour
		attr.data = pm_start_time.tm_hour;
		meinfo_me_attr_set(me, 4, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) {		// 5: pm_start_minute
		attr.data = pm_start_time.tm_min;
		meinfo_me_attr_set(me, 5, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 6)) {		// 6: pm_start_second
		attr.data = pm_start_time.tm_sec;
		meinfo_me_attr_set(me, 6, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 7)) {		// 7: pm_end_year
		attr.data = 1900 + pm_end_time.tm_year;
		meinfo_me_attr_set(me, 7, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 8)) {		// 8: pm_end_month
		attr.data = 1 + pm_end_time.tm_mon;
		meinfo_me_attr_set(me, 8, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 9)) {		// 9: pm_end_day
		attr.data = pm_end_time.tm_mday;
		meinfo_me_attr_set(me, 9, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 10)) {	// 10: pm_end_hour
		attr.data = pm_end_time.tm_hour;
		meinfo_me_attr_set(me, 10, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 11)) {	// 11: pm_end_minute
		attr.data = pm_end_time.tm_min;
		meinfo_me_attr_set(me, 11, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 12)) {	// 12: pm_end_second
		attr.data = pm_end_time.tm_sec;
		meinfo_me_attr_set(me, 12, &attr, 0);
	}

	return 0;
}

struct meinfo_callback_t meinfo_cb_fiberhome_65348 = {
	.get_cb		= meinfo_cb_65348_get,
};
