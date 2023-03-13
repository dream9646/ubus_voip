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
 * File    : meinfo_cb_171.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo_cb.h"
#include "omciutil_vlan.h"
#include "util.h"

//classid 171 9.3.13 Extended VLAN tagging operation configuration data

static int
get_classid_by_171_assoc_type(unsigned char assoc_type)
{
	switch (assoc_type) {
	case 0: return 47;	// mac_bridge_port_configuration_data
	case 1: return 130;	// 802.1p mapper service
	case 2:	return 11;	// pptp_ethernet_uni
	case 3: return 134;	// ip host config data
	case 4: return 98;	// pptp_xdsl_part1
	case 5: return 266;	// gem iw tp
	case 6: return 281;	// mcast gem iwtp
	case 7: return 162;	// pptp moca uni
	case 8: return 91;	// pptp_802.11_uni
	case 9: return 286;	// ethernet flow tp
	case 10: return 329;	// virtual_ethernet
	}
	dbprintf(LOG_ERR, "assoc_type %d is undefined\n", assoc_type);	
	return -1;
}

static int
meinfo_cb_171_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	if (attr_order==7)
		return get_classid_by_171_assoc_type(meinfo_util_me_attr_data(me, 1));
	return -1;
}

static int 
meinfo_cb_171_entry_isdel(unsigned short classid, unsigned char attr_order, unsigned char *entry)
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

	//total 16 bytes, last eight bytes all 0xFF
	current_pos = 8*8;
	bit_width = 8*8;

	value = util_bitmap_get_value(entry, table_ptr->entry_byte_size*8, current_pos, bit_width);

	if (value == (long long) -1 )
		return MEINFO_CB_DELETE_ENTRY;
	else
		return MEINFO_CB_DELETE_NONE;
}

//return value: 0: not, 1: yes
static int
meinfo_cb_171_entry_cmp_is_default_rule(unsigned char *entry)
{
	unsigned long long field_value_outer_priority, field_value_inner_priority, field_value_ethertype;
	struct meinfo_t *miptr=meinfo_util_miptr(171);
	struct attrinfo_table_t *table_ptr;

	if (miptr==NULL)
		return 0;
	
	table_ptr = miptr->attrinfo[6].attrinfo_table_ptr;
	if( table_ptr == NULL){
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", 171);
		return 0;	
	}

	field_value_outer_priority = util_bitmap_get_value(entry, table_ptr->entry_byte_size*8,
						0, table_ptr->field[0].bit_width);

	field_value_inner_priority = util_bitmap_get_value(entry, table_ptr->entry_byte_size*8,
						32, table_ptr->field[4].bit_width);

	field_value_ethertype = util_bitmap_get_value(entry, table_ptr->entry_byte_size*8,
						60, table_ptr->field[8].bit_width);
	
	if ((field_value_outer_priority == 15 &&
		field_value_inner_priority == 15 &&
		field_value_ethertype == 0) || //no tags, Ethertype don't care
		(field_value_outer_priority == 15 &&
		field_value_inner_priority == 14 &&
		field_value_ethertype == 0) || //1 tag, Ethertype don't care
		(field_value_outer_priority == 14 &&
		field_value_inner_priority == 14 &&
		field_value_ethertype == 0)) //2 tags, Ethertype don't care
	{
		return 1;
	} else {
		return 0;
	}
}

//return value: -1: orig_entry < new_entry, 0: NOT one and only one is default rule, 1: orig_entry > new_entry
static int
meinfo_cb_171_entry_cmp_default_rule(unsigned char *orig_entry, unsigned char *new_entry)
{
	if ((meinfo_cb_171_entry_cmp_is_default_rule(orig_entry) ^
		meinfo_cb_171_entry_cmp_is_default_rule(new_entry)) == 1)
	{
		if (meinfo_cb_171_entry_cmp_is_default_rule(orig_entry))
		{
			return MEINFO_CB_ENTRY_LESS;
		} else {
			return MEINFO_CB_ENTRY_GREATER;
		}
	} else {
		return MEINFO_CB_ENTRY_MATCH;
	}
}

static int
meinfo_cb_171_entry_cmp(unsigned short classid, unsigned char attr_order, unsigned char *orig_entry, unsigned char *new_entry)
{
	int ret;
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	unsigned long long value_org, value_new;
	struct attrinfo_table_t *table_ptr;

	if (miptr==NULL)
		return -1;
	
	table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr;
	if( table_ptr == NULL){
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", classid);
		return -1;	
	}

	if ((ret = meinfo_cb_171_entry_cmp_default_rule(orig_entry, new_entry)) != MEINFO_CB_ENTRY_MATCH)
	{
		//one of them is default rule
		return ret;
	}

	value_org = util_bitmap_get_value(orig_entry, table_ptr->entry_byte_size*8, 0, 64);
	value_new = util_bitmap_get_value(new_entry, table_ptr->entry_byte_size*8, 0, 64);

	if (value_new > value_org)
		return MEINFO_CB_ENTRY_GREATER;
	else if (value_new < value_org)
		return MEINFO_CB_ENTRY_LESS;

	return MEINFO_CB_ENTRY_MATCH;
}

static int 
meinfo_cb_171_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==1) {		// tp type
		if (get_classid_by_171_assoc_type(attr->data)<0)
			return 0;
	}

	if (attr_order == 6) {
		int action = meinfo_me_attr_get_table_entry_action(me, attr_order, attr->ptr);

		if (action==ATTR_ENTRY_ACTION_ADD) {
			int entry_count = meinfo_me_attr_get_table_entry_count(me, attr_order);
			struct attr_value_t attr_max_table_size;
			
			meinfo_me_attr_get(me, 2, &attr_max_table_size);
			if (entry_count >= attr_max_table_size.data)
				return 0;
		}
		
		if (action == ATTR_ENTRY_ACTION_ADD || action == ATTR_ENTRY_ACTION_MODIFY) {
			unsigned short input_tpid, output_tpid;
			struct attr_value_t attr_input_tpid, attr_output_tpid;
			struct omciutil_vlan_rule_fields_t rule_fields;

			memset(&rule_fields, 0x00, sizeof(struct omciutil_vlan_rule_fields_t));
			// fill to struct from entry
			meinfo_me_attr_get(me, 3, &attr_input_tpid);
			input_tpid = attr_input_tpid.data;
			meinfo_me_attr_get(me, 4, &attr_output_tpid);
			output_tpid = attr_output_tpid.data;

			omciutil_vlan_collect_rule_fields_from_table_entry(attr->ptr, input_tpid, output_tpid, &rule_fields);

			// check valid
			if (!omciutil_vlan_is_valid_rule_fields(&rule_fields)) {
				dbprintf(LOG_ERR, "invalid vlan translation:\n");
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%s\n", omciutil_vlan_str_rule_fields(&rule_fields));
				util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%s\n", omciutil_vlan_str_desc(&rule_fields, "\n\t", "", 0));
				if (!omci_env_g.tag_treatment_correct)
					return 0;
			}
		}
	}

	return 1;
}

struct meinfo_callback_t meinfo_cb_171 = {
	.table_entry_cmp_cb	= meinfo_cb_171_entry_cmp,
	.table_entry_isdel_cb	= meinfo_cb_171_entry_isdel,
	.get_pointer_classid_cb	= meinfo_cb_171_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_171_is_attr_valid,
};

