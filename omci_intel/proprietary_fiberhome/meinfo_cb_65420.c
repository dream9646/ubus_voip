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
 * File    : meinfo_cb_65420.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"

//classid 65420 9.99.33 WRI_softswitch_progile_config

static int 
meinfo_cb_65420_create(struct me_t *me, unsigned char attr_mask[2])
{
	if (me == NULL || omci_env_g.voip_enable == ENV_VOIP_DISABLE)
	{
		return -1;
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_fiberhome_65420 = {
	.create_cb			= meinfo_cb_65420_create,
};

