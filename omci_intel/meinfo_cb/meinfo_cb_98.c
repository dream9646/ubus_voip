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
 * File    : meinfo_cb_98.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"

//classid 98 9.7.1 Physical_path_termination_point_xDSL_UNI_part_1

static int
meinfo_cb_98_set_admin_lock(struct me_t *me, int lockval)
{
	struct attr_value_t attr={ .data=lockval?1:0, .ptr=NULL, };	
	meinfo_me_attr_set(me, 2, &attr, 0);

	// lock child classid 264 UNI-G
	meinfo_me_set_child_admin_lock(me, 264, lockval);
	return 0;
}

static int 
meinfo_cb_98_is_admin_locked(struct me_t *me)
{
	return me?meinfo_util_me_attr_data(me, 2):0;
}

static int 
meinfo_cb_98_set(struct me_t *me, unsigned char attr_mask[2])
{
	return meinfo_me_set_arc_timestamp(me, attr_mask, 9, 10);
}

static int 
meinfo_cb_98_is_arc_enabled(struct me_t *me)
{
	return meinfo_me_check_arc_interval(me, 9, 10);
}

struct meinfo_callback_t meinfo_cb_98 = {
	.set_cb				= meinfo_cb_98_set,
	.set_admin_lock_cb	= meinfo_cb_98_set_admin_lock,
	.is_admin_locked_cb	= meinfo_cb_98_is_admin_locked,
	.is_arc_enabled_cb		= meinfo_cb_98_is_arc_enabled,
};

