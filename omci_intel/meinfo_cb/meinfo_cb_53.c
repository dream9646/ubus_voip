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
 * File    : meinfo_cb_53.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"
#include "batchtab.h"

//classid 53 9.9.1 Physical_path_termination_point_POTS_UNI

static int 
meinfo_cb_53_create(struct me_t *me, unsigned char attr_mask[2])
{
	if (me == NULL || omci_env_g.voip_enable == ENV_VOIP_DISABLE)
	{
		return -1;
	}
	batchtab_omci_update("pots_mkey");
	return 0;
}

static int
meinfo_cb_53_set_admin_lock(struct me_t *me, int lockval)
{
	struct attr_value_t attr={ .data=lockval?1:0, .ptr=NULL, };	
	meinfo_me_attr_set(me, 1, &attr, 0);

	// lock child classid 264 UNI-G
	meinfo_me_set_child_admin_lock(me, 264, lockval);
	return 0;
}

static int 
meinfo_cb_53_is_admin_locked(struct me_t *me)
{
	return me?meinfo_util_me_attr_data(me, 1):0;
}

static int 
meinfo_cb_53_set(struct me_t *me, unsigned char attr_mask[2])
{
	batchtab_omci_update("pots_mkey");
	return meinfo_me_set_arc_timestamp(me, attr_mask, 3, 4);
}

static int 
meinfo_cb_53_is_arc_enabled(struct me_t *me)
{
	return meinfo_me_check_arc_interval(me, 3, 4);
}

static int 
meinfo_cb_53_test_is_supported(struct me_t *me, unsigned char *req)
{
	if (me == NULL || omci_env_g.voip_enable == ENV_VOIP_DISABLE)
	{
		return 0;
	}
	
	return 1;
}

struct meinfo_callback_t meinfo_cb_53 = {
	.create_cb			= meinfo_cb_53_create,
	.set_cb				= meinfo_cb_53_set,
	.set_admin_lock_cb	= meinfo_cb_53_set_admin_lock,
	.is_admin_locked_cb	= meinfo_cb_53_is_admin_locked,
	.is_arc_enabled_cb		= meinfo_cb_53_is_arc_enabled,
	.test_is_supported_cb	= meinfo_cb_53_test_is_supported,
};

