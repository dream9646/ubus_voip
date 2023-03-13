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
 * File    : meinfo_cb_65317.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "er_group.h"
#include "omcimsg.h"
#include "util.h"

// 9.99.18 65317 CalixRgConfig

static int 
meinfo_cb_65317_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	unsigned char zero_attr_mask[2] = {0, 0};
	// trigger meinfo_hw get to init me with hw current value
	if (miptr->hardware.get_hw)
		miptr->hardware.get_hw(me, zero_attr_mask);
	return 0;
}

struct meinfo_callback_t meinfo_cb_calix_65317 = {
	.create_cb		= meinfo_cb_65317_create,
};

