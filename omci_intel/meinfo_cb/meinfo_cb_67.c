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
 * File    : meinfo_cb_67.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 67 9.4.3 IP_port_configuration_data

static int
get_classid_by_67_tp_type(unsigned char tp_type)
{
	switch (tp_type) {
	case 1:	return 11;	// pptp_ethernet_uni
	case 2: return 266;	// gem iw tp
	case 3: return 47;	// mac bridge port config data
	case 4: return 130;	// 802.1p mapper service
	case 5: return 98;	// pptp_xdsl_part1
	case 6: return 281;	// mcast gem iwtp
	case 7: return 162;	// pptp moca uni
	case 8: return 91;	// pptp_802.11_uni
	case 9: return 286;	// ethernet flow tp
	}
	dbprintf(LOG_ERR, "tp_type %d is undefined\n", tp_type);	
	return -1;
}

static int 
meinfo_cb_67_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==3) { 
		return get_classid_by_67_tp_type(meinfo_util_me_attr_data(me, 2));
	}
	return -1;
}

static int 
meinfo_cb_67_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==2) {		// tp type
		if (get_classid_by_67_tp_type(attr->data)<0)
			return 0;
	}
	return 1;
}

static int
meinfo_cb_67_set_admin_lock(struct me_t *me, int lockval)
{
	struct attr_value_t attr={ .data=lockval?1:0, .ptr=NULL, };	
	meinfo_me_attr_set(me, 7, &attr, 0);
	return 0;
}

static int 
meinfo_cb_67_is_admin_locked(struct me_t *me)
{
	return me?meinfo_util_me_attr_data(me, 7):0;
}

struct meinfo_callback_t meinfo_cb_67 = {
	.set_admin_lock_cb	= meinfo_cb_67_set_admin_lock,
	.is_admin_locked_cb	= meinfo_cb_67_is_admin_locked,
	.get_pointer_classid_cb	= meinfo_cb_67_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_67_is_attr_valid,
};

