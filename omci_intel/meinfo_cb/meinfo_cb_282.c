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
 * File    : meinfo_cb_282.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 282 9.8.5 Pseudowire_termination_point

static int 
meinfo_cb_282_set(struct me_t *me, unsigned char attr_mask[2])
{
	return meinfo_me_set_arc_timestamp(me, attr_mask, 14, 15);
}

static int 
meinfo_cb_282_is_arc_enabled(struct me_t *me)
{
	return meinfo_me_check_arc_interval(me, 14, 15);
}

struct meinfo_callback_t meinfo_cb_282 = {
	.set_cb				= meinfo_cb_282_set,
	.is_arc_enabled_cb		= meinfo_cb_282_is_arc_enabled,
};

