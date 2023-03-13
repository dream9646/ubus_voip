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
 * File    : meinfo_hw_65299.c
 *
 ******************************************************************/

#include <stdlib.h>

#include "meinfo.h"
#include "meinfo_hw.h"
#include "util.h"
#include "switch.h"

// 9.99.17 Onu mac aging
static int 
meinfo_hw_65299_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;

	if (util_attr_mask_get_bit(attr_mask, 1)) {
		unsigned int time=0;
		unsigned int enable=1;

		if (switch_hw_g.mac_aging_get!=NULL) {
			if (switch_hw_g.mac_aging_get(&time)<0)
				return -1;
			// FIXME, KEVIN
			// get the port 0 enable state as whole switch return value
			if (switch_hw_g.mac_aging_enable_get(0, &enable)<0)
				return -1;
			if (!enable)
				time=0;										
		}
		attr.data = (unsigned short)time;
		meinfo_me_attr_set(me, 1, &attr, 0);	
	}	
	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65299 = {
	.get_hw		= meinfo_hw_65299_get,
};
