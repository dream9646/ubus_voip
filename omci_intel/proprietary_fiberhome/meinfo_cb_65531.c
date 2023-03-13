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
 * File    : meinfo_cb_65531.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 309 9.3.x Extended Multicast operations profile

static int meinfo_cb_65531_entry_isdel(unsigned short classid, unsigned char attr_order, unsigned char *entry)
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
	//7: Dynamic_access_control_list_table (30byte,table,RW)
	//8: Static_access_control_list_table (30byte,table,RW)
	if( attr_order == 7 || attr_order == 8){
		//total 30 bytes, first field 2byte, use MSB bit 16..15
		current_pos = 0;
		bit_width = 2;
		value = util_bitmap_get_value(entry, table_ptr->entry_byte_size*8, current_pos, bit_width);

		if (value == 2)	//10 delete this entry
			return MEINFO_CB_DELETE_ENTRY;
		else if (value == 3)	//11 clear all entries
			return MEINFO_CB_DELETE_ALL_ENTRY;
		else
			return MEINFO_CB_DELETE_NONE;
	}
	dbprintf(LOG_ERR, "Attr order error classid=%u, attr_order=%d\n", classid, attr_order);
	return -1;       
}

static int meinfo_cb_65531_entry_cmp(unsigned short classid, unsigned char attr_order, unsigned char *entry1, unsigned char *entry2)
{
	struct attrinfo_table_t *table_ptr;
	struct meinfo_t *miptr=meinfo_util_miptr(classid);

	if (miptr==NULL)
		return -1;

	table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr;
	if( table_ptr == NULL){
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", classid);
		return -1;       
	}
	//7: Dynamic_access_control_list_table (30byte,table,RW)
	//8: Static_access_control_list_table (30byte,table,RW)
	if( attr_order == 7 || attr_order == 8) {
		unsigned short index1_row_part = util_bitmap_get_value(entry1, table_ptr->entry_byte_size*8, 2, 2);	// bit2..bit3
		unsigned short index2_row_part = util_bitmap_get_value(entry2, table_ptr->entry_byte_size*8, 2, 2);	// bit2..bit3
		unsigned short index1_row_id = util_bitmap_get_value(entry1, table_ptr->entry_byte_size*8, 7, 9);	// bit7..bit15
		unsigned short index2_row_id = util_bitmap_get_value(entry2, table_ptr->entry_byte_size*8, 7, 9);	// bit7..bit15
		if (index1_row_id<index2_row_id)
			return MEINFO_CB_ENTRY_GREATER;
		else if (index1_row_id>index2_row_id)
			return MEINFO_CB_ENTRY_LESS;
		else { // ==
			if (index1_row_part<index2_row_part)
				return MEINFO_CB_ENTRY_GREATER;
			else if (index1_row_part>index2_row_part)
				return MEINFO_CB_ENTRY_LESS;
			else // ==
				return MEINFO_CB_ENTRY_MATCH;
		}
	}
	dbprintf(LOG_ERR, "Attr order error classid=%u, attr_order=%d\n", classid, attr_order);
	return -1;       
}

struct meinfo_callback_t meinfo_cb_fiberhome_65531 = {
	.table_entry_cmp_cb	= meinfo_cb_65531_entry_cmp,
	.table_entry_isdel_cb	= meinfo_cb_65531_entry_isdel,
};

