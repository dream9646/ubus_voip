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
 * File    : meinfo_cb_153.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"
#include "batchtab.h"
#include "cli.h"

//classid 153 9.9.2 SIP_user_data

static int 
meinfo_cb_153_create(struct me_t *me, unsigned char attr_mask[2])
{
	if (me == NULL || omci_env_g.voip_enable == ENV_VOIP_DISABLE)
	{
		return -1;
	}
	cli_pots_mkey_me_153(me->meid);
	batchtab_omci_update("pots_mkey");
	return 0;
}

struct meinfo_callback_t meinfo_cb_153 = {
	.create_cb			= meinfo_cb_153_create,
};

