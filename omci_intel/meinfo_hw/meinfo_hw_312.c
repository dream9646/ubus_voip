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
 * File    : meinfo_hw_312.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_hw.h"
#include "er_group.h"
#include "util.h"
#include "hwresource.h"
#include "gpon_sw.h"

// 312 FEC_performance_monitoring_history_data

static int 
meinfo_hw_312_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int i;
	struct attr_value_t attr;
	struct gpon_counter_global_t counter_global;

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input me error, null\n");
		return -1;
	}

	//get current data
	gpon_hw_g.counter_global_get(&counter_global); 

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		memset(&attr, 0x00, sizeof(attr));

		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			//case 1: //interval end time
			//case 2: //threshold data 1/2 id
			case 3:
				meinfo_me_attr_update_pm(me, i, counter_global.rx_fec_correct_bit);
				break;
			case 4:
				meinfo_me_attr_update_pm(me, i, counter_global.rx_fec_correct_byte);
				break;
			case 5:
				meinfo_me_attr_update_pm(me, i, counter_global.rx_fec_uncor_cw);
				break;
			case 6:
				meinfo_me_attr_update_pm(me, i, counter_global.rx_fec_correct_cw);
				break;
			//case 7: //fec seconds, not support
			default:
				continue;//do nothing
			}
		}
	}

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		// PM Get with no attributes indicates that the counts associated with the ME should be reset
		if(attr_mask[0] == 0 && attr_mask[1] == 0) {
			for (i = 3; i <= meinfo_util_get_attr_total(me->classid); i++)
				meinfo_me_attr_clear_pm(me, i);
			gpon_hw_g.pm_refresh(1);
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_312 = {
	.get_hw		= meinfo_hw_312_get,
};

