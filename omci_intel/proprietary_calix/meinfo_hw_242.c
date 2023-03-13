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
 * Module  : proprietary_calix
 * File    : meinfo_hw_242.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_hw.h"
#include "util.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "switch.h"
#include "metacfg_adapter.h"
#include "conv.h"

int meinfo_hw_242_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int i,ret;
	struct switch_port_info_t sw_port_info;
	struct switch_counter_port_t sw_port_counter;
	
	if (me == NULL)
	{
		dbprintf(LOG_ERR, "Can't find related info\n");
		return -1;
	}
	
	// get value from HW depend on different switch configure
	memset( &sw_port_counter, 0, sizeof(struct switch_counter_port_t) );
	if (switch_get_port_info_by_uni_num(me->instance_num, &sw_port_info) == 0) {
		if( (ret=switch_hw_g.counter_port_get(sw_port_info.logical_port_id, &sw_port_counter)) !=0 ) {
			dbprintf(LOG_ERR, "ret = %d\n", ret);
			return -1;
		}
	} else {
		dbprintf(LOG_ERR, "Can't get hw information in switch\n");
		return -1;
	}
	
	//dbprintf(LOG_ERR, "me->instance_num=%d, %d\n", me->instance_num, sw_port_info.logical_port_id);
	for (i = 1; i <= meinfo_util_get_attr_total(me->classid); i++)
	{
		if (util_bitmap_get_value(attr_mask, 8*2, i - 1, 1))
		{
			struct attr_value_t attr;
			unsigned char mac[6];
#ifdef OMCI_METAFILE_ENABLE
			struct metacfg_t kv_config;
			char *macstr, mac_key[32];
#endif
			
			attr.ptr = NULL;
			attr.data = 0;
			
			switch (i)
			{
				//case 1: // VideoTxMode;
				//case 2: // IfCountReset
				//case 11: // ForceOperMode
				//    break;
				case 3: // IfInOctets
					attr.data = sw_port_counter.ifInOctets;
					break;
				case 4: // IfInUcastPkts
					attr.data = sw_port_counter.ifInUcastPkts;
					break;
				case 5: // IfInMcastPkts
					attr.data = sw_port_counter.ifInMulticastPkts;
					break;
				case 6: // IfInBcastPkts
					attr.data = sw_port_counter.ifInBroadcastPkts;
					break;
				case 7: // IfOutOctets
					attr.data = sw_port_counter.ifOutOctets;
					break;
				case 8: // IfOutUcastPkts
					attr.data = sw_port_counter.ifOutUcastPkts;
					break;
				case 9: // IfOutMcastPkts
					attr.data = sw_port_counter.ifOutMulticastPkts;
					break;
				case 10: // IfOutBcastPkts
					attr.data = sw_port_counter.ifOutBroadcastPkts;
					break;
				case 12: // PhysAddress
					attr.data=0;
					attr.ptr= malloc_safe(6);
					memset(mac, 0, sizeof(mac));
					
#ifdef OMCI_METAFILE_ENABLE
					//change to refer /var/run/macaddr.conf 
					memset(&kv_config, 0x0, sizeof(struct metacfg_t));
					metacfg_adapter_config_init(&kv_config);
					metacfg_adapter_config_file_load_safe(&kv_config, "/var/run/macaddr.conf");
					snprintf(mac_key, 32, "LAN%d_MAC", sw_port_info.logical_port_id);
					macstr= metacfg_adapter_config_get_value(&kv_config, mac_key, 1);
					if(macstr) {
						macstr=util_quotation_marks_trim(macstr);
						//conv_str_mac_to_mem
						conv_str_mac_to_mem(mac, macstr);
						memcpy(attr.ptr, mac, sizeof(mac));
						meinfo_me_attr_set(me, 12, &attr, 0);
					}
					metacfg_adapter_config_release(&kv_config);
#endif
					break;
				default:
					continue;//do nothing
			}
	
			meinfo_me_attr_set(me, i, &attr, 0);
			if (attr.ptr) free_safe(attr.ptr);
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

struct meinfo_hardware_t meinfo_hw_calix_242 = {
	.get_hw		= meinfo_hw_242_get,
};
