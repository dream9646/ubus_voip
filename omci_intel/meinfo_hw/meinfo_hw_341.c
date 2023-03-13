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
 * File    : meinfo_hw_341.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "meinfo_hw.h"
#include "er_group.h"
#include "util.h"
#include "hwresource.h"
#include "gpon_sw.h"
#include "tm.h"

// 341 GEM_port_network_CTP_performance_monitoring_history_data

static int 
meinfo_hw_341_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct gpon_counter_dsflow_t counter_dsflow;
	struct gpon_counter_usflow_t counter_usflow;
	unsigned int i;
	struct me_t *gem_port_me;
	struct attr_value_t attr;
	int flowid=-1;

	if (me == NULL || (gem_port_me = meinfo_me_get_by_meid(268, me->meid)) == NULL)
	{
		dbprintf(LOG_ERR, "Can't find related gem port\n");
		return -1;
	}

	if((flowid=gem_flowid_get_by_gem_me( gem_port_me, 0)) < 0 )	//system in temporary state, skip it
		return 0;

	gpon_hw_g.pm_refresh(0); // (0/1): reset
	memset(&counter_dsflow, 0, sizeof(counter_dsflow));
	memset(&counter_usflow, 0, sizeof(counter_usflow));
	gpon_hw_g.pm_dsflow_get(flowid, &counter_dsflow);
	gpon_hw_g.pm_usflow_get(flowid, &counter_usflow);

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		memset(&attr, 0x00, sizeof(attr));

		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			//case 1: // Interval_end_time
			//case 2: // Threshold_data_1/2_id
			case 3: // Transmitted_GEM_frames
				meinfo_me_attr_update_pm(me, i, counter_usflow.tx_gem_block );
				break;
			case 4: // Received_GEM_frames
				meinfo_me_attr_update_pm(me, i, counter_dsflow.rx_gem_block );
				break;
			case 5: // Received_payload_bytes
				meinfo_me_attr_update_pm(me, i, counter_dsflow.rx_gem_byte );
				break;
			case 6: // Transmitted_payload_bytes
				meinfo_me_attr_update_pm(me, i, counter_usflow.tx_gem_byte );
				break;
			case 7: // Encryption_key_errors: not support
				//meinfo_me_attr_update_pm(me, i, counter_dsflow.rx_gem_block );
				break;
			default:
				continue; //do nothing
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

struct meinfo_hardware_t meinfo_hw_341 = {
	.get_hw		= meinfo_hw_341_get,
};

