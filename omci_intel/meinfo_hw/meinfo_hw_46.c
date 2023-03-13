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
 * File    : meinfo_hw_46.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"

/*
class id: 46 
name: MAC_bridge_configuration_data
section: 9.3.2
*/

static int 
meinfo_hw_46_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	
	if (util_attr_mask_get_bit(attr_mask, 1)) {	// attr1: Bridge_MAC_address
		unsigned char mac[6];
		attr.data=0;
		attr.ptr= malloc_safe(6);
		memset(mac, 0, sizeof(mac));
		if (switch_hw_g.eth_address_get)
			switch_hw_g.eth_address_get(0, mac);
		memcpy(attr.ptr, mac, sizeof(mac));
		meinfo_me_attr_set(me, 1, &attr, 0);	// check_avc==0 means none update avc_change_mask
		if (attr.ptr)
			free_safe(attr.ptr);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 3)) {	// attr3: Designated_root
		unsigned char mac[6];
		unsigned short priority = htons(me->instance_num);
		attr.data=0;
		attr.ptr= malloc_safe(8);
		memset(mac, 0, sizeof(mac));
		if (switch_hw_g.eth_address_get)
			switch_hw_g.eth_address_get(0, mac);
		memcpy(attr.ptr, (void *) &priority, 2);
		memcpy(attr.ptr+2, mac, sizeof(mac));
		meinfo_me_attr_set(me, 3, &attr, 0);	// check_avc==0 means none update avc_change_mask
		if (attr.ptr)
			free_safe(attr.ptr);
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_46 = {
	.get_hw		= meinfo_hw_46_get,
};
