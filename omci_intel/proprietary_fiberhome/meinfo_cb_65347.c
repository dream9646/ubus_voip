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
 * Module  : proprietary_fiberhome
 * File    : meinfo_cb_65347.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 50 9.3.8 MAC_bridge_port_bridge_table_data

static unsigned short 
meinfo_cb_65347_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(47, instance_num);
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 47, instance_num);
		return instance_num;
	}
	return parent_me->meid;
}

struct meinfo_callback_t meinfo_cb_fiberhome_65347 = {
	.meid_gen_cb	= meinfo_cb_65347_meid_gen,
};
