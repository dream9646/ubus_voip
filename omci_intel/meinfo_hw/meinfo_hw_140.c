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
 * File    : meinfo_hw_140.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"

//9.9.12 Call_control_performance_monitoring_history_data
static int 
meinfo_hw_140_get(struct me_t *me, unsigned char attr_mask[2])
{
	int i;
	unsigned int pm_value;
	unsigned char omci_attr_mask[2];
	struct voip_hw_call_control_performance_monitoring_history_data_t ccpmhd;

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&ccpmhd, 0x00, sizeof(ccpmhd));

	if (meinfo_hw_util_get_voip_param_by_port(me, &ccpmhd, attr_mask, omci_attr_mask) < 0)
	{
		//only show error message, but return success
		dbprintf(LOG_ERR, "get param error: classid=%u, meid=%u\n", me->classid, me->meid);
		return 0;
	}

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		if (util_bitmap_get_value(omci_attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			//case 1:
			//interval_end_time
			//case 2:
			//threshold_data_1_2_id
			case 3:
				pm_value = ccpmhd.call_setup_failures;
				break;
			case 4:
				pm_value = ccpmhd.call_setup_timer;
				break;
			case 5:
				pm_value = ccpmhd.call_terminate_failures;
				break;
			case 6:
				pm_value = ccpmhd.analog_port_releases;
				break;
			case 7:
				pm_value = ccpmhd.analog_port_off_hook_timer;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_update_pm(me, i, pm_value);
		}
	}

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		// PM Get with no attributes indicates that the counts associated with the ME should be reset
		if(attr_mask[0] == 0 && attr_mask[1] == 0) {
			for (i = 3; i <= meinfo_util_get_attr_total(me->classid); i++)
				meinfo_me_attr_clear_pm(me, i);
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_140 = {
	.get_hw		= meinfo_hw_140_get
};
