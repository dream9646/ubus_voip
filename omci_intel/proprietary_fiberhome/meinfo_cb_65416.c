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
 * File    : meinfo_cb_65416.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"

//classid 65416 9.9.14 WRI_MG_port_register_control
static int 
meinfo_cb_65416_create(struct me_t *me, unsigned char attr_mask[2])
{
	if (me == NULL || omci_env_g.voip_enable == ENV_VOIP_DISABLE)
	{
		return -1;
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_fiberhome_65416 = {
	.create_cb			= meinfo_cb_65416_create,
};
