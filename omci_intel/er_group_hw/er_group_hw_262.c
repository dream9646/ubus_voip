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
 * File    : er_group_hw_262.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "fwk_timer.h"
#include "coretask.h"
#include "batchtab.h"


extern unsigned char ont_state_o1_flag;
int tcont_align_timer_id;

// 262 T_CONT

//262@1,262@3
int
er_group_hw_mac_traffic_control_second_ts(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
        // FIXME, KEVIN
        // the upstream tcont/ts/pq configuration is moved into batchtab_cb
        
        batchtab_omci_update("tm");
	return 0;
}

//262@1
int
er_group_hw_mac_tcont_allocate(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
        // FIXME, KEVIN
        // the upstream tcont/ts/pq configuration is moved into batchtab_cb
        
        batchtab_omci_update("tm");
	return 0;
}

