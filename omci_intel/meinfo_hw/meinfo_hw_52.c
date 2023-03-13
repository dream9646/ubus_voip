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
 * File    : meinfo_hw_52.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_hw.h"
#include "er_group.h"
#include "util.h"
#include "hwresource.h"
#include "switch.h"

// 52 MAC_bridge_port_performance_monitoring_history_data

static int 
meinfo_hw_52_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int i;
	int ret = -1;
	struct me_t *bridge_port_me;
	struct attr_value_t attr;
	struct switch_bridge_port_pm_t bridge_port_pm;
	struct switch_port_info_t sw_port_info;

	//find MAC_bridge_port_configuration_data for internal bridge port id
	if (me == NULL || (bridge_port_me = meinfo_me_get_by_meid(47, me->meid)) == NULL)
	{
		dbprintf(LOG_ERR, "Can't find related internal bridge port\n");
		return -1;
	}

	// get value from HW depend on different switch configure
	if (switch_get_port_info_by_me(bridge_port_me, &sw_port_info) == 0)
	{
		if(switch_hw_g.bridge_port_pm_data == NULL ||
			(ret=switch_hw_g.bridge_port_pm_data(sw_port_info.logical_port_id, &bridge_port_pm)) !=0 )
		{
			dbprintf(LOG_ERR, "ret = %d\n", ret);
			return -1;
		}
	} else {
		dbprintf(LOG_ERR, "Can't get hw information in switch\n");
		return -1;
	}


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
				meinfo_me_attr_update_pm(me, i, bridge_port_pm.forwarded_frame);
				break;
			//case 4: //delay exceeded discard counter, not support
			//case 5: //MTU exceeded discard counter, not support
			case 6:
				meinfo_me_attr_update_pm(me, i, bridge_port_pm.received_frame);
				break;
			case 7:
				meinfo_me_attr_update_pm(me, i, bridge_port_pm.received_and_discarded);
				break;
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
			switch_hw_g.pm_refresh(1);
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_52 = {
	.get_hw		= meinfo_hw_52_get,
};

