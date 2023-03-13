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
 * File    : meinfo_cb_413.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "util.h"

//classid 413 Internal pptp ethernet uni

static int 
meinfo_cb_413_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=NULL;

	if ((miptr=meinfo_util_miptr(413)) == NULL) {
		dbprintf(LOG_ERR, "Fail, because of miptr NULL!\n");
		return -1;
	}

	if( miptr->hardware.get_hw )
		miptr->hardware.get_hw(me, attr_mask);	//update attrs

	return 0;
}

struct meinfo_callback_t meinfo_cb_413 = {
	.create_cb = meinfo_cb_413_create,
};

