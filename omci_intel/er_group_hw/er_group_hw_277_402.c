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
 * File    : er_group_hw_277_402.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"

// 277 Priority_queue_G 
// 402 Upstream pseudo obj 

//277@13,277@14,277@15,277@16,402@255
int
er_group_hw_mac_traffic_control_red(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	// FIXME, KEVIN
	// pq red configuration is not supported by 2510
	
	// on the other hand, to utilize the pq in best manner, the PQ me to real PQ mapping will be determine by batch_cb "tm"
	// so the red configuration will be moved to batch_cb "tm" if the hw support RED
	
	return 0;
}
