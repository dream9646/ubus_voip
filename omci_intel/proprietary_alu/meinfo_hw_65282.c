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
 * Module  : proprietary_alu
 * File    : meinfo_hw_65282.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "hwresource.h"
#include "switch.h"

static int 
meinfo_hw_65282_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	unsigned int i, ret;
	struct me_t *eth_port_me, *veip_port_me;
	struct switch_ethernet_frame_pm_ud_t ethernet_frame_pm_us, ethernet_frame_pm_ds;
	struct switch_port_info_t sw_port_info;
	
	if (me == NULL) {
		dbprintf(LOG_ERR, "Can't find me\n");
		return -1;
	}
	
	//find MAC_bridge_port_configuration_data for internal bridge port id
	if((eth_port_me = meinfo_me_get_by_meid(11, me->meid)) == NULL)
	{
	    	if((veip_port_me = meinfo_me_get_by_meid(329, me->meid)) == NULL)
		{
			dbprintf(LOG_ERR, "Can't find related internal bridge port\n");
			return -1;
		}
	}

	// get value from HW depend on different switch configure
	memset( &ethernet_frame_pm_us, 0, sizeof(struct switch_ethernet_frame_pm_ud_t) );
	memset( &ethernet_frame_pm_ds, 0, sizeof(struct switch_ethernet_frame_pm_ud_t) );
	
	if(eth_port_me) { // For UNI port, get UNI port counters
		if (switch_get_port_info_by_me(eth_port_me, &sw_port_info) == 0) {
			if( (ret=switch_hw_g.eth_pm_data_downstream(sw_port_info.logical_port_id, &ethernet_frame_pm_ds)) !=0 ) {
				dbprintf(LOG_ERR, "ret = %d\n", ret);
				return -1;
			}
			if( (ret=switch_hw_g.eth_pm_data_upstream(sw_port_info.logical_port_id, &ethernet_frame_pm_us)) !=0 ) {
				dbprintf(LOG_ERR, "ret = %d\n", ret);
				return -1;
			}
		} else {
			dbprintf(LOG_ERR, "Can't get hw information in switch\n");
			return -1;
		}
	} else if(veip_port_me) { // For VEIP port, get PON port counters
		if( (ret=switch_hw_g.eth_pm_data_upstream(5, &ethernet_frame_pm_us)) !=0 ) {
			dbprintf(LOG_ERR, "ret = %d\n", ret);
			return -1;
		}
		if( (ret=switch_hw_g.eth_pm_data_downstream(5, &ethernet_frame_pm_ds)) !=0 ) {
			dbprintf(LOG_ERR, "ret = %d\n", ret);
			return -1;
		}
	}
	
	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		memset(&attr, 0x00, sizeof(attr));

		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			switch (i)
			{
			//case 1: // Interval_End_Time
			//case 2: // Threshold_Data_1/2_Id
			case 3: // Frames_Upstream
			case 14: // Packets_Upstream_Counter_64
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm_us.packets);
				break;
			case 4: // Frames_Downstream
			case 15: // Packets_Downstream_Counter_64
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm_ds.packets);
				break;
			case 5: // Bytes_Upstream
			case 12: // Bytes_Upstream_Counter_64
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm_us.octets);
				break;
			case 6: // Bytes_Downstream
			case 13: // Bytes_Downstream_Counter_64
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm_ds.octets);
				break;
			case 7: // Dropped_Frames_Upstream
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm_us.drop_events);
				break;
			//case 8: // Dropped_Bytes_Upstream
			//	meinfo_me_attr_update_pm(me, i, ethernet_frame_pm_us.drop_events);
			//	break;
			case 9: // Dropped_Frames_Downstream
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm_ds.drop_events);
				break;
			case 10: // Multicast_Frames_Upstream
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm_us.multicast_packets);
			case 11: // Multicast_Frames_Downstream
				meinfo_me_attr_update_pm(me, i, ethernet_frame_pm_ds.multicast_packets);
				break;
			default:
				continue;//do nothing
			}
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_alu_65282 = {
	.get_hw		= meinfo_hw_65282_get
};
