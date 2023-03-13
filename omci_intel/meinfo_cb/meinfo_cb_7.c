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
 * File    : meinfo_cb_7.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "me_related.h"
#include "omcimsg.h"
#include "hwresource.h"
#include "util.h"

//classid 7 9.1.4 Software_image

static int 
meinfo_cb_7_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=NULL;
	struct me_t *private_me;

	unsigned char attr_mask2[2]={0xf0,0x0};

	if (hwresource_is_related(me->classid) && hwresource_public2private(me)==NULL)
		hwresource_alloc(me);

	private_me=hwresource_public2private(me);
	if (private_me==NULL) {
		dbprintf(LOG_ERR, "Can't found related private me?\n");
		return -1;
	}

	omcimsg_sw_download_set_diagram_state(0);
	if ((miptr=meinfo_util_miptr(7)) == NULL) {
		dbprintf(LOG_ERR, "Fail, because of miptr NULL!\n");
		return -1;
	}

	if( miptr->hardware.get_hw )
		miptr->hardware.get_hw(me, attr_mask2);	//update all attr

	omcimsg_sw_download_init_diagram_state(me);		//value will correct after image1 create
	return 0;
}

struct meinfo_callback_t meinfo_cb_7 = {
	.create_cb	= meinfo_cb_7_create
};
