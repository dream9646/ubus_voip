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
 * File    : meinfo_cb_11.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "util.h"
#include "switch.h"
//classid 11 9.5.1 Physical_path_termination_point_Ethernet_UNI

static int
meinfo_cb_11_set_admin_lock(struct me_t *me, int lockval)
{
	struct attr_value_t attr={ .data=lockval?1:0, .ptr=NULL, };	
	meinfo_me_attr_set(me, 5, &attr, 0);	// admin lock

	if (omci_env_g.adminlock_trigger_opstate)
		meinfo_me_attr_set(me, 6, &attr, 0);	// operation state

	meinfo_me_set_child_admin_lock(me, 264, lockval);	// UNI-G
	return 0;
}

static int 
meinfo_cb_11_is_admin_locked(struct me_t *me)
{
	struct me_t *parent_me;

	if( meinfo_util_me_attr_data(me, 5))
		return 1;

	parent_me=me_related_find_parent_me(me, 6);	//PPTP-UNI 's parent is circuit pack classid 6	
	if(parent_me){
		struct meinfo_t *miptr=meinfo_util_miptr(parent_me->classid);
		if (miptr && miptr->callback.is_admin_locked_cb)
			return miptr->callback.is_admin_locked_cb(parent_me);
	}
	return 0;
}

static int 
meinfo_cb_11_set(struct me_t *me, unsigned char attr_mask[2])
{

	struct switch_pptp_eth_uni_data updata;	
	
	updata.expected_type =  meinfo_util_me_attr_data(me, 1);
	updata.auto_detect_cfg =  meinfo_util_me_attr_data(me, 3);
	updata.ethernet_loopback =  meinfo_util_me_attr_data(me, 4);
	updata.max_frame_size =  meinfo_util_me_attr_data(me, 8);
	updata.dte_dce_ind =  meinfo_util_me_attr_data(me, 9);
	updata.pause_time =  meinfo_util_me_attr_data(me, 10);
	updata.bridge_or_router_cfg =  meinfo_util_me_attr_data(me, 11);
	updata.pppoe_filter =  meinfo_util_me_attr_data(me, 14);
	updata.power_control =  meinfo_util_me_attr_data(me, 15);

	if(switch_hw_g.port_inf_update!=NULL)
		switch_hw_g.port_inf_update(me->meid, &updata);
	
	return meinfo_me_set_arc_timestamp(me, attr_mask, 12, 13);
}

static int 
meinfo_cb_11_is_arc_enabled(struct me_t *me)
{
	return meinfo_me_check_arc_interval(me, 12, 13);
}

static int 
meinfo_cb_11_create(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short meid =  me->meid;
	if(switch_hw_g.port_inf_set)
		switch_hw_g.port_inf_set(meid,1);
	return 0;	
}

static int 
meinfo_cb_11_delete(struct me_t *me)
{
	unsigned short meid =  me->meid;
	if(switch_hw_g.port_inf_set)
		switch_hw_g.port_inf_set(meid,0);
	return 0;	
}

struct meinfo_callback_t meinfo_cb_11 = {
	.create_cb                      = meinfo_cb_11_create,
	.delete_cb                      = meinfo_cb_11_delete,
	.set_cb				= meinfo_cb_11_set,
	.set_admin_lock_cb	= meinfo_cb_11_set_admin_lock,
	.is_admin_locked_cb	= meinfo_cb_11_is_admin_locked,
	.is_arc_enabled_cb		= meinfo_cb_11_is_arc_enabled,
};

