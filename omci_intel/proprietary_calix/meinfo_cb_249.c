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
 * File    : meinfo_cb_249.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo_cb.h"
#include "omciutil_vlan.h"
#include "util.h"

//classid 249 9.99.6 ExtL2VlanTagConfigData

static int
get_classid_by_249_assoc_type(unsigned char assoc_type)
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
meinfo_cb_249_get_pointer_classid(struct me_t *me, unsigned char attr_order)
{
	return get_classid_by_249_assoc_type(meinfo_util_me_attr_data(me, 1));
}

static int 
meinfo_cb_249_entry_isdel(unsigned short classid, unsigned char attr_order, unsigned char *entry)
{
	struct attrinfo_table_t *table_ptr;
	unsigned long long value;
	struct meinfo_t *miptr=meinfo_util_miptr(classid);

	if (miptr==NULL)
		return -1;

	table_ptr = miptr->attrinfo[attr_order].attrinfo_table_ptr;
	if( table_ptr == NULL){
		dbprintf(LOG_ERR, "classid=%u, null attrinfo_table_ptr?\n", classid);
		return -1;       
	}

	//get op code to check
	value = util_bitmap_get_value(entry, table_ptr->entry_byte_size*8, 0, 2);

	if (value == 2) //0 - reserved, 1 - add, 2 - delete, 3 reserved
		return MEINFO_CB_DELETE_ENTRY;
	else
		return MEINFO_CB_DELETE_NONE;
}

static int meinfo_cb_249_entry_cmp(unsigned short classid, unsigned char attr_order, unsigned char *entry1, unsigned char *entry2)
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

	//entry field index	
	if( attr_order == 5) {
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
meinfo_cb_249_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	if (attr_order==1) {		// tp type
		if (get_classid_by_249_assoc_type(attr->data)<0)
			return 0;
	}

	if (attr_order == 5) {
		int action = meinfo_me_attr_get_table_entry_action(me, attr_order, attr->ptr);

		if (action == ATTR_ENTRY_ACTION_ADD || action == ATTR_ENTRY_ACTION_MODIFY) {
			unsigned short input_tpid, output_tpid;
			struct attr_value_t attr_input_tpid, attr_output_tpid;
			struct omciutil_vlan_rule_fields_t rule_fields;

			memset(&rule_fields, 0x00, sizeof(struct omciutil_vlan_rule_fields_t));
			// fill to struct from entry
			meinfo_me_attr_get(me, 2, &attr_input_tpid);
			input_tpid = attr_input_tpid.data;
			meinfo_me_attr_get(me, 3, &attr_output_tpid);
			output_tpid = attr_output_tpid.data;

			//fill default fileds
			omciutil_vlan_collect_249_rules_fill_default_fields(&rule_fields);

			omciutil_vlan_collect_rule_fields_from_l2_table_entry((unsigned char *)attr->ptr, input_tpid, output_tpid, &rule_fields);

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

struct meinfo_callback_t meinfo_cb_calix_249 = {
	.table_entry_cmp_cb	= meinfo_cb_249_entry_cmp,
	.table_entry_isdel_cb	= meinfo_cb_249_entry_isdel,
	.get_pointer_classid_cb	= meinfo_cb_249_get_pointer_classid,
	.is_attr_valid_cb	= meinfo_cb_249_is_attr_valid,
};

