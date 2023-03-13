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
 * File    : meinfo_cb_49.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 49 9.3.6 MAC bridge port filter table data

static unsigned short 
meinfo_cb_49_meid_gen(unsigned short instance_num)
{
	struct me_t *parent_me=meinfo_me_get_by_instance_num(47, instance_num);
	if (!parent_me) {
		dbprintf(LOG_ERR, "unable to find parent me, classid=%u, instance=%u\n", 47, instance_num);
		return instance_num;
	}
	return parent_me->meid;
}

static int meinfo_cb_49_entry_isdel(unsigned short classid, unsigned char attr_order, unsigned char *entry)
{
	struct attrinfo_table_t *table_ptr;
	unsigned long long value;
	int current_pos, bit_width;
	struct meinfo_t *miptr=meinfo_util_miptr(classid);

	if (miptr==NULL)
		return -1;

	table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr;
	if( table_ptr == NULL){
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", classid);
		return -1;       
	}

	current_pos = table_ptr->field[0].bit_width;
	bit_width=table_ptr->field[1].bit_width;

	value = util_bitmap_get_value(entry, table_ptr->entry_byte_size*8, current_pos, bit_width);

	switch (value) {
		case 0:
			return MEINFO_CB_DELETE_ENTRY;
		case 1:
		default:
			return MEINFO_CB_DELETE_NONE;
		case 2:
			return MEINFO_CB_DELETE_ALL_ENTRY;
	}
}

struct meinfo_callback_t meinfo_cb_49 = {
	.meid_gen_cb	= meinfo_cb_49_meid_gen,
	.table_entry_isdel_cb	= meinfo_cb_49_entry_isdel,
};

