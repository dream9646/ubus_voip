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
 * File    : meinfo_cb_288.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "omcimsg.h"
#include "util.h"

//classid 288 9.12.9 Managed Entity

static int 
meinfo_cb_288_get(struct me_t *me, unsigned char attr_mask[2])
{	
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	unsigned short classid_target=me->meid;
	struct meinfo_t *miptr_target=meinfo_util_miptr(classid_target);
	int i;

	// clear previous value 
	for (i = 1; i <= miptr->attr_total; i++) {
		meinfo_me_attr_clear_value(me->classid, i, &me->attr[i].value);
	}

	if (miptr_target==NULL || miptr_target->is_private)
		return MEINFO_ERROR_ME_NOT_FOUND;

	if (util_attr_mask_get_bit(attr_mask, 1)) { // Name
		struct attr_value_t attr;
		attr.ptr=malloc_safe(miptr->attrinfo[1].byte_size);
		memcpy(attr.ptr, miptr_target->name, miptr->attrinfo[1].byte_size);
		meinfo_me_attr_set(me, 1, &attr, 0);
		free_safe(attr.ptr);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 2)) // Attributes Table
	{
#if 0
		// below code can not be used as classid 289 is virtual
		// virtual classid instance list has only one instance to serve all meid request
		struct meinfo_t *miptr_289 = meinfo_util_miptr(289); 
		if (miptr_289) {
			struct me_t *p = NULL;
			list_for_each_entry(p, &miptr_289->me_instance_list, instance_node) {
				unsigned short meid=p->meid;
				if((meid>>4) == (me->meid&0xFFF)) {
					meid = htons(p->meid);
					meinfo_me_attr_set_table_entry(me, 2, &meid, 0);
				}
			}
		}
#else
		unsigned short meid_289, value;
		unsigned char attr_order;
		
		for (attr_order=0; attr_order<miptr_target->attr_total; attr_order++) {
			// the meid in class 289 is encoded by target classid and attr_order
			meid_289=(classid_target & 0xfff)<<4 | attr_order;
			value = htons(meid_289);
			meinfo_me_attr_set_table_entry(me, 2, &value, 0);
		}
#endif
	}
	
	if (util_attr_mask_get_bit(attr_mask, 3)) { // Access
		struct attr_value_t attr;
		attr.data = miptr_target->access;
		meinfo_me_attr_set(me, 3, &attr, 0);
	}

	if (util_attr_mask_get_bit(attr_mask, 4)) // Alarms Table
	{
		int index;
		for (index = 0; index < 224; index++) {
			unsigned short alarm = (miptr_target->alarm_mask[index/8] >> (7-(index%8))) & 0x1;
			if(alarm) meinfo_me_attr_set_table_entry(me, 4, &index, 0);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 5)) // AVCs Table
	{
		int index;
		for (index = 0; index < 16; index++) {
			unsigned short avc = (miptr_target->avc_mask[index/8] >> (index%8)) & 0x1;
			if(avc) meinfo_me_attr_set_table_entry(me, 5, &index, 0);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 6)) { // Actions
		unsigned int action = htonl(miptr_target->action_mask);
		struct attr_value_t attr;
		attr.ptr=malloc_safe(miptr->attrinfo[6].byte_size);
		memcpy(attr.ptr, &action, miptr->attrinfo[6].byte_size);
		meinfo_me_attr_set(me, 6, &attr, 0);
		free_safe(attr.ptr);
	}

	if (util_attr_mask_get_bit(attr_mask, 7)) // Instances Table
	{
		struct me_t *p = NULL;
		list_for_each_entry(p, &miptr_target->me_instance_list, instance_node) {
			unsigned short meid=htons(p->meid);
			meinfo_me_attr_set_table_entry(me, 7, &meid, 0);
		}
	}

	if (util_attr_mask_get_bit(attr_mask, 8)) { // Support
		struct attr_value_t attr;
		attr.data = miptr_target->config.support;
		meinfo_me_attr_set(me, 8, &attr, 0);
	}

	return 0;
}

struct meinfo_callback_t meinfo_cb_288 = {
	.get_cb			= meinfo_cb_288_get,
};
