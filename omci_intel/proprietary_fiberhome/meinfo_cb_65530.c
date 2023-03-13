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
 * File    : meinfo_cb_65530.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "meinfo.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "util.h"

//classid 65530 LOID Authentication

static unsigned char loid_auth_state=0;

static int
meinfo_cb_65530_create(struct me_t *me, unsigned char attr_mask[2])
{
	char value[64];
	struct attr_value_t attr;

	// onu_loid
	if (util_get_value_from_file(GPON_DAT_FILE, "onu_loid", value, 64)>0) {
		attr.ptr = value;
		meinfo_me_attr_set(me, 2, &attr, 0);
	}

	// onu_load_password
	if (util_get_value_from_file(GPON_DAT_FILE, "onu_loid_password", value, 64)>0) {
		attr.ptr = value;
		meinfo_me_attr_set(me, 3, &attr, 0);
	}

	// Auth State
	attr.data=loid_auth_state;
	meinfo_me_attr_set(me, 4, &attr, 0);

	return 0;
}

static int
meinfo_cb_65530_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	
	if (util_attr_mask_get_bit(attr_mask, 4)) {	// Auth State
		attr.data=loid_auth_state;
		meinfo_me_attr_set(me, 4, &attr, 0);
	}
	return 0;
}


static int 
meinfo_cb_65530_set(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 4)) {	// Auth State
		struct attr_value_t attr;
		meinfo_me_attr_get(me, 4, &attr);
		loid_auth_state=(unsigned char)attr.data;
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_fiberhome_65530 = {
	.create_cb	= meinfo_cb_65530_create,
	.get_cb		= meinfo_cb_65530_get,
	.set_cb		= meinfo_cb_65530_set,
};

