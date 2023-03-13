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
 * Module  : proprietary_alu
 * File    : meinfo_cb_65296.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 65296 9.99.114 ONT Generic V2

static int
meinfo_cb_65296_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=NULL;
	unsigned char attr_mask2[2]={0x00,0x40};
	
	if ((miptr=meinfo_util_miptr(65296)) == NULL) {
		dbprintf(LOG_ERR, "Fail, because of miptr NULL!\n");
		return -1;
	}
	
	if(miptr->hardware.get_hw)
		miptr->hardware.get_hw(me, attr_mask2); // update attribute #10
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_alu_65296 = {
	.create_cb	= meinfo_cb_65296_create,
};
