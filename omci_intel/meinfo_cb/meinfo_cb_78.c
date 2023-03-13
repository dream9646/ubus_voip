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
 * File    : meinfo_cb_78.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 78 9.3.12 VLAN_tagging_operation_configuration_data

static int
get_classid_by_78_assoc_type(unsigned char assoc_type)
{
	switch (assoc_type) {
	case 0:	return 11;	// pptp_ethernet_uni
	case 1: return 134;	// ip host config data
	case 2: return 130;	// 802.1p mapper service
	case 3: return 47;	// mac_bridge_port_configuration_data
	case 4: return 98;	// pptp_xdsl_part1
	case 5: return 266;	// gem iw tp
	case 6: return 281;	// mcast gem iwtp
	case 7: return 162;	// pptp moca uni
	case 8: return 91;	// pptp_802.11_uni
	case 9: return 286;	// ethernet flow tp
	case 10:return 11;	// pptp_ethernet_uni
	case 11: return 329;	// virtual_ethernet
	}
	dbprintf(LOG_ERR, "assoc_type %d is undefined\n", assoc_type);	
	return -1;
}

static int 
meinfo_cb_78_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==5) {
		return get_classid_by_78_assoc_type(meinfo_util_me_attr_data(me, 4));
	}
	return -1;
}

static int 
meinfo_cb_78_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==4) {		// tp type
		if (get_classid_by_78_assoc_type(attr->data)<0)
			return 0;
	}
	return 1;
}

struct meinfo_callback_t meinfo_cb_78 = {
	.get_pointer_classid_cb	= meinfo_cb_78_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_78_is_attr_valid,
};
