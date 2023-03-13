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
 * File    : meinfo_hw_47.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"
#include "hwresource.h"

/*
class id: 47 
name: MAC_bridge_port_configuration_data
section: 9.3.4
*/

static int 
meinfo_hw_47_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	
	if (util_attr_mask_get_bit(attr_mask, 10)) {	// attr1: Port_MAC_address
		unsigned char mac[6];
		attr.data=0;
		attr.ptr= malloc_safe(6);
		memset(mac, 0, sizeof(mac));
		if (switch_hw_g.eth_address_get) {
			struct me_t *private_me=0;
			struct attr_value_t attr;
			private_me=hwresource_public2private(me);
			if (private_me==NULL) {
				dbprintf(LOG_ERR, "Can't found related private me?\n");
				return -1;
			}
			meinfo_me_attr_get(private_me, 4, &attr);
			switch_hw_g.eth_address_get((attr.data==2)?5:0, mac);
		}
		memcpy(attr.ptr, mac, sizeof(mac));
		meinfo_me_attr_set(me, 10, &attr, 0);	// check_avc==0 means none update avc_change_mask
		if(attr.ptr)
			free_safe(attr.ptr);
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_47 = {
	.get_hw		= meinfo_hw_47_get,
};
