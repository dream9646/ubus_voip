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
 * Module  : meinfo_cb
 * File    : meinfo_cb_277.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "me_related.h"
#include "omciutil_misc.h"
#include "util.h"

//class 277 9.11.1 Priority_queue_G

static int
meinfo_cb_277_pseudo_ts_pointer(struct me_t *me) 
{
	if (omci_env_g.pseudo_ts_pointer_enable) {
		struct attr_value_t attr_ts_pointer={0, NULL};
		meinfo_me_attr_get(me, 7, &attr_ts_pointer);
	
		if (attr_ts_pointer.data!=0) {	// ts pointer has value, so this is us pq
			unsigned short priority = util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 6), 4 * 8, 16, 16);
			struct me_t *ts_me, *ts_neighbor;

			if (priority>7)
				priority=7;

			ts_me= meinfo_me_get_by_meid(278, attr_ts_pointer.data);
			if (ts_me && omciutil_misc_locate_ts_neighbor_by_pq_priority(priority, ts_me, &ts_neighbor)) {
				// try to find a TS in same level with priority equal to this pq
				if (ts_neighbor->meid!=ts_me->meid) {
					dbprintf(LOG_CRIT, "pseudo ts pointer from meid 0x%x to 0x%x in pq 0x%x\n", ts_me->meid, ts_neighbor->meid, me->meid);
					attr_ts_pointer.data = ts_neighbor->meid;
					meinfo_me_attr_set(me, 7, &attr_ts_pointer, 0);
					return 1;
				}
			}
		}
	}

	return 0;
}

static int 
meinfo_cb_277_create(struct me_t *me, unsigned char attr_mask[2])
{
	// check upstream pq for alu workaround
	/*if (me->meid & 0x8000) {
		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
			struct attr_value_t attr_related_port={0, NULL};

			// the fist 2 byte is the associated tcont pointer of this pq
			// alu olt needs this pointer being set as zero
			meinfo_me_attr_get(me, 6, &attr_related_port);
			util_bitmap_set_value(attr_related_port.ptr, 32, 0, 16, 0);
			meinfo_me_attr_set(me, 6, &attr_related_port, 0);

			meinfo_me_attr_release(277, 6, &attr_related_port);
		}		
	}*/

	// attr_mask is always 0 is create
	meinfo_cb_277_pseudo_ts_pointer(me);

	return 0;
}

static int 
meinfo_cb_277_set(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 6) ||	
	    util_attr_mask_get_bit(attr_mask, 7)) {
	    
        	// recalc pesudo pointer when either pq priority or ts pointer is changed
		meinfo_cb_277_pseudo_ts_pointer(me);
		// recalc the gem-pq relation in rate_ctrl mode
		if (omciutil_misc_traffic_management_option_is_rate_control()) {
                	struct meinfo_t *miptr_gem = meinfo_util_miptr(268);
        	        unsigned char gem_attr_mask[2]={0, 0};
                	struct me_t *gem_me;

                	util_attr_mask_set_bit(gem_attr_mask, 2);
                	util_attr_mask_set_bit(gem_attr_mask, 4);                	
                	list_for_each_entry(gem_me, &miptr_gem->me_instance_list, instance_node) {
                	        if (me_related(gem_me, me)) {			
                			if (omciutil_misc_reassign_pq_for_gem_ctp_in_rate_control_mode(gem_me) >0)
        	                		meinfo_me_flush(gem_me, gem_attr_mask);	// trigger er_group_hw related to the gem_me
                		}
                	}
                }
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_277 = {
	.create_cb		= meinfo_cb_277_create,
	.set_cb			= meinfo_cb_277_set,
};
