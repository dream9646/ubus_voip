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
 * File    : meinfo_hw_244.c
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
meinfo_hw_244_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	unsigned char status=0;
	struct attr_value_t attr;
	int i, ret = 0;

	if (me == NULL) {
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	port_id = me->instance_num;
	voip_hw_potsuniext(VOIP_HW_ACTION_GET, port_id, attr_mask, &status);

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
				attr.data = status;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(me, i, &attr, 0);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_244 = {
	.get_hw		= meinfo_hw_244_get
};

