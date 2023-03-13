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
 * File    : er_group_hw_65302..c
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

//65302 Non-ALU_ONT_Generic_1

//65302@1
int
er_group_hw_rf_filter(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	// the upstream tcont/ts/pq configuration is moved into batchtab_cb
	batchtab_omci_update("alu_sp");
	return 0;
}

