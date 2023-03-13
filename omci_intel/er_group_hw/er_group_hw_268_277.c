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
 * File    : er_group_hw_268_277.c
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

// 268 GEM_port_network_CTP 
//@4:Traffic management pointer for upstream

// 277 Priority_queue_G
//268@4, 277@255
int
er_group_hw_generator_pq_ts_tcont(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
        // FIXME, Justin KU
        // the upstream tcont/ts/pq configuration is moved into batchtab_cb
        
        batchtab_omci_update("tm");
	return 0;
}

