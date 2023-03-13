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
 * File    : meinfo_cb_83.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"

//classid 83 9.13.3 Physical_path_termination_point_LCT_UNI

static int
meinfo_cb_83_set_admin_lock(struct me_t *me, int lockval)
{
	struct attr_value_t attr={ .data=lockval?1:0, .ptr=NULL, };	
	meinfo_me_attr_set(me, 1, &attr, 0);

	// lock child classid 264 UNI-G
	meinfo_me_set_child_admin_lock(me, 264, lockval);
	return 0;
}

static int 
meinfo_cb_83_is_admin_locked(struct me_t *me)
{
	return me?meinfo_util_me_attr_data(me, 1):0;
}

struct meinfo_callback_t meinfo_cb_83 = {
	.set_admin_lock_cb	= meinfo_cb_83_set_admin_lock,
	.is_admin_locked_cb	= meinfo_cb_83_is_admin_locked,
};

