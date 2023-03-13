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
 * File    : meinfo_hw_65311.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_hw.h"
#include "util.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "switch.h"

static int 
meinfo_hw_65311_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int i;
	struct me_t *bridge_port_me, *private_me;
	struct switch_port_info_t sw_port_info;
 
	if (me == NULL)
	{
		dbprintf(LOG_ERR, "Can't find related internal bridge port\n");
		return -1;
	}
	
	//find MAC_bridge_port_configuration_data for internal bridge port id
	struct meinfo_t *me47_miptr = NULL;
	me47_miptr = meinfo_util_miptr(47);
	bridge_port_me = NULL;
	
	list_for_each_entry(private_me, &me47_miptr->me_instance_list, instance_node) {
		if (meinfo_util_me_attr_data(private_me , 3) == 1 && meinfo_util_me_attr_data(private_me , 4) == me->meid)
		{
			bridge_port_me = private_me;
			private_me = NULL;
			break;
		}				
	}
	
	if (bridge_port_me == NULL)
	{
		dbprintf(LOG_WARNING, "Matching bridge port ME 47 does not exist\n");
		return 0;
	}
		 
	if ((private_me=hwresource_public2private(bridge_port_me)) == NULL)
	{
		dbprintf(LOG_ERR, "Can't find related internal bridge port\n");
		return -1;
	}

	// get value from HW depend on different switch configure
	if (switch_get_port_info_by_me(bridge_port_me, &sw_port_info) != 0) {
		dbprintf(LOG_ERR, "Can't get hw information in switch\n");
		return -1;
	}
	
	if (util_attr_mask_get_bit(attr_mask, 2) || util_attr_mask_get_bit(attr_mask, 3) ||
		util_attr_mask_get_bit(attr_mask, 4) || util_attr_mask_get_bit(attr_mask, 5) ||
		util_attr_mask_get_bit(attr_mask, 6) || util_attr_mask_get_bit(attr_mask, 7) ||
		util_attr_mask_get_bit(attr_mask, 8) || util_attr_mask_get_bit(attr_mask, 9) ||
		util_attr_mask_get_bit(attr_mask, 10) || util_attr_mask_get_bit(attr_mask, 11)) {
		struct switch_counter_port_t sw_port_counter;
		memset(&sw_port_counter, 0, sizeof(struct switch_counter_port_t));
		if(switch_hw_g.counter_port_get(sw_port_info.logical_port_id, &sw_port_counter) !=0)
			return -1;
		
		for (i = 2; i <= 11; i++)
		{
			if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
			{
				struct attr_value_t attr;
				attr.ptr = NULL;
				attr.data = 0;
				
				switch (i)
				{
					case 2:  // IfInDiscards
						attr.data = sw_port_counter.ifInDiscards;
						break;
					case 3:  // IfInErrors
						attr.data = sw_port_counter.etherStatsFragments +
							sw_port_counter.etherStatsOversizePkts +
							sw_port_counter.etherStatsFragments +
							sw_port_counter.etherStatsJabbers +
							sw_port_counter.etherStatsCollisions +
							sw_port_counter.etherStatsCRCAlignErrors + 
							sw_port_counter.etherStatsFragments;
						break;
					case 4:  // IfHCInOctets
						attr.data = sw_port_counter.ifInOctets;
						break;
					case 5:  // IfHCInUcastPkts
						attr.data = sw_port_counter.ifInUcastPkts;
						break;
					case 6:  // IfHCInMcastPkts
						attr.data = sw_port_counter.ifInMulticastPkts;
						break;
					case 7:  // IfHCInBcastPkts
						attr.data = sw_port_counter.ifInBroadcastPkts;
						break;
					case 8:  // IfHCOutOctets
						attr.data = sw_port_counter.ifOutOctets;
						break;
					case 9:  // IfHCOutUcastPkts
						attr.data = sw_port_counter.ifOutUcastPkts;
						break;
					case 10:  // IfHCOutMcastPkts
						attr.data = sw_port_counter.ifOutMulticastPkts;
						break;
					case 11:  // IfHCOutBcastPkts
						attr.data = sw_port_counter.ifOutBroadcastPkts;
						break;
					default:
						continue;//do nothing
				}
				meinfo_me_attr_set(me, i, &attr, 0);
				if (attr.ptr) free_safe(attr.ptr);
			}
		}
	}
	
	if (util_attr_mask_get_bit(attr_mask, 12) || util_attr_mask_get_bit(attr_mask, 13) ||
		util_attr_mask_get_bit(attr_mask, 14) || util_attr_mask_get_bit(attr_mask, 15)) {
		unsigned long long rate_us = 0, rate_ds = 0;
		if(switch_hw_g.eth_pm_rate_status_get(sw_port_info.logical_port_id, &rate_us, &rate_ds) != 0)
			return -1;
		
		for (i = 12; i <= 15; i++)
		{
			if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
			{
				struct attr_value_t attr;
				attr.ptr = NULL;
				attr.data = 0;
				
				switch (i)
				{
					case 12:  // IfInRate
					case 13:  // IfInMaxRate
						attr.data = rate_us/1000; // bps->Kbps
						break;
					case 14:  // IfOutRate
					case 15:  // IfOutMaxRate
						attr.data = rate_ds/1000; // bps->Kbps
						break;
					default:
						continue;//do nothing
				}
				meinfo_me_attr_set(me, i, &attr, 0);
				if (attr.ptr) free_safe(attr.ptr);
			}
		}
	}
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) {
		// PM Get with no attributes indicates that the counts associated with the ME should be reset
		if(attr_mask[0] == 0 && attr_mask[1] == 0) {
			for (i = 2; i <= meinfo_util_get_attr_total(me->classid); i++)
				meinfo_me_attr_clear_pm(me, i);
			switch_hw_g.pm_refresh(1);
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65311 = {
	.get_hw		= meinfo_hw_65311_get,
};
