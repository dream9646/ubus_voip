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
 * File    : meinfo_cb_287.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "omcimsg.h"
#include "util.h"

//classid 287 9.12.8 OMCI

static int 
meinfo_cb_287_get(struct me_t *me, unsigned char attr_mask[2])
{
	int index;
	unsigned char msgtype;
	
	if (meinfo_me_attr_is_table_empty(me, 1) == 1) //empty
	{
		for (index = 0; index < MEINFO_INDEX_TOTAL; index++) {
			unsigned short classid=meinfo_index2classid(index);
			struct meinfo_t *miptr = meinfo_util_miptr(classid); 
			if (miptr && !miptr->is_private) {
				unsigned short classid_n=htons(classid);
				meinfo_me_attr_set_table_entry(me, 1, &classid_n, 0);
			}
		}
	}

	if (meinfo_me_attr_is_table_empty(me, 2) == 1) //empty
	{		
		for (msgtype=0; msgtype<32; msgtype++) {
			if (omcimsg_type_attr[msgtype].supported) {
				meinfo_me_attr_set_table_entry(me, 2, &msgtype, 0);
			}
		}
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_287 = {
	.get_cb			= meinfo_cb_287_get,
};
