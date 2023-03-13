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
 * Module  : meinfo_cb
 * File    : meinfo_cb_289.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "omcimsg.h"
#include "util.h"

//classid 289 9.12.10 Attributes
//instance id of this ME (2 bytes): 12-bits for classid and 4-bits for attributeid(1~16)
//for proprietary classid: strip first 4-bits (0xFF02 => 0xF02), so that classid 3840~4095 will be overwritten by 65280~65535

static int 
meinfo_cb_289_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	int i;

	unsigned short classid_target = ((me->meid>>4)>=0xF00) ? ((me->meid>>4)|0xF000) : (me->meid>>4);
	unsigned short attr_order_target = (me->meid&0xF) + 1;
	struct meinfo_t *miptr_target = meinfo_util_miptr(classid_target);
	struct attrinfo_t *aiptr_target = meinfo_util_aiptr(classid_target, attr_order_target);
	struct attr_value_t attr;
	
	// clear previous value 
	for (i = 1; i <= miptr->attr_total; i++) {
		meinfo_me_attr_clear_value(me->classid, i, &me->attr[i].value);
	}

	if (miptr_target == NULL || miptr_target->is_private)
		return MEINFO_ERROR_ME_NOT_FOUND;
	if (attr_order_target > miptr_target->attr_total)
		return MEINFO_ERROR_ME_NOT_FOUND;

	if (util_attr_mask_get_bit(attr_mask, 1)) { // Name
		attr.ptr=malloc_safe(miptr->attrinfo[1].byte_size);
		memcpy(attr.ptr, aiptr_target->name, miptr->attrinfo[1].byte_size);
		meinfo_me_attr_set(me, 1, &attr, 0);
		free_safe(attr.ptr);
	}

	if (util_attr_mask_get_bit(attr_mask, 2)) { // Size
		attr.data = aiptr_target->byte_size;
		meinfo_me_attr_set(me, 2, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 3)) { // Access
		attr.data = aiptr_target->access_bitmap;
		meinfo_me_attr_set(me, 3, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 4)) { // Format
		attr.data = aiptr_target->format;
		meinfo_me_attr_set(me, 4, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 5)) { // Lower limit
		attr.data = aiptr_target->lower_limit;
		meinfo_me_attr_set(me, 5, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 6)) { // Upper limit
		attr.data = aiptr_target->upper_limit;
		meinfo_me_attr_set(me, 6, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 7)) { // Bit field
		attr.data = aiptr_target->bit_field_mask;
		meinfo_me_attr_set(me, 7, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 8)) { // Code point table
		struct attr_code_point_t *p = NULL;
		list_for_each_entry(p, &aiptr_target->code_points_list, code_points_list) {
			unsigned short code=htons(p->code);
			meinfo_me_attr_set_table_entry(me, 8, &code, 0);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 9)) { // Support
		attr.data = miptr_target->config.support;
		meinfo_me_attr_set(me, 9, &attr, 0);
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_289 = {
	.get_cb			= meinfo_cb_289_get,
};
