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
 * File    : meinfo_hw_65425.c
 *
 ******************************************************************/

#include <string.h>

#include "er_group.h"
#include "voip_hw.h"
#include "util.h"
#include "er_group_hw.h"
#include "omcimsg.h"
#include "proprietary_fiberhome.h"

//9.99.27 WRI_NGN_resource_info
static int 
meinfo_hw_65425_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret, i;
	unsigned char omci_attr_mask[2];
	struct voip_hw_wri_ngn_resource_info_t wnri;
	struct attr_value_t attr;

	if (me == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//port id
	port_id = VOIP_HW_GLOBAL_PORT_DEF;

	memset(&wnri, 0x00, sizeof(wnri));

	if ((ret = er_group_hw_voip_wrapper_wri_ngn_resource_info(VOIP_HW_ACTION_GET, port_id, attr_mask, &wnri)) < 0)
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
				attr.data = wnri.received_ngn_req_packets;
				break;
			case 2:
				attr.data = wnri.send_ngn_req_packets;
				break;
			case 3:
				attr.data = wnri.received_ngn_resp_packets;
				break;
			case 4:
				attr.data = wnri.send_ngn_resp_packets;
				break;
			case 5:
				attr.data = wnri.received_rtp_packets;
				break;
			case 6:
				attr.data = wnri.send_rtp_packets;
				break;
			case 7:
				attr.data = wnri.received_rtp_bytes;
				break;
			case 8:
				attr.data = wnri.send_rtp_bytes;
				break;
			case 9:
				attr.data = wnri.packet_loss;
				break;
			case 10:
				attr.data = wnri.network_avg_delay;
				break;
			case 11:
				attr.data = wnri.rtp_avg_jitter_buffer;
				break;
			case 12:
				attr.data = wnri.bandwidth_usage;
				break;
			case 13:
				attr.data = wnri.lost_sig_packets;
				break;
			case 14:
				attr.data = wnri.retransmit_packets;
				break;
			case 15:
				attr.data = wnri.wrong_sig_packets;
				break;
			case 16:
				attr.data = wnri.unknown_sig_packets;
				break;
			default:
				continue;//do nothing
			}

			meinfo_me_attr_set(me, i, &attr, 1);
		}
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65425 = {
	.get_hw		= meinfo_hw_65425_get
};

