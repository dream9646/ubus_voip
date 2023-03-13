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
 * File    : er_group_hw_47_78_134.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "omciutil_vlan.h"

//47@2,78@1,78@2,78@3
int
er_group_hw_l2s_tags_create_add01_ip_host(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	return er_group_hw_l2s_tags_create_add01_bridge(action, attr_inst, me, attr_mask);
}
