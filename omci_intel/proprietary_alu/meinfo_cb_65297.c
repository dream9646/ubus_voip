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
 * File    : meinfo_cb_65297.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "me_related.h"
#include "batchtab.h"
#include "util.h"

//classid 65297 9.99.116 UNI Supplemental 1 V2

static unsigned short 
meinfo_cb_65297_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(11, instance_num);
	if (!parent_me || meinfo_me_is_exist(65297, parent_me->meid)) {
		parent_me = meinfo_me_get_by_instance_num(329, instance_num);
		if (!parent_me || meinfo_me_is_exist(65297, parent_me->meid)) {
			parent_me = meinfo_me_get_by_instance_num(134, instance_num);
			if (!parent_me || meinfo_me_is_exist(65297, parent_me->meid)) {
				dbprintf(LOG_ERR, "unable to find parent me, classid=%u or %u or %u, instance=%u\n", 11, 329, 134, instance_num);	
				return instance_num;
			}
		}
	}
	return parent_me->meid;
}

static int
meinfo_cb_65297_set(struct me_t *me, unsigned char attr_mask[2])
{
	if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		batchtab_omci_update("alu_ds_mcastvlan");
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_alu_65297 = {
	.meid_gen_cb	= meinfo_cb_65297_meid_gen,
	.set_cb		= meinfo_cb_65297_set,
};
