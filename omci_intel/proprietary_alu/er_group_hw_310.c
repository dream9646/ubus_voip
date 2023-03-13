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
 * Module  : er_group_hw
 * File    : er_group_hw_310.c
 *
 ******************************************************************/
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include "list.h"
#include "util.h"
#include "meinfo.h"
#include "hwresource.h"
#include "er_group.h"
#include "me_related.h"
#include "notify_chain.h"
#include "mcast_const.h"
#include "mcast.h"
#include "cpuport.h"
#include "switch.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "igmp_omci.h"
#include "mcast_switch.h"
#include "mcast_timer.h"
#include "fwk_timer.h"
#include "proprietary_alu.h"

extern unsigned short alu_olt_ethertype;

// 310 Multicast_subscriber_config_info

//310@6
int
er_group_hw_vid_to_ethertype(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	// This er_group only applies on ALU old OLT (FTTU7342)
	if(proprietary_alu_get_olt_version() != 0) {
		// Return 0 directly if this is not ALU FTTU7342
		return 0;
	} else {
		struct me_t *meptr = meinfo_me_get_by_meid(65302, 0);
		
		if(meptr && meinfo_util_me_attr_data(meptr, 1) == 1) { // Activate this workaround when ME65302 RF_Filter==1
			struct attr_table_header_t *tab_header;
			struct attr_table_entry_t *tab_entry;
			unsigned short vlan_id=0;
			unsigned short ethertype=0;
			
			//mcast_me = er_attr_group_util_attrinst2me(attr_inst, 0);
			//tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(mcast_subscribe_config_me , 6);
			tab_header = attr_inst->er_attr_instance[0].attr_value.ptr;
			if ( tab_header == NULL)
				return 0;
		
			list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
			
				if (tab_entry->entry_data_ptr == NULL) 
					continue;
					
				vlan_id = (unsigned short)util_bitmap_get_value(tab_entry->entry_data_ptr, 20*8, 2*8, 16);
		
				if ( vlan_id == 4081 )
					ethertype = 4081;
				else if( vlan_id == 4082 ) 
					ethertype = 4082;
				else if( vlan_id == 4083 ) 
					ethertype = 4083;
				else if( vlan_id == 4084 ) 
					ethertype = 4084;
				else if( vlan_id == 4085 ) 
					ethertype = 4085;
				else if( vlan_id == 4086 ) 
					ethertype = 4086;
				else if( vlan_id == 4087 ) 
					ethertype = 4087;
				else if( vlan_id == 4088 ) 
					ethertype = 4088;
				
			}
		
			if ( ethertype == 0 ) 
				alu_olt_ethertype = 0;
			else 
				alu_olt_ethertype = ethertype;
		}
	}
	
	return 0;
}
