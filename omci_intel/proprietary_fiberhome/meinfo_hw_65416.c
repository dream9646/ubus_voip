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
 * File    : meinfo_hw_65416.c
 *
 ******************************************************************/

#include <string.h>

#include "er_group.h"
#include "voip_hw.h"
#include "util.h"
#include "er_group_hw.h"
#include "omcimsg.h"
#include "meinfo_hw_util.h"

//9.99.14 WRI_MG_port_register_control
static int 
meinfo_hw_65416_get(struct me_t *me, unsigned char attr_mask[2])
{
	int i;
	unsigned char large_string1[376];
	unsigned char omci_attr_mask[2];
	struct voip_hw_wri_mg_port_register_control_t wmprc;
	struct attr_value_t attr;

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&wmprc, 0x00, sizeof(wmprc));
	memset(&large_string1, 0x00, sizeof(large_string1));

	wmprc.mg_ip_pointer = large_string1;

	if (meinfo_hw_util_get_voip_param_by_port(me, &wmprc, attr_mask, omci_attr_mask) < 0)
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
			//case 1:
			//mg_ip_pointer
			case 2:
				attr.data = wmprc.mg_act;
				break;
			//case 3:
			//port
			case 4:
				attr.data = wmprc.port_act;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(me, i, &attr, 1);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65416 = {
	.get_hw		= meinfo_hw_65416_get
};

