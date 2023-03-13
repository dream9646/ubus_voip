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
 * File    : meinfo_cb_65302.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "util.h"
#include "proprietary_alu.h"

//classid 65302 9.99.121 Non-ALU ONT Generic 1

static int
meinfo_cb_65302_set(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 1)) {     // RF Filter
		// Workaround for ALU OLT 7342 which cannot support SP
		if((omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) &&
		   (proprietary_alu_get_olt_version() == 0)) {
			struct meinfo_t *miptr = meinfo_util_miptr(278);
			struct attr_value_t attr = {0, NULL};
			meinfo_me_attr_get(me, 1, &attr);
			if((unsigned char)attr.data == 0) { // RFFILTER=0 means turn on SP workaround
				// Set all TS policy to SP
				list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
					attr.data = 1; 
					meinfo_me_attr_set(me, 3, &attr, 0);
				}
			} else {
				// Set all TS policy to original value
				list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
					struct meinfo_instance_def_t *me_def = meinfo_instance_find_def(278, me->instance_num);
					attr.data = (unsigned char) me_def->custom_default_value[3].data;
					meinfo_me_attr_set(me, 3, &attr, 0);
				}
			}
		}
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_alu_65302 = {
	.set_cb		= meinfo_cb_65302_set,
};
