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
 * File    : meinfo_cb_266.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 266 9.2.4 GEM_interworking_termination_point

static int
get_service_profile_classid_by_266_iw_option(unsigned char iw_option)
{
	switch (iw_option) {
	case 0:	return 21;	// CES_service_profile_G
	case 1: return 45;	// MAC_bridge_service_profile
	case 3: return 68;	// IP_router_service_profile
	case 4: return 128;	// Video_return_path_service_profile
	case 5: return 130;	// 802.1p_mapper_service_profile
	case 6: return 0;	// Downstream_broadcast
	}
	dbprintf(LOG_ERR, "iw_option %d is undefined\n", iw_option);	
	return -1;
}

static int
get_gal_profile_classid_by_266_iw_option(unsigned char iw_option)
{
	switch (iw_option) {
	case 0:	return 271;	// GAL_TDM_profile
	case 1: return 272;	// GAL_Ethernet_profile
	case 3: return 272;	// GAL_Ethernet_profile (for data service)
	case 4: return 272;	// GAL_Ethernet_profile (for video return path)
	case 5: return 272;	// GAL_Ethernet_profile (for 802.1p mapper service)
	case 6: return 0;	// Downstream_broadcast
	}
	dbprintf(LOG_ERR, "iw_option %d is undefined\n", iw_option);	
	return -1;
}

static int 
meinfo_cb_266_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==3) {
		return get_service_profile_classid_by_266_iw_option(meinfo_util_me_attr_data(me, 2));
	} else if (attr_order==4) { //?
		return 0;
	} else if (attr_order==7) {
		return get_gal_profile_classid_by_266_iw_option(meinfo_util_me_attr_data(me, 2));
	}
	return -1;
}

static int 
meinfo_cb_266_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==2) {		// tp type
		if (get_service_profile_classid_by_266_iw_option(attr->data)<0)
			return 0;
	}
	return 1;
}

struct meinfo_callback_t meinfo_cb_266 = {
	.get_pointer_classid_cb	= meinfo_cb_266_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_266_is_attr_valid,
};
