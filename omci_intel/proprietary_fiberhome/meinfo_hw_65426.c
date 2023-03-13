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
 * File    : meinfo_hw_65426.c
 *
 ******************************************************************/

#include <string.h>

#include "er_group.h"
#include "voip_hw.h"
#include "util.h"
#include "er_group_hw.h"
#include "omcimsg.h"
#include "meinfo_hw_util.h"

//9.99.28 WRI_NGN_port_statics
static int 
meinfo_hw_65426_get(struct me_t *me, unsigned char attr_mask[2])
{
	int i;
	unsigned char omci_attr_mask[2];
	struct voip_hw_wri_ngn_port_statics_t wnps;
	struct attr_value_t attr;
	unsigned char bitmap_temp[7];

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}

	memset(&wnps, 0x00, sizeof(wnps));

	if (meinfo_hw_util_get_voip_param_by_port(me, &wnps, attr_mask, omci_attr_mask) < 0)
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
			//case 1: //pptp pointer
			case 2:
				attr.data = wnps.received_rtp_packets;
				break;
			case 3:
				attr.data = wnps.send_rtp_packets;
				break;
			case 4:
				attr.data = wnps.received_rtp_bytes;
				break;
			case 5:
				attr.data = wnps.send_rtp_bytes;
				break;
			case 6:
				attr.data = wnps.rtp_jitter_buffer;
				break;
			case 7:
				memset(bitmap_temp, 0x00, sizeof(bitmap_temp));
				util_bitmap_set_value(bitmap_temp, 8*7, 0, 16, wnps.latest_call_begin_time.year);
				util_bitmap_set_value(bitmap_temp, 8*7, 16, 8, wnps.latest_call_begin_time.month);
				util_bitmap_set_value(bitmap_temp, 8*7, 24, 8, wnps.latest_call_begin_time.day);
				util_bitmap_set_value(bitmap_temp, 8*7, 32, 8, wnps.latest_call_begin_time.hour);
				util_bitmap_set_value(bitmap_temp, 8*7, 40, 8, wnps.latest_call_begin_time.minute);
				util_bitmap_set_value(bitmap_temp, 8*7, 48, 8, wnps.latest_call_begin_time.second);

				attr.ptr = bitmap_temp;
				break;
			case 8:
				memset(bitmap_temp, 0x00, sizeof(bitmap_temp));
				util_bitmap_set_value(bitmap_temp, 8*7, 0, 16, wnps.latest_call_end_time.year);
				util_bitmap_set_value(bitmap_temp, 8*7, 16, 8, wnps.latest_call_end_time.month);
				util_bitmap_set_value(bitmap_temp, 8*7, 24, 8, wnps.latest_call_end_time.day);
				util_bitmap_set_value(bitmap_temp, 8*7, 32, 8, wnps.latest_call_end_time.hour);
				util_bitmap_set_value(bitmap_temp, 8*7, 40, 8, wnps.latest_call_end_time.minute);
				util_bitmap_set_value(bitmap_temp, 8*7, 48, 8, wnps.latest_call_end_time.second);

				attr.ptr = bitmap_temp;
				break;
			case 9:
				attr.data = wnps.last_call_duration;
				break;
			case 10:
				attr.data = wnps.total_call_times;
				break;
			case 11:
				attr.data = wnps.total_call_duration;
				break;
			case 12:
				attr.data = wnps.upstream_rates;
				break;
			case 13:
				attr.data = wnps.downstream_rates;
				break;
			case 14:
				attr.data = wnps.avg_network_delay;
				break;
			case 15:
				attr.data = wnps.packet_loss;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(me, i, &attr, 1);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65426 = {
	.get_hw		= meinfo_hw_65426_get
};

