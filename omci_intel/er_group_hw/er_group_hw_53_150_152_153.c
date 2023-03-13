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
 * File    : er_group_hw_53_150_152_153.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "voip_hw.h"
#include "er_group_hw.h"
#include "er_group_hw_util.h"

// 53 Physical_path_termination_point_POTS_UNI
// 150 SIP_agent_config_data
// 152 SIP_call_initiation_performance_monitoring_history_data
// 153 SIP_user_data

//53@254,152@3,152@4,152@5,152@6,152@7
int
er_group_hw_sip_call_initiation_performance_monitoring_history_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return 0;
}
