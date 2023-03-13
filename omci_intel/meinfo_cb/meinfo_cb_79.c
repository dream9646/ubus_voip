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
 * File    : meinfo_cb_79.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 79 9.3.7 MAC bridge port filter pre-assign table

static unsigned short 
meinfo_cb_79_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(47, instance_num);
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 47, instance_num);
		return instance_num;
	}
	return parent_me->meid;
}

static int 
meinfo_cb_79_create(struct me_t *me, unsigned char attr_mask[2])
{
	/* 	
		To support downstream unknown multicast,
		have to prohibit UNI ports to forward multicast traffic,
		and redirect downstream multicast to CPU ports.
		(Except for coming from multicast gem port)
		Default IPv4/IPv6 multicast in UNI ports must be set to "Filter".
	*/
	
	struct me_t *parent_me=meinfo_me_get_by_meid(47, me->meid);
	extern int igmpproxytask_loop_g;
	
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 47, me->meid);
		return -1;
	}
	
	if(igmpproxytask_loop_g && meinfo_util_me_attr_data(parent_me, 3) == 1) { // TP type == Physical path termination point Ethernet UNI
		struct attr_value_t attr;
		attr.data = (igmpproxytask_loop_g) ? 1 : 0; // Filter multicast stream on UNI ports if IGMP task is on
		meinfo_me_attr_set(me, 1, &attr, 0); // IPv4_multicast_filtering
#if 0 // TODO: Do not handle IPv6 multicast since MLD snooping is not ready now
		meinfo_me_attr_set(me, 2, &attr, 0); // IPv6_multicast_filtering
#endif
	}
	
	return 0;
}

static int
meinfo_cb_79_set(struct me_t *me, unsigned char attr_mask[2])
{
	/* 	
		To support downstream unknown multicast,
		have to prohibit UNI ports to forward multicast traffic,
		and redirect downstream multicast to CPU ports.
		(Except for coming from multicast gem port)
		Any "set action" for IPv4/IPv6 multicast in UNI ports must be reset to "Filter".
	*/
	
	if (util_attr_mask_get_bit(attr_mask, 1)) {	// IPv4_multicast_filtering
		struct me_t *parent_me=meinfo_me_get_by_meid(47, me->meid);
		extern int igmpproxytask_loop_g;
		
		if (!parent_me) {
			dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 47, me->meid);
			return -1;
		}
		
		if(igmpproxytask_loop_g && meinfo_util_me_attr_data(parent_me, 3) == 1) { // TP type == Physical path termination point Ethernet UNI
			struct attr_value_t attr;
			attr.data = 1; // Filter multicast stream on UNI ports if IGMP task is on
			meinfo_me_attr_set(me, 1, &attr, 0); // IPv4_multicast_filtering
		}
	}
#if 0 // TODO: Do not handle IPv6 multicast since MLD snooping is not ready now
	if (util_attr_mask_get_bit(attr_mask, 2)) {	// IPv6_multicast_filtering
		struct me_t *parent_me=meinfo_me_get_by_meid(47, me->meid);
		extern int igmpproxytask_loop_g;
		
		if (!parent_me) {
			dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 47, me->meid);
			return -1;
		}
		
		if(igmpproxytask_loop_g && meinfo_util_me_attr_data(parent_me, 3) == 1) { // TP type == Physical path termination point Ethernet UNI
			struct attr_value_t attr;
			attr.data = 1; // Filter multicast stream on UNI ports if IGMP task is on
			meinfo_me_attr_set(me, 2, &attr, 0); // IPv6_multicast_filtering
		}
	}
#endif
	return 0;
}

struct meinfo_callback_t meinfo_cb_79 = {
	.meid_gen_cb	= meinfo_cb_79_meid_gen,
	.create_cb	= meinfo_cb_79_create,
	.set_cb		= meinfo_cb_79_set,
};

