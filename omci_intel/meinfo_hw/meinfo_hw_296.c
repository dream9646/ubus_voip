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
 * File    : meinfo_hw_296.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_hw.h"
#include "er_group.h"
#include "util.h"
#include "switch.h"
#include "hwresource.h"

// 296 Ethernet_performance_monitoring_history_data_3

static int 
meinfo_hw_296_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int i,ret;
	struct me_t *eth_pptp_uni_me;
	struct switch_ethernet_frame_pm_t ethernet_frame_pm;
	struct switch_port_info_t sw_port_info;

	//find Physical_path_termination_point_Ethernet_UNI for internal ethernet port id
	if (me == NULL || (eth_pptp_uni_me = meinfo_me_get_by_meid(11, me->meid)) == NULL)
	{
		dbprintf(LOG_ERR, "Can't find related ethernet pptp uni\n");
		return -1;
	}

	// get value from HW depend on different switch configure

	memset( &ethernet_frame_pm, 0, sizeof(struct switch_ethernet_frame_pm_t) );
	if (switch_get_port_info_by_me(eth_pptp_uni_me, &sw_port_info) == 0) {
		if( (ret=switch_hw_g.eth_pm_data3(sw_port_info.logical_port_id, &ethernet_frame_pm)) !=0 ) {
			dbprintf(LOG_ERR, "ret = %d\n", ret);
			return -1;
		}
	} else {
		dbprintf(LOG_ERR, "Can't get hw information in switch\n");
		return -1;
	}

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			//case 1: //interval end time
			//case 2: //threshold data 1/2 id
			//case 3: //drop event, not support
			case 4:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.octets);
				break;
			case 5:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets);
				break;
			case 6:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.broadcast_packets);
				break;
			case 7:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.multicast_packets);
				break;
			case 8:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.undersize_packets);
				break;
			case 9:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.fragments);
				break;
			case 10:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.jabbers);
				break;
			case 11:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_64_octets);
				break;
			case 12:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_65_to_127_octets);
				break;
			case 13:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_128_to_255_octets);
				break;
			case 14:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_256_to_511_octets);
				break;
			case 15:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_512_to_1023_octets);
				break;
			case 16:
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_1024_to_1518_octets);
				break;
			default:
				continue;//do nothing
			}
		}
	}

	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		// PM Get with no attributes indicates that the counts associated with the ME should be reset
		if(attr_mask[0] == 0 && attr_mask[1] == 0) {
			for (i = 3; i <= meinfo_util_get_attr_total(me->classid); i++)
				meinfo_me_attr_clear_pm(me, i);
			switch_hw_g.pm_refresh(1);
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_296 = {
	.get_hw		= meinfo_hw_296_get,
};

