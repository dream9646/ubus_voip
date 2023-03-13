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
 * File    : meinfo_hw_262.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "meinfo_hw.h"
#include "util.h"
#include "hwresource.h"
#include "gpon_sw.h"

//classid 262 9.2.2 T_CONT

static int 
meinfo_hw_262_is_ready(struct me_t *me)
{
	struct attr_value_t attr_value;
	int tcont_id;	// hw internal id
	
	//get the tcont value in the me (attr_value.data)
	meinfo_me_attr_get(me, 1, &attr_value);

	if (gpon_hw_g.tm_tcont_get_by_allocid) {
		if(attr_value.data!=0xff && attr_value.data!=0xffff){			
		gpon_hw_g.tm_tcont_get_by_allocid(attr_value.data, &tcont_id);
		if (tcont_id < 0)
			return 0; 
	}
	}
	return 1;
}

struct meinfo_hardware_t meinfo_hw_262 = {
	.is_ready_hw		= meinfo_hw_262_is_ready,
};

