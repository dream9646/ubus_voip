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
 * Module  : proprietary_tellion
 * File    : meinfo_hw_244.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"

//classid 244 ONT_Self_Loop_Detect

static int
meinfo_hw_244_get(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 5)) {	// ONU_Uni_Detect_State
		struct attr_value_t attr;
		unsigned int portmask = 0;
		switch_loop_detect_mask_get(&portmask);
		attr.data = (portmask) ? 1 : 0;
		meinfo_me_attr_set(me, 5, &attr, 1);
	}

	if (util_attr_mask_get_bit(attr_mask, 6)) {	// ONU_Uni_Block_State
		struct attr_value_t attr;
		unsigned int portmask = 0;
		switch_loop_detect_mask_get(&portmask);
		attr.data = ((omci_env_g.rldp_enable >= 3) && (portmask != 0)) ? 1 : 0;
		meinfo_me_attr_set(me, 6, &attr, 1);
	}

	if (util_attr_mask_get_bit(attr_mask, 7)) {	// Unblock_Reason
		struct attr_value_t attr;
		unsigned int portmask = 0;
		switch_loop_detect_mask_get(&portmask);
		if ((omci_env_g.rldp_enable >= 3) && (portmask != 0)) {
			attr.data = 0; // UNBLOCK_BY_INIT
		} else {
			extern long block_start_time;
			if (meinfo_util_me_attr_data(me, 1) == 0)
				attr.data = 3; // UNBLOCK_BY_DISABLE
			else if (block_start_time != 0)
				attr.data = 2; // UNBLOCK_BY_TIMER_EXPIRE
			else if (omci_env_g.rldp_def_portmask == 0)
				attr.data = 1; // UNBLOCK_BY_CMD
			else
				attr.data = 0; // UNBLOCK_BY_INIT
		}
		meinfo_me_attr_set(me, 7, &attr, 1);
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_tellion_244 = {
	.get_hw		= meinfo_hw_244_get,
};
