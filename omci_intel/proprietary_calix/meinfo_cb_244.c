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
 * File    : meinfo_cb_244.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "me_related.h"
#include "batchtab.h"
#include "util.h"

//classid 244 9.99.4 PotsUniExt

static unsigned short 
meinfo_cb_244_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(53, instance_num);
	if (!parent_me || meinfo_me_is_exist(244, parent_me->meid)) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 53, instance_num);	
		return instance_num;
	}
	return parent_me->meid;
}

struct meinfo_callback_t meinfo_cb_calix_244 = {
	.meid_gen_cb	= meinfo_cb_244_meid_gen,
};
