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
 * File    : meinfo_hw_65283.c
 *
 ******************************************************************/

#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "alarmtask.h"
#include "er_group_hw.h"


static int 
meinfo_hw_65283_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;

	struct voip_hw_voip_call_statistics_extension4_t call_statistics_extension4;
	struct attr_value_t attr;
	int i, ret = 0;

	if (me == NULL) {
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	port_id = me->instance_num;
	memset(&call_statistics_extension4, 0x00, sizeof(call_statistics_extension4));
	voip_hw_voip_call_statistics_extension4(VOIP_HW_ACTION_GET, port_id, attr_mask, &call_statistics_extension4);

	if (ret < 0) {
		//only show error message, but return success
		dbprintf(LOG_ERR, "get param error: classid=%u, meid=%u\n", me->classid, me->meid);
		return -1;
	}

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++) {
		memset(&attr, 0x00, sizeof(attr));
		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1)) {
			switch (i)
			{
			case 1:
				attr.data = call_statistics_extension4.localudpport;
				break;
			case 2:
				attr.data = ntohl(call_statistics_extension4.remipadd);
				break;
			case 3:
				attr.data = call_statistics_extension4.remudpport;
				break;
			case 4:
				attr.data = ntohl(call_statistics_extension4.secremipaddr);
				break;
			case 5:
				attr.data = call_statistics_extension4.secremudpport;
				break;
			case 6:
				attr.data = call_statistics_extension4.qos;
				break;
			case 7:
				attr.data = call_statistics_extension4.iplinestatus;
				break;
			case 8:
				attr.data = call_statistics_extension4.active911call;
				break;
			case 9:
				attr.data = call_statistics_extension4.clrerrorcntrs;
				break;
			case 10:
				attr.data = call_statistics_extension4.clrcallcntrs;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(me, i, &attr, 0);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65283 = {
	.get_hw		= meinfo_hw_65283_get
};

