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
 * Module  : proprietary_calix
 * File    : meinfo_cb_65288.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 65288 9.99.26 OamMeg

static int 
meinfo_cb_65288_get(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 8)) { // RemoteMepTSize
		struct attr_value_t attr;
		attr.data = meinfo_me_attr_get_table_entry_count(me, 9);
		meinfo_me_attr_set(me, 8, &attr, 0);
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_calix_65288 = {
	.get_cb		= meinfo_cb_65288_get,
};
