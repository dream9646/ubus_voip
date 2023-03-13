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
 * File    : er_group_hw_53_150_151_153.c
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
// 151 SIP_agent_performance_monitoring_history_data
// 153 SIP_user_data

//53@254,151@3,151@4,151@5,151@6,151@7,151@8,151@9,151@10,151@11,151@12,151@13,151@14,151@15
int
er_group_hw_sip_agent_performance_monitoring_history_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return 0;
}
