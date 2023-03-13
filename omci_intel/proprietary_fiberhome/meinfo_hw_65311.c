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
 * File    : meinfo_hw_65282.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "er_group.h"
#include "util.h"
#include "hwresource.h"
//#include "switch.h"
#include "cpuport.h"
#include "switch.h"
#include "mcast.h"
#include "counter/igmp_counter_simple.h"

static int igmp_counter_65311_summer(struct igmp_pkt_vid_msg_counter_t *pos, void *data)
{
        unsigned int *tmp = (unsigned int *)data;
        
        // TX counters
        tmp[1] += pos->general_query;
        tmp[2] += pos->specific_query;
        // RX counters
        tmp[0] += pos->received_igmp; 
        tmp[3] += pos->total_join;
        tmp[4] += pos->successful_join ;
        tmp[5] += pos->total_leave;
        
        return 0;
}

static int 
meinfo_hw_65311_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;

	if (me == NULL) {
		dbprintf(LOG_ERR, "invalid me\n");
		return -1;
	}


	struct meinfo_t *miptr= meinfo_util_miptr(422);		
	if (miptr==NULL) {
		dbprintf(LOG_ERR, "class=%d, null miptr?\n", 311);
		return -1;
	}		

	struct me_t *private_bridge_port=NULL, *pos=NULL;
	list_for_each_entry(pos, &miptr->me_instance_list, instance_node) {
		if( meinfo_util_me_attr_data(pos, 2) == me->meid) {
			private_bridge_port = pos;
			break;
		}
	}
	if (private_bridge_port == NULL) {
		dbprintf(LOG_ERR, "class=%d private bridge port not found.\n", 65311);
		return -1;
	}		
		
	unsigned int received_igmp=0;
	unsigned int general_query=0;
	unsigned int specific_query=0;
	unsigned int total_join=0;
	unsigned int successful_join=0;
	unsigned int total_leave=0;

	// this ME is hardwired to simple counter module
        // if 'simple' module is not used, all return 0 as different counter
        // module have different data structure
	struct igmp_counter_module_t *m=NULL;
        
        list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
                if (strcmp(m->module_name(), "simple") != 0)
			continue;

                // call module iterator
                unsigned int tmp[6] =  { 0, 0, 0, 0, 0, 0 };
                m->igmp_counter_iterator(igmp_counter_65311_summer, &tmp);
                received_igmp = tmp[0];
                general_query = tmp[1];
                specific_query = tmp[2];
                total_join = tmp[3];
                successful_join = tmp[4];
                total_leave = tmp[5]; 
        }

	attr.ptr=NULL;
	if (util_attr_mask_get_bit(attr_mask, 1)) {	
		attr.data = total_join - successful_join;
		meinfo_me_attr_set(me, 1, &attr, 0);	// check_avc==0 means none update avc_change_mask
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		attr.data = total_leave ;
		meinfo_me_attr_set(me, 2, &attr, 0);	// check_avc==0 means none update avc_change_mask
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {	
		attr.data = general_query;
		meinfo_me_attr_set(me, 3, &attr, 0);	// check_avc==0 means none update avc_change_mask
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		attr.data = specific_query;
		meinfo_me_attr_set(me, 4, &attr, 0);	// check_avc==0 means none update avc_change_mask
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) {	
		attr.data = received_igmp - total_join - total_leave;
		meinfo_me_attr_set(me, 5, &attr, 0);	// check_avc==0 means none update avc_change_mask
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65311 = {
	.get_hw		= meinfo_hw_65311_get
};
