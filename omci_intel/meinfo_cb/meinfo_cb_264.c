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
 * File    : meinfo_cb_264.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "me_related.h"
#include "batchtab.h"
#include "util.h"

//classid 264 9.12.1 UNI_G

static int
meinfo_cb_264_set_admin_lock(struct me_t *me, int lockval)
{
	struct attr_value_t attr={ .data=lockval?1:0, .ptr=NULL, };	
	meinfo_me_attr_set(me, 2, &attr, 0);	
	return 0;
}

static int 
meinfo_cb_264_is_admin_locked(struct me_t *me)
{
	struct me_t *parent_me;

	if( meinfo_util_me_attr_data(me, 2))
		return 1;

	parent_me=me_related_find_parent_me(me, 11);	//UNI-G's parent is PPTP-UNI classid 11
	if(parent_me){
		struct meinfo_t *miptr=meinfo_util_miptr(parent_me->classid);
		if (miptr && miptr->callback.is_admin_locked_cb)
			return miptr->callback.is_admin_locked_cb(parent_me);
	}
	return 0;
}

static int 
meinfo_cb_264_create(struct me_t *me, unsigned char attr_mask[2])
{
	batchtab_omci_update("calix_apas");
	return 0;
}

static int 
meinfo_cb_264_delete(struct me_t *me)
{
	batchtab_omci_update("calix_apas");
	return 0;
}

static int 
meinfo_cb_264_set(struct me_t *me, unsigned char attr_mask[2])
{
	batchtab_omci_update("calix_apas");
	return 0;
}

struct meinfo_callback_t meinfo_cb_264 = {
	.create_cb		= meinfo_cb_264_create,
	.delete_cb		= meinfo_cb_264_delete,
	.set_cb			= meinfo_cb_264_set,
	.set_admin_lock_cb	= meinfo_cb_264_set_admin_lock,
	.is_admin_locked_cb	= meinfo_cb_264_is_admin_locked,
};

