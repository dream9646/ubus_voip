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
 * File    : meinfo_cb_262.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"
#include "batchtab.h"
#include "gpon_sw.h"

//classid 262 9.2.2 T_CONT


static int 
meinfo_cb_262_create(struct me_t *me, unsigned char attr_mask[2])
{
	if (omci_env_g.sim_tcont_enable) {	
		struct attr_value_t attr;
		attr.data = 256 + me->meid % GPON_TCONT_ID_TOTAL;	// fake allocid
		meinfo_me_attr_set(me, 1, &attr, 0);
	}
	return 0;
}

static int 
meinfo_cb_262_set(struct me_t *me, unsigned char attr_mask[2])
{
	batchtab_omci_update("linkready");
	return 0;
}

static int 
meinfo_cb_262_is_ready(struct me_t *me)
{
	struct attr_value_t attr_value;

	meinfo_me_attr_get(me, 1, &attr_value);

	// allocid < 255: for omcc
	// allocid ==255: invalid
	// allocid > 255: for data traffic 
	if (attr_value.data > 255)
	{
		return 1;
	} else {
		return 0;
	}
}

struct meinfo_callback_t meinfo_cb_262 = {
	.create_cb	= meinfo_cb_262_create,
	.set_cb		= meinfo_cb_262_set,
	.is_ready_cb	= meinfo_cb_262_is_ready,
};

