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
 * File    : meinfo_cb_241.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "me_related.h"
#include "batchtab.h"
#include "util.h"

//classid 241 Information

static int 
meinfo_cb_241_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=NULL;
	unsigned char attr_mask2[2]={0x40,0x50};

	if ((miptr=meinfo_util_miptr(241)) == NULL) {
		dbprintf(LOG_ERR, "Fail, because of miptr NULL!\n");
		return -1;
	}

	if( miptr->hardware.get_hw )
		miptr->hardware.get_hw(me, attr_mask2);	//update attr > 2

	return 0;
}

struct meinfo_callback_t meinfo_cb_calix_241 = {
	.create_cb      = meinfo_cb_241_create
};
