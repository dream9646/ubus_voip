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
 * File    : meinfo_hw_277.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "meinfo_hw.h"
#include "omciutil_misc.h"
#include "util.h"
#include "hwresource.h"
#include "gpon_sw.h"

//9.11.1 Priority_queue_G
static int 
meinfo_hw_277_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==7) {	// ts pointer
		unsigned short public_ts_meid=(unsigned short)attr->data;
		struct me_t *public_ts_me;

		if (public_ts_meid == 0)	// allow null pointer assignment for PQ
			return 1;		
		if ((public_ts_me=meinfo_me_get_by_meid(278, public_ts_meid))== NULL) {
			dbprintf(LOG_ERR, "ts meid 0x%x(%u) doesn't exist?\n", public_ts_meid, public_ts_meid); 
			return 0;
		}
	}
	return 1;
}

struct meinfo_hardware_t meinfo_hw_277 = {
	.is_attr_valid_hw	= meinfo_hw_277_is_attr_valid,
};

