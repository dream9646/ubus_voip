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
 * File    : meinfo_cb_281.c
 *
 ******************************************************************/

#include "meinfo_cb.h"
#include "util.h"

//classid 281 9.2.5 Multicast GEM interworking termination point

static int
get_service_profile_classid_by_281_iw_option(unsigned char iw_option)
{
	switch (iw_option) {
	case 0:	return 0;	// don't care
	case 1: return 45;	// MAC_bridge_service_profile
	case 3: return 68;	// IP_router_service_profile
	case 5: return 130;	// 802.1p_mapper_service_profile
	}
	dbprintf(LOG_ERR, "iw_option %d is undefined\n", iw_option);	
	return -1;
}

static int 
meinfo_cb_281_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==3) {	// iwtp pointer
		return get_service_profile_classid_by_281_iw_option(meinfo_util_me_attr_data(me, 2));
	} else if (attr_order==4) {	// Interworking_termination_point_pointer_NOT_USED
		return 0;
	} else if (attr_order==7) {	// GAL_profile_pointer_NOT_USED
		return 0;
	}
	return -1;
}

static int 
meinfo_cb_281_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==2) {		// tp type
		if (get_service_profile_classid_by_281_iw_option(attr->data)<0)
			return 0;
	}
	return 1;
}

static int 
meinfo_cb_281_entry_isdel(unsigned short classid, unsigned char attr_order, unsigned char *entry)
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
	current_pos += table_ptr->field[1].bit_width;
	bit_width=(4+4)*8;

	value = util_bitmap_get_value(entry, table_ptr->entry_byte_size*8, current_pos, bit_width);

	if (value == 0 )
		return MEINFO_CB_DELETE_ENTRY;
	else
		return MEINFO_CB_DELETE_NONE;
}

struct meinfo_callback_t meinfo_cb_281 = {
	.table_entry_isdel_cb	= meinfo_cb_281_entry_isdel,
	.get_pointer_classid_cb	= meinfo_cb_281_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_281_is_attr_valid,
};

