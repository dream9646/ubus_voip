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
 * File    : meinfo_cb_309.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "me_related.h"
#include "batchtab.h"
#include "util.h"

//classid 309 9.3.27 Multicast operations profile

static int meinfo_cb_309_entry_isdel(unsigned short classid, unsigned char attr_order, unsigned char *entry)
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
	//7: Dynamic_access_control_list_table (24byte,table,RW)
	//8: Static_access_control_list_table (24byte,table,RW)
	if( attr_order == 7 || attr_order == 8){
		//total 24 bytes, first field 2byte, use MSB bit 16..15
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

static int meinfo_cb_309_entry_cmp(unsigned short classid, unsigned char attr_order, unsigned char *entry1, unsigned char *entry2)
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
	//7: Dynamic_access_control_list_table (24byte,table,RW)
	//8: Static_access_control_list_table (24byte,table,RW)
	if( attr_order == 7 || attr_order == 8) {
		unsigned short index1 = util_bitmap_get_value(entry1, table_ptr->entry_byte_size*8, 2, 14);	// bit2..biy15
		unsigned short index2 = util_bitmap_get_value(entry2, table_ptr->entry_byte_size*8, 2, 14);	// bit2..biy15
		if (index1<index2)
			return MEINFO_CB_ENTRY_GREATER;
		else if (index1>index2)
			return MEINFO_CB_ENTRY_LESS;
		else
			return MEINFO_CB_ENTRY_MATCH;
	}
	dbprintf(LOG_ERR, "Attr order error classid=%u, attr_order=%d\n", classid, attr_order);
	return -1;       
}

static int
meinfo_cb_309_set(struct me_t *me, unsigned char attr_mask[2])
{
	if (util_attr_mask_get_bit(attr_mask, 1)) { // IGMP_version
		if (omci_env_g.igmp_version_overwrite) {
			struct attr_value_t attr = {0, NULL};
			attr.data = omci_env_g.igmp_version_overwrite;
			meinfo_me_attr_set(me, 1, &attr, 0);
		}
	}
	
	if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
	   (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU)) {
		batchtab_omci_update("alu_ds_mcastvlan");
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_309 = {
	.table_entry_cmp_cb	= meinfo_cb_309_entry_cmp,
	.table_entry_isdel_cb	= meinfo_cb_309_entry_isdel,
	.set_cb			= meinfo_cb_309_set,
};
