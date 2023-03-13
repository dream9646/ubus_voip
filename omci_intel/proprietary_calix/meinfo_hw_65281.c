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
 * File    : meinfo_hw_65281.c
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
meinfo_hw_65281_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;

	struct voip_hw_voip_call_statistics_extension2_t call_statistics_extension2;
	struct attr_value_t attr;
	int i, ret = 0;

	if (me == NULL) {
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	port_id = me->instance_num;
	memset(&call_statistics_extension2, 0x00, sizeof(call_statistics_extension2));
	voip_hw_voip_call_statistics_extension2(VOIP_HW_ACTION_GET, port_id, attr_mask, &call_statistics_extension2);

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
				attr.data = call_statistics_extension2.outcallpeerdisconnects;
				break;
			case 2:
				attr.data = call_statistics_extension2.outcallontdisconnects;
				break;
			case 3:
				attr.data = call_statistics_extension2.e911callattempts;
				break;
			case 4:
				attr.data = call_statistics_extension2.e911callcompletions;
				break;
			case 5:
				attr.data = call_statistics_extension2.e911callbusy;
				break;
			case 6:
				attr.data = call_statistics_extension2.e911callpeerdisconnects;
				break;
			case 7:
				attr.data = call_statistics_extension2.e911callonhooks;
				break;
			case 8:
				attr.data = call_statistics_extension2.vmwinotifymsgswaiting;
				break;
			case 9:
				attr.data = call_statistics_extension2.vmwinotifynomsgswaiting;
				break;
			case 10:
				attr.data = call_statistics_extension2.rtppktssent;
				break;
			case 11:
				attr.data = call_statistics_extension2.rtppktsrecv;
				break;
			case 12:
				attr.data = call_statistics_extension2.rtpnullipsent;
				break;
			case 13:
				attr.data = call_statistics_extension2.rtpnulliprecv;
				break;
			case 14:
				attr.data = call_statistics_extension2.ext2activecallcntr;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(me, i, &attr, 0);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65281 = {
	.get_hw		= meinfo_hw_65281_get
};

