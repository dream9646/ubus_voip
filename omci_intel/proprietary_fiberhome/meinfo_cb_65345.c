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
 * File    : meinfo_cb_65345.c
 *
 ******************************************************************/

#include <stdlib.h>

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"
#include "regexp.h"
#include "omciutil_usage.h"

// 9.99.20 ONU performance
static int 
meinfo_cb_65345_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;

	if (util_attr_mask_get_bit(attr_mask, 1)) {
		attr.data = (unsigned short)omciutil_usage_cpu_used_percentage();
		meinfo_me_attr_set(me, 1, &attr, 0);	
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		attr.data = (unsigned short)omciutil_usage_mem_used_percentage();
		meinfo_me_attr_set(me, 2, &attr, 0);	
	}	
	return 0;
}

struct meinfo_callback_t meinfo_cb_fiberhome_65345 = {
	.get_cb		= meinfo_cb_65345_get,
};
