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
 * File    : meinfo_cb_310.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "meinfo_cb.h"
#include "me_related.h"
#include "util.h"

//classid 310 9.3.28 Multicast subscriber config info
static int
meinfo_cb_310_update_timeleft_in_preview_allowed_table(struct me_t *me)
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	struct attrinfo_table_t *table_ptr;
	struct attr_table_header_t *tab_header;
	struct attr_table_entry_t *tab_entry, *n_tab_entry;

	if (miptr==NULL)
		return -1;

	table_ptr = miptr->attrinfo[7].attrinfo_table_ptr;
	if( table_ptr == NULL){
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", me->classid);
		return -1;       
	}
	tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me , 7);
	if( tab_header == NULL){
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_header?\n", me->classid);
		return -1;       
	}
	list_for_each_entry_safe(tab_entry, n_tab_entry, &tab_header->entry_list, entry_node) {
		int duration, timeleft;
		if (tab_entry->entry_data_ptr==NULL)
			continue;
		// calc duration, timeleft in seconds while they are in minutes in mib
		duration=util_bitmap_get_value(tab_entry->entry_data_ptr, table_ptr->entry_byte_size*8, 18*8, 32)*60;	
		timeleft=duration-(time(0)-tab_entry->timestamp);
		if (timeleft>0) {
			util_bitmap_set_value(tab_entry->entry_data_ptr, table_ptr->entry_byte_size*8, 20*8, 32, timeleft/60);
		} else {
			// remove preview allowed entry if its timeleft==0
			//util_bitmap_set_value(tab_entry->entry_data_ptr, table_ptr->entry_byte_size*8, 20*8, 32, 0);
			tab_header->entry_count--;
			list_del(&tab_entry->entry_node);
			free_safe(tab_entry->entry_data_ptr);
			free_safe(tab_entry);
		}
	}
	return 0;       
}

static int 
meinfo_cb_310_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==6) {		// multicast_service_package_table
		if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
		     (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU))
			return 0;
	}
	return 1;
}

static int 
meinfo_cb_310_get(struct me_t *me, unsigned char attr_mask[2])
{
	int ret6 = 0, ret7 = 0;
	if (util_attr_mask_get_bit(attr_mask, 6)) {	// mcast_service_package_table
		// For ALU OLT, only supports 1:1 ME310->ME309 mapping
		// to solve untag IGMP packet cannot find ME309 issue if there is no untag ME171 rule
		if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
		     (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU))
			ret6 = MEINFO_ERROR_ATTR_UNDEF;	// disable this attr for alu olt
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) {	// preview allowed table
		ret7 = meinfo_cb_310_update_timeleft_in_preview_allowed_table(me);
	}
	if (ret6)
		return ret6;
	if (ret7)
		return ret7;
	return 0;       
}

static int 
meinfo_cb_310_set(struct me_t *me, unsigned char attr_mask[2])
{
	/*
	if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) &&
	     (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX)) {
		// Workaround for Calix OLT which cannot remove DS multicast tag on VEIP
		if (util_attr_mask_get_bit(attr_mask, 2) || util_attr_mask_get_bit(attr_mask, 6)) {
		   	struct me_t *meptr_47 = NULL;
			struct meinfo_t *miptr = meinfo_util_miptr(47);
		   	list_for_each_entry(meptr_47, &miptr->me_instance_list, instance_node) {
		   		if(me_related(meptr_47, me)) {
		   			if(meinfo_util_me_attr_data(meptr_47, 3) == 11) { // VEIP
						struct meinfo_t *miptr = meinfo_util_miptr(309);
					   	struct me_t *meptr_309 = NULL;
					   	list_for_each_entry(meptr_309, &miptr->me_instance_list, instance_node) {
						   	if(me_related(meptr_309, me)) {
							   	struct attr_value_t attr = {0, NULL};
								meinfo_me_attr_get(meptr_309, 16, &attr);
								util_bitmap_set_value(attr.ptr, 3*8, 0, 1*8, 1); // 1: Remove tag
								meinfo_me_attr_set(meptr_309, 16, &attr, 0);
								//break;
							}
						}
					}
					break;
				}
			}
		}
	}*/
	
	if (util_attr_mask_get_bit(attr_mask, 7)) {	// preview allowed table
		return meinfo_cb_310_update_timeleft_in_preview_allowed_table(me);		
	}
	return 0;       
}

static int meinfo_cb_310_entry_isdel(unsigned short classid, unsigned char attr_order, unsigned char *entry)
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
	//6: Multicast_service_package_table (30byte,table,RW)
	//7: Allowed_preview_groups_table (22byte,table,RW)
	if( attr_order == 6 || attr_order == 7){
		//use MSB bit 16..15
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

static int meinfo_cb_310_entry_cmp(unsigned short classid, unsigned char attr_order, unsigned char *entry1, unsigned char *entry2)
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
	//6: Multicast_service_package_table (30byte,table,RW)
	if( attr_order == 6) {
		return meinfo_me_attr_table_entry_cmp(classid, attr_order, entry1, entry2);
	}
	//7: Allowed_preview_groups_table (22byte,table,RW)
	if( attr_order == 7) {
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

struct meinfo_callback_t meinfo_cb_310 = {
	.get_cb			= meinfo_cb_310_get,
	.set_cb			= meinfo_cb_310_set,
	.is_attr_valid_cb	= meinfo_cb_310_is_attr_valid,
	.table_entry_cmp_cb	= meinfo_cb_310_entry_cmp,
	.table_entry_isdel_cb	= meinfo_cb_310_entry_isdel,
};

