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
 * File    : meinfo_hw_24.c
 *
 ******************************************************************/


#include <string.h>
#include "meinfo_hw.h"
#include "meinfo_hw_util.h"
#include "er_group.h"
#include "util.h"
#include "switch.h"
#include "hwresource.h"
#include "meinfo.h"
#include "proprietary_alu.h"


// 24 Ethernet_performance_monitoring_history_data

static int 
meinfo_hw_24_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int i,ret;
	struct me_t *eth_pptp_uni_me;
	struct switch_ethernet_pm_t ethernet_pm;
	struct switch_ethernet_frame_pm_ud_t ethernet_pm_us, ethernet_pm_ds;
	struct switch_port_info_t sw_port_info;

	//find Physical_path_termination_point_Ethernet_UNI for internal ethernet port id
	if (me == NULL || (eth_pptp_uni_me = meinfo_me_get_by_meid(11, me->meid)) == NULL)
	{
		dbprintf(LOG_ERR, "Can't find related ethernet pptp uni\n");
		return -1;
	}

	// get value from HW depend on different switch configure
	memset( &ethernet_pm, 0, sizeof(struct switch_ethernet_pm_t) );
	if (switch_get_port_info_by_me(eth_pptp_uni_me, &sw_port_info) == 0) {
		if( (ret=switch_hw_g.eth_pm_data(sw_port_info.logical_port_id, &ethernet_pm)) !=0 ) {
			dbprintf(LOG_ERR, "ret = %d\n", ret);
			return -1;
		}
		// ALU OLT workaround for CHT case
		if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) && 
		     (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU)) {
			memset( &ethernet_pm_us, 0, sizeof(struct switch_ethernet_frame_pm_ud_t) );
			if( (ret=switch_hw_g.eth_pm_data_upstream(sw_port_info.logical_port_id, &ethernet_pm_us)) !=0 ) {
				dbprintf(LOG_ERR, "ret = %d\n", ret);
				return -1;
			}
			memset( &ethernet_pm_ds, 0, sizeof(struct switch_ethernet_frame_pm_ud_t) );
			if( (ret=switch_hw_g.eth_pm_data_downstream(sw_port_info.logical_port_id, &ethernet_pm_ds)) !=0 ) {
				dbprintf(LOG_ERR, "ret = %d\n", ret);
				return -1;
			}
		}
	} else {
		dbprintf(LOG_ERR, "Can't get hw information in switch\n");
		return -1;
	}

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			// ALU OLT FTTU7342 workaround for CHT case
			if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) && 
			     (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) && 
			     (proprietary_alu_get_olt_version()==0)) {
				switch (i)
				{
				//case 1: //interval end time
				//case 2: //threshold data 1/2 id
				case 3: // No workaround
					meinfo_me_attr_update_pm(me, i, ethernet_pm.fcs_error);
					break;
				case 4: // Broadcast (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.broadcast_packets);
					break;
				case 5: // Packet size 128B~255B (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.packets_128_to_255_octets);
					break;
				case 6: // Oversize (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.oversize_packets);
					break;
				case 7: // Packet size 512B~1023B (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.packets_512_to_1023_octets);
					break;
				case 8: // Packet size 1024B~1518B (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.packets_1024_to_1518_octets);
					break;
				case 9: // No workaround
					meinfo_me_attr_update_pm(me, i, ethernet_pm.single_collision_frame_counter);
					break;
				case 10: // Packet size 256B~511B (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.packets_256_to_511_octets);
					break;
				case 11: // Packet size 1519B~1532B
					meinfo_me_attr_update_pm(me, i, ethernet_pm.frames_too_long);
					break;
				case 12: // Packet size 65B~127B (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.packets_65_to_127_octets);
					break;
				case 13: // Unicast TX (ME321, ETH_PM_DS)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_ds.packets - ethernet_pm_ds.multicast_packets - ethernet_pm_ds.broadcast_packets);
					break;
				case 14: // No workaround
					meinfo_me_attr_update_pm(me, i, ethernet_pm.carrier_sense_error_counter);
					break;
				case 15: // Packet size 64B (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.packets_64_octets);
					break;
				case 16: // Unicast RX (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.packets - ethernet_pm_us.multicast_packets - ethernet_pm_us.broadcast_packets);
					break;
				default:
					continue;//do nothing
				}
			} // ALU OLT ISAM7360 workaround for CHT case
			else if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) && 
			     (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) && 
			     (proprietary_alu_get_olt_version()==1)) {
				switch (i)
				{
				//case 1: //interval end time
				//case 2: //threshold data 1/2 id
				case 3:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.fcs_error);
					break;
				case 4:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.excessive_collision_counter);
					break;
				case 5:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.late_collision_counter);
					break;
				case 6:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.frames_too_long);
					break;
				case 7:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.buffer_overflows_on_receive);
					break;
				case 8: //buffer overflows on transmit
					meinfo_me_attr_update_pm(me, i, ethernet_pm.buffer_overflows_on_transmit);
					break;
				case 9:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.single_collision_frame_counter);
					break;
				case 10:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.multiple_collisions_frame_counter);
					break;
				case 11: //sqe counter
					meinfo_me_attr_update_pm(me, i, ethernet_pm.sqe_counter);
					break;
				case 12:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.deferred_transmission_counter);
					break;
				case 13: // Unicast TX (ME321, ETH_PM_DS)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_ds.packets - ethernet_pm_ds.multicast_packets - ethernet_pm_ds.broadcast_packets);
					break;
				case 14: //carrier sense error counter
					meinfo_me_attr_update_pm(me, i, ethernet_pm.carrier_sense_error_counter);
					break;
				case 15:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.alignment_error_counter);
					break;
				case 16: // Unicast RX (ME322, ETH_PM_US)
					meinfo_me_attr_update_pm(me, i, ethernet_pm_us.packets - ethernet_pm_us.multicast_packets - ethernet_pm_us.broadcast_packets);
					break;
				default:
					continue;//do nothing
				}
			} else {
				switch (i)
				{
				//case 1: //interval end time
				//case 2: //threshold data 1/2 id
				case 3:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.fcs_error);
					break;
				case 4:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.excessive_collision_counter);
					break;
				case 5:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.late_collision_counter);
					break;
				case 6:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.frames_too_long);
					break;
				case 7:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.buffer_overflows_on_receive);
					break;
				case 8: //buffer overflows on transmit
					meinfo_me_attr_update_pm(me, i, ethernet_pm.buffer_overflows_on_transmit);
					break;
				case 9:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.single_collision_frame_counter);
					break;
				case 10:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.multiple_collisions_frame_counter);
					break;
				case 11: //sqe counter
					meinfo_me_attr_update_pm(me, i, ethernet_pm.sqe_counter);
					break;
				case 12:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.deferred_transmission_counter);
					break;
				case 13:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.internal_mac_transmit_error_counter);
					break;
				case 14: //carrier sense error counter
					meinfo_me_attr_update_pm(me, i, ethernet_pm.carrier_sense_error_counter);
					break;
				case 15:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.alignment_error_counter);
					break;
				case 16:
					meinfo_me_attr_update_pm(me, i, ethernet_pm.internal_mac_receive_error_counter);
					break;
				default:
					continue;//do nothing
				}
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

struct meinfo_hardware_t meinfo_hw_24 = {
	.get_hw		= meinfo_hw_24_get,
};

