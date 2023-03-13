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
 * File    : meinfo_cb_65307.c
 *
 ******************************************************************/

#include <stdlib.h>

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"
#include "me_related.h"

// 65307 9.99.22 ONU switchport_config

static int 
meinfo_cb_65307_create(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 1)) {	// FE_enable
		struct me_t *pptp_uni_me=meinfo_me_get_by_instance_num(11, me->meid);
		unsigned char get_attr_mask[2] = {0, 0};
		struct attr_value_t attr={0, NULL};
		unsigned char admin_state;

		if (pptp_uni_me==NULL)
			dbprintf(LOG_ERR, "class 11, instance %d not found?\n", me->meid);
		// call hw and cb before mib read
		util_attr_mask_set_bit(get_attr_mask, 5);
		meinfo_me_refresh(pptp_uni_me, get_attr_mask);
		admin_state=meinfo_util_me_attr_data(pptp_uni_me, 5);
		attr.data=!admin_state;
		meinfo_me_attr_set(me, 1, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 6)) {	// Us_limit_enable
		struct attr_value_t attr={0, NULL};
		attr.data=meinfo_util_me_attr_data(me, 10);	// Upstream_rate_control_enable
		meinfo_me_attr_set(me, 6, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) {	// Us_limie_speed
		struct attr_value_t attr={0, NULL};
		attr.data=meinfo_util_me_attr_data(me, 12);	// Upstream_burst_rate
		meinfo_me_attr_set(me, 7, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 8)) {	// Ds_limit_enable
		struct attr_value_t attr={0, NULL};
		attr.data=meinfo_util_me_attr_data(me, 14);	// Downstream_rate_control_enable
		meinfo_me_attr_set(me, 8, &attr, 0);
	}
	if (util_attr_mask_get_bit(attr_mask, 9)) {	// Ds_limie_speed
		struct attr_value_t attr={0, NULL};
		attr.data=meinfo_util_me_attr_data(me, 16);	// Downstream_burst_rate
		meinfo_me_attr_set(me, 9, &attr, 0);
	}
	return 0;
}

static int
meinfo_cb_65307_get(struct me_t *me, unsigned char attr_mask[2])
{
	return meinfo_cb_65307_create(me, attr_mask);
}

static int 
meinfo_cb_65307_set(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 1)) {	// FE_enable
		struct me_t *pptp_uni_me=meinfo_me_get_by_instance_num(11, me->meid);
		unsigned char fe_enable=meinfo_util_me_attr_data(me, 1);
		struct attr_value_t attr={0, NULL};
		unsigned char attr_mask[2]={0};

		if (pptp_uni_me==NULL)
			dbprintf(LOG_ERR, "class 11, instance %d not found?\n", me->meid);

		attr.data=!fe_enable;
		meinfo_me_attr_set(pptp_uni_me, 5, &attr, 0);
		util_attr_mask_set_bit(attr_mask, 5);

		// call cb and hw after mib write
		meinfo_me_flush(pptp_uni_me, attr_mask);
	}

	// as hw callback will be called after cb for the same me,
	// so we don't call meinfo_me_flush for the following attr in this cb

	if (util_attr_mask_get_bit(attr_mask, 6)) {	// Us_limit_enable
		struct attr_value_t attr={0, NULL};
		attr.data=meinfo_util_me_attr_data(me, 6);
		meinfo_me_attr_set(me, 10, &attr, 0);	// Upstream_rate_control_enable
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) {	// Us_limie_speed
		struct attr_value_t attr={0, NULL};
		attr.data=meinfo_util_me_attr_data(me, 7);
		meinfo_me_attr_set(me, 12, &attr, 0);	// Upstream_burst_rate
	}

	if (util_attr_mask_get_bit(attr_mask, 8)) {	// Ds_limit_enable
		struct attr_value_t attr={0, NULL};
		attr.data=meinfo_util_me_attr_data(me, 8);
		meinfo_me_attr_set(me, 14, &attr, 0);	// Downstream_rate_control_enable
	}
	if (util_attr_mask_get_bit(attr_mask, 9)) {	// Ds_limie_speed
		struct attr_value_t attr={0, NULL};
		attr.data=meinfo_util_me_attr_data(me, 9);
		meinfo_me_attr_set(me, 16, &attr, 0);	// Downstream_burst_rate
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_fiberhome_65307 = {
	.create_cb	= meinfo_cb_65307_create,
	.get_cb		= meinfo_cb_65307_get,
	.set_cb		= meinfo_cb_65307_set,
};
