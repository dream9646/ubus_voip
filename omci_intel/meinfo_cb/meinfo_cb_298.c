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
 * File    : meinfo_cb_298.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 298 9.3.18 Dot1_rate_limiter

static int
get_classid_by_298_tp_type(unsigned char tp_type)
{
	switch (tp_type) {
	case 1: return 45;	// 45 MAC bridge service profile
	case 2: return 130;	// 802.1p mapper service
	}
	dbprintf(LOG_ERR, "tp_type %d is undefined\n", tp_type);	
	return -1;
}

static int 
meinfo_cb_298_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==1) {
		return get_classid_by_298_tp_type(meinfo_util_me_attr_data(me, 2));
	}
	return -1;
}

static int 
meinfo_cb_298_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==2) {		// tp type
		if (get_classid_by_298_tp_type(attr->data)<0)
			return 0;
	}
	return 1;
}

struct meinfo_callback_t meinfo_cb_298 = {
	.get_pointer_classid_cb	= meinfo_cb_298_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_298_is_attr_valid,
};

