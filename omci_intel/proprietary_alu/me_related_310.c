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
 * File    : me_related_310.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.3.28 [310] Multicast subscriber config info

// relation to 9.3.x [65531] Extended multicast operation profile
int 
me_related_310_65531(struct me_t *me1, struct me_t *me2)	
{
	struct attrinfo_table_t *attrinfo_table_ptr = meinfo_util_aitab_ptr(310,6);
	struct attr_table_header_t *table_header = (struct attr_table_header_t *) me1->attr[6].value.ptr;    

	// get table for extended_multicast_operations_profile_pointer
	if (attrinfo_table_ptr && table_header && !list_empty(&table_header->entry_list)) {
		struct attr_table_entry_t *table_entry_pos;
		
		// check multicast service package table
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				//get Extended_multicast_operations_profile_pointer
				unsigned short ext_pointer = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*attrinfo_table_ptr->entry_byte_size, 160, 16);
	
				if (me2->meid == ext_pointer)
				{
					return 1;
				}
			}
		}
	} else {
		struct attr_value_t attr;

		// check attr2 (multicast_operations_profile_pointer) 
		meinfo_me_attr_get(me1, 2, &attr);
		if ((unsigned short)(attr.data)==me2->meid)
		{
			return 1;
		}
	}

	return 0;
}
