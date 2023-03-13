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
 * File    : meinfo_hw_147.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"

//9.9.9 VoIP_feature_access_codes
static int 
meinfo_hw_147_get(struct me_t *me, unsigned char attr_mask[2])
{
	int i;
	unsigned char omci_attr_mask[2];
	struct voip_hw_voip_feature_access_codes_t vfac;
	struct attr_value_t attr;

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&vfac, 0x00, sizeof(vfac));

	if (meinfo_hw_util_get_voip_param_by_port(me, &vfac, attr_mask, omci_attr_mask) < 0)
	{
		//only show error message, but return success
		dbprintf(LOG_ERR, "get param error: classid=%u, meid=%u\n", me->classid, me->meid);
		return 0;
	}

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		memset(&attr, 0x00, sizeof(attr));

		if (util_bitmap_get_value(omci_attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			case 1:
				attr.ptr = vfac.cancel_call_waiting;
				break;
			case 2:
				attr.ptr = vfac.call_hold;
				break;
			case 3:
				attr.ptr = vfac.call_park;
				break;
			case 4:
				attr.ptr = vfac.caller_id_activate;
				break;
			case 5:
				attr.ptr = vfac.caller_id_deactivate;
				break;
			case 6:
				attr.ptr = vfac.do_not_disturb_activation;
				break;
			case 7:
				attr.ptr = vfac.do_not_disturb_deactivation;
				break;
			case 8:
				attr.ptr = vfac.do_not_disturb_pin_change;
				break;
			case 9:
				attr.ptr = vfac.emergency_service_number;
				break;
			case 10:
				attr.ptr = vfac.intercom_service;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(me, i, &attr, 1);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_147 = {
	.get_hw		= meinfo_hw_147_get
};
