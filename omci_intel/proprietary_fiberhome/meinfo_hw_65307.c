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
 * Module  : proprietary_fiberhome
 * File    : meinfo_hw_65307.c
 *
 ******************************************************************/

#include <stdlib.h>

#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"

// 9.99.22 ONU_switchport_config
static int 
meinfo_hw_65307_get(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 3) ||	// speed
	    util_attr_mask_get_bit(attr_mask, 4) ||	// duplex
	    util_attr_mask_get_bit(attr_mask, 5)) {	// flowctrl
	    	struct switch_port_mac_config_t port_mac_status;	    
		struct attr_value_t attr={0, NULL};
		int ret;

		if (switch_hw_g.port_status_get &&
		    (ret=switch_hw_g.port_status_get(me->meid, &port_mac_status))<0) {
			dbprintf(LOG_ERR, "port_status_get port=%d, err=0x%x\n", me->meid, ret);
		}

		attr.data=port_mac_status.speed;
		meinfo_me_attr_set(me, 3, &attr, 0);

		attr.data=port_mac_status.duplex;
		meinfo_me_attr_set(me, 4, &attr, 0);

		attr.data=(port_mac_status.tx_pause || port_mac_status.rx_pause);
		meinfo_me_attr_set(me, 5, &attr, 0);
	}

	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65307 = {
	.get_hw		= meinfo_hw_65307_get,
};
