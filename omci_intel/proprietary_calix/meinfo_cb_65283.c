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
 * File    : meinfo_cb_65283.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "me_related.h"
#include "batchtab.h"
#include "util.h"

//classid 65283 Voip_Call_Statistics_Extension4 

static unsigned short 
meinfo_cb_65283_meid_gen(unsigned short instance_num)
{
	struct me_t *associated_me=meinfo_me_get_by_instance_num(53, instance_num);
	if (!associated_me) {
		dbprintf(LOG_ERR, "unable to find associated me, classid=%u, instance=%u\n", 53, instance_num);	
		return instance_num;
	}
	return associated_me->meid;
}

static int 
meinfo_cb_65283_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=NULL;
	struct attr_value_t attr;
	unsigned char attr_order;
	unsigned char attr_mask2[2]={0xbf,0xff};

	if ((miptr=meinfo_util_miptr(me->classid)) == NULL) {
		dbprintf(LOG_ERR, "Fail, because of miptr NULL!\n");
		return -1;
	}

	if( miptr->hardware.get_hw )
		miptr->hardware.get_hw(me, attr_mask2);	//update attr > 2

	attr.data=0; 
	attr.ptr=NULL;

	for( attr_order=2; attr_order <= miptr->attr_total; attr_order++) {
		//set current and history to 0
		meinfo_me_attr_set(me, attr_order, &attr, 0);
	}

	return 0;
}

struct meinfo_callback_t meinfo_cb_calix_65283 = {
	.meid_gen_cb	= meinfo_cb_65283_meid_gen,
	.create_cb      = meinfo_cb_65283_create
};
