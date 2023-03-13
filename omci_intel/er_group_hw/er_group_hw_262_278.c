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
 * Module  : er_group_hw
 * File    : er_group_hw_262_278.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "batchtab.h"

// 262 T_CONT
// 278 Traffic_scheduler_G

//262@1,278@255,278@3,278@4
int
er_group_hw_mac_traffic_control_first_ts(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
        // FIXME, KEVIN
        // the upstream tcont/ts configuration is moved into batchtab_cb
        
        batchtab_omci_update("tm");
	return 0;
}
