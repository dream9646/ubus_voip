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
 * File    : meinfo_hw_268.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "hwresource.h"
#include "gpon_sw.h"
#include "tm.h"

// 9.2.3 GEM_port_network_CTP

static int 
meinfo_hw_268_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct gpon_tm_dsflow_config_t dsflow_config;
	if (util_attr_mask_get_bit(attr_mask, 8)) {	// attr8: Encryption_state
		struct attr_value_t attr;
		int flowid = -1;
		if( (flowid = gem_flowid_get_by_gem_me(me, 0)) < 0)
			return 0;
		
		if (gpon_hw_g.tm_dsflow_get(flowid, &dsflow_config) == 0) {
			meinfo_me_attr_get(me, 8, &attr);
			attr.data = dsflow_config.aes_enable;
			meinfo_me_attr_set(me, 8, &attr, 0);
		}
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_268 = {
	.get_hw			= meinfo_hw_268_get,
};

