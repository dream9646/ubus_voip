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
 * File    : meinfo_hw_50.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "er_group.h"
#include "util.h"
#include "hwresource.h"
#include "switch.h"

static int 
me_65347_update(struct me_t *me_65347, struct switch_mac_tab_entry_t *switch_mac_tab_entry){
	void *me_65347_tab_entry= NULL;

	me_65347_tab_entry = malloc_safe(2);
	if (omci_env_g.untag_reserved_vid == switch_mac_tab_entry->vid)
		util_bitmap_set_value(me_65347_tab_entry, 2*8, 0, 16, 0xffff);	
	else
		util_bitmap_set_value(me_65347_tab_entry, 2*8, 0, 16, switch_mac_tab_entry->vid);	
	meinfo_me_attr_set_table_entry(me_65347,1,me_65347_tab_entry,0);
	if(me_65347_tab_entry)
		free_safe(me_65347_tab_entry);
	return 0;
}

static int
meinfo_hw_50_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_table_header_t *tab_header=NULL; 
	struct me_t *bridge_port;
	struct switch_port_info_t port_info;
	unsigned int entry_num=0, age_time=0;
	unsigned short entry_count = 0;
	struct switch_mac_tab_entry_t switch_mac_tab_entry;
	unsigned char forward;
	struct me_t *me_65347 = NULL;
	char *zero_macaddr[]= {0,0,0,0,0,0};
	void *tab_entry= NULL;
	unsigned int phy_portid, ext_portid;
	if (switch_hw_g.portid_logical_to_private==NULL)
		return -1;

	if (me == NULL || switch_hw_g.mac_l2uc_get_next==NULL) {
		return -1;
	}
	tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me , 1);

	//clear all mac entry information in mac table
	meinfo_me_attr_release_table_all_entries(tab_header);

	bridge_port = meinfo_me_get_by_meid( 47 , me->meid);		
	if ( bridge_port == NULL )
		return -1;

	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME) == 0) {
		me_65347 = meinfo_me_get_by_meid( 65347 , me->meid);
		
		if (me_65347) {
			struct attr_table_header_t *me_65347_tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me_65347 , 1);
			meinfo_me_attr_release_table_all_entries(me_65347_tab_header);
		}
	}
	
	if (switch_get_port_info_by_me(bridge_port , &port_info) <0)
		return -1;
	if (switch_hw_g.mac_aging_get(&age_time) < 0)
		return -1;
	if (switch_hw_g.portid_logical_to_private(port_info.logical_port_id, &phy_portid, &ext_portid) < 0)
		return -1;

	while (entry_count < 8192) {	// mactab entry range 0..2047, use 8192 for safety, better than while(1)
		memset(&switch_mac_tab_entry,0, sizeof(struct switch_mac_tab_entry_t));
		if (switch_hw_g.mac_l2uc_get_next(port_info.logical_port_id, &entry_num, &switch_mac_tab_entry) <= 0)
			break;	// not found or error
	    	if (memcmp(switch_mac_tab_entry.mac_addr, zero_macaddr, 6)==0)
	    		break;	// null mac entry ?, break immediately

		tab_entry = malloc_safe(8);
		forward = (switch_mac_tab_entry.port_bitmap & (1 << phy_portid))?0:1;
		util_bitmap_set_value(tab_entry, 8*8, 0, 1, forward);	
		util_bitmap_set_value(tab_entry, 8*8, 2, 1, switch_mac_tab_entry.is_dynamic);
		if(switch_mac_tab_entry.is_dynamic) { // only meaningful for dynamic entry
			// hw value is 0~7, each is age_time/8
			util_bitmap_set_value(tab_entry, 8*8, 4, 12, ((switch_mac_tab_entry.age)+1)*age_time/8);
		} else {
			util_bitmap_set_value(tab_entry, 8*8, 4, 12, 0);
		}
		int i = 0;
		for ( i = 0 ; i < 6 ; i++)
			util_bitmap_set_value(tab_entry, 8*8, 8*2+i*8, 8, switch_mac_tab_entry.mac_addr[i]);	
		meinfo_me_attr_set_table_entry(me,1,tab_entry,0);
		if (tab_entry)
			free_safe(tab_entry);

		if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_FIBERHOME) == 0) {
			if (me_65347) {
				me_65347_update(me_65347, &switch_mac_tab_entry);
			}
		}

		entry_num++;
		entry_count++;

		// FIXME, KEVIN, do we need to limit total entry get max to 255, so me table content won't be too large?
		if (entry_count>255)
			break;
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_50 = {
	.get_hw		= meinfo_hw_50_get
};
