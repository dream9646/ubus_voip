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
 * Module  : proprietary_alu
 * File    : meinfo_hw_65296.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"

//classid 65296 ONT Generic V2

static int 
meinfo_hw_65296_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;

	if (me == NULL) {
		dbprintf(LOG_ERR, "invalid me\n");
		return -1;
	}
	
	// 5: ethernet port-to-port traffic enable indicator
	if (util_attr_mask_get_bit(attr_mask, 5)) {
		unsigned char enable;
		enable = (omci_env_g.port2port_enable)? 1 : 0;
		attr.data = enable;
		meinfo_me_attr_set(me, 5, &attr, 0);
	}
	
	// 10: YP_Serial_Number
	if (util_attr_mask_get_bit(attr_mask, 10)) {
		// Workaround for CHT need to read ONT MAC
		if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
			unsigned char mac[6];
			attr.data=0;
			attr.ptr= malloc_safe(12);
			memset(mac, 0, sizeof(mac));
			if (switch_hw_g.eth_address_get)
				switch_hw_g.eth_address_get(0, mac);
			sprintf(attr.ptr, "%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
			meinfo_me_attr_set(me, 10, &attr, 0);
			if (attr.ptr)
				free_safe(attr.ptr);
		}
	}
	
	// 11: ALU_Special_Behaviors_Field
	if (util_attr_mask_get_bit(attr_mask, 11)) {
		unsigned char enable;
		enable = (omci_env_g.invert_weight_enable)? 0 : 1;
		attr.data = enable;
		meinfo_me_attr_set(me, 11, &attr, 0);
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_alu_65296 = {
	.get_hw		= meinfo_hw_65296_get,
};


