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
 * File    : meinfo_hw_65318.c
 *
 ******************************************************************/

#include "util.h"
#include "switch.h"
#include "meinfo_hw_util.h"
#include "meinfo_hw_poe.h"

// 9.99.23 PsE_Control

int poe_mgt_cap=-1;

static int 
meinfo_hw_65318_get(struct me_t *me, unsigned char attr_mask[2])
{
	int port_id, power_budget = 0, power = 0;
	extern struct poe_result_t poe_result;
	
	// get counters from hw
	if (meinfo_hw_poe_get_power_budget(&power_budget)<0)
		return 0;
	if (meinfo_hw_poe_get_management_capability(&poe_mgt_cap)<0)
		return 0;

	// get consumed power
	for (port_id= 0; port_id < SWITCH_LOGICAL_PORT_TOTAL; port_id ++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			power += poe_result.poe_report[port_id].power;
		}
	}

	// Management_Capabilities
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		struct attr_value_t attr;
		attr.ptr=NULL;
		attr.data=poe_mgt_cap;
		meinfo_me_attr_set(me, 1, &attr, 1);
	}
	
	// Max_Power_Budget
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		struct attr_value_t attr;
		attr.ptr=NULL;
		attr.data=power_budget;
		meinfo_me_attr_set(me, 2, &attr, 0);
	}
	
	// Available_Power_Budget
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		struct attr_value_t attr;
		attr.ptr=NULL;
		attr.data=(power_budget*1000<power) ? 0 : (power_budget*1000-power)/100;
		meinfo_me_attr_set(me, 3, &attr, 0);
	}
	
	// Aggregate_Output_Power
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		struct attr_value_t attr;
		attr.ptr=NULL;
		attr.data=power/100;
		meinfo_me_attr_set(me, 4, &attr, 0);
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_calix_65318 = {
	.get_hw			= meinfo_hw_65318_get,
};

