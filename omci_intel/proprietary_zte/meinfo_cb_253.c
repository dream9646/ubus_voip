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
 * File    : meinfo_cb_253.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 253 9.99.10 Extended_Physical_path_termination_point_Ethernet_UNI

static unsigned short 
meinfo_cb_253_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(11, instance_num);
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 11, instance_num);
		return instance_num;
	}
	return parent_me->meid;
}

struct meinfo_callback_t meinfo_cb_zte_253 = {
	.meid_gen_cb	= meinfo_cb_253_meid_gen,
};
