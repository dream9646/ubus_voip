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
 * File    : meinfo_hw_65313.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"
#include "proprietary_alu.h"

static int 
meinfo_hw_65313_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct switch_ethernet_pm_t switch_ethernet_pm_us, switch_ethernet_pm_ds;
	struct switch_ethernet_frame_pm_t switch_ethernet_frame_pm_us, switch_ethernet_frame_pm_ds;
	struct switch_ethernet_frame_pm_ud_t ethernet_frame_pm_us, ethernet_frame_pm_ds;
	unsigned int i, errors_us = 0, errors_ds = 0;
	unsigned long long rate_us = 0, rate_ds = 0;
	unsigned long long octets_us = 0, octets_ds = 0;
	unsigned long long packets_us = 0, packets_ds = 0;
	unsigned long long unicast_packets_us = 0, unicast_packets_ds = 0;
	unsigned long long multicast_packets_us = 0, multicast_packets_ds = 0;
	unsigned long long broadcast_packets_us = 0, broadcast_packets_ds = 0;
	
	memset( &switch_ethernet_pm_us, 0, sizeof(struct switch_ethernet_pm_t) );
	memset( &switch_ethernet_pm_ds, 0, sizeof(struct switch_ethernet_pm_t) );
	memset( &switch_ethernet_frame_pm_us, 0, sizeof(struct switch_ethernet_frame_pm_t) );
	memset( &switch_ethernet_frame_pm_ds, 0, sizeof(struct switch_ethernet_frame_pm_t) );
	memset( &ethernet_frame_pm_us, 0, sizeof(struct switch_ethernet_frame_pm_ud_t) );
	memset( &ethernet_frame_pm_ds, 0, sizeof(struct switch_ethernet_frame_pm_ud_t) );
	
	if(util_bitmap_get_value(attr_mask, 8*2, 4, 1) || util_bitmap_get_value(attr_mask, 8*2, 5, 1))
		switch_hw_g.eth_pm_rate_status_get(5, &rate_us, &rate_ds);
	switch_hw_g.eth_pm_data(5, &switch_ethernet_pm_ds);
	switch_hw_g.eth_pm_data3(5, &switch_ethernet_frame_pm_us);
	switch_hw_g.eth_pm_data_upstream(5, &ethernet_frame_pm_us);
	switch_hw_g.eth_pm_data_downstream(5, &ethernet_frame_pm_ds);
	
	errors_us = (	ethernet_frame_pm_us.drop_events + 
			switch_ethernet_frame_pm_us.crc_errored_packets + 
			switch_ethernet_frame_pm_us.fragments + 
			switch_ethernet_frame_pm_us.jabbers + 
			switch_ethernet_pm_us.fcs_error + 
			switch_ethernet_pm_us.excessive_collision_counter +
			switch_ethernet_pm_us.late_collision_counter +
			switch_ethernet_pm_us.single_collision_frame_counter +
			switch_ethernet_pm_us.multiple_collisions_frame_counter);
	octets_us = ethernet_frame_pm_us.octets;
	packets_us = ethernet_frame_pm_us.packets;
	unicast_packets_us = (ethernet_frame_pm_us.packets - ethernet_frame_pm_us.multicast_packets - ethernet_frame_pm_us.broadcast_packets);
	multicast_packets_us = ethernet_frame_pm_us.multicast_packets;
	broadcast_packets_us = ethernet_frame_pm_us.broadcast_packets;
	
	errors_ds = (	ethernet_frame_pm_ds.drop_events + 
			switch_ethernet_frame_pm_ds.crc_errored_packets + 
			switch_ethernet_frame_pm_ds.fragments + 
			switch_ethernet_frame_pm_ds.jabbers + 
			switch_ethernet_pm_ds.fcs_error + 
			switch_ethernet_pm_ds.excessive_collision_counter +
			switch_ethernet_pm_ds.late_collision_counter +
			switch_ethernet_pm_ds.single_collision_frame_counter +
			switch_ethernet_pm_ds.multiple_collisions_frame_counter);
	octets_ds = ethernet_frame_pm_ds.octets;
	packets_ds = ethernet_frame_pm_ds.packets;
	unicast_packets_ds = (ethernet_frame_pm_ds.packets - ethernet_frame_pm_ds.multicast_packets - ethernet_frame_pm_ds.broadcast_packets);
	multicast_packets_ds = ethernet_frame_pm_ds.multicast_packets;
	broadcast_packets_ds = ethernet_frame_pm_ds.broadcast_packets;

	// workaround for cht, use all uni ports downstream multicast counters
	// (because multicast gem port counter includes non user-subscribed traffic)
	if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		multicast_packets_ds = 0;
		for (i=0; i<SWITCH_LOGICAL_PORT_TOTAL; i++) {
			struct switch_ethernet_frame_pm_ud_t pm_ds;
			memset( &pm_ds, 0, sizeof(struct switch_ethernet_frame_pm_ud_t) );

			if (((1<<i) & switch_get_uni_logical_portmask()) == 0)
				continue;

			if( switch_hw_g.eth_pm_data_downstream(i, &pm_ds) !=0 )
				return -1;

			multicast_packets_ds += pm_ds.multicast_packets;
		}
	}

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			struct attr_value_t attr;
			switch (i)
			{
			//case 1: // Interval_End_Time
			//case 2: // Threshold_Data_1/2_Id
			case 3: // Upstream_Errors
				meinfo_me_attr_update_pm(me, i, errors_us);
				break;
			case 4: // Downstream_Errors
				meinfo_me_attr_update_pm(me, i, errors_ds);
				break;
			case 5: // Upstream_Actual_Rate (bps)
				attr.data = 0;
				meinfo_me_attr_set_pm_current_data(me, i, attr);
				meinfo_me_attr_update_pm(me, i, rate_us);
				break;
			case 6: // Downstream_Actual_Rate (bps)
				attr.data = 0;
				meinfo_me_attr_set_pm_current_data(me, i, attr);
				meinfo_me_attr_update_pm(me, i, rate_ds);
				break;
			case 7: // Upstream_Octets
				meinfo_me_attr_update_pm(me, i, octets_us);
				break;
			case 8: // Downstream_Octets
				meinfo_me_attr_update_pm(me, i, octets_ds);
				break;
			case 9: // Upstream_Packets
				meinfo_me_attr_update_pm(me, i, packets_us);
				break;
			case 10: // Downstream_Packets
				meinfo_me_attr_update_pm(me, i, packets_ds);
				break;
			case 11: // Upstream_Unicast_Packets
				meinfo_me_attr_update_pm(me, i, unicast_packets_us);
				break;
			case 12: // Downstream_Unicast_Packets
				meinfo_me_attr_update_pm(me, i, unicast_packets_ds);
				break;
			case 13: // Upstream_Multicast_Packets
				meinfo_me_attr_update_pm(me, i, multicast_packets_us);
				break;
			case 14: // Downstream_Multicast_Packets
				meinfo_me_attr_update_pm(me, i, multicast_packets_ds);
				break;
			case 15: // Upstream_Broadcast_Packets
				meinfo_me_attr_update_pm(me, i, broadcast_packets_us);
				break;
			case 16: // Downstream_Broadcast_Packets
				meinfo_me_attr_update_pm(me, i, broadcast_packets_ds);
				break;
			default:
				continue;//do nothing
			}
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_alu_65313 = {
	.get_hw		= meinfo_hw_65313_get
};
