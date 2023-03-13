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
 * File    : meinfo_cb_311.c
 *
 ******************************************************************/

#include <time.h>
#include "meinfo_cb.h"
#include "util.h"

//classid 311 9.3.29 Multicast subscriber monitor

static int 
meinfo_cb_311_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	struct attrinfo_table_t *table_ptr;
	struct attr_table_header_t *tab_header;
	struct attr_table_entry_t *tab_entry;

	if (miptr==NULL)
		return -1;

	if (util_attr_mask_get_bit(attr_mask, 5)) {	// active group list
		table_ptr = miptr->attrinfo[5].attrinfo_table_ptr;
		if( table_ptr == NULL){
			dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", me->classid);
			return -1;       
		}
		tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me , 5);
		list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
			// update "time since last join"
			util_bitmap_set_value(tab_entry->entry_data_ptr, table_ptr->entry_byte_size*8, 18*8, 32, time(0)-tab_entry->timestamp);	
		}
	}
       //Add for IPv6 MLD 
      if (util_attr_mask_get_bit(attr_mask, 6)) {
            table_ptr = miptr->attrinfo[6].attrinfo_table_ptr;
            if( table_ptr == NULL){
                  dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", me->classid);
                  return -1;
            }
            tab_header = (struct attr_table_header_t *)meinfo_util_me_attr_ptr(me, 6);
            list_for_each_entry(tab_entry, &tab_header->entry_list, entry_node) {
                // update "time since last join"
                util_bitmap_set_value(tab_entry->entry_data_ptr, table_ptr->entry_byte_size*8, 54*8, 32, time(0)-tab_entry->timestamp);
            }
      }
	return 0;       
}

struct meinfo_callback_t meinfo_cb_311 = {
	.get_cb	= meinfo_cb_311_get,
};

