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
 * File    : meinfo_hw_65413.c
 *
 ******************************************************************/

#include <string.h>

#include "er_group.h"
#include "voip_hw.h"
#include "util.h"
#include "er_group_hw.h"
#include "omcimsg.h"
#include "proprietary_fiberhome.h"

//9.99.11 WRI_keepalive_config_data
static int 
meinfo_hw_65413_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret, i;
	unsigned char omci_attr_mask[2];
	struct voip_hw_wri_keepalive_config_data_t wkcd;
	struct attr_value_t attr;

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//port id
	port_id = VOIP_HW_GLOBAL_PORT_DEF;

	memset(&wkcd, 0x00, sizeof(wkcd));

	if ((ret = er_group_hw_voip_wrapper_wri_keepalive_config_data(VOIP_HW_ACTION_GET, port_id, attr_mask, &wkcd)) < 0)
	{
		dbprintf(LOG_ERR, "ret=%d\n", ret);
		return -1;
	}

	omci_attr_mask[0] = attr_mask[0];
	omci_attr_mask[1] = attr_mask[1];

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		memset(&attr, 0x00, sizeof(attr));

		if (util_bitmap_get_value(omci_attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			case 1:
				attr.data = wkcd.keepalive_enable;
				break;
			case 2:
				attr.data = wkcd.keepalive_interva;
				break;
			case 3:
				attr.data = wkcd.keepalive_times;
				break;
			case 4:
				attr.data = wkcd.keepalive_mode;
				break;
			case 5:
				attr.data = wkcd.rereg_start_time_enable;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(me, i, &attr, 1);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65413 = {
	.get_hw		= meinfo_hw_65413_get
};

