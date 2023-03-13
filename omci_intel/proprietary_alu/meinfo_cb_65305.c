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
 * Module  : proprietary_alu
 * File    : meinfo_cb_65305.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 65305 9.99.124 Downstream VLAN Tag Egress Behavior Supplemental 1

static unsigned short 
meinfo_cb_65305_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(11, instance_num);
	if (!parent_me || meinfo_me_is_exist(65305, parent_me->meid)) {
		parent_me = meinfo_me_get_by_instance_num(329, instance_num);
		if (!parent_me || meinfo_me_is_exist(65305, parent_me->meid)) {
			parent_me = meinfo_me_get_by_instance_num(134, instance_num);
			if (!parent_me || meinfo_me_is_exist(65305, parent_me->meid)) {
				dbprintf(LOG_ERR, "unable to find parent me, classid=%u or %u or %u, instance=%u\n", 11, 329, 134, instance_num);	
				return instance_num;
			}
		}
	}
	return parent_me->meid;
}

static int 
meinfo_cb_65305_entry_isdel(unsigned short classid, unsigned char attr_order, unsigned char *entry)
{
	unsigned char value = 0;
	struct attrinfo_table_t *table_ptr;
	struct meinfo_t *miptr=meinfo_util_miptr(classid);

	if (miptr==NULL)
		return -1;

	table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr;
	if( table_ptr == NULL)
	{
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", classid);
		return -1;       
	}

	//16th bit, add_remove. 0: remove , 1: add
	value = util_bitmap_get_value(entry, table_ptr->entry_byte_size*8, 8, 1);

	if (value == 0 )
		return MEINFO_CB_DELETE_ENTRY;
	else
		return MEINFO_CB_DELETE_NONE;
}


struct meinfo_callback_t meinfo_cb_alu_65305 = {
	.meid_gen_cb	= meinfo_cb_65305_meid_gen,
	.table_entry_isdel_cb	= meinfo_cb_65305_entry_isdel
};
