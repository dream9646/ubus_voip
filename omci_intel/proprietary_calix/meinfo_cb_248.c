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
 * File    : meinfo_cb_248.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "me_related.h"
#include "batchtab.h"
#include "util.h"

//classid 248 9.99.5 VideoAniExtension

static unsigned short 
meinfo_cb_248_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(90, instance_num);
	if (!parent_me || meinfo_me_is_exist(248, parent_me->meid)) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 90, instance_num);	
		return instance_num;
	}
	return parent_me->meid;
}

struct meinfo_callback_t meinfo_cb_calix_248 = {
	.meid_gen_cb	= meinfo_cb_248_meid_gen,
};
