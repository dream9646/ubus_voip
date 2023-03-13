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
 * File    : meinfo_hw_290.c
 *
 ******************************************************************/

#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"

// 9.3.14 Dot1X_port_extension_package

static int 
meinfo_hw_290_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct switch_port_info_t port_info;
	struct me_t *parent_me=meinfo_me_get_by_meid(11, me->meid);
	
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, meid=0x%x\n", 11, me->meid);	
		return -1;
	}
	
	if (switch_get_port_info_by_me(parent_me, &port_info)<0) {
		dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", me->meid);	
		return -1;
	}
	
	// only apply to uni port
	if(!((1<<port_info.logical_port_id) & switch_get_uni_logical_portmask()))
		return 0;
	
	if (util_attr_mask_get_bit(attr_mask, 1)) {	// attr1: enable
		struct attr_value_t attr;
		unsigned int ret, enable;
		if ((ret = switch_hw_g.dot1x_mode_get(port_info.logical_port_id, &enable)) < 0) {
			dbprintf(LOG_ERR, "dot1x_mode_get error, port=%d, ret=%d\n", port_info.logical_port_id, ret);
			return -1;
		}
		attr.data = (unsigned char) enable;
		meinfo_me_attr_set(me, 1, &attr, 0);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 6)) {	// attr6: direction
		struct attr_value_t attr;
		unsigned int ret, dir;
		if ((ret = switch_hw_g.dot1x_dir_get(port_info.logical_port_id, &dir)) < 0) {
			dbprintf(LOG_ERR, "dot1x_dir_get error, port=%d, ret=%d\n", port_info.logical_port_id, ret);
			return -1;
		}
		attr.data = (unsigned char) dir;
		meinfo_me_attr_set(me, 6, &attr, 0);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 7)) {	// attr7: status
		struct attr_value_t attr;
		unsigned int ret, state;
		if ((ret = switch_hw_g.dot1x_state_get(port_info.logical_port_id, &state)) < 0) {
			dbprintf(LOG_ERR, "dot1x_state_get error, port=%d, ret=%d\n", port_info.logical_port_id, ret);
			return -1;
		}
		attr.data = (unsigned char) state;
		meinfo_me_attr_set(me, 7, &attr, 0);
	}
	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_290 = {
	.get_hw			= meinfo_hw_290_get,
};

