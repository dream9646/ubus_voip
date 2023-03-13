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
 * Module  : meinfo_hw
 * File    : meinfo_hw_171.c
 *
 ******************************************************************/

#include "meinfo_hw.h"
#include "util.h"
#include "er_group_hw.h"

//9.3.13 Extended_VLAN_tagging_operation
static int 
meinfo_hw_171_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	int action;

	if (attr_order == 6)
	{
		action = meinfo_me_attr_get_table_entry_action(me, attr_order, attr->ptr);

		switch(action)
		{
		case ATTR_ENTRY_ACTION_DELETE:
		case ATTR_ENTRY_ACTION_MODIFY:
			return 1;
		case ATTR_ENTRY_ACTION_ADD:
			// FIXME, KEVIN, some basic check for tag table add is required?
			return 1;
		default:
			return 0;			
		}
	}

	return 1;
}

struct meinfo_hardware_t meinfo_hw_171 = {
	.is_attr_valid_hw	= meinfo_hw_171_is_attr_valid,
};

