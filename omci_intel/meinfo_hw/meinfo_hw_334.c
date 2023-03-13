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
 * File    : meinfo_hw_334.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo_hw.h"
#include "util.h"
#include "er_group_hw.h"
#include "switch.h"
#include "gpon_sw.h"
#include "tm.h"
#include "me_related.h"

#define CONTROL_CONTINUOUS_ACCUMULATION         0x1
#define CONTROL_COLLECT_DOWNSTREAM              0x2

int meinfo_hw_334_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int i,ret;
	struct me_t *parent_me=NULL;
	struct switch_ethernet_frame_pm_ud_t ethernet_frame_pm;
	unsigned int crc_errored_packets = 0;

	// read control block information
	// meid of Threshold_Data_3        
	int threshold_meid = htons(((unsigned short *) meinfo_util_me_attr_ptr(me, 2))[0]);
	// classid of associated port
	int parent_classid = htons(((unsigned short *) meinfo_util_me_attr_ptr(me, 2))[1]);
	// meid of associated port
	int parent_meid = htons(((unsigned short *) meinfo_util_me_attr_ptr(me, 2))[2]);
	// process accumulation disable
	unsigned short accum = htons(((unsigned short *) meinfo_util_me_attr_ptr(me, 2))[3]);
	// check continuous accumulation bit
	unsigned char cont_accum = (htons(((unsigned short *) meinfo_util_me_attr_ptr(me, 2))[5]) & CONTROL_CONTINUOUS_ACCUMULATION) ? 1 : 0;
	// check upstream(1)/downstream(0) bit
	unsigned char dir = (htons(((unsigned short *) meinfo_util_me_attr_ptr(me, 2))[5]) & CONTROL_COLLECT_DOWNSTREAM) ? 0 : 1;

	if((parent_me=meinfo_me_get_by_meid(parent_classid, parent_meid)) == NULL) {
		dbprintf(LOG_ERR, "Can't find parent me (classid:%d, meid:0x%x)\n", parent_classid, parent_meid);
		return -1;
	}

	dbprintf(LOG_DEBUG, "ME%d, threshold meid-%d parent classid-%d, parent meid-0x%x, %s\n", me->classid, threshold_meid, parent_classid, parent_meid, (dir)?"upstream":"downstream");
	
	memset( &ethernet_frame_pm, 0, sizeof(struct switch_ethernet_frame_pm_ud_t) );
	
	if(parent_classid==11 || parent_classid==47 || parent_classid==263 || parent_classid==329) {
		struct switch_port_info_t sw_port_info;
		memset(&sw_port_info, 0x00, sizeof(struct switch_port_info_t));
		if(parent_classid==263) {
			sw_port_info.bridge_port_me = parent_me;
			sw_port_info.port_type = ENV_BRIDGE_PORT_TYPE_WAN;
			sw_port_info.port_type_index = 0;
			sw_port_info.logical_port_id = switch_get_wan_logical_portid();
		} else if(parent_classid==329) {
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
				sw_port_info.bridge_port_me = parent_me;
				sw_port_info.port_type = ENV_BRIDGE_PORT_TYPE_WAN;
				sw_port_info.port_type_index = 0;
				sw_port_info.logical_port_id = switch_get_wan_logical_portid();
			} else {
				sw_port_info.bridge_port_me = parent_me;
				sw_port_info.port_type = ENV_BRIDGE_PORT_TYPE_CPU;
				sw_port_info.port_type_index = 0;
				sw_port_info.logical_port_id = switch_get_cpu_logical_portid(0);
			}
		} else {
			if(switch_get_port_info_by_me(parent_me, &sw_port_info) < 0) {
				dbprintf(LOG_ERR,"get port_info error (classid:%d, meid:0x%x)\n", parent_classid, parent_meid);
				return -1;
			}
		}
		
		// get value from HW depend on different switch configure
		if (dir) // upstream
		{
			if( (ret=switch_hw_g.eth_pm_data_upstream(sw_port_info.logical_port_id, &ethernet_frame_pm)) !=0 ) {
				dbprintf(LOG_ERR, "ret = %d\n", ret);
				return -1;
			}
			crc_errored_packets = ethernet_frame_pm.crc_errored_packets;
		}
		else // downstream
		{
			// get value from WAN port for CRC errored counter (CRC is always RX only, no TX)
			if( (ret=switch_hw_g.eth_pm_data_downstream(switch_get_wan_logical_portid(), &ethernet_frame_pm)) !=0 ) {
				dbprintf(LOG_ERR, "ret = %d\n", ret);
				return -1;
			}
			crc_errored_packets = ethernet_frame_pm.crc_errored_packets;
			if( (ret=switch_hw_g.eth_pm_data_downstream(sw_port_info.logical_port_id, &ethernet_frame_pm)) !=0 ) {
				dbprintf(LOG_ERR, "ret = %d\n", ret);
				return -1;
			}
		}
		// for calix case, get associated gem port byte/packet counters for veip pm me
		if((parent_classid==329) && (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)) {
			struct me_t *veip_me = meinfo_me_get_by_meid(parent_classid, parent_meid), *gem_me = NULL;
			struct meinfo_t *miptr = meinfo_util_miptr(268);
			list_for_each_entry(gem_me, &miptr->me_instance_list, instance_node) {
				if (me_related(veip_me, gem_me)) { // 329 -> 268
					struct gpon_counter_dsflow_t counter_dsflow;
					struct gpon_counter_usflow_t counter_usflow;
					int flowid = -1;
					if((flowid=gem_flowid_get_by_gem_me(gem_me, 0))>=0) {
						gpon_hw_g.pm_refresh(0); // (0/1): reset
						memset(&counter_dsflow, 0, sizeof(counter_dsflow));
						memset(&counter_usflow, 0, sizeof(counter_usflow));
						gpon_hw_g.pm_dsflow_get(flowid, &counter_dsflow);
						gpon_hw_g.pm_usflow_get(flowid, &counter_usflow);
						ethernet_frame_pm.octets += (dir) ? counter_usflow.tx_gem_byte : counter_dsflow.rx_gem_byte;
						ethernet_frame_pm.packets += (dir) ? counter_usflow.tx_eth_cnt : counter_dsflow.rx_eth_pkt;
					}
				}
			}
		}
	} else if (parent_classid==46) { // for mac bridge, accumulate all bridge ports in this bridge
		struct me_t *mac_bridge_me = meinfo_me_get_by_meid(45, parent_meid);
		struct me_t *bridge_port_me = NULL;
		struct meinfo_t *miptr = meinfo_util_miptr(47);
		struct switch_port_info_t sw_port_info;
		memset(&sw_port_info, 0x00, sizeof(struct switch_port_info_t));
		list_for_each_entry(bridge_port_me, &miptr->me_instance_list, instance_node) {
			if (me_related(bridge_port_me, mac_bridge_me)) {
				if(switch_get_port_info_by_me(bridge_port_me, &sw_port_info)==0) {
					struct switch_ethernet_frame_pm_ud_t pm;
					memset(&pm, 0x00, sizeof(struct switch_ethernet_frame_pm_ud_t));
					if (dir) {
						switch_hw_g.eth_pm_data_upstream(sw_port_info.logical_port_id, &pm);
					} else {
						switch_hw_g.eth_pm_data_downstream(sw_port_info.logical_port_id, &ethernet_frame_pm);
					}
					crc_errored_packets += pm.crc_errored_packets;
					ethernet_frame_pm.drop_events += pm.drop_events;
					ethernet_frame_pm.octets += pm.octets;
					ethernet_frame_pm.packets += pm.packets;
					ethernet_frame_pm.broadcast_packets += pm.broadcast_packets;
					ethernet_frame_pm.multicast_packets += pm.multicast_packets;
					ethernet_frame_pm.undersize_packets += pm.undersize_packets;
					ethernet_frame_pm.oversize_packets += pm.oversize_packets;
					ethernet_frame_pm.packets_64_octets += pm.packets_64_octets;
					ethernet_frame_pm.packets_65_to_127_octets += pm.packets_65_to_127_octets;
					ethernet_frame_pm.packets_128_to_255_octets += pm.packets_128_to_255_octets;
					ethernet_frame_pm.packets_256_to_511_octets += pm.packets_256_to_511_octets;
					ethernet_frame_pm.packets_512_to_1023_octets += pm.packets_512_to_1023_octets;
					ethernet_frame_pm.packets_1024_to_1518_octets += pm.packets_1024_to_1518_octets;
				}
			}
		}
	} else if (parent_classid==266 || parent_classid==281) { // for gem port, only support byte/packet counters
		struct meinfo_t *miptr = meinfo_util_miptr(268);
		struct me_t *gem_port_me = NULL;
		list_for_each_entry(gem_port_me, &miptr->me_instance_list, instance_node) {
			if (me_related(parent_me, gem_port_me)) {
				struct gpon_counter_dsflow_t counter_dsflow;
				struct gpon_counter_usflow_t counter_usflow;
				int flowid = -1;
				if((flowid=gem_flowid_get_by_gem_me(gem_port_me, 0))>=0) {
					gpon_hw_g.pm_refresh(0); // (0/1): reset
					memset(&counter_dsflow, 0, sizeof(counter_dsflow));
					memset(&counter_usflow, 0, sizeof(counter_usflow));
					gpon_hw_g.pm_dsflow_get(flowid, &counter_dsflow);
					gpon_hw_g.pm_usflow_get(flowid, &counter_usflow);
					ethernet_frame_pm.octets = (dir) ? counter_usflow.tx_gem_byte : counter_dsflow.rx_gem_byte;
					ethernet_frame_pm.packets = (dir) ? counter_usflow.tx_eth_cnt : counter_dsflow.rx_eth_pkt;
				}
				break;
			}
		}
	} else { // classid 98, 162
		dbprintf(LOG_ERR,"unsupport classid (classid:%d, meid:0x%x)\n", parent_classid, parent_meid);
		return 0;
	}

	// test global enable/disable
	if (accum & 0x4000)
	{
		me->pm_is_accumulated_mask[0] = 0;
		me->pm_is_accumulated_mask[1] = 0;
	} else {
		me->pm_is_accumulated_mask[0] =	((~accum&0x1)>>0<<5) | ((~accum&0x2)>>1<<4) | \
						((~accum&0x4)>>2<<3) | ((~accum&0x8)>>3<<2) | \
						((~accum&0x10)>>4<<1) | ((~accum&0x20)>>5<<0); // 3~8
		me->pm_is_accumulated_mask[1] = ((~accum&0x40)>>6<<7) | ((~accum&0x80)>>7<<6) | \
						((~accum&0x100)>>8<<5) | ((~accum&0x200)>>9<<4) | \
						((~accum&0x400)>>10<<3) | ((~accum&0x800)>>11<<2) | \
						((~accum&0x1000)>>12<<1) | ((~accum&0x2000)>>13<<0); // 9~16
	}
	// clear counters
	if (accum & 0x8000)
	{
		for (i = 3; i <= meinfo_util_get_attr_total(me->classid); i++)
			meinfo_me_attr_clear_pm(me, i);
		switch_hw_g.pm_refresh(1);
	}

	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			struct attr_value_t attr;
			attr.ptr = NULL;
			switch (i)
			{
				//case 1: // Interval_end_time;
				//case 2: // Control
				//    break;
				case 3: // Drop_events
					if(cont_accum)
						attr.data = ethernet_frame_pm.drop_events;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.drop_events);
					break;
				case 4: // Octets
					if(cont_accum)
						attr.data = ethernet_frame_pm.octets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.octets);
					break;
				case 5: // Frames
					if(cont_accum)
						attr.data = ethernet_frame_pm.packets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets);
					break;
				case 6: // Broadcast_frames
					if(cont_accum)
						attr.data = ethernet_frame_pm.broadcast_packets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.broadcast_packets);
					break;
				case 7: // Multicast_frames
					if(cont_accum)
						attr.data = ethernet_frame_pm.multicast_packets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.multicast_packets);
					break;
				case 8: // CRC_errored_frames
					if(cont_accum)
						attr.data = crc_errored_packets;
					else
						meinfo_me_attr_update_pm(me, i, crc_errored_packets);
					break;
				case 9: // Undersize_frames
					if(cont_accum)
						attr.data = ethernet_frame_pm.undersize_packets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.undersize_packets);
					break;
				case 10: // Oversize_frames
					if(cont_accum)
						attr.data = ethernet_frame_pm.oversize_packets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.oversize_packets);
					break;
				case 11: // Frames_64_octets
					if(cont_accum)
						attr.data = ethernet_frame_pm.packets_64_octets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_64_octets);
					break;
				case 12: // Frames_65_to_127_octets
					if(cont_accum)
						attr.data = ethernet_frame_pm.packets_65_to_127_octets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_65_to_127_octets);
					break;
				case 13: // Frames_128_to_255_octets
					if(cont_accum)
						attr.data = ethernet_frame_pm.packets_128_to_255_octets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_128_to_255_octets);
					break;
				case 14: // Frames_256_to_511_octets
					if(cont_accum)
						attr.data = ethernet_frame_pm.packets_256_to_511_octets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_256_to_511_octets);
					break;
				case 15: // Frames_512_to_1023_octets
					if(cont_accum)
						attr.data = ethernet_frame_pm.packets_512_to_1023_octets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_512_to_1023_octets);
					break;
				case 16: // Frames_1024_to_1518_octets
					if(cont_accum)
						attr.data = ethernet_frame_pm.packets_1024_to_1518_octets;
					else
						meinfo_me_attr_update_pm(me, i, ethernet_frame_pm.packets_1024_to_1518_octets);
					break;
				default:
				continue;//do nothing
			}
			if(cont_accum) {
				meinfo_me_attr_set(me, i, &attr, 0);
				if (attr.ptr) free_safe(attr.ptr);
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

struct meinfo_hardware_t meinfo_hw_334 = {
	.get_hw		= meinfo_hw_334_get,
};
