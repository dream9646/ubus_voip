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
 * File    : meinfo_hw_151.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"

//9.9.14 SIP_agent_performance_monitoring_history_data
static int 
meinfo_hw_151_get(struct me_t *me, unsigned char attr_mask[2])
{
	int i;
	unsigned int pm_value;
	unsigned char omci_attr_mask[2];
	struct voip_hw_sip_agent_performance_monitoring_history_data_t sapmhd;

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&sapmhd, 0x00, sizeof(sapmhd));

	if (meinfo_hw_util_get_voip_param_by_port(me, &sapmhd, attr_mask, omci_attr_mask) < 0)
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
				pm_value = sapmhd.transactions;
				break;
			case 4:
				pm_value = sapmhd.rx_invite_reqs;
				break;
			case 5:
				pm_value = sapmhd.rx_invite_retrans;
				break;
			case 6:
				pm_value = sapmhd.rx_noninvite_reqs;
				break;
			case 7:
				pm_value = sapmhd.rx_noninvite_retrans;
				break;
			case 8:
				pm_value = sapmhd.rx_response;
				break;
			case 9:
				pm_value = sapmhd.rx_response_retransmissions;
				break;
			case 10:
				pm_value = sapmhd.tx_invite_reqs;
				break;
			case 11:
				pm_value = sapmhd.tx_invite_retrans;
				break;
			case 12:
				pm_value = sapmhd.tx_noninvite_reqs;
				break;
			case 13:
				pm_value = sapmhd.tx_noninvite_retrans;
				break;
			case 14:
				pm_value = sapmhd.tx_response;
				break;
			case 15:
				pm_value = sapmhd.tx_response_retransmissions;
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

struct meinfo_hardware_t meinfo_hw_151 = {
	.get_hw		= meinfo_hw_151_get
};
