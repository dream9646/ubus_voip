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
 * Module  : omciutil
 * File    : omciutil_vlan_collect.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "conv.h"
#include "meinfo.h"
#include "hwresource.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "omciutil_misc.h"
#include "me_related.h"
#include "switch.h"
#include "proprietary_alu.h"
#include "batchtab.h"
#include "batchtab_cb.h"

static inline void
er_attr_group_dbprintf_rule_fields(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	if (omci_env_g.er_group_hw_debug_enable)
		util_dbprintf(omci_env_g.debug_level, LOG_ERR, 0, "%s", omciutil_vlan_str_rule_fields(rule_fields));
}

int 
omciutil_vlan_collect_171_dscp2pbit_usage_by_port(struct me_t *bridge_port_me,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info,
	unsigned char force_flag)
{
	struct meinfo_t *miptr_171;
	struct me_t *me_pos_171;
	struct attr_value_t attr_value1, attr_value2;
	struct me_t *me;

	if (bridge_port_me == NULL ||
		dscp2pbit_info == NULL)
	{
		dbprintf(LOG_ERR, "param error\n");
		return -1;
	}

	//check 9.3.13, 171 Extended_VLAN_tagging_operation_configuration_data
	if ((miptr_171 = meinfo_util_miptr(171)) == NULL)
	{
		dbprintf(LOG_ERR, "get 171 miptr error\n");
		return -1;
	}

	list_for_each_entry(me_pos_171, &miptr_171->me_instance_list, instance_node)
	{
		//check association type
		meinfo_me_attr_get(me_pos_171, 1, &attr_value1);
		//get association pointer meid
		meinfo_me_attr_get(me_pos_171, 7, &attr_value2);
		switch (attr_value1.data)
		{
		case 0:
			//bridge port configuration data, 47
			if ((me = meinfo_me_get_by_meid(47, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=47, meid=%u, for class 171 related\n", attr_value2);
			} else {
				if (me == bridge_port_me)
				{
					omciutil_vlan_get_171_dscp2pbit_info(me_pos_171, dscp2pbit_info, force_flag);
				}
			}
			break;
		case 1:
			//802.1p mapper service profile, 130
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(130, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=130, meid=%u, for class 171 related\n", attr_value2);
			} else {
				if (me_related(bridge_port_me, me) == 1) //47->130
				{
					omciutil_vlan_get_171_dscp2pbit_info(me_pos_171, dscp2pbit_info, force_flag);
				}
			}
			break;
		case 2:
			//pptp ethernet uni, 11
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(11, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=11, meid=%u, for class 171 related\n", attr_value2);
			} else {
				if (me_related(bridge_port_me, me) == 1) //47->11
				{
					omciutil_vlan_get_171_dscp2pbit_info(me_pos_171, dscp2pbit_info, force_flag);
				}
			}
			break;
		case 3:
			//IP host config data, 134
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(134, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=134, meid=%u, for class 171 related\n", attr_value2);
			} else {
				if (me_related(bridge_port_me, me) == 1) //47->134
				{
					omciutil_vlan_get_171_dscp2pbit_info(me_pos_171, dscp2pbit_info, force_flag);
				}
			}
			break;
		case 10:
			//VEIP, 329
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(329, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=329, meid=%u, for class 171 related\n", attr_value2);
			} else {
				if (me_related(bridge_port_me, me) == 1) //47->329
				{
					omciutil_vlan_get_171_dscp2pbit_info(me_pos_171, dscp2pbit_info, force_flag);
				}
			}
			break;
		//others not supported
		default:
			dbprintf(LOG_ERR, "not supported classid=171, meid=%u association type=%u\n", me_pos_171->meid, attr_value1.data);
		}
	}

	return 0;
}

int 
omciutil_vlan_collect_130_dscp2pbit_usage_by_port(struct me_t *bridge_port_me,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info)
{
	struct me_t *me_130;
	struct attr_value_t attr_value1={0, NULL}, attr_value2={0, NULL}, attr_untag_option={0, NULL}, attr_dscp2pbit_bitmap={0, NULL};

	if (bridge_port_me == NULL ||
		dscp2pbit_info == NULL)
	{
		dbprintf(LOG_ERR, "param error\n");
		return -1;
	}

	//check bridge port 
	meinfo_me_attr_get(bridge_port_me, 3, &attr_value1); //determine me type, 3 is 130, 802.1p
	meinfo_me_attr_get(bridge_port_me, 4, &attr_value2); //tp pointer
	
	if (attr_value1.data == 3 && 
		(me_130 = meinfo_me_get_by_meid(130, attr_value2.data)) != NULL)
	{
		//check untag pbit assignment option
		meinfo_me_attr_get(me_130, 10, &attr_untag_option);
		
		if (attr_untag_option.data == 0) // from dscp
		{
			meinfo_me_attr_get(me_130, 11, &attr_dscp2pbit_bitmap); //ptr
			//get dscp to pbit table, analyse the table to extract result pbit mask
			omciutil_vlan_analyse_dscp2pbit_bitmap(attr_dscp2pbit_bitmap.ptr, dscp2pbit_info->table, &dscp2pbit_info->pbit_mask);			
			dscp2pbit_info->enable = 1; //dscp-2-pbit
			meinfo_me_attr_clear_value(130, 11, &attr_dscp2pbit_bitmap);
			dscp2pbit_info->from_me = me_130;
		} else {
			dscp2pbit_info->enable = 0;

		}
	}

	return 0;
}

int 
omciutil_vlan_collect_dscp2pbit_info_global(struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info)
{
	unsigned char lowest_port_type = 255, lowest_port_type_index = 255;
	unsigned short bridge_meid;
	struct meinfo_t *miptr_47;
	struct me_t *me_pos_47;
	struct switch_port_info_t each_port_info;
	struct omciutil_vlan_dscp2pbit_info_t dscp2pbit_info_temp;
	
	if (dscp2pbit_info == NULL)
	{
		dbprintf(LOG_ERR, "param error\n");
		return -1;
	}

	//get dscp2pbit info with proprietary policy
	if ((miptr_47 = meinfo_util_miptr(47)) == NULL)
	{
		dbprintf(LOG_ERR, "get 47 miptr error\n");
		return -1;
	} else {
		if (omci_env_g.classf_dscp2pbit_mode == 2 || //force to assign veip 171 dscp-2-pbit mapping table
			autouni_is_required(&bridge_meid))
		{
			//for ALU pvlan, only find CPU(VEIP) port 171 dscp-2-pbit table
			list_for_each_entry(me_pos_47, &miptr_47->me_instance_list, instance_node)
			{
				if (switch_get_port_info_by_me(me_pos_47, &each_port_info) != 0)
				{
					continue;
				}

				memset(&dscp2pbit_info_temp, 0x00, sizeof(struct omciutil_vlan_dscp2pbit_info_t));

				if (each_port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU)
				{
					//force to get dscp2pbit info with own 171 me
					omciutil_vlan_collect_171_dscp2pbit_usage_by_port(me_pos_47, &dscp2pbit_info_temp, 1);

					if (dscp2pbit_info_temp.enable &&
						each_port_info.port_type_index < lowest_port_type_index)
					{
						lowest_port_type_index = each_port_info.port_type_index;
						*dscp2pbit_info = dscp2pbit_info_temp;
					}
				}
			}
		} else {
			//current policy
			//find lowest port type and lowest port type index, capable it's dscp2pbit
			list_for_each_entry(me_pos_47, &miptr_47->me_instance_list, instance_node)
			{
				if (switch_get_port_info_by_me(me_pos_47, &each_port_info) != 0)
				{
					continue;
				}

				memset(&dscp2pbit_info_temp, 0x00, sizeof(struct omciutil_vlan_dscp2pbit_info_t));

				switch(each_port_info.port_type)
				{
				case ENV_BRIDGE_PORT_TYPE_CPU:
				case ENV_BRIDGE_PORT_TYPE_UNI:
				case ENV_BRIDGE_PORT_TYPE_IPHOST: //lan port, get 171 for dscp to pbit
					//get dscp2pbit info with own 171 me
					omciutil_vlan_collect_171_dscp2pbit_usage_by_port(me_pos_47, &dscp2pbit_info_temp, 0);
					break;
				case ENV_BRIDGE_PORT_TYPE_WAN: //wan port get 130 for dscp to pbit
					//get dscp2pbit info with own 130 me
					omciutil_vlan_collect_130_dscp2pbit_usage_by_port(me_pos_47, &dscp2pbit_info_temp);
					break;
				default:
					continue; //do nothing, go next one
				}
				if (dscp2pbit_info_temp.enable == 1 &&  //enabled
					(each_port_info.port_type < lowest_port_type || //lowest port type and lowest port type index
					(each_port_info.port_type == lowest_port_type && each_port_info.port_type_index < lowest_port_type_index)))
				{
					lowest_port_type = each_port_info.port_type;
					lowest_port_type_index = each_port_info.port_type_index;
					*dscp2pbit_info = dscp2pbit_info_temp;
				}
			}
		}
	}

	return 0;
}

int 
omciutil_vlan_collect_dscp2pbit_info_by_port(struct me_t *bridge_port_me,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info)
{
	unsigned short bridge_meid;
	struct switch_port_info_t port_info;

	//get port info
	if (bridge_port_me == NULL || dscp2pbit_info == NULL ||
		switch_get_port_info_by_me(bridge_port_me, &port_info) != 0)
	{
		dbprintf(LOG_ERR, "param error\n");
		return -1;
	}

	//reset
	memset(dscp2pbit_info, 0x00, sizeof(struct omciutil_vlan_dscp2pbit_info_t));

	if (omci_env_g.classf_dscp2pbit_mode == 0 &&
		!autouni_is_required(&bridge_meid))
	{
		switch(port_info.port_type)
		{
		case ENV_BRIDGE_PORT_TYPE_CPU:
		case ENV_BRIDGE_PORT_TYPE_UNI:
		case ENV_BRIDGE_PORT_TYPE_IPHOST: //lan port, get 171 for dscp to pbit
			//get dscp2pbit info with own 171 me
			omciutil_vlan_collect_171_dscp2pbit_usage_by_port(bridge_port_me, dscp2pbit_info, 0);
			break;
		case ENV_BRIDGE_PORT_TYPE_WAN: //wan port get 130 for dscp to pbit
			//get dscp2pbit info with own 130 me
			omciutil_vlan_collect_130_dscp2pbit_usage_by_port(bridge_port_me, dscp2pbit_info);
			break;
		default:
			; //do nothing
		}
	} else {
		omciutil_vlan_collect_dscp2pbit_info_global(dscp2pbit_info);
	}

	return 0;
}

//insert entry_fields to list by increasing order, reject the same one
int
omciutil_vlan_insert_rule_fields_to_list(struct omciutil_vlan_rule_fields_t *rule_fields,
	struct list_head *rule_list)
{
	int ret;
	struct omciutil_vlan_rule_fields_t *rule_fields_pos;
	struct list_head *insert_point;
	
	insert_point = rule_list;

	list_for_each_entry(rule_fields_pos, rule_list, rule_node)
	{
		if ((ret = omciutil_vlan_compare_rule_fields(rule_fields, rule_fields_pos)) == 0)
		{
			//the same, no need to insert
			return -1;
		} else {
			if (ret < 0) {
				break;
			}
			//otherwise, check next one
		}
		insert_point = &rule_fields_pos->rule_node;
	}

	//insert;
	list_add(&rule_fields->rule_node, insert_point);

	return 0;
}

//from class 171 table entry
void
omciutil_vlan_collect_rule_fields_from_table_entry(unsigned char *entry,
	unsigned short input_tpid,
	unsigned short output_tpid,
	struct omciutil_vlan_rule_fields_t *rule_fields)
{
	int i, entry_size, bit_start = 0;
	struct attrinfo_table_t *attrinfo_table_ptr;
	
	attrinfo_table_ptr = meinfo_util_aitab_ptr(171,6);
	entry_size = attrinfo_table_ptr->entry_byte_size;

	//extract entry fields
 	for (i = 0; i < attrinfo_table_ptr->field_total; i++)
	{
		if ( i == 0) {
			bit_start = 0;
		} else {
			bit_start += attrinfo_table_ptr->field[i-1].bit_width;
		}
		switch(i)
		{
		case 0:
			rule_fields->filter_outer.priority = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 1:
			rule_fields->filter_outer.vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 2:
			rule_fields->filter_outer.tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 3:
			rule_fields->filter_outer.pad = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 4:
			rule_fields->filter_inner.priority = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 5:
			rule_fields->filter_inner.vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 6:
			rule_fields->filter_inner.tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 7:
			rule_fields->filter_inner.pad = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 8:
			rule_fields->filter_ethertype = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 9:
			rule_fields->treatment_tag_to_remove = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 10:
			rule_fields->treatment_outer.pad = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 11:
			rule_fields->treatment_outer.priority = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 12:
			rule_fields->treatment_outer.vid = util_bitmap_get_value(entry, 8*entry_size, bit_start,  attrinfo_table_ptr->field[i].bit_width);
			break;
		case 13:
			rule_fields->treatment_outer.tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 14:
			rule_fields->treatment_inner.pad = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 15:
			rule_fields->treatment_inner.priority = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 16:
			rule_fields->treatment_inner.vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 17:
			rule_fields->treatment_inner.tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		default:
			; //do nothing
		}
	}

	rule_fields->input_tpid = input_tpid;
	rule_fields->output_tpid = output_tpid;

	return;
}

//from class 249 table entry
void
omciutil_vlan_collect_rule_fields_from_l2_table_entry(unsigned char *entry,
	unsigned short input_tpid,
	unsigned short output_tpid,
	struct omciutil_vlan_rule_fields_t *rule_fields)
{
	int i, j, entry_size, bit_start = 0;
	struct attrinfo_table_t *attrinfo_table_ptr;
	
	attrinfo_table_ptr = meinfo_util_aitab_ptr(249,5);
	entry_size = attrinfo_table_ptr->entry_byte_size;

	//extract entry fields
 	for (i = 0; i < attrinfo_table_ptr->field_total; i++)
	{
		if ( i == 0) {
			bit_start = 0;
		} else {
			bit_start += attrinfo_table_ptr->field[i-1].bit_width;
		}
		switch(i)
		{
		//skip field 0
		case 1:
			rule_fields->l2_filter.index = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 2:
			//mac mask
			for (j = 0; j < 6; j++)
			{
				rule_fields->l2_filter.mac_mask[j] = util_bitmap_get_value(entry, 8*entry_size, (j * 8) + bit_start, 8);
			}
			break;
		case 3:
			//mac
			for (j = 0; j < 6; j++)
			{
				rule_fields->l2_filter.mac[j] = util_bitmap_get_value(entry, 8*entry_size, (j * 8) + bit_start, 8);
			}
			break;
		case 4:
			//inner tpid
			rule_fields->treatment_inner.tpid_de = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 5:
			//inner vid
			rule_fields->treatment_inner.vid = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		case 8:
			//inner priority
			rule_fields->treatment_inner.priority = util_bitmap_get_value(entry, 8*entry_size, bit_start, attrinfo_table_ptr->field[i].bit_width);
			break;
		default:
			; //do nothing
		}
	}

	rule_fields->input_tpid = input_tpid;
	rule_fields->output_tpid = output_tpid;

	return;
}


//1: auto reverse, else not
static char
omciutil_vlan_check_auto_reverse(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	if (rule_fields == NULL)
	{
		return 0;
	}

	if (rule_fields->treatment_tag_to_remove == 0 && omciutil_vlan_get_rule_treatment_tag_num(rule_fields) == 0
		&& strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)!=0)
	{
		return 1;
	}

	return 0;
}

static int
omciutil_vlan_expand_dscp2pbit_reversing_rule_to_list_ds(struct omciutil_vlan_rule_fields_t *rule_fields_reverse,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info,
	struct list_head *rule_list_ds)
{
	unsigned int i;
	unsigned char pbit_mask;

	struct omciutil_vlan_rule_fields_t *rule_fields_reverse_new;

	if (rule_fields_reverse == NULL || dscp2pbit_info == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	if (dscp2pbit_info->enable == 1 &&
		dscp2pbit_info->pbit_mask != 0 &&
		(rule_fields_reverse->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13 ||
				rule_fields_reverse->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)) //enable
	{
		//check
		if (dscp2pbit_info->pbit_mask == 0xff) //all pbit, set filter to don't care
		{
			rule_fields_reverse_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			*rule_fields_reverse_new = *rule_fields_reverse;

			if (rule_fields_reverse->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
			{
				rule_fields_reverse_new->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
			}
			if (rule_fields_reverse->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
			{
				rule_fields_reverse_new->filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
			}

			//insert to downstream
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_reverse_new, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields_reverse_new);

				//release entry_fields_reverse
				free_safe(rule_fields_reverse_new);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields_reverse_new);
			}
		} else {
			for (i = 0; i < 8; i++)
			{
				pbit_mask = (0x1 << i);
				if ((pbit_mask & dscp2pbit_info->pbit_mask)) // add this pbit for filter
				{
					rule_fields_reverse_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
					*rule_fields_reverse_new = *rule_fields_reverse;

					if (rule_fields_reverse->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
					{
						rule_fields_reverse_new->filter_inner.priority = i;
					}
					if (rule_fields_reverse->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
					{
						rule_fields_reverse_new->filter_outer.priority = i;
					}

					//insert to downstream
					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_reverse_new, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_reverse_new);

						//release entry_fields_reverse
						free_safe(rule_fields_reverse_new);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_reverse_new);
					}
				}
			}
		}
	} else {
		dbprintf(LOG_ERR, "dscp-2-pbit not enable for rule expanding\n");
		return -1;
	}

	return 0;
}

static void
omciutil_vlan_process_rule_fields_check_tlan_workaround_flag(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	int rule_tag_num_filter, rule_tag_num_treatment;

	if (rule_fields == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return;
	}

	rule_tag_num_filter = omciutil_vlan_get_rule_filter_tag_num(rule_fields);
	rule_tag_num_treatment = omciutil_vlan_get_rule_treatment_tag_num(rule_fields);

	if ((rule_tag_num_filter == 1 &&
		rule_fields->treatment_tag_to_remove == 1 &&
		rule_tag_num_treatment == 1 &&
		(rule_fields->filter_inner.vid == rule_fields->treatment_inner.vid ||
		rule_fields->treatment_inner.vid == 4096)) || //copy from inner
		(rule_tag_num_filter == 1 &&
		rule_fields->treatment_tag_to_remove == 0 &&
		rule_tag_num_treatment == 0)) {
		//1, one tag, remove one, adds one for transparent
		//2, one tag, transparent
		rule_fields->tlan_workaround_flag = 1;
	} else if (rule_tag_num_filter == 0 &&
		rule_tag_num_treatment == 1) {
		//1, untag, add one
		rule_fields->tlan_workaround_flag = 2;
	} else if ((rule_tag_num_filter == 1 &&
		rule_fields->treatment_tag_to_remove == 0 &&
		rule_tag_num_treatment == 1) ||
		(rule_tag_num_filter == 1 &&
		rule_fields->treatment_tag_to_remove == 1 &&
		rule_tag_num_treatment == 2)) {
		//1, one tag, adds one
		//2, one tag, remove one, add two
		rule_fields->tlan_workaround_flag = 3;
	} else if (rule_tag_num_filter == 1 &&
		rule_fields->treatment_tag_to_remove == 1 &&
		rule_tag_num_treatment == 1 &&
		(rule_fields->filter_inner.vid != rule_fields->treatment_inner.vid ||
		rule_fields->treatment_inner.vid != 4096)) { //NOT copy from inner
		//1. change tag
		//2. copy from inner
		rule_fields->tlan_workaround_flag = 4;
	}
	//otherwise, do nothing

	return;
}

// according the reversing rule to compare with tlan vid in the same bridge,
// if this is tlan rule, try to set workaround flag.
static void
omciutil_vlan_process_rule_fields_check_tlan_workaround(struct omciutil_vlan_rule_fields_t *rule_fields_reverse,
				struct omciutil_vlan_rule_fields_t *rule_fields,
				struct omciutil_vlan_tlan_table_t *tlan_table_bridge)
{
	int rule_tag_num;
	unsigned short vid;
	int i;

	if (rule_fields_reverse == NULL ||
		rule_fields == NULL || 
		tlan_table_bridge == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return;
	}

	if (tlan_table_bridge->total > 0) //check for with tlan
	{
		rule_tag_num = omciutil_vlan_get_rule_filter_tag_num(rule_fields_reverse);

		switch (rule_tag_num)
		{
		case 1:
			vid = rule_fields_reverse->filter_inner.vid;
			break;
		case 2:
			vid = rule_fields_reverse->filter_outer.vid;
			break;
		default:
			return; //impossible
		}

		if (vid <= 4095)
		{
			for (i = 0; i < tlan_table_bridge->total; i++)
			{
				if (vid == tlan_table_bridge->vid[i])
				{
					//check tpid_flag
					omciutil_vlan_process_rule_fields_check_tlan_workaround_flag(rule_fields);
					rule_fields->tlan_vid = tlan_table_bridge->vid[i]; //for meid
					break;
				}
			}
		} else if (vid == 4096) { //wildcard term, one to many
			//check tpid_flag
			omciutil_vlan_process_rule_fields_check_tlan_workaround_flag(rule_fields);
			rule_fields->tlan_vid = tlan_table_bridge->vid[0]; //for meid, set to first tlan
		}
		//otherwise, do nothing
	}

	return;
}

static void
omciutil_vlan_process_rule_fields(struct omciutil_vlan_rule_fields_t *rule_fields,
	unsigned char ds_mode,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds,
	struct omciutil_vlan_tlan_table_t *tlan_table_bridge)
{
	struct omciutil_vlan_rule_fields_t *rule_fields_reverse;
	char auto_reverse_flag;

	if (rule_fields == NULL || rule_list_us == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return;
	}

	auto_reverse_flag = omciutil_vlan_check_auto_reverse(rule_fields);

	//reverse
	if (rule_fields->unreal_flag == 0 && (ds_mode == 0 || (omci_env_g.tag_auto_reverse == 1 && ds_mode == 1 && auto_reverse_flag == 1))) //0 is reverse mode
	{
		//allocate an rule_fields_reverse and fill it
		rule_fields_reverse = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields_reverse->owner_me=rule_fields->owner_me;
		rule_fields_reverse->bport_me=rule_fields->bport_me;
		rule_fields_reverse->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;

		if (ds_mode == 1 && auto_reverse_flag == 1)
		{
			rule_fields_reverse->unreal_flag = 1; //unreal
		} else {
			rule_fields_reverse->unreal_flag = 0; //real
		}

		if (omciutil_vlan_reverse_rule_generate(rule_fields, rule_fields_reverse) == 0)
		{
			//use the reversing rule to check with tlan_table_bridge to find tlan workaround functions
			if (tlan_table_bridge != NULL)
			{
				omciutil_vlan_process_rule_fields_check_tlan_workaround(rule_fields_reverse, rule_fields, tlan_table_bridge);
			}
			
			//check for dscp-2-pbit reversing
			if (rule_fields_reverse->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13 ||
				rule_fields_reverse->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
			{
				//expand dscp-2-pbit bitmap
				if (omciutil_vlan_expand_dscp2pbit_reversing_rule_to_list_ds(rule_fields_reverse, dscp2pbit_info, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "generate reverse rule error\n");
					//insert to downstream
					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_reverse, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_reverse);

						//release entry_fields_reverse
						free_safe(rule_fields_reverse);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_reverse);
					}
				} else {
					//expand ok, free original one
					free_safe(rule_fields_reverse);
				}
			} else {
				//insert to downstream
				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_reverse, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields_reverse);

					//release entry_fields_reverse
					free_safe(rule_fields_reverse);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields_reverse);
				}
			}
		} else {
			if (rule_fields->treatment_tag_to_remove < 3) // >= 3 , discard treats as normal
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "generate reverse rule error\n");
			}
			//release entry_fields_reverse
			free_safe(rule_fields_reverse);
		}
	}

	//insert to list by correct order;
	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
	{
		er_attr_group_dbprintf(LOG_ERR, 0, "US-insert to us list error: ");
		er_attr_group_dbprintf_rule_fields(rule_fields);

		//release entry_fields
		free_safe(rule_fields);
	} else {
		er_attr_group_dbprintf(LOG_ERR, 0, "US: ");
		er_attr_group_dbprintf_rule_fields(rule_fields);
	}

	return;
}

static void
omciutil_vlan_process_l2_rule_fields(struct omciutil_vlan_rule_fields_t *rule_fields,
	unsigned char ds_mode,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds,
	struct omciutil_vlan_tlan_table_t *tlan_table_bridge)
{
	struct omciutil_vlan_rule_fields_t *rule_fields_reverse;

	if (rule_fields == NULL || rule_list_us == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return;
	}

	//reverse
	if (rule_fields->unreal_flag == 0 && ds_mode == 0) //0 is reverse mode
	{
		//allocate an rule_fields_reverse and fill it
		rule_fields_reverse = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields_reverse->owner_me=rule_fields->owner_me;
		rule_fields_reverse->bport_me=rule_fields->bport_me;
		rule_fields_reverse->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_HIGHER_2;
		rule_fields_reverse->unreal_flag = 0; //real
		if (omci_env_g.tag_l2_rules_ds_mac)
		{
			//enable l2 filter
			rule_fields_reverse->l2_filter_enable = 1;
			//copy l2 filter
			rule_fields_reverse->l2_filter = rule_fields->l2_filter;
		}

		if (omciutil_vlan_reverse_rule_generate(rule_fields, rule_fields_reverse) == 0)
		{
			//use the reversing rule to check with tlan_table_bridge to find tlan workaround functions
			if (tlan_table_bridge != NULL)
			{
				omciutil_vlan_process_rule_fields_check_tlan_workaround(rule_fields_reverse, rule_fields, tlan_table_bridge);
			}
			
			//check for dscp-2-pbit reversing
			if (rule_fields_reverse->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13 ||
				rule_fields_reverse->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
			{
				//expand dscp-2-pbit bitmap
				if (omciutil_vlan_expand_dscp2pbit_reversing_rule_to_list_ds(rule_fields_reverse, dscp2pbit_info, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "generate reverse rule error\n");
					//insert to downstream
					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_reverse, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_reverse);

						//release entry_fields_reverse
						free_safe(rule_fields_reverse);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_reverse);
					}
				} else {
					//expand ok, free original one
					free_safe(rule_fields_reverse);
				}
			} else {
				//insert to downstream
				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_reverse, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields_reverse);

					//release entry_fields_reverse
					free_safe(rule_fields_reverse);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields_reverse);
				}
			}
		} else {
			if (rule_fields->treatment_tag_to_remove < 3) // >= 3 , discard treats as normal
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "generate reverse rule error\n");
			}
			//release entry_fields_reverse
			free_safe(rule_fields_reverse);
		}
	}

	//insert to list by correct order;
	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
	{
		er_attr_group_dbprintf(LOG_ERR, 0, "US-insert to us list error: ");
		er_attr_group_dbprintf_rule_fields(rule_fields);

		//release entry_fields
		free_safe(rule_fields);
	} else {
		er_attr_group_dbprintf(LOG_ERR, 0, "US: ");
		er_attr_group_dbprintf_rule_fields(rule_fields);
	}

	return;
}


static int
omciutil_vlan_collect_alu_vid_ethertype_workaround(struct omciutil_vlan_rule_fields_t *rule_fields, unsigned char ds_mode,
				struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info, struct list_head *rule_list_us, struct list_head *rule_list_ds)
{
	struct omciutil_vlan_rule_fields_t *rule_fields_new;

	if (rule_fields == NULL)
	{
		return -1;
	}

	if (rule_fields->filter_inner.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15)
	{
		//check inner filter vid
		switch(rule_fields->filter_inner.vid)
		{
		case 4081:
			//set ethertype to 14, IP/ARP
			if (rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_IP_ARP;
				rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;

				//set outer filter vid and tpid/de fields to don't care, for sorting depend on "inner filter vid"
				rule_fields->filter_outer.vid = 4096;
				rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;

				if (rule_fields->treatment_tag_to_remove < 3)
				{
					rule_fields->treatment_tag_to_remove = 0;
				}
			} else {
				//warning
				dbprintf(LOG_ERR, "outer filter not empty!!\n");
			}
			break;
		case 4082:
			//set ethertype to 2, PPPOE
			if (rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_PPPOE;
				rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;

				//set outer filter vid and tpid/de fields to don't care, for sorting depend on "inner filter vid"
				rule_fields->filter_outer.vid = 4096;
				rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;

				if (rule_fields->treatment_tag_to_remove < 3)
				{
					rule_fields->treatment_tag_to_remove = 0;
				}
			} else {
				//warning
				dbprintf(LOG_ERR, "outer filter not empty!!\n");
			}
			break;
		case 4083:
			//set ethertype to 15, IPv6
			if (rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_IPV6;
				rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;

				//set outer filter vid and tpid/de fields to don't care, for sorting depend on "inner filter vid"
				rule_fields->filter_outer.vid = 4096;
				rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;

				if (rule_fields->treatment_tag_to_remove < 3)
				{
					rule_fields->treatment_tag_to_remove = 0;
				}
			} else {
				//warning
				dbprintf(LOG_ERR, "outer filter not empty!!\n");
			}
			break;
		case 4084:
			//set ethertype to others (default)
			if (rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_DEFAULT;
				rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;

				//set outer filter vid and tpid/de fields to don't care, for sorting depend on "inner filter vid"
				rule_fields->filter_outer.vid = 4096;
				rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;

				if (rule_fields->treatment_tag_to_remove < 3)
				{
					rule_fields->treatment_tag_to_remove = 0;
				}
			} else {
				//warning
				dbprintf(LOG_ERR, "outer filter not empty!!\n");
			}
			break;
		case 4085:
			// TLS
			if (rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				//proprietary_alu_tls_mode(1); // Move to batchtab
				
				rule_fields->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_DEFAULT;
				rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;

				//set outer filter vid and tpid/de fields to don't care, for sorting depend on "inner filter vid"
				rule_fields->filter_outer.vid = 4096;
				rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;

				if (rule_fields->treatment_tag_to_remove < 3)
				{
					rule_fields->treatment_tag_to_remove = 0;
				}

				//add a brand new rule for one tag
				rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				memcpy(rule_fields_new, rule_fields, sizeof(struct omciutil_vlan_rule_fields_t));

				//assign for inner tag filter
				rule_fields_new->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
				rule_fields_new->filter_inner.vid = 4096;
				rule_fields_new->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
				
				omciutil_vlan_process_rule_fields(rule_fields_new, ds_mode, dscp2pbit_info, rule_list_us, rule_list_ds, NULL);
			} else {
				//warning
				dbprintf(LOG_ERR, "outer filter not empty!!\n");
			}
			break;
		case 4086:
			// New SLA, set unreal_flag to 1, do not reverse and set to hw
			if (rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->unreal_flag = 1; //unreal;
			} else {
				//warning
				dbprintf(LOG_ERR, "outer filter not empty!!\n");
			}
			break;
		
		case 4087:
			//set ethertype to 13, IP/ARP/IPv6
			if (rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_IP_ARP_IPV6;
				rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;

				//set outer filter vid and tpid/de fields to don't care, for sorting depend on "inner filter vid"
				rule_fields->filter_outer.vid = 4096;
				rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;

				if (rule_fields->treatment_tag_to_remove < 3)
				{
					rule_fields->treatment_tag_to_remove = 0;
				}
			} else {
				//warning
				dbprintf(LOG_ERR, "outer filter not empty!!\n");
			}
			break;
		case 4088:
			//set ethertype to 12, PPPoE/IPv6
			if (rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_PPPOE_IPV6;
				rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;

				//set outer filter vid and tpid/de fields to don't care, for sorting depend on "inner filter vid"
				rule_fields->filter_outer.vid = 4096;
				rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;

				if (rule_fields->treatment_tag_to_remove < 3)
				{
					rule_fields->treatment_tag_to_remove = 0;
				}
			} else {
				//warning
				dbprintf(LOG_ERR, "outer filter not empty!!\n");
			}
			break;
		default:
			;//do nothing
		}
	}

	return 0;
}
#if 0 // ALU decides to use VLAN=4085 for TLS with ALU 7342/7360 OLT
static int
omciutil_vlan_collect_alu_tls_workaround(struct omciutil_vlan_rule_fields_t *rule_fields, unsigned char ds_mode,
				struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info, struct list_head *rule_list_us, struct list_head *rule_list_ds)
{
	if (rule_fields == NULL)
	{
		return -1;
	}

	if (rule_fields->treatment_inner.vid == 4095) // TLS enable for ALU OLT
	{
		struct omciutil_vlan_rule_fields_t *rule_fields_double_tag;
		
		//add a brand new rule for double tag
		rule_fields_double_tag = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		memcpy(rule_fields_double_tag, rule_fields, sizeof(struct omciutil_vlan_rule_fields_t));
		//assign for double tag filter
		rule_fields_double_tag->filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
		rule_fields_double_tag->filter_outer.vid = 4096;
		rule_fields_double_tag->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_101;
		rule_fields_double_tag->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
		rule_fields_double_tag->filter_inner.vid = 4096;
		rule_fields_double_tag->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_101;
		rule_fields_double_tag->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields_double_tag->treatment_tag_to_remove = 0;
		rule_fields_double_tag->treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields_double_tag->treatment_outer.vid = 0;
		rule_fields_double_tag->treatment_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
		rule_fields_double_tag->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields_double_tag->treatment_inner.vid = 0;
		rule_fields_double_tag->treatment_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
		omciutil_vlan_process_rule_fields(rule_fields_double_tag, ds_mode, dscp2pbit_info, rule_list_us, rule_list_ds);
	}

	return 0;
}
#endif
static int
omciutil_vlan_collect_huawei_tls_workaround(struct omciutil_vlan_rule_fields_t *rule_fields, unsigned char ds_mode,
				struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info, struct list_head *rule_list_us, struct list_head *rule_list_ds)
{
	struct omciutil_vlan_rule_fields_t *rule_fields_untag;

	if (rule_fields == NULL)
	{
		return -1;
	}

	if (rule_fields->treatment_inner.pad == 2048) // TLS enable for Huawei OLT
	{
		//add a brand new rule for untag
		rule_fields_untag = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		memcpy(rule_fields_untag, rule_fields, sizeof(struct omciutil_vlan_rule_fields_t));
		//assign for untag filter
		rule_fields_untag->filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		rule_fields_untag->filter_outer.vid = 4096;
		rule_fields_untag->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
		rule_fields_untag->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		rule_fields_untag->filter_inner.vid = 4096;
		rule_fields_untag->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
		rule_fields_untag->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_DEFAULT;
		rule_fields_untag->treatment_tag_to_remove = 0;
		rule_fields_untag->treatment_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		rule_fields_untag->treatment_outer.vid = 4096;
		rule_fields_untag->treatment_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
		rule_fields_untag->treatment_inner.priority = rule_fields->treatment_inner.priority;
		rule_fields_untag->treatment_inner.vid = rule_fields->treatment_inner.vid;
		rule_fields_untag->treatment_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_100;
		omciutil_vlan_process_rule_fields(rule_fields_untag, ds_mode, dscp2pbit_info, rule_list_us, rule_list_ds, NULL);
		
		//rule for single tag
		rule_fields->filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		rule_fields->filter_outer.vid = 4096;
		rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
		rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
		rule_fields->filter_inner.vid = 4096;
		rule_fields->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
		rule_fields->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_DEFAULT;
		rule_fields->treatment_tag_to_remove = 0;
		rule_fields->treatment_outer.priority = rule_fields->treatment_inner.priority;
		rule_fields->treatment_outer.vid = rule_fields->treatment_inner.vid;
		rule_fields->treatment_outer.tpid_de = rule_fields->treatment_inner.tpid_de;
		rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields->treatment_inner.vid = 4096;
		rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
	}

	return 0;
}

static void
omciutil_vlan_correct_rule_treatment(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	int rule_tag_num;

	rule_tag_num = omciutil_vlan_get_rule_filter_tag_num(rule_fields);

	if (rule_fields->treatment_tag_to_remove<=2)
	{
		//remove
		if (rule_fields->treatment_tag_to_remove>rule_tag_num)
		{
			rule_fields->treatment_tag_to_remove=rule_tag_num;
		}

		//priority
		if (!omciutil_vlan_is_valid_treat_priority(rule_fields->treatment_outer.priority, rule_tag_num))
		{
			rule_fields->treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_0;
		}
		if (!omciutil_vlan_is_valid_treat_priority(rule_fields->treatment_inner.priority, rule_tag_num)) 
		{
			rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_0;
		}

		//treatment outer
		if (rule_fields->treatment_outer.priority!=15)
		{
			if (!omciutil_vlan_is_valid_treat_vid(rule_fields->treatment_outer.vid, rule_tag_num))
			{
				rule_fields->treatment_outer.vid = 0;
			}

			switch(rule_fields->treatment_outer.tpid_de) {
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_000: // tpid_de=itpid_de
				if (rule_tag_num < 1)
				{
					rule_fields->treatment_outer.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
				}
				break;
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_001: // tpid_de=otpid_de
				if (rule_tag_num < 2)
				{
					rule_fields->treatment_outer.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
				}
				break;
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010: // tpid=output_tpid,de=itag_de
				if (rule_tag_num < 1)
				{
					rule_fields->treatment_outer.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_110;
				}
				break;
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_011: // tpid=output_tpid,de=otag_de
				if (rule_tag_num < 2)
				{
					rule_fields->treatment_outer.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_110;
				}
				break;
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_101:
				rule_fields->treatment_outer.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
				break;
			default:
				; //do nothing
			}
		}
		//treatment inner
		if (rule_fields->treatment_inner.priority!=15) {
			if (!omciutil_vlan_is_valid_treat_vid(rule_fields->treatment_inner.vid, rule_tag_num))
			{
				rule_fields->treatment_inner.vid = 0;
			}

			switch(rule_fields->treatment_inner.tpid_de) {
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_000: // tpid_de=itpid_de
				if (rule_tag_num < 1)
				{
					rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
				}
				break;
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_001: // tpid_de=otpid_de
				if (rule_tag_num < 2)
				{
					rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
				}
				break;
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010: // tpid=output_tpid,de=itag_de
				if (rule_tag_num < 1)
				{
					rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_110;
				}
				break;
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_011: // tpid=output_tpid,de=otag_de
				if (rule_tag_num < 2)
				{
					rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_110;
				}
				break;
			case OMCIUTIL_VLAN_TREATMENT_TPID_DE_101:
				rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
				break;
			default:
				; //do nothing
			}
		}
	}

	return;
}

static int
omciutil_vlan_create_rules_from_65305_entry(struct me_t *bport_me, struct me_t *owner_me, 
														unsigned char *entry, struct list_head *rule_list_ds, unsigned char tag_mode)
{
	//create rules by entry
	int entry_size;
	unsigned int action;
	unsigned short vid;
	struct attrinfo_table_t *attrinfo_table_ptr;
	struct omciutil_vlan_rule_fields_t *rule_fields;
	
	attrinfo_table_ptr = meinfo_util_aitab_ptr(65305, 2);
	entry_size = attrinfo_table_ptr->entry_byte_size;

	//get action
	action = util_bitmap_get_value(entry, 8*entry_size, 14, 2);
	//get vid
	vid = util_bitmap_get_value(entry, 8*entry_size, 16, 16);

	switch(action)
	{
		case 0:
			//do nothing
			break;
		case 1:
			//remove outer tag;
			//one tag
			//allocate a rule_fields and fill it
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			rule_fields->unreal_flag = 0; //real

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=vid & 0x0FFF;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}

			//two tags
			//allocate a rule_fields and fill it
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			rule_fields->unreal_flag = 0; //real

			rule_fields->filter_outer.priority=8;
			rule_fields->filter_outer.vid=vid & 0x0FFF;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}

			break;
		case 2:
			//pass
			//one tag
			//allocate a rule_fields and fill it
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			rule_fields->unreal_flag = 0; //real

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=vid & 0x0FFF;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=0;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}

			//two tags
			if (tag_mode != 1) //impossible to add another tag
			{
				//allocate a rule_fields and fill it
				rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
				rule_fields->unreal_flag = 0; //real

				rule_fields->filter_outer.priority=8;
				rule_fields->filter_outer.vid=vid & 0x0FFF;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=4096;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=0;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=15;
				rule_fields->treatment_inner.vid=0;
				rule_fields->treatment_inner.tpid_de=0;

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}
			}

			break;
		case 3:
			//chang to priority tag
			//one tag
			//allocate a rule_fields and fill it
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			rule_fields->unreal_flag = 0; //real

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=vid & 0x0FFF;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=8; //copy from inner
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0; //copy from inner

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}

			//two tags
			if (tag_mode != 1) //impossible to add another tag
			{
				//allocate a rule_fields and fill it
				rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
				rule_fields->unreal_flag = 0; //real

				rule_fields->filter_outer.priority=8;
				rule_fields->filter_outer.vid=vid & 0x0FFF;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=4096;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=9; //copy from outer
				rule_fields->treatment_inner.vid=0;
				rule_fields->treatment_inner.tpid_de=1; //copy from outer

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}
			}
			break;
		default:
			; //do nothing
	}

	return 0;
}

static int
omciutil_vlan_collect_65305_rules(struct me_t *bport_me, struct me_t *owner_me, 
				struct list_head *rule_list_ds, unsigned char tag_mode)
{
	struct attr_value_t attr_value;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	struct omciutil_vlan_rule_fields_t *rule_fields;

	unsigned char default_tx_mode;

	if (bport_me == NULL || owner_me == NULL || rule_list_ds == NULL ||
		owner_me->classid != 65305)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//get default tx mode
	meinfo_me_attr_get(owner_me, 1, &attr_value);
	default_tx_mode = attr_value.data;

	//get tagging table
	if ((table_header = (struct attr_table_header_t *) owner_me->attr[2].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "attr value was not table\n");
		return (-1);
	}

	list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
		{
			//create ds rule fields
			omciutil_vlan_create_rules_from_65305_entry(bport_me, owner_me, (unsigned char *)table_entry_pos->entry_data_ptr, rule_list_ds, tag_mode);
		}
	}

	//create ds default rules
	switch (default_tx_mode)
	{
	case 0:
		//do nothing
		break;
	case 1:
		//remove outer tag;
		//one tag
		//allocate a rule_fields and fill it
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0; //real

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=14;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}

		//two tags
		//allocate a rule_fields and fill it
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0; //real

		rule_fields->filter_outer.priority=14;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=14;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}

		break;
	case 2:
		//pass
		//one tag
		//allocate a rule_fields and fill it
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0; //real

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=14;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=0;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}

		//two tags
		if (tag_mode != 1) //impossible to add another tag
		{
			//allocate a rule_fields and fill it
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			rule_fields->unreal_flag = 0; //real

			rule_fields->filter_outer.priority=14;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=14;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=0;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
		}

		break;
	case 3:
		//chang to priority tag
		//one tag
		//allocate a rule_fields and fill it
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0; //real

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=14;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=8; //copy from inner
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0; //copy from inner

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}

		//two tags
		if (tag_mode != 1) //impossible to add another tag
		{
			//allocate a rule_fields and fill it
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			rule_fields->unreal_flag = 0; //real

			rule_fields->filter_outer.priority=14;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=14;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=9; //copy from outer
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=1; //copy from outer

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
		}
		break;
	default:
		; //do nothing
	}

	return 0;
}

static int
omciutil_vlan_modify_rule_by_65305_action(struct omciutil_vlan_rule_fields_t *rule_fields, 
											enum omciutil_vlan_tag_code_t code,
											int action)
{
	switch(action)
	{
	case 1: //remove outer tag
		//add one to remove_tag
		switch (code)
		{
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
			rule_fields->treatment_tag_to_remove++;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER:
			rule_fields->treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
			rule_fields->treatment_outer.vid = 0;
			rule_fields->treatment_outer.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
			rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
			rule_fields->treatment_inner.vid = 0;
			rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
			break;
		default:
			dbprintf(LOG_ERR, "result code error\n");
			return -1;
		}
		break;
	case 2: //pass
		// do nothing
		break;
	case 3: //priority tag
		//add one to remove_tag
		switch (code)
		{
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
			rule_fields->treatment_tag_to_remove++; //remove it
			//add new one by treatment inner
			rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_9;
			rule_fields->treatment_inner.vid = 0; //priority tag
			rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_001;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
			rule_fields->treatment_tag_to_remove++; //remove it
			//add new one by treatment inner
			rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
			rule_fields->treatment_inner.vid = 0; //priority tag
			rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER:
			rule_fields->treatment_outer.vid = 0; //only change the vid to 0
			break;
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
			rule_fields->treatment_inner.vid = 0; //only change the vid to 0
			break;
		default:
			dbprintf(LOG_ERR, "result code error\n");
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "action error: %u\n", action);
	}

	return 0;
}

static int
omciutil_vlan_modify_rule_by_65297_mode(struct omciutil_vlan_rule_fields_t *rule_fields, 
											enum omciutil_vlan_tag_code_t code,
											int mode)
{
	switch(mode)
	{
	case 1: //remove outer tag
		//add one to remove_tag
		switch (code)
		{
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
			rule_fields->treatment_tag_to_remove++;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER:
			rule_fields->treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
			rule_fields->treatment_outer.vid = 0;
			rule_fields->treatment_outer.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
			rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
			rule_fields->treatment_inner.vid = 0;
			rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
			break;
		default:
			dbprintf(LOG_ERR, "result code error\n");
			return -1;
		}
		break;
	case 2: //pass
		// do nothing
		break;
	default:
		dbprintf(LOG_ERR, "action error: %u\n", mode);
	}

	return 0;
}

static int
omciutil_vlan_join_171_ds_rules_with_65305(struct me_t *bport_me,
				struct me_t *me_65305,
				struct list_head *rule_list_ds)
{
	struct attr_value_t attr_value;
	struct omciutil_vlan_rule_fields_t *rule_fields_pos, *rule_fields_n, *rule_fields_new;
	struct omciutil_vlan_rule_result_list_t rule_result_list;
	struct omciutil_vlan_rule_result_node_t *result_node;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	struct attrinfo_table_t *attrinfo_table_ptr;
	struct list_head new_rule_list_ds;
	int default_tx_mode;
	unsigned short vid = 0;
	int entry_size;
	unsigned int action;
	unsigned short entry_vid;
	unsigned int modified;

	if (bport_me == NULL || me_65305 == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//get default tx mode
	meinfo_me_attr_get(me_65305, 1, &attr_value);
	default_tx_mode = attr_value.data;

	//get tagging table
	if ((table_header = (struct attr_table_header_t *) me_65305->attr[2].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "attr value was not table\n");
		return (-1);
	}

	attrinfo_table_ptr = meinfo_util_aitab_ptr(65305, 2);
	entry_size = attrinfo_table_ptr->entry_byte_size;

	//alloc a new ds rule list
	INIT_LIST_HEAD(&new_rule_list_ds);

	list_for_each_entry(rule_fields_pos, rule_list_ds, rule_node)
	{
		//generate result
		if (omciutil_vlan_generate_rule_result(rule_fields_pos, &rule_result_list) < 0)
		{
			dbprintf(LOG_ERR, "could not get rule result\n");
			continue;
		}

		//join with 65305, check rule_result_list.tag_count, 4096 was wildcard for vid
		switch (rule_result_list.tag_count)
		{
		case 0: //untag
			//do nothing
			break;
		case 1: //one tag
		case 2: //two tags
			//get result outer tag
			result_node = list_entry(rule_result_list.tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
			//check result_node to get vid
			switch (result_node->code)
			{
			case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
				vid = rule_fields_pos->filter_outer.vid;
				break;
			case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
				vid = rule_fields_pos->filter_inner.vid;
				break;
			case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER:
				if (rule_fields_pos->treatment_outer.vid <= 4095)
				{
					vid = rule_fields_pos->treatment_outer.vid;
				} else if (rule_fields_pos->treatment_outer.vid == 4096) {
					vid = (rule_fields_pos->filter_inner.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields_pos->filter_inner.vid : 0);
				} else if (rule_fields_pos->treatment_outer.vid == 4097) {
					vid = (rule_fields_pos->filter_outer.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields_pos->filter_outer.vid : 0);
				}
				break;
			case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
				if (rule_fields_pos->treatment_inner.vid <= 4095)
				{
					vid = rule_fields_pos->treatment_inner.vid;
				} else if (rule_fields_pos->treatment_inner.vid == 4096) {
					vid = (rule_fields_pos->filter_inner.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields_pos->filter_inner.vid : 0);
				} else if (rule_fields_pos->treatment_inner.vid == 4097) {
					vid = (rule_fields_pos->filter_outer.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields_pos->filter_outer.vid : 0);
				}
				break;
			default:
				dbprintf(LOG_ERR, "result code error\n");
				omciutil_vlan_release_rule_result(&rule_result_list);
				return -1;
			}

			//according outer vid, join with 65305
			if (vid == 4096) // wildcard, don't care
			{
				//traverse 65305 table entries, add more rules
				list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
				{
					if (table_entry_pos->entry_data_ptr != NULL)
					{
						//get action
						action = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*entry_size, 14, 2);
						//get vid
						entry_vid = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*entry_size, 16, 16);

						rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
						memcpy(rule_fields_new, rule_fields_pos, sizeof(struct omciutil_vlan_rule_fields_t)); // inherit all fields from rule_fields

						//according result code, modify it
						switch (result_node->code)
						{
						case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
							rule_fields_new->filter_outer.vid = entry_vid;
							break;
						case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
							rule_fields_new->filter_inner.vid = entry_vid;
							break;
						case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER:
							if (rule_fields_pos->treatment_outer.vid == 4096) {
								rule_fields_new->filter_inner.vid = entry_vid;
							} else if (rule_fields_pos->treatment_outer.vid == 4097) {
								rule_fields_new->filter_outer.vid = entry_vid;
							}
							break;
						case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
							if (rule_fields_pos->treatment_inner.vid == 4096) {
								rule_fields_new->filter_inner.vid = entry_vid;
							} else if (rule_fields_pos->treatment_inner.vid == 4097) {
								rule_fields_new->filter_outer.vid = entry_vid;
							}
							break;
						default:
							dbprintf(LOG_ERR, "result code error\n");
							omciutil_vlan_release_rule_result(&rule_result_list);
							return -1;
						}

						//modify this new one by entry action
						omciutil_vlan_modify_rule_by_65305_action(rule_fields_new, result_node->code, action);
					}

					//add this one to new list
					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, &new_rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_new);

						//release entry_fields_reverse
						free_safe(rule_fields_new);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_new);
					}
				}
 
				//modify this rule by default
				omciutil_vlan_modify_rule_by_65305_action(rule_fields_pos, result_node->code, default_tx_mode);
			} else { //regular vid, compare with 65305 table
				
				modified = 0;
				//traverse all table entry to find matching
				list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
				{
					if (table_entry_pos->entry_data_ptr != NULL)
					{
						//get action
						action = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*entry_size, 14, 2);
						//get vid
						entry_vid = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*entry_size, 16, 16);
						//match
						if (entry_vid == vid)
						{
							//by action, modify rule
							omciutil_vlan_modify_rule_by_65305_action(rule_fields_pos, result_node->code, action);
							modified = 1;
							break;
						}
					}
				}

				//default, use default tx mode
				if (!modified)
				{
					omciutil_vlan_modify_rule_by_65305_action(rule_fields_pos, result_node->code, default_tx_mode);
				}
			}

			break;
		default: // > two tags, impossible
			dbprintf(LOG_ERR, "unreasonable tag count: %u\n", rule_result_list.tag_count);
		}

		omciutil_vlan_release_rule_result(&rule_result_list);
	}

	//merge new_rule_list_ds to rule_list_ds
	list_for_each_entry_safe(rule_fields_pos, rule_fields_n, &new_rule_list_ds, rule_node)
	{
		list_del(&rule_fields_pos->rule_node);

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_pos, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields_pos);

			//release entry_fields_reverse
			free_safe(rule_fields_pos);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields_pos);
		}
	}

	return 0;
}

static int
omciutil_vlan_join_171_ds_rules_with_65297(struct me_t *bport_me,
				struct me_t *me_65297,
				struct list_head *rule_list_ds)
{
	struct attr_value_t attr_value;
	struct omciutil_vlan_rule_fields_t *rule_fields_pos;
	struct omciutil_vlan_rule_result_list_t rule_result_list;
	struct omciutil_vlan_rule_result_node_t *result_node;
	int ds_tag_handling_mode;

	if (bport_me == NULL || me_65297 == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//get default tx mode
	meinfo_me_attr_get(me_65297, 1, &attr_value);
	ds_tag_handling_mode = attr_value.data;

	list_for_each_entry(rule_fields_pos, rule_list_ds, rule_node)
	{
		//generate result
		if (omciutil_vlan_generate_rule_result(rule_fields_pos, &rule_result_list) < 0)
		{
			dbprintf(LOG_ERR, "could not get rule result\n");
			continue;
		}

		//join with 65297, check rule_result_list.tag_count
		switch (rule_result_list.tag_count)
		{
		case 0: //untag
			//do nothing
			break;
		case 1: //one tag
		case 2: //two tags
			result_node = list_entry(rule_result_list.tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);

			omciutil_vlan_modify_rule_by_65297_mode(rule_fields_pos, result_node->code, ds_tag_handling_mode);

			break;
		default: // > two tags, impossible
			dbprintf(LOG_ERR, "unreasonable tag count: %u\n", rule_result_list.tag_count);
		}
	}

	return 0;
}

static int
omciutil_vlan_collect_171_rules(struct me_t *bport_me, struct me_t *owner_me, 
				struct list_head *rule_list_us, struct list_head *rule_list_ds, unsigned char tag_mode,
				struct omciutil_vlan_tlan_table_t *tlan_table_bridge)
{
	struct attr_value_t attr_value;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	struct omciutil_vlan_rule_fields_t *rule_fields, *rule_fields_pos, *rule_fields_n;
	struct list_head temp_rule_list_ds;
	struct switch_port_info_t port_info;
	struct meinfo_t *miptr_65305, *miptr_65297, *miptr_47;
	struct me_t *me_pos_65305, *me_pos_65297, *me_pos_47;
	struct me_t *me;
	struct omciutil_vlan_dscp2pbit_info_t dscp2pbit_info;
	unsigned short itpid;
	unsigned short otpid;
	unsigned char ds_mode;
	int found;

	if (bport_me == NULL || owner_me == NULL || rule_list_us == NULL || rule_list_ds == NULL ||
		owner_me->classid != 171)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	if (switch_get_port_info_by_me(bport_me, &port_info) != 0)
	{
		dbprintf(LOG_ERR, "get port info error, classid=%u, meid=%u\n", bport_me->classid, bport_me->meid);
		return -1;
	}

	//get input tpid
	meinfo_me_attr_get(owner_me, 3, &attr_value);
	itpid = attr_value.data;

	//get output tpid
	meinfo_me_attr_get(owner_me, 4, &attr_value);
	otpid = attr_value.data;

	//get downsteam mode
	meinfo_me_attr_get(owner_me, 5, &attr_value);
	ds_mode = attr_value.data;

	//FIX, workaround for 988 values, temporary apply inversing action
	if (ds_mode > 1)
	{
		ds_mode = 0;
	}

	//get tagging table
	if ((table_header = (struct attr_table_header_t *) owner_me->attr[6].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "attr value was not table\n");
		return (-1);
	}

	//collect dscp2pbit info
	omciutil_vlan_collect_dscp2pbit_info_by_port(bport_me, &dscp2pbit_info);

	er_attr_group_dbprintf(LOG_ERR, 0, "VLAN tagging collect 171 rules, meid=%u:\n", owner_me->meid);

	//alloc a temporay ds rule list to store all rules generated from 171
	INIT_LIST_HEAD(&temp_rule_list_ds);
	
	list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
		{
			//allocate an rule_fields and fill it
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			rule_fields->unreal_flag = 0; //real

			//fill to entry fields
			omciutil_vlan_collect_rule_fields_from_table_entry((unsigned char *)table_entry_pos->entry_data_ptr, itpid, otpid, rule_fields);

			//correct the rule
			if (omci_env_g.tag_treatment_correct == 1)
			{
				omciutil_vlan_correct_rule_treatment(rule_fields);
			}

			if (tag_mode == 1 && omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 2)
			{
				//ignore this rule
				free_safe(rule_fields);
				continue;
			}

			//alu proprietary
			if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
			   (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU))
			{
				//convert for alu vid <-> ethertype workaround
				struct me_t *meptr = meinfo_me_get_by_meid(65302, 0);
				if(meptr && meinfo_util_me_attr_data(meptr, 1) == 1) // Activate this workaround when ME65302 RF_Filter==1
					omciutil_vlan_collect_alu_vid_ethertype_workaround(rule_fields, ds_mode, &dscp2pbit_info, rule_list_us, rule_list_ds);
#if 0 // ALU decides to use VLAN=4085 for TLS with ALU 7342/7360 OLT
				if (proprietary_alu_get_olt_version() == 1) //for 7302/7360 olt
				{
					//convert for alu tls workaround
					omciutil_vlan_collect_alu_tls_workaround(rule_fields, ds_mode, &dscp2pbit_info, rule_list_us, &temp_rule_list_ds);
				}
#endif
			}
			//convert for huawei tls workaround
			if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI) == 0) &&
			    (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_HUAWEI)) // huawei
			{
				omciutil_vlan_collect_huawei_tls_workaround(rule_fields, ds_mode, &dscp2pbit_info, rule_list_us, &temp_rule_list_ds);
			}

			//calix, overwrite pbit treatment to dscp-2-pbit in 1-filter, 1-removement, 1-treatment rules. 
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0 &&
				port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU &&
				omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 1 &&
				rule_fields->treatment_tag_to_remove == 1 &&
				omciutil_vlan_get_rule_treatment_tag_num(rule_fields) == 1) // calix
			{
				int i;
				struct batchtab_cb_wand2p_t *t = batchtab_table_data_get("wand2p");
				if (t != NULL)
				{
					fwk_mutex_lock(&t->mutex);
					
					for (i = 0; i < WANIF_INDEX_TOTAL; i++)
					{
						if (t->wand2p[i].enable == 1 &&
							t->wand2p[i].vid == rule_fields->filter_inner.vid &&
							t->wand2p[i].dscp2pbit_enable == 1)
						{
							//overwrite to dscp2pbit if it is not dscp2pbit
							if (rule_fields->treatment_inner.priority != OMCIUTIL_VLAN_TREATMENT_PRIORITY_10)
							{
								rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_10;
							}
						}
					}
					
					fwk_mutex_unlock(&t->mutex);
					batchtab_table_data_put("wand2p");
				}
			}

			omciutil_vlan_process_rule_fields(rule_fields,
						ds_mode,
						&dscp2pbit_info,
						rule_list_us,
						&temp_rule_list_ds,
						tlan_table_bridge);
		}
	}

	//alu proprietary, join with 65305 or 65297
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0 &&
		(port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI ||
		 port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU ||
		 port_info.port_type == ENV_BRIDGE_PORT_TYPE_IPHOST)) //UNI
	{
		if (proprietary_alu_get_olt_version() == 0) //old
		{
			//find the related 65297 me
			if ((miptr_65297 = meinfo_util_miptr(65297)) == NULL) // 65297 UNI_Supplemental_1_V2
			{
				dbprintf(LOG_ERR, "get 65297 miptr error\n");
				return -1;
			}

			found = 0;
			list_for_each_entry(me_pos_65297, &miptr_65297->me_instance_list, instance_node)
			{
				//pptp ethernet uni, 11, 329, 134				
				me = meinfo_me_get_by_meid(11, me_pos_65297->meid);		// pptp
				if (me == NULL)
					me = meinfo_me_get_by_meid(329, me_pos_65297->meid);	// veip
				if (me == NULL)
					me = meinfo_me_get_by_meid(134, me_pos_65297->meid);	// iphost
				if (me == NULL)
				{
					dbprintf(LOG_ERR, "get me error, classid=11/329/134, meid=%u, for class 65297 related\n", me_pos_65297->meid);
				} else {
					//bridge port configuration data, 47
					if ((miptr_47 = meinfo_util_miptr(47)) == NULL)
					{
						dbprintf(LOG_ERR, "get 47 miptr error\n");
						continue;
					} else {
						list_for_each_entry(me_pos_47, &miptr_47->me_instance_list, instance_node)
						{
							if (me_related(me_pos_47, me) == 1 && me_pos_47 == bport_me) //47->11/329/134
							{
								//found
								found = 1;
								break;
							}
						}
					}
					if (found)
						break;
				}
			}

			if (found)
			{
				//join temp_rule_list_ds with alu proprietory me 65297
				omciutil_vlan_join_171_ds_rules_with_65297(bport_me, me_pos_65297, &temp_rule_list_ds);
			} else {
				dbprintf(LOG_ERR, "get me error, classid=47, meid=%u, for class 65297 related\n", bport_me->meid);
			}
		} else { //new
			//find the related 65305 me
			if ((miptr_65305 = meinfo_util_miptr(65305)) == NULL) // 65305 Downstream_VLAN_Tag_Egress_Behavior_Supplemental_1
			{
				dbprintf(LOG_ERR, "get 65305 miptr error\n");
				return -1;
			}

			found = 0;
			list_for_each_entry(me_pos_65305, &miptr_65305->me_instance_list, instance_node)
			{
				//pptp ethernet uni, 11, 329, 134				
				me = meinfo_me_get_by_meid(11, me_pos_65305->meid);		// pptp
				if (me == NULL)
					me = meinfo_me_get_by_meid(329, me_pos_65305->meid);	// veip
				if (me == NULL)
					me = meinfo_me_get_by_meid(134, me_pos_65305->meid);	// iphost
				if (me == NULL)
				{
					dbprintf(LOG_ERR, "get me error, classid=11/329/134, meid=%u, for class 65305 related\n", me_pos_65305->meid);
				} else {
					//bridge port configuration data, 47
					if ((miptr_47 = meinfo_util_miptr(47)) == NULL)
					{
						dbprintf(LOG_ERR, "get 47 miptr error\n");
						continue;
					} else {
						list_for_each_entry(me_pos_47, &miptr_47->me_instance_list, instance_node)
						{
							if (me_related(me_pos_47, me) == 1 && me_pos_47 == bport_me) //47->11/329/134
							{
								//found
								found = 1;
								break;
							}
						}
					}
					if (found)
						break;
				}
			}

			if (found)
			{
				//join temp_rule_list_ds with alu proprietory me 65305
				omciutil_vlan_join_171_ds_rules_with_65305(bport_me, me_pos_65305, &temp_rule_list_ds);
				if (omci_env_g.tag_insert_65305_rules == 1) //insert 65305 rules
				{
					//insert 65305 ds rules to tem_rule_list_ds
					omciutil_vlan_collect_65305_rules(bport_me, me_pos_65305, &temp_rule_list_ds, tag_mode);
				}
			} else {
				dbprintf(LOG_ERR, "get me error, classid=47, meid=%u, for class 65305 related\n", bport_me->meid);
			}
		}
	}

	//merge temp_rule_list_ds to rule_list_ds
	list_for_each_entry_safe(rule_fields_pos, rule_fields_n, &temp_rule_list_ds, rule_node)
	{
		list_del(&rule_fields_pos->rule_node);

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_pos, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields_pos);

			//release entry_fields_reverse
			free_safe(rule_fields_pos);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields_pos);
		}
	}

	return 0;
}

void
omciutil_vlan_collect_249_rules_fill_default_fields(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	if (rule_fields == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return;
	}
	
	//filter parts for untag
	rule_fields->filter_outer.priority=15;
	rule_fields->filter_outer.vid=4096;
	rule_fields->filter_outer.tpid_de=0;
	rule_fields->filter_inner.priority=15;
	rule_fields->filter_inner.vid=4096;
	rule_fields->filter_inner.tpid_de=0;
	
	rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
	rule_fields->treatment_tag_to_remove=0;
	//treatment parts
	rule_fields->treatment_outer.priority=15;
	rule_fields->treatment_outer.vid=0;
	rule_fields->treatment_outer.tpid_de=0;

	return;
}

static int
omciutil_vlan_collect_249_rules(struct me_t *bport_me,
	struct me_t *owner_me, 
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds,
	struct omciutil_vlan_tlan_table_t *tlan_table_bridge)
{
	struct attr_value_t attr_value;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	struct omciutil_vlan_rule_fields_t *rule_fields, *rule_fields_pos, *rule_fields_n;
	struct list_head temp_rule_list_ds;
	struct omciutil_vlan_dscp2pbit_info_t dscp2pbit_info;
	unsigned short itpid;
	unsigned short otpid;

	if (bport_me == NULL || owner_me == NULL || rule_list_us == NULL || rule_list_ds == NULL ||
		owner_me->classid != 249)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//get input tpid
	meinfo_me_attr_get(owner_me, 2, &attr_value);
	itpid = attr_value.data;

	//get output tpid
	meinfo_me_attr_get(owner_me, 3, &attr_value);
	otpid = attr_value.data;

	//get l2 tagging table
	if ((table_header = (struct attr_table_header_t *) owner_me->attr[5].value.ptr) == NULL) {
		dbprintf(LOG_ERR, "attr value was not table\n");
		return (-1);
	}

	//collect dscp2pbit info
	omciutil_vlan_collect_dscp2pbit_info_by_port(bport_me, &dscp2pbit_info);

	er_attr_group_dbprintf(LOG_ERR, 0, "VLAN tagging collect 249 rules, meid=%u:\n", owner_me->meid);

	//alloc a temporay ds rule list to store all rules generated from 171
	INIT_LIST_HEAD(&temp_rule_list_ds);
	
	list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
		{
			//allocate an rule_fields and fill it with default value
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_HIGHER_2;
			rule_fields->unreal_flag = 0; //real

			//fill default fileds
			omciutil_vlan_collect_249_rules_fill_default_fields(rule_fields);

			//extrac entry fields
			rule_fields->l2_filter_enable = 1;
			omciutil_vlan_collect_rule_fields_from_l2_table_entry((unsigned char *)table_entry_pos->entry_data_ptr, itpid, otpid, rule_fields);

			//correct the rule
			if (omci_env_g.tag_treatment_correct == 1)
			{
				omciutil_vlan_correct_rule_treatment(rule_fields);
			}

			//collect reversing rules
			omciutil_vlan_process_l2_rule_fields(rule_fields,
				omci_env_g.tag_l2_rules_reverse,
				&dscp2pbit_info,
				rule_list_us,
				&temp_rule_list_ds,
				tlan_table_bridge);
		}
	}

	//merge temp_rule_list_ds to rule_list_ds
	list_for_each_entry_safe(rule_fields_pos, rule_fields_n, &temp_rule_list_ds, rule_node)
	{
		list_del(&rule_fields_pos->rule_node);

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_pos, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields_pos);

			//release entry_fields_reverse
			free_safe(rule_fields_pos);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields_pos);
		}
	}

	return 0;
}

static int
omciutil_vlan_collect_78_rules(struct me_t *bport_me, struct me_t *owner_me, 
				struct list_head *rule_list_us, struct list_head *rule_list_ds, unsigned char tag_mode)
{
	unsigned char *tci_map, upstream_op_mode,downstream_op_mode;
	struct omciutil_vlan_rule_fields_t *rule_fields;
	unsigned char de;

	if (bport_me == NULL || owner_me == NULL || rule_list_us == NULL || rule_list_ds == NULL ||
		owner_me->classid != 78)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	upstream_op_mode=owner_me->attr[1].value.data;
	downstream_op_mode=owner_me->attr[3].value.data;

	tci_map=owner_me->attr[2].value.ptr;
	de=util_bitmap_get_value(tci_map, 16, 3, 1);

	er_attr_group_dbprintf(LOG_ERR, 0, "VLAN tagging collect 78 rules, meid=%u:\n", owner_me->meid);

	switch(upstream_op_mode) {	//Upstream_VLAN_tagging_operation_mode
	case 0:
		//do nothing
		break;
	case 1:
	//orig 0 tag,add 1 tag(use gps l2s tags create-add1)
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0;

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=15;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=0;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=util_bitmap_get_value(tci_map, 16, 0, 3);
		rule_fields->treatment_inner.vid=util_bitmap_get_value(tci_map, 16, 4, 12);
		if (de) {
			rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011 ;
		} else {
			rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010 ;
		}
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "US-insert to us list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "US: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}
		 // for ericsson case, only untag rule is needed
		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ERICSSON)
			break;
	//orig 1 tagged, modify 1 tag (use gps l2s tags create-add1)
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0;

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=util_bitmap_get_value(tci_map, 16, 0, 3);
		rule_fields->treatment_inner.vid=util_bitmap_get_value(tci_map, 16, 4, 12);
		if (de) {
			rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011 ;
		} else {
			rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010 ;
		}
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "US-insert to us list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "US: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}

	//orig 2 tagged, modify 1 tag (use gps l2s tags create-add1)
		if (tag_mode != 1)
		{
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			rule_fields->unreal_flag = 0;

			rule_fields->filter_outer.priority=8;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=util_bitmap_get_value(tci_map, 16, 0, 3);
			rule_fields->treatment_inner.vid=util_bitmap_get_value(tci_map, 16, 4, 12);
			if (de) {
				rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011 ;
			} else {
				rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010 ;
			}
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "US-insert to us list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "US: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
		}
		break;
	case 2:
	//orig 0 tag,add 1 tag(use gps l2s tags create-add1)
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0;

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=15;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=0;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=util_bitmap_get_value(tci_map, 16, 0, 3);
		rule_fields->treatment_inner.vid=util_bitmap_get_value(tci_map, 16, 4, 12);
		if (de) {
			rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011 ;
		} else {
			rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010 ;
		}
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "US-insert to us list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "US: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}

	//orig 1 tagged, add 1 tag (use gps l2s tags create-add1)
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0;

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=0;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=util_bitmap_get_value(tci_map, 16, 0, 3);
		rule_fields->treatment_inner.vid=util_bitmap_get_value(tci_map, 16, 4, 12);
		if (de) {
			rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011 ;
		} else {
			rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010 ;
		}
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "US-insert to us list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "US: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}
	//orig 2 tagged, can't add another tag
		break;
	default:
		break;
	}

	switch(downstream_op_mode) {	//Downstream_VLAN_tagging_operation_mode
	case 0:
		//do nothing
		break;
	case 1:
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0;

	//1 tagged, strip outter tag (use gps l2s tags create-add0)
		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ERICSSON) {
			// for ericsson case, only strip tag with us match vlan
			rule_fields->filter_inner.priority=util_bitmap_get_value(tci_map, 16, 0, 3);
			rule_fields->filter_inner.vid=util_bitmap_get_value(tci_map, 16, 4, 12);
		} else {
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
		}
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}

	//2 tagged, strip outter tag (use gps l2s tags create-add0)
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields->unreal_flag = 0;

		rule_fields->filter_outer.tpid_de=0;
		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ERICSSON) {
			// for ericsson case, only strip tag with us match vlan
			rule_fields->filter_outer.priority=util_bitmap_get_value(tci_map, 16, 0, 3);
			rule_fields->filter_outer.vid=util_bitmap_get_value(tci_map, 16, 4, 12);
		} else {
			rule_fields->filter_outer.priority=8;
			rule_fields->filter_outer.vid=4096;
		}
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}
		break;
	}
	return 0;
}

//zte proprietary
static int
omciutil_vlan_collect_65282_rules(struct me_t *bport_me, struct me_t *owner_me, 
				struct list_head *rule_list_us, struct list_head *rule_list_ds, unsigned char tag_mode)
{
	int i;
	unsigned char tag_control_mode,tag_list_num;
	unsigned char de;
	struct omciutil_vlan_rule_fields_t *rule_fields;

	if (bport_me == NULL || owner_me == NULL || rule_list_us == NULL || rule_list_ds == NULL ||
		owner_me->classid != 65282)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	tag_control_mode = owner_me->attr[1].value.data;
	tag_list_num = owner_me->attr[2].value.data;

	//add rule to downstream;
	switch(tag_control_mode)
	{
	case 0:
		//discard no tag
		if (omci_env_g.mc_ds_untag_discard_enable)
		{
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=15;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=3; //discard

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;
			
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
		}
		break;
	case 1:
		//create 2 rules for discarding no tag, strip 1 tag and 2 tags
		//one tag
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}

		//two tags
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

		rule_fields->filter_outer.priority=8;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=2;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}
		
		//discard no tag
		if (omci_env_g.mc_ds_untag_discard_enable)
		{
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=15;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=3; //discard

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;
			
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
		}
		break;
	case 2: 
		//according tag number to create rules for one tag and two tags, and discard no tag
		//one tag
		for (i = 0; i < tag_list_num && i < 6 && owner_me->attr[3].value.ptr; i++)
		{
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32, 3);
			rule_fields->filter_inner.vid=util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+4, 12);
			if((de = util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+3, 1)) == 0)
			{
				rule_fields->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1011;
			} else {
				rule_fields->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1100;
			}

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1; //remove 1 tag

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+16, 3);
			rule_fields->treatment_inner.vid=util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+20, 12);
			if((de = util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+19, 1)) == 0)
			{
				rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_10000;
			} else {
				rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_10001;
			}
			
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
		}
		//two tags
		if (tag_mode != 1) //impossible to add another tag
		{
			for (i = 0; i < tag_list_num && i < 6 && owner_me->attr[3].value.ptr; i++)
			{
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

				rule_fields->filter_outer.priority=util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32, 3);
				rule_fields->filter_outer.vid=util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+4, 12);
				if((de = util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+3, 1)) == 0)
				{
					rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1011;
				} else {
					rule_fields->filter_outer.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1100;
				}
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=4096;
				rule_fields->filter_inner.tpid_de = 0;
				
				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1; //remove 1 tag

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+16, 3);
				rule_fields->treatment_inner.vid=util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+20, 12);
				if((de = util_bitmap_get_value(owner_me->attr[3].value.ptr, 24*8, i*32+19, 1)) == 0)
				{
					rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_10010;
				} else {
					rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_10011;
				}
				
				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}
			}
		}
		//discard no tag
		if (omci_env_g.mc_ds_untag_discard_enable)
		{
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=15;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=3; //discard

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;
			
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
		}
		break;
	default:
		;
	}

	return 0;
}

//huawei proprietary
static int
omciutil_vlan_collect_367_rules(struct me_t *bport_me, struct me_t *owner_me, 
				struct list_head *rule_list_us, struct list_head *rule_list_ds, unsigned char tag_mode)
{
	unsigned char tag_control_mode;
	struct omciutil_vlan_rule_fields_t *rule_fields;
	struct switch_port_info_t port_info;

	if (bport_me == NULL || owner_me == NULL || rule_list_us == NULL || rule_list_ds == NULL ||
		owner_me->classid != 367)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}
	
	//get port info
	if (switch_get_port_info_by_me(bport_me, &port_info) != 0 )
	{
		dbprintf(LOG_ERR, "can not get port info, meid=0x%.4x\n", bport_me->meid);
		return -1;
	}

	// skip cpu port collection
	if(port_info.port_type == ENV_BRIDGE_PORT_TYPE_IPHOST || port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU)
	{
		return 0;
	}
	
	tag_control_mode = owner_me->attr[1].value.data;

	//add rule to downstream;
	switch(tag_control_mode)
	{
	case 1:	// untag
		//create 2 rules for strip 1 tag and 2 tags
		//one tag
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}

		//two tags
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

		rule_fields->filter_outer.priority=8;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=2;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}
		break;
	case 2: 	// translation
		//create 2 rules for translate 1 tag and 2 tags
		//one tag
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1; //remove 1 tag

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=0;
		rule_fields->treatment_inner.vid=(unsigned short) owner_me->attr[2].value.data;
		rule_fields->treatment_inner.tpid_de=0;
		
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}
		//two tags
		if (tag_mode != 1) //impossible to add another tag
		{
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

			rule_fields->filter_outer.priority=8;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;
			
			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1; //remove 1 tag

			rule_fields->treatment_outer.priority=0;
			rule_fields->treatment_outer.vid=(unsigned short) owner_me->attr[2].value.data;
			rule_fields->treatment_outer.tpid_de=1;
			rule_fields->treatment_inner.priority=8;
			rule_fields->treatment_inner.vid=4096;
			rule_fields->treatment_inner.tpid_de=0;
			
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
		}
		break;
	case 3:	// transparent
	default:
		break;
	}

	//discard no tag
	if (omci_env_g.mc_ds_untag_discard_enable)
	{
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)

		rule_fields->filter_outer.priority=15;
		rule_fields->filter_outer.vid=4096;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=15;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=3; //discard

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;
		
		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}
	}
	
	return 0;
}

static int
omciutil_vlan_collect_create_310_ds_rules_from_up_tci(unsigned short us_tag_tci,
	struct me_t *bport_me, struct me_t *owner_me, struct list_head *rule_list_ds)
{
	struct omciutil_vlan_rule_fields_t *rule_fields;

	if (bport_me == NULL || owner_me == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return 0;
	}

	//strip one tag
	rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
	rule_fields->owner_me=owner_me;
	rule_fields->bport_me=bport_me;
	rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
	rule_fields->unreal_flag = 0;

	rule_fields->filter_outer.priority=15;
	rule_fields->filter_outer.vid=4096;
	rule_fields->filter_outer.tpid_de=0;
	rule_fields->filter_inner.priority=(us_tag_tci & 0xE000) >> 13;
	rule_fields->filter_inner.vid=(us_tag_tci & 0x0FFF);
	if ((us_tag_tci & 0x1000) == 0)
	{
		rule_fields->filter_inner.tpid_de=OMCIUTIL_VLAN_FILTER_TPID_DE_1011;
	} else {
		rule_fields->filter_inner.tpid_de=OMCIUTIL_VLAN_FILTER_TPID_DE_1100;
	}


	rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
	rule_fields->treatment_tag_to_remove=1;

	rule_fields->treatment_outer.priority=15;
	rule_fields->treatment_outer.vid=0;
	rule_fields->treatment_outer.tpid_de=0;
	rule_fields->treatment_inner.priority=15;
	rule_fields->treatment_inner.vid=0;
	rule_fields->treatment_inner.tpid_de=0;

	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
	{
		er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
		er_attr_group_dbprintf_rule_fields(rule_fields);

		//release entry_fields
		free_safe(rule_fields);
	} else {
		er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
		er_attr_group_dbprintf_rule_fields(rule_fields);
	}

	if (omci_env_g.tag_mc_ds_from_us == 2)
	{
		//strip two tags
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=owner_me;
		rule_fields->bport_me=bport_me;
		rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
		rule_fields->unreal_flag = 0;

		rule_fields->filter_outer.priority=(us_tag_tci & 0xE000) >> 13;
		rule_fields->filter_outer.vid=(us_tag_tci & 0x0FFF);
		if ((us_tag_tci & 0x1000) == 0)
		{
			rule_fields->filter_outer.tpid_de=OMCIUTIL_VLAN_FILTER_TPID_DE_1011;
		} else {
			rule_fields->filter_outer.tpid_de=OMCIUTIL_VLAN_FILTER_TPID_DE_1100;
		}

		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=1;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
		{
			er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);

			//release entry_fields
			free_safe(rule_fields);
		} else {
			er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
			er_attr_group_dbprintf_rule_fields(rule_fields);
		}
	}

	return 0;
}

static int
omciutil_vlan_collect_create_310_ds_rules(unsigned char tag_control_mode, unsigned short ds_tag_tci, unsigned short vid_uni, unsigned short vid_ani,
	struct me_t *bport_me, struct me_t *owner_me, struct list_head *rule_list_ds, unsigned char tag_mode)
{
	struct omciutil_vlan_rule_fields_t *rule_fields;

	if (bport_me == NULL || owner_me == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return 0;
	}

	switch (tag_control_mode)
	{
		case 0:
			//do nothing;
			break;
		case 1:
			//create 2 rules for stripping outer tag of 1 and 2 tags by filtering vid_ani
			if ((vid_ani & 0x0FFF) != 0)
			{
				//one tag
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=15;
				rule_fields->filter_outer.vid=4096;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=vid_ani & 0x0FFF;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=15;
				rule_fields->treatment_inner.vid=0;
				rule_fields->treatment_inner.tpid_de=0;

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}

				//two tags
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=8;
				rule_fields->filter_outer.vid=vid_ani & 0x0FFF;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=4096;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=15;
				rule_fields->treatment_inner.vid=0;
				rule_fields->treatment_inner.tpid_de=0;

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}
			}
			break;
		case 2:
			if ((vid_ani & 0x0FFF) == 0)
			{
				//create untag add one tag
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=15;
				rule_fields->filter_outer.vid=4096;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=15;
				rule_fields->filter_inner.vid=4096;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=0;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
				rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
				if ((ds_tag_tci & 0x1000) == 0)
				{
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
				} else {
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011;
				}
				
				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}
			} else {
				//create one tag add one tag
				if (tag_mode != 1) //impossible to add another tag
				{
					rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
					rule_fields->owner_me=owner_me;
					rule_fields->bport_me=bport_me;
					rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
					rule_fields->unreal_flag = 0;

					rule_fields->filter_outer.priority=15;
					rule_fields->filter_outer.vid=4096;
					rule_fields->filter_outer.tpid_de=0;
					rule_fields->filter_inner.priority=8;
					rule_fields->filter_inner.vid=vid_ani & 0x0FFF;
					rule_fields->filter_inner.tpid_de=0;

					rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
					rule_fields->treatment_tag_to_remove=0;

					rule_fields->treatment_outer.priority=15;
					rule_fields->treatment_outer.vid=0;
					rule_fields->treatment_outer.tpid_de=0;
					rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
					rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
					if ((ds_tag_tci & 0x1000) == 0)
					{
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110;
					} else {
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111;
					}
					
					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);

						//release entry_fields
						free_safe(rule_fields);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);
					}
				}
			}
			break;
		case 3:
			//create 2 rules for replace outer tag of 1 and 2 tags by filtering vid_ani
			if ((vid_ani & 0x0FFF) != 0)
			{
				//one tag
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=15;
				rule_fields->filter_outer.vid=4096;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=vid_ani & 0x0FFF;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
				rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
				if ((ds_tag_tci & 0x1000) == 0)
				{
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
				} else {
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011;
				}

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}

				//two tags
				if (tag_mode != 1) //impossible to add another tag
				{
					rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
					rule_fields->owner_me=owner_me;
					rule_fields->bport_me=bport_me;
					rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
					rule_fields->unreal_flag = 0;

					rule_fields->filter_outer.priority=8;
					rule_fields->filter_outer.vid=vid_ani & 0x0FFF;
					rule_fields->filter_outer.tpid_de=0;
					rule_fields->filter_inner.priority=8;
					rule_fields->filter_inner.vid=4096;
					rule_fields->filter_inner.tpid_de=0;

					rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
					rule_fields->treatment_tag_to_remove=1;

					rule_fields->treatment_outer.priority=15;
					rule_fields->treatment_outer.vid=0;
					rule_fields->treatment_outer.tpid_de=0;
					rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
					rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);;
					if ((ds_tag_tci & 0x1000) == 0)
					{
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110;
					} else {
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111;
					}

					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);

						//release entry_fields
						free_safe(rule_fields);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);
					}
				}
			}
			break;
		case 4:
			//create 2 rules for replace outer tag vid of 1 and 2 tags by filtering vid_ani
			if ((vid_ani & 0x0FFF) != 0)
			{
				//one tag
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=15;
				rule_fields->filter_outer.vid=4096;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=vid_ani & 0x0FFF;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
				rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
				rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}

				//two tags
				if (tag_mode != 1) //impossible to add another tag
				{
					rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
					rule_fields->owner_me=owner_me;
					rule_fields->bport_me=bport_me;
					rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
					rule_fields->unreal_flag = 0;

					rule_fields->filter_outer.priority=8;
					rule_fields->filter_outer.vid=vid_ani & 0x0FFF;
					rule_fields->filter_outer.tpid_de=0;
					rule_fields->filter_inner.priority=8;
					rule_fields->filter_inner.vid=4096;
					rule_fields->filter_inner.tpid_de=0;

					rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
					rule_fields->treatment_tag_to_remove=1;

					rule_fields->treatment_outer.priority=15;
					rule_fields->treatment_outer.vid=0;
					rule_fields->treatment_outer.tpid_de=0;
					rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_9;
					rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_001;

					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);

						//release entry_fields
						free_safe(rule_fields);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);
					}
				}
			}
			break;
		case 5:
			if ((vid_ani & 0x0FFF) == 0)
			{
				//create untag add one tag
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=15;
				rule_fields->filter_outer.vid=4096;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=15;
				rule_fields->filter_inner.vid=4096;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=0;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				if (vid_uni <= 4095)
				{
					rule_fields->treatment_inner.priority=0;
					rule_fields->treatment_inner.vid=(vid_uni & 0x0FFF);
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
				} else {
					rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
					rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
					if ((ds_tag_tci & 0x1000) == 0)
					{
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
					} else {
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011;
					}
				}

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}
			} else {
				//create one tag add one tag
				if (tag_mode != 1) //impossible to add another tag
				{
					rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
					rule_fields->owner_me=owner_me;
					rule_fields->bport_me=bport_me;
					rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
					rule_fields->unreal_flag = 0;

					rule_fields->filter_outer.priority=15;
					rule_fields->filter_outer.vid=4096;
					rule_fields->filter_outer.tpid_de=0;
					rule_fields->filter_inner.priority=8;
					rule_fields->filter_inner.vid=vid_ani & 0x0FFF;
					rule_fields->filter_inner.tpid_de=0;

					rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
					rule_fields->treatment_tag_to_remove=0;

					rule_fields->treatment_outer.priority=15;
					rule_fields->treatment_outer.vid=0;
					rule_fields->treatment_outer.tpid_de=0;
					if (vid_uni <= 4095)
					{
						rule_fields->treatment_inner.priority=0;
						rule_fields->treatment_inner.vid=(vid_uni & 0x0FFF);
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110;
					} else {
						rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
						rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
						if ((ds_tag_tci & 0x1000) == 0)
						{
							rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110;
						} else {
							rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111;
						}
					}
					
					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);

						//release entry_fields
						free_safe(rule_fields);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);
					}
				}
			}
			break;
		case 6:
			//create 2 rules for replace outer tag of 1 and 2 tags by filtering vid_ani
			if ((vid_ani & 0x0FFF) != 0)
			{
				//one tag
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=15;
				rule_fields->filter_outer.vid=4096;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=vid_ani & 0x0FFF;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				if (vid_uni <= 4095)
				{
					rule_fields->treatment_inner.priority=0;
					rule_fields->treatment_inner.vid=(vid_uni & 0x0FFF);
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
				} else if (vid_uni == 4096) {
					rule_fields->treatment_inner.priority=15;
					rule_fields->treatment_inner.vid=0;
					rule_fields->treatment_inner.tpid_de=0;
				} else { //4097 and others
					rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
					rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
					if ((ds_tag_tci & 0x1000) == 0)
					{
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
					} else {
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011;
					}
				}

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}

				//two tags
				if (tag_mode != 1) //impossible to add another tag
				{
					rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
					rule_fields->owner_me=owner_me;
					rule_fields->bport_me=bport_me;
					rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
					rule_fields->unreal_flag = 0;

					rule_fields->filter_outer.priority=8;
					rule_fields->filter_outer.vid=vid_ani & 0x0FFF;
					rule_fields->filter_outer.tpid_de=0;
					rule_fields->filter_inner.priority=8;
					rule_fields->filter_inner.vid=4096;
					rule_fields->filter_inner.tpid_de=0;

					rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
					rule_fields->treatment_tag_to_remove=1;

					rule_fields->treatment_outer.priority=15;
					rule_fields->treatment_outer.vid=0;
					rule_fields->treatment_outer.tpid_de=0;
					if (vid_uni <= 4095)
					{
						rule_fields->treatment_inner.priority=0;
						rule_fields->treatment_inner.vid=(vid_uni & 0x0FFF);
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110;
					} else if (vid_uni == 4096) {
						rule_fields->treatment_inner.priority=15;
						rule_fields->treatment_inner.vid=0;
						rule_fields->treatment_inner.tpid_de=0;
					} else { //4097 and others
						rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
						rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
						if ((ds_tag_tci & 0x1000) == 0)
						{
							rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110;
						} else {
							rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111;
						}
					}

					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);

						//release entry_fields
						free_safe(rule_fields);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);
					}
				}
			}
			break;
		case 7:
			//create 2 rules for replace outer tag of 1 and 2 tags by filtering vid_ani
			if ((vid_ani & 0x0FFF) != 0)
			{
				//one tag
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=15;
				rule_fields->filter_outer.vid=4096;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=vid_ani & 0x0FFF;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				if (vid_uni <= 4095)
				{
					rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
					rule_fields->treatment_inner.vid=(vid_uni & 0x0FFF);
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
				} else if (vid_uni == 4096) {
					rule_fields->treatment_inner.priority=15;
					rule_fields->treatment_inner.vid=0;
					rule_fields->treatment_inner.tpid_de=0;
				} else { //4097 and others
					rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
					rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
				}

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}

				//two tags
				if (tag_mode != 1) //impossible to add another tag
				{
					rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
					rule_fields->owner_me=owner_me;
					rule_fields->bport_me=bport_me;
					rule_fields->sort_priority = omci_env_g.tag_mc_sort_priority;//higher priority then default(5)
					rule_fields->unreal_flag = 0;

					rule_fields->filter_outer.priority=8;
					rule_fields->filter_outer.vid=vid_ani & 0x0FFF;
					rule_fields->filter_outer.tpid_de=0;
					rule_fields->filter_inner.priority=8;
					rule_fields->filter_inner.vid=4096;
					rule_fields->filter_inner.tpid_de=0;

					rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
					rule_fields->treatment_tag_to_remove=1;

					rule_fields->treatment_outer.priority=15;
					rule_fields->treatment_outer.vid=0;
					rule_fields->treatment_outer.tpid_de=0;
					if (vid_uni <= 4095)
					{
						rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_9;
						rule_fields->treatment_inner.vid=(vid_uni & 0x0FFF);
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_001;
					} else if (vid_uni == 4096) {
						rule_fields->treatment_inner.priority=15;
						rule_fields->treatment_inner.vid=0;
						rule_fields->treatment_inner.tpid_de=0;
					} else { //4097 and others
						rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_9;
						rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
						rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_001;
					}

					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);

						//release entry_fields
						free_safe(rule_fields);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields);
					}
				}
			}
			break;
		default:
			;//do nothing
	}

	return 0;
}

static int
omciutil_vlan_collect_create_310_ds_wildcard_rules(struct me_t *bport_me, struct me_t *owner_me, struct list_head *rule_list_ds, unsigned char tag_mode)
{
	unsigned char ds_tag_mode;
	unsigned short ds_tag_tci;
	struct attr_value_t attr;
	struct me_t *multicast_me;
	struct omciutil_vlan_rule_fields_t *rule_fields;

	if (bport_me == NULL || owner_me == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return 0;
	}

	//get 309 from 310 attr 2
	meinfo_me_attr_get(owner_me, 2, &attr);

	if ((multicast_me = meinfo_me_get_by_meid(309, attr.data)) != NULL)
	{
		//generate rules by ds tci option
		if (multicast_me->attr[16].value.ptr != NULL)
		{
			ds_tag_mode = util_bitmap_get_value(multicast_me->attr[16].value.ptr, 8*3, 0, 8);
			ds_tag_tci = util_bitmap_get_value(multicast_me->attr[16].value.ptr, 8*3, 8, 16);
			if (ds_tag_mode > 4)
			{
				//set rules for stripping
				ds_tag_mode = 1;
			}
		} else {
			//set rules for stripping
			ds_tag_mode = 1;
		}
	} else {
		//set rules for stripping
		ds_tag_mode = 1;			
	}

	switch (ds_tag_mode)
	{
		case 0:
			//do nothing;
			break;
		case 1:
			//create 2 rules for stripping outer tag of 1 and 2 tags by filtering wildcard
			//one tag
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_2;//lower 2 priority then default(5)
			rule_fields->unreal_flag = 0;

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}

			//two tags
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_2;//lower 2 priority then default(5)
			rule_fields->unreal_flag = 0;

			rule_fields->filter_outer.priority=8;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
			break;
		case 2:
			//create untag add one tag
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_2;//lower 2 priority then default(5)
			rule_fields->unreal_flag = 0;

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=15;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=0;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
			rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
			if ((ds_tag_tci & 0x1000) == 0)
			{
				rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
			} else {
				rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011;
			}
			
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}
			break;
		case 3:
			//create 2 rules for replace outer tag of 1 and 2 tags by filtering vid_ani
			//one tag
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_2;//lower 2 priority then default(5)
			rule_fields->unreal_flag = 0;

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
			rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
			if ((ds_tag_tci & 0x1000) == 0)
			{
				rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
			} else {
				rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011;
			}

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}

			//two tags
			if (tag_mode != 1) //impossible to add another tag
			{
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_2;//lower 2 priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=8;
				rule_fields->filter_outer.vid=4096;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=4096;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=(ds_tag_tci & 0xE000) >> 13;
				rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);;
				if ((ds_tag_tci & 0x1000) == 0)
				{
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110;
				} else {
					rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111;
				}

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}
			}
			break;
		case 4:
			//create 2 rules for replace outer tag vid of 1 and 2 tags by filtering vid_ani
			//one tag
			rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=owner_me;
			rule_fields->bport_me=bport_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_2;//lower 2 priority then default(5)
			rule_fields->unreal_flag = 0;

			rule_fields->filter_outer.priority=15;
			rule_fields->filter_outer.vid=4096;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
			rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
			rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}

			//two tags
			if (tag_mode != 1) //impossible to add another tag
			{
				rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
				rule_fields->owner_me=owner_me;
				rule_fields->bport_me=bport_me;
				rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT_LOWER_2;//lower 2 priority then default(5)
				rule_fields->unreal_flag = 0;

				rule_fields->filter_outer.priority=8;
				rule_fields->filter_outer.vid=4096;
				rule_fields->filter_outer.tpid_de=0;
				rule_fields->filter_inner.priority=8;
				rule_fields->filter_inner.vid=4096;
				rule_fields->filter_inner.tpid_de=0;

				rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
				rule_fields->treatment_tag_to_remove=1;

				rule_fields->treatment_outer.priority=15;
				rule_fields->treatment_outer.vid=0;
				rule_fields->treatment_outer.tpid_de=0;
				rule_fields->treatment_inner.priority=OMCIUTIL_VLAN_TREATMENT_PRIORITY_9;
				rule_fields->treatment_inner.vid=(ds_tag_tci & 0x0FFF);
				rule_fields->treatment_inner.tpid_de=OMCIUTIL_VLAN_TREATMENT_TPID_DE_001;

				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
				{
					er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);

					//release entry_fields
					free_safe(rule_fields);
				} else {
					er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
					er_attr_group_dbprintf_rule_fields(rule_fields);
				}
			}
			break;
		default:
			;//do nothing
	}

	return 0;
}

static int //1: found, 0: not found
omciutil_vlan_collect_310_rules_find_any(struct list_head *rule_list_ds, struct me_t *owner_me)
{
	struct omciutil_vlan_rule_fields_t *rule_fields_pos;

	if (owner_me == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return 0;
	}
	
	list_for_each_entry(rule_fields_pos, rule_list_ds, rule_node)
	{
		if (rule_fields_pos->owner_me == owner_me)
		{
			return 1; //found
		}
	}
	
	return 0; //not found
}

//only collect ds rules
static int
omciutil_vlan_collect_310_rules(struct me_t *bport_me, struct me_t *owner_me, 
				struct list_head *rule_list_ds, unsigned char tag_mode)
{
	unsigned short ext_pointer;
	unsigned char row_part;
	unsigned char ds_tag_mode;
	unsigned short ds_tag_tci;
	unsigned short vid_uni, vid_ani, vid_uni_preview, vid_ani_preview;
	struct attr_value_t attr;
	struct attrinfo_table_t *attrinfo_table_ptr_package, *attrinfo_table_ptr_acl, *attrinfo_table_ptr_preview;
	struct attr_table_header_t *table_header_package, *table_header_acl, *table_header_preview;
	struct attr_table_entry_t *table_entry_pos_package, *table_entry_pos_acl, *table_entry_pos_preview;
	struct me_t *multicast_me;

	if (bport_me == NULL || owner_me == NULL || rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return 0;
	}

	if (omci_env_g.tag_mc_ds_wildcard == 0)
	{ //traditional 310,309 downstream multicast tagging rules
		//check for 310->309 and 310->65531
		if ((attrinfo_table_ptr_package = meinfo_util_aitab_ptr(310,6)) == NULL ||
			(table_header_package = (struct attr_table_header_t *) owner_me->attr[6].value.ptr) == NULL ||
			list_empty(&table_header_package->entry_list))
		{
			//none join mode

			//310->309
			meinfo_me_attr_get(owner_me, 2, &attr);

			if ((multicast_me = meinfo_me_get_by_meid(309, attr.data)) != NULL)
			{
				//check flag
				if (omci_env_g.tag_mc_ds_from_us != 0)
				{
					struct attr_value_t attr_us_tag_control;
					unsigned short us_tag_tci;

					//get control mode == 1, and upstream tci
					meinfo_me_attr_get(multicast_me, 5, &attr_us_tag_control);
					if (attr_us_tag_control.data == 1)
					{
						us_tag_tci = util_bitmap_get_value(multicast_me->attr[4].value.ptr, 8*2, 0, 16);
						omciutil_vlan_collect_create_310_ds_rules_from_up_tci(us_tag_tci, bport_me, owner_me, rule_list_ds);
					}

				} else {

					vid_uni = 0; //no use for 310->309
					if (multicast_me->attr[16].value.ptr != NULL)
					{
						ds_tag_mode = util_bitmap_get_value(multicast_me->attr[16].value.ptr, 8*3, 0, 8);
						ds_tag_tci = util_bitmap_get_value(multicast_me->attr[16].value.ptr, 8*3, 8, 16);
						if (ds_tag_mode > 4)
						{
							ds_tag_mode = 0;
						}

						//check dynamic
						if ((attrinfo_table_ptr_acl = meinfo_util_aitab_ptr(309, 7)) != NULL &&
							(table_header_acl = (struct attr_table_header_t *) multicast_me->attr[7].value.ptr) != NULL)
						{
							list_for_each_entry(table_entry_pos_acl, &table_header_acl->entry_list, entry_node)
							{
								vid_ani = util_bitmap_get_value(table_entry_pos_acl->entry_data_ptr, 8*attrinfo_table_ptr_acl->entry_byte_size, 32, 16);

								//create ds rules
								omciutil_vlan_collect_create_310_ds_rules(ds_tag_mode, ds_tag_tci, vid_uni, vid_ani, bport_me, owner_me, rule_list_ds, tag_mode);
							}
						}

						//check static
						if ((attrinfo_table_ptr_acl = meinfo_util_aitab_ptr(309, 8)) != NULL &&
							(table_header_acl = (struct attr_table_header_t *) multicast_me->attr[8].value.ptr) != NULL)
						{
							list_for_each_entry(table_entry_pos_acl, &table_header_acl->entry_list, entry_node)
							{
								vid_ani = util_bitmap_get_value(table_entry_pos_acl->entry_data_ptr, 8*attrinfo_table_ptr_acl->entry_byte_size, 32, 16);

								//create ds rules
								omciutil_vlan_collect_create_310_ds_rules(ds_tag_mode, ds_tag_tci, vid_uni, vid_ani, bport_me, owner_me, rule_list_ds, tag_mode);
							}
						}

						//check preview table
						if (omci_env_g.tag_preview_table)
						{
							if ((attrinfo_table_ptr_preview = meinfo_util_aitab_ptr(310,7)) != NULL &&
								(table_header_preview = (struct attr_table_header_t *) owner_me->attr[7].value.ptr) != NULL)
							{
								list_for_each_entry(table_entry_pos_preview, &table_header_preview->entry_list, entry_node)
								{
									if (table_entry_pos_preview->entry_data_ptr != NULL &&
										util_bitmap_get_value(table_entry_pos_preview->entry_data_ptr, 8*attrinfo_table_ptr_preview->entry_byte_size, 2, 2) == 0)
									{
										vid_uni_preview = util_bitmap_get_value(table_entry_pos_preview->entry_data_ptr, 8*attrinfo_table_ptr_preview->entry_byte_size, 160, 16);
										vid_ani_preview = util_bitmap_get_value(table_entry_pos_preview->entry_data_ptr, 8*attrinfo_table_ptr_preview->entry_byte_size, 144, 16);

										//create ds rules
										omciutil_vlan_collect_create_310_ds_rules(ds_tag_mode, ds_tag_tci, vid_uni_preview, vid_ani_preview, bport_me, owner_me, rule_list_ds, tag_mode);
									}
								}
							}
						}
					}
				}
			}
		} else {
			//join mode

			//310@6 table entry -> 309
			list_for_each_entry(table_entry_pos_package, &table_header_package->entry_list, entry_node)
			{
				if (table_entry_pos_package->entry_data_ptr != NULL)
				{
					//get Extended_multicast_operations_profile_pointer
					ext_pointer = util_bitmap_get_value(table_entry_pos_package->entry_data_ptr, 8*attrinfo_table_ptr_package->entry_byte_size, 80, 16);
					vid_uni = util_bitmap_get_value(table_entry_pos_package->entry_data_ptr, 8*attrinfo_table_ptr_package->entry_byte_size, 16, 16);

					if ((multicast_me = meinfo_me_get_by_meid(309, ext_pointer)) != NULL)
					{
						if (multicast_me->attr[16].value.ptr == NULL)
						{
							continue; //package
						}

						ds_tag_mode = util_bitmap_get_value(multicast_me->attr[16].value.ptr, 8*3, 0, 8);
						ds_tag_tci = util_bitmap_get_value(multicast_me->attr[16].value.ptr, 8*3, 8, 16);

						//check dynamic
						if ((attrinfo_table_ptr_acl = meinfo_util_aitab_ptr(309, 7)) != NULL &&
							(table_header_acl = (struct attr_table_header_t *) multicast_me->attr[7].value.ptr) != NULL)
						{
							list_for_each_entry(table_entry_pos_acl, &table_header_acl->entry_list, entry_node)
							{
								if ((row_part = util_bitmap_get_value(table_entry_pos_acl->entry_data_ptr, 8*attrinfo_table_ptr_acl->entry_byte_size, 2, 3)) == 0) //format 0
								{
									vid_ani = util_bitmap_get_value(table_entry_pos_acl->entry_data_ptr, 8*attrinfo_table_ptr_acl->entry_byte_size, 32, 16);

									//create ds rules
									omciutil_vlan_collect_create_310_ds_rules(ds_tag_mode, ds_tag_tci, vid_uni, vid_ani, bport_me, owner_me, rule_list_ds, tag_mode);
								}
							}
						}

						//check static
						if ((attrinfo_table_ptr_acl = meinfo_util_aitab_ptr(309, 8)) != NULL &&
							(table_header_acl = (struct attr_table_header_t *) multicast_me->attr[8].value.ptr) != NULL)
						{
							list_for_each_entry(table_entry_pos_acl, &table_header_acl->entry_list, entry_node)
							{
								if ((row_part = util_bitmap_get_value(table_entry_pos_acl->entry_data_ptr, 8*attrinfo_table_ptr_acl->entry_byte_size, 2, 3)) == 0) //format 0
								{
									vid_ani = util_bitmap_get_value(table_entry_pos_acl->entry_data_ptr, 8*attrinfo_table_ptr_acl->entry_byte_size, 32, 16);

									//create ds rules
									omciutil_vlan_collect_create_310_ds_rules(ds_tag_mode, ds_tag_tci, vid_uni, vid_ani, bport_me, owner_me, rule_list_ds, tag_mode);
								}
							}
						}
					}
				}
			}

			//check preview table
			if (omci_env_g.tag_preview_table)
			{
				if ((attrinfo_table_ptr_preview = meinfo_util_aitab_ptr(310,7)) != NULL &&
					(table_header_preview = (struct attr_table_header_t *) owner_me->attr[7].value.ptr) != NULL)
				{
					list_for_each_entry(table_entry_pos_preview, &table_header_preview->entry_list, entry_node)
					{
						if (table_entry_pos_preview->entry_data_ptr != NULL &&
							util_bitmap_get_value(table_entry_pos_preview->entry_data_ptr, 8*attrinfo_table_ptr_preview->entry_byte_size, 2, 2) == 0)
						{
							vid_uni_preview = util_bitmap_get_value(table_entry_pos_preview->entry_data_ptr, 8*attrinfo_table_ptr_preview->entry_byte_size, 160, 16);
							vid_ani_preview = util_bitmap_get_value(table_entry_pos_preview->entry_data_ptr, 8*attrinfo_table_ptr_preview->entry_byte_size, 144, 16);

							//create ds rules, replace the tag
							omciutil_vlan_collect_create_310_ds_rules(6, 0, vid_uni_preview, vid_ani_preview, bport_me, owner_me, rule_list_ds, tag_mode);
						}
					}
				}
			}
		}

		//check if no 310 rules created by myself, add ds wildcard rules
		if (!omciutil_vlan_collect_310_rules_find_any(rule_list_ds, owner_me))
		{
			//generate ds wildcard rules
			omciutil_vlan_collect_create_310_ds_wildcard_rules(bport_me, owner_me, rule_list_ds, tag_mode);
		}
	} else {
		//generate ds wildcard rules
		omciutil_vlan_collect_create_310_ds_wildcard_rules(bport_me, owner_me, rule_list_ds, tag_mode);
	}

	return 0;
}

int
omciutil_vlan_pbit_dont_care_workaround(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	if (rule_fields == NULL)
	{
		return -1;
	}

	//check outer first
	if (rule_fields->filter_outer.priority <= OMCIUTIL_VLAN_FILTER_PRIORITY_7)
	{
		rule_fields->filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
	}
	//check inner
	if (rule_fields->filter_inner.priority <= OMCIUTIL_VLAN_FILTER_PRIORITY_7)
	{
		rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
	}

	return 0;
}

int
omciutil_vlan_pbit_remap_workaround(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	if (rule_fields == NULL)
	{
		return -1;
	}

	//check outer first
	if (rule_fields->filter_outer.priority <= OMCIUTIL_VLAN_FILTER_PRIORITY_7)
	{
		rule_fields->filter_outer.priority = omci_env_g.tag_pbit_remap_table[rule_fields->filter_outer.priority];
	}
	//check inner
	if (rule_fields->filter_inner.priority <= OMCIUTIL_VLAN_FILTER_PRIORITY_7)
	{
		rule_fields->filter_inner.priority = omci_env_g.tag_pbit_remap_table[rule_fields->filter_inner.priority];
	}

	return 0;
}


int
omciutil_vlan_pbit_dont_care_workaround_transparent(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	if (rule_fields == NULL)
	{
		return -1;
	}

	if (omciutil_vlan_get_rule_filter_tag_num(rule_fields) > 0)
	{
		//modify outer treatment
		if (rule_fields->treatment_outer.priority <= OMCIUTIL_VLAN_TREATMENT_PRIORITY_7)
		{
			if (rule_fields->filter_outer.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				//pbit copy from outer
				rule_fields->treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_9;
			} else {
				//pbit copy from inner
				rule_fields->treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
			}
		}

		//modify inner treatment
		if (rule_fields->treatment_inner.priority <= OMCIUTIL_VLAN_TREATMENT_PRIORITY_7)
		{
			if (rule_fields->filter_outer.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				//pbit copy from outer
				rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_9;
			} else {
				//pbit copy from inner
				rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
			}
		}

		//modify outer filter
		if (rule_fields->filter_outer.priority <= OMCIUTIL_VLAN_FILTER_PRIORITY_7)
		{
			rule_fields->filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
		}
		//modify inner filter
		if (rule_fields->filter_inner.priority <= OMCIUTIL_VLAN_FILTER_PRIORITY_7)
		{
			rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
		}
	}

	return 0;
}



#define OMCIUTIL_VLAN_COLLECT_CTC_RULE_TOKEN_NUM 5

/*
# port  match vlan,pbit action vlan,pbit 
#------ -------------   ----------------- 
0,      untag,0,        2000,7 
1,      1000,*,         untag,0 
2,      1000,*,         none,0   
3,      *,*,            2000,none 
4,      *,*,            none,none 

port num: 0..4 
match vlan: it could be untag, specific vid or * (* means don't care) 
match pbit: it could be 0..7 or * (* means dont care) 
vlan action: it could be untag, specific vid or none (none means dont change) 
pbit action: it could be 0..7 or none (none means dont change) 
*/

static int
omciutil_vlan_collect_ctc_rules(struct me_t *bridge_port_me, struct list_head *rule_list_us, struct list_head *rule_list_ds)
{
	unsigned int portid;
	char line[LINE_MAXSIZE];
	char *rule_array[OMCIUTIL_VLAN_COLLECT_CTC_RULE_TOKEN_NUM];
	FILE *fp;
	struct switch_port_info_t port_info;
	struct omciutil_vlan_rule_fields_t *rule_fields, *rule_fields_reverse;

	if (bridge_port_me == NULL ||
		rule_list_us == NULL ||
		rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//get port info
	if (switch_get_port_info_by_me(bridge_port_me, &port_info) != 0 )
	{
		dbprintf(LOG_ERR, "can not get port info, meid=0x%.4x\n", bridge_port_me->meid);
		return -1;
	}

	//open external file
	if ((fp = fopen(omci_env_g.tag_ctc_translation_path, "r")) == NULL)
	{
		dbprintf(LOG_WARNING, "open external file=%s error!!\n", omci_env_g.tag_ctc_translation_path);
		return -1;
	}

	//find the same port rule
	while (fgets(line, LINE_MAXSIZE, fp) != NULL)
	{
		if (line[0]=='#')	// comment, ignore
		{
			continue;
		}

		//rule->port_id, filter_vid, filter_pri, treatment_vid, treatment_pri
		if (conv_str2array(line, ",", rule_array, OMCIUTIL_VLAN_COLLECT_CTC_RULE_TOKEN_NUM) < OMCIUTIL_VLAN_COLLECT_CTC_RULE_TOKEN_NUM)
		{
			continue; //skip if token number is not 5
		}

		//check port id first
		if ((portid = atoi(rule_array[0])) != port_info.logical_port_id)
		{
			continue;
		}

		//fulfill the rule
		rule_fields=malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me = NULL;
		rule_fields->bport_me = bridge_port_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default(5)
		rule_fields->unreal_flag = 0;

		//set rule default, filter
		rule_fields->filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		rule_fields->filter_outer.vid = 0;
		rule_fields->filter_outer.tpid_de = 0;
		rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		rule_fields->filter_inner.vid = 0;
		rule_fields->filter_inner.tpid_de = 0;
		rule_fields->filter_ethertype = OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		//set rule default, treatment
		rule_fields->treatment_tag_to_remove = 0;
		rule_fields->treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields->treatment_outer.vid = 0;
		rule_fields->treatment_outer.tpid_de = 0;
		rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields->treatment_inner.vid = 0;
		rule_fields->treatment_inner.tpid_de = 0;

		//filter_vid
		if (!strcmp(rule_array[1], "*")) //one tag,vid do not care
		{
			rule_fields->filter_inner.vid = 4096;
		} else if (!strcmp(rule_array[1], "untag")) { //untag
			rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		} else if (util_str_is_number(rule_array[1])) { //exactly one tag
			rule_fields->filter_inner.vid = atoi(rule_array[1]) <= 4095 ? atoi(rule_array[1]) : 4095;
		} else {
			//error token, release
			dbprintf(LOG_ERR, "ctc rule token filter vid error!!\n");
			free_safe(rule_fields);
			continue;
		}

		//filter_pri
		if (strcmp(rule_array[1], "untag")) //NOT untag
		{
			if (!strcmp(rule_array[2], "*")) //do not care pri
			{
				rule_fields->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
			} else if (util_str_is_number(rule_array[2])) { //exactly pri
				rule_fields->filter_inner.priority = atoi(rule_array[2]) <= 7 ? atoi(rule_array[2]) : 7;
			} else {
				//error token, release
				dbprintf(LOG_ERR, "ctc rule token filter pri error!!\n");
				free_safe(rule_fields);
				continue;
			}
		}

		//treatment_vid
		if (!strcmp(rule_array[3], "untag"))
		{
			if (rule_fields->filter_inner.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->treatment_tag_to_remove = 1;
			}
		} else if (!strcmp(rule_array[3], "none")) {
			if (rule_fields->filter_inner.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->treatment_inner.vid = 4096;
			}
		} else if (util_str_is_number(rule_array[3])) {
			rule_fields->treatment_inner.vid = atoi(rule_array[3]) <= 4095 ? atoi(rule_array[3]) : 4095;
		} else {
			//error token, release
			dbprintf(LOG_ERR, "ctc rule token treatment vid error!!\n");
			free_safe(rule_fields);
			continue;
		}

		//treatment_pri, set tag_to_remove, pri, tpid_de
		if (strcmp(rule_array[3], "untag")) //NOT remove tag
		{
			if (rule_fields->filter_inner.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15)
			{
				rule_fields->treatment_tag_to_remove = 1;
			} else {
				rule_fields->treatment_tag_to_remove = 0;
			}			

			if (!strcmp(rule_array[4], "none")) {
				if (rule_fields->filter_inner.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15)
				{
					rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
					rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
				} else if (strcmp(rule_array[3], "none")) { //treatment vid is NOT none, but untag, assign pri to 0
					rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_0;
					rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
				}
			} else if (util_str_is_number(rule_array[4])) {
				rule_fields->treatment_inner.priority= atoi(rule_array[4]) <= 7 ? atoi(rule_array[4]) : 7;
				rule_fields->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
			} else {
				//error token, release
				dbprintf(LOG_ERR, "ctc rule token treatment pri error!!\n");
				free_safe(rule_fields);
				continue;
			}
		}

		//reverse for downstream
		rule_fields_reverse = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields_reverse->owner_me = NULL;
		rule_fields_reverse->bport_me = bridge_port_me;
		rule_fields_reverse->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
		rule_fields_reverse->unreal_flag = 0; //real

		if (omciutil_vlan_reverse_rule_generate(rule_fields, rule_fields_reverse) == 0)
		{
			//insert to upstream list
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "US-insert to us list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields_reverse
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "US: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}

			//insert to downstream list
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_reverse, rule_list_ds) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields_reverse);

				//release entry_fields_reverse
				free_safe(rule_fields_reverse);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
				er_attr_group_dbprintf_rule_fields(rule_fields_reverse);
			}
		} else {
			//error token, release
			free_safe(rule_fields);
			free_safe(rule_fields_reverse);
			continue;
		}
	}

	fclose(fp);

	return 0;
}

//tag mode: 0, collect all tagging rules. 1: ignore 2 tags rules.
static int 
omciutil_vlan_collect_rules_by_port_for_171(int action,
	struct me_t *issue_bridge_port_me,
	struct me_t *issue_me,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds,
	unsigned char tag_mode,
	struct omciutil_vlan_tlan_table_t *tlan_table_bridge)
{
	struct meinfo_t *miptr_171;
	struct me_t *me_pos_171;
	struct attr_value_t attr_value1, attr_value2;
	struct me_t *me;

	//collect 9.3.13 
	if ((miptr_171 = meinfo_util_miptr(171)) == NULL) // 171 Extended_VLAN_tagging_operation_configuration_data
	{
		dbprintf(LOG_ERR, "get 171 miptr error\n");
		return -1;
	}

	list_for_each_entry(me_pos_171, &miptr_171->me_instance_list, instance_node)
	{
		//check association type
		meinfo_me_attr_get(me_pos_171, 1, &attr_value1);
		//get association pointer meid
		meinfo_me_attr_get(me_pos_171, 7, &attr_value2);
		switch (attr_value1.data)
		{
		case 0:
			//bridge port configuration data, 47
			if ((me = meinfo_me_get_by_meid(47, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=47, meid=%u, for class 171 related\n", attr_value2.data);
			} else {
				if (me == issue_bridge_port_me &&
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_171))
				{
					//merge all
					omciutil_vlan_collect_171_rules(issue_bridge_port_me,
						me_pos_171,
						rule_list_us,
						rule_list_ds,
						tag_mode,
						tlan_table_bridge);
				}
			}
			break;
		case 1:
			//802.1p mapper service profile, 130
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(130, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=130, meid=%u, for class 171 related\n", attr_value2.data);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->130
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_171))
				{
					//merge all
					omciutil_vlan_collect_171_rules(issue_bridge_port_me,
						me_pos_171,
						rule_list_us,
						rule_list_ds,
						tag_mode,
						tlan_table_bridge);
				}
			}
			break;
		case 2:
			//pptp ethernet uni, 11
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(11, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=11, meid=%u, for class 171 related\n", attr_value2.data);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->11
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_171))
				{
					//merge all
					omciutil_vlan_collect_171_rules(issue_bridge_port_me,
						me_pos_171,
						rule_list_us,
						rule_list_ds,
						tag_mode,
						tlan_table_bridge);
				}
			}
			break;
		case 3:
			//IP host config data, 134
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(134, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=134, meid=%u, for class 171 related\n", attr_value2.data);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->134
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_171))
				{
					//merge all
					omciutil_vlan_collect_171_rules(issue_bridge_port_me,
						me_pos_171,
						rule_list_us,
						rule_list_ds,
						tag_mode,
						tlan_table_bridge);
				}
			}
			break;
		case 10:
			//VEIP, 329
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(329, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=329, meid=%u, for class 171 related\n", attr_value2.data);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->329
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_171))
				{
					//merge all
					omciutil_vlan_collect_171_rules(issue_bridge_port_me,
						me_pos_171,
						rule_list_us,
						rule_list_ds,
						tag_mode,
						tlan_table_bridge);
				}
			}
			break;
		//others not supported
		default:
			dbprintf(LOG_ERR, "not supported classid=171, meid=%u association type=%u\n", me_pos_171->meid, attr_value1.data);
		}
	}

	return 0;
}

//tag mode: 0, collect all tagging rules. 1: ignore 2 tags rules.
static int 
omciutil_vlan_collect_rules_by_port_for_249(int action,
	struct me_t *issue_bridge_port_me,
	struct me_t *issue_me,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds,
	unsigned char tag_mode,
	struct omciutil_vlan_tlan_table_t *tlan_table_bridge)
{
	struct meinfo_t *miptr_249;
	struct me_t *me_pos_249;
	struct attr_value_t attr_value1;
	struct me_t *me;

	//collect 9.99.6 
	if ((miptr_249 = meinfo_util_miptr(249)) == NULL) // 249 ExtL2VlanTagConfigData
	{
		dbprintf(LOG_ERR, "get 249 miptr error\n");
		return -1;
	}

	list_for_each_entry(me_pos_249, &miptr_249->me_instance_list, instance_node)
	{
		//check association type
		meinfo_me_attr_get(me_pos_249, 1, &attr_value1);
		
		switch (attr_value1.data)
		{
		case 0:
			//bridge port configuration data, 47
			if ((me = meinfo_me_get_by_meid(47, me_pos_249->meid)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=47, meid=%u, for class 249 related\n", me_pos_249->meid);
			} else {
				if (me == issue_bridge_port_me &&
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_249))
				{
					//merge all
					omciutil_vlan_collect_249_rules(issue_bridge_port_me,
						me_pos_249,
						rule_list_us,
						rule_list_ds,
						tlan_table_bridge);
				}
			}
			break;
		case 1:
			//802.1p mapper service profile, 130
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(130, me_pos_249->meid)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=130, meid=%u, for class 249 related\n", me_pos_249->meid);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->130
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_249))
				{
					//merge all
					omciutil_vlan_collect_249_rules(issue_bridge_port_me,
						me_pos_249,
						rule_list_us,
						rule_list_ds,
						tlan_table_bridge);
				}
			}
			break;
		case 2:
			//pptp ethernet uni, 11
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(11, me_pos_249->meid)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=11, meid=%u, for class 249 related\n", me_pos_249->meid);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->11
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_249))
				{
					//merge all
					omciutil_vlan_collect_249_rules(issue_bridge_port_me,
						me_pos_249,
						rule_list_us,
						rule_list_ds,
						tlan_table_bridge);
				}
			}
			break;
		case 3:
			//IP host config data, 134
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(134, me_pos_249->meid)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=134, meid=%u, for class 249 related\n", me_pos_249->meid);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->134
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_249))
				{
					//merge all
					omciutil_vlan_collect_249_rules(issue_bridge_port_me,
						me_pos_249,
						rule_list_us,
						rule_list_ds,
						tlan_table_bridge);
				}
			}
			break;
		case 10:
			//VEIP, 329
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(329, me_pos_249->meid)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=329, meid=%u, for class 249 related\n", me_pos_249->meid);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->329
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_249))
				{
					//merge all
					omciutil_vlan_collect_249_rules(issue_bridge_port_me,
						me_pos_249,
						rule_list_us,
						rule_list_ds,
						tlan_table_bridge);
				}
			}
			break;
		//others not supported
		default:
			dbprintf(LOG_ERR, "not supported classid=249, meid=%u association type=%u\n", me_pos_249->meid, attr_value1.data);
		}
	}

	return 0;
}

#if 0 //temporary, should review after tlan implementation finished.
//for 65313
static void
omciutil_vlan_collect_add_transparent_rules(
	struct me_t *issue_bridge_port_me,
	struct me_t *issue_me, //me 65313
	struct list_head *rule_list,
	unsigned char tag_mode)
{
	unsigned short transparent_vid;
	struct omciutil_vlan_rule_fields_t *rule_fields;
	
	if (issue_bridge_port_me == NULL ||
		issue_me == NULL ||
		rule_list == NULL)
	{
		return;
	}

	//get transparent vid
	transparent_vid = issue_me->meid & 0xfff;
	
	//one tag
	rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
	rule_fields->owner_me=issue_me;
	rule_fields->bport_me=issue_bridge_port_me;
	rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_LOWEST; //lowest priority
	rule_fields->unreal_flag = 0; //real

	rule_fields->filter_outer.priority=15;
	rule_fields->filter_outer.vid=4096;
	rule_fields->filter_outer.tpid_de=0;
	rule_fields->filter_inner.priority=8;
	rule_fields->filter_inner.vid=transparent_vid;
	rule_fields->filter_inner.tpid_de=0;

	rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
	rule_fields->treatment_tag_to_remove=0;

	rule_fields->treatment_outer.priority=15;
	rule_fields->treatment_outer.vid=0;
	rule_fields->treatment_outer.tpid_de=0;
	rule_fields->treatment_inner.priority=15;
	rule_fields->treatment_inner.vid=0;
	rule_fields->treatment_inner.tpid_de=0;

	if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list) < 0)
	{
		dbprintf(LOG_ERR, "can not insert 65313 transparent one tag rule, bridge port meid=%u!!\n", issue_bridge_port_me->meid);
		//release entry_fields
		free_safe(rule_fields);
	}

	if (tag_mode == 0) //collect all rules(include 2 tags)
	{
		//two tags
		rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
		rule_fields->owner_me=issue_me;
		rule_fields->bport_me=issue_bridge_port_me;
		rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_LOWEST; //lowest priority
		rule_fields->unreal_flag = 0; //real

		rule_fields->filter_outer.priority=8;
		rule_fields->filter_outer.vid=transparent_vid;
		rule_fields->filter_outer.tpid_de=0;
		rule_fields->filter_inner.priority=8;
		rule_fields->filter_inner.vid=4096;
		rule_fields->filter_inner.tpid_de=0;

		rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
		rule_fields->treatment_tag_to_remove=0;

		rule_fields->treatment_outer.priority=15;
		rule_fields->treatment_outer.vid=0;
		rule_fields->treatment_outer.tpid_de=0;
		rule_fields->treatment_inner.priority=15;
		rule_fields->treatment_inner.vid=0;
		rule_fields->treatment_inner.tpid_de=0;

		if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list) < 0)
		{
			dbprintf(LOG_ERR, "can not insert 65313 transparent two tag rule, bridge port meid=%u!!\n", issue_bridge_port_me->meid);
			//release entry_fields
			free_safe(rule_fields);
		}
	}
	
	return;
}

//tag mode: 0, collect all tagging rules. 1: ignore 2 tags rules.
static int 
omciutil_vlan_collect_rules_by_port_for_65313(int action,
	struct me_t *issue_bridge_port_me,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds,
	unsigned char tag_mode)
{
	struct meinfo_t *miptr_65313;
	struct me_t *me_pos_65313;
	struct attr_value_t attr_value;
	struct switch_port_info_t port_info;

	//collect 9.99.16
	if ((miptr_65313 = meinfo_util_miptr(65313)) == NULL) // 65313 CalixVlan
	{
		dbprintf(LOG_ERR, "get 65313 miptr error\n");
		return -1;
	}

	//get first one and assume only one
	list_for_each_entry(me_pos_65313, &miptr_65313->me_instance_list, instance_node)
	{
		//Tlan
		meinfo_me_attr_get(me_pos_65313, 4, &attr_value); 
		if (attr_value.data > 0)
		{
			//check for lan bridge port port only
			if (switch_get_port_info_by_me(issue_bridge_port_me, &port_info) == 0 &&
				(port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI ||
				port_info.port_type == ENV_BRIDGE_PORT_TYPE_IPHOST ||
				port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU))
			{
				//add upstream rule
				omciutil_vlan_collect_add_transparent_rules(issue_bridge_port_me,
					me_pos_65313,
					rule_list_us,
					tag_mode);

				//add downstream rule
				omciutil_vlan_collect_add_transparent_rules(issue_bridge_port_me,
					me_pos_65313,
					rule_list_ds,
					tag_mode);
			}
		}
		
		break; //only one 65313
	}

	return 0;
}
#endif

static int
omciutil_vlan_collect_rules_by_port_for_65313(struct me_t *issue_bridge_port_me,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds)
{
	struct switch_port_info_t port_info;
	struct omciutil_vlan_rule_fields_t *rule_fields, *rule_fields_pos, *rule_fields_reverse;
	struct omciutil_vlan_dscp2pbit_info_t dscp2pbit_info;

	if (issue_bridge_port_me == NULL ||
		rule_list_us == NULL ||
		rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	if (switch_get_port_info_by_me(issue_bridge_port_me, &port_info) != 0 ||
		(port_info.port_type != ENV_BRIDGE_PORT_TYPE_UNI &&
		port_info.port_type != ENV_BRIDGE_PORT_TYPE_IPHOST &&
		port_info.port_type != ENV_BRIDGE_PORT_TYPE_CPU))
	{
		//do notthing
		return 0;
	}

	list_for_each_entry(rule_fields_pos, rule_list_us, rule_node)
	{
		switch (rule_fields_pos->tlan_workaround_flag)
		{
		case 1:
			//transparent double tags rule for us
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=meinfo_me_get_by_meid(65313, rule_fields_pos->tlan_vid); // vid = meid
			rule_fields->bport_me=issue_bridge_port_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default priority
			rule_fields->unreal_flag = 0; //real

			rule_fields->filter_outer.priority=8;
			rule_fields->filter_outer.vid=rule_fields_pos->filter_inner.vid;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=0;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
			{
				dbprintf(LOG_ERR, "can not insert us 65313 tlan transparent two tags rule, bridge port meid=%u!!\n", issue_bridge_port_me->meid);
				//release entry_fields
				free_safe(rule_fields);
			}

			//transparent double tags rule for ds, the same with us
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=meinfo_me_get_by_meid(65313, rule_fields_pos->tlan_vid); // vid = meid;
			rule_fields->bport_me=issue_bridge_port_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default priority
			rule_fields->unreal_flag = 0; //real

			rule_fields->filter_outer.priority=8;
			rule_fields->filter_outer.vid=rule_fields_pos->filter_inner.vid;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=0;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			rule_fields->treatment_inner.priority=15;
			rule_fields->treatment_inner.vid=0;
			rule_fields->treatment_inner.tpid_de=0;

			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_ds) < 0)
			{
				dbprintf(LOG_ERR, "can not insert ds 65313 tlan transparent two tags rule, bridge port meid=%u!!\n", issue_bridge_port_me->meid);
				//release entry_fields
				free_safe(rule_fields);
			}
			
			break;
		case 4:
			//change double tags rule for us, reference rule_fields_pos must be "1 tag - 1 tag + 1 tag"
			rule_fields = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields->owner_me=meinfo_me_get_by_meid(65313, rule_fields_pos->tlan_vid); // vid = meid
			rule_fields->bport_me=issue_bridge_port_me;
			rule_fields->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT; //default priority
			rule_fields->unreal_flag = 0; //real

			rule_fields->filter_outer.priority=rule_fields_pos->filter_inner.priority;
			rule_fields->filter_outer.vid=rule_fields_pos->filter_inner.vid;
			rule_fields->filter_outer.tpid_de=0;
			rule_fields->filter_inner.priority=8;
			rule_fields->filter_inner.vid=4096;
			rule_fields->filter_inner.tpid_de=0;

			rule_fields->filter_ethertype=OMCIUTIL_VLAN_FILTER_ET_IGNORE;
			rule_fields->treatment_tag_to_remove=1;

			rule_fields->treatment_outer.priority=15;
			rule_fields->treatment_outer.vid=0;
			rule_fields->treatment_outer.tpid_de=0;
			if (rule_fields_pos->treatment_inner.priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_8) //copy from inner
			{
				rule_fields->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_9; //copy from outer
			} else {
				rule_fields->treatment_inner.priority = rule_fields_pos->treatment_inner.priority; //normal pbit
			}
			if (rule_fields_pos->treatment_inner.vid == 4096) //copy from inner
			{
				rule_fields->treatment_inner.vid = 4097; //copy from outer
			} else {
				rule_fields->treatment_inner.vid = rule_fields_pos->treatment_inner.vid;
			}
			rule_fields->treatment_inner.tpid_de = 1; //copy from outer

			//allocate an rule_fields_reverse and fill it
			rule_fields_reverse = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
			rule_fields_reverse->owner_me=meinfo_me_get_by_meid(65313, rule_fields_pos->tlan_vid); // vid = meid
			rule_fields_reverse->bport_me=issue_bridge_port_me;
			rule_fields_reverse->sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_DEFAULT;
			rule_fields_reverse->unreal_flag = 0; //real

			if (omciutil_vlan_reverse_rule_generate(rule_fields, rule_fields_reverse) == 0)
			{
				//check for dscp-2-pbit reversing
				if (rule_fields_reverse->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13 ||
					rule_fields_reverse->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13)
				{
					//collect dscp2pbit info
					omciutil_vlan_collect_dscp2pbit_info_by_port(issue_bridge_port_me, &dscp2pbit_info);
					
					//expand dscp-2-pbit bitmap
					if (omciutil_vlan_expand_dscp2pbit_reversing_rule_to_list_ds(rule_fields_reverse, &dscp2pbit_info, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "generate reverse rule error\n");
						//insert to downstream
						if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_reverse, rule_list_ds) < 0)
						{
							er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
							er_attr_group_dbprintf_rule_fields(rule_fields_reverse);

							//release entry_fields_reverse
							free_safe(rule_fields_reverse);
						} else {
							er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
							er_attr_group_dbprintf_rule_fields(rule_fields_reverse);
						}
					} else {
						//expand ok, free original one
						free_safe(rule_fields_reverse);
					}
				} else {
					//insert to downstream
					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_reverse, rule_list_ds) < 0)
					{
						er_attr_group_dbprintf(LOG_ERR, 0, "DS-insert to ds list error: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_reverse);

						//release entry_fields_reverse
						free_safe(rule_fields_reverse);
					} else {
						er_attr_group_dbprintf(LOG_ERR, 0, "DS: ");
						er_attr_group_dbprintf_rule_fields(rule_fields_reverse);
					}
				}
			}

			//insert to list by correct order;
			if (omciutil_vlan_insert_rule_fields_to_list(rule_fields, rule_list_us) < 0)
			{
				er_attr_group_dbprintf(LOG_ERR, 0, "US-insert to us list error: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);

				//release entry_fields
				free_safe(rule_fields);
			} else {
				er_attr_group_dbprintf(LOG_ERR, 0, "US: ");
				er_attr_group_dbprintf_rule_fields(rule_fields);
			}

			break;
		default:
			; //do nothing
		}
	}
	
	return 0;
}

static int 
omciutil_vlan_collect_handle_veip_bridge_rules(unsigned char action, //0: mark only, 1: mark and copy to dest

	struct list_head *rule_list_us,
	struct list_head *rule_list_ds,
	struct list_head *rule_list_us_dest,
	struct list_head *rule_list_ds_dest)
{
	unsigned int found;
	unsigned short vid;
	struct omciutil_vlan_rule_fields_t *rule_fields, *rule_fields_n, *rule_fields_new;
	struct omciutil_vlan_rule_result_list_t rule_result_list;
	struct omciutil_vlan_rule_result_node_t *result_node;

	if (rule_list_us == NULL ||
		rule_list_ds == NULL ||
		(action == 1 && 
		(rule_list_us_dest == NULL || rule_list_ds_dest == NULL)))
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//check upstream list
	list_for_each_entry_safe(rule_fields, rule_fields_n, rule_list_us, rule_node)
	{
		if (rule_fields->owner_me != NULL && rule_fields->owner_me->classid == 171)
		{
			if (omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 0 || //untag rules
				(omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 1 && //priority tag rules
				rule_fields->filter_inner.vid == 0) ||
				(omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 1 && //1tag + 1tag rules
				rule_fields->treatment_tag_to_remove == 0 &&
				omciutil_vlan_get_rule_treatment_tag_num(rule_fields) == 1))
			{
				//mark or copy to dest
				if (action == 0)
				{ //mark
					rule_fields->veip_flag = 1; //mark for veip bridge rules
				} else { //copy to dest
					rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
					*rule_fields_new = *rule_fields; //copy

					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, rule_list_us_dest) < 0)
					{
						free_safe(rule_fields_new);
						dbprintf(LOG_ERR, "reassign us tagging rule error!!\n");
					}
				}
			}
		}
	}

	//check downstream list
	list_for_each_entry_safe(rule_fields, rule_fields_n, rule_list_ds, rule_node)
	{
		if (rule_fields->owner_me != NULL && rule_fields->owner_me->classid == 171)
		{
			//generate result
			if (omciutil_vlan_generate_rule_result(rule_fields, &rule_result_list) < 0)
			{
				dbprintf(LOG_ERR, "could not get rule result\n");
				continue;
			}

			found = 0;
			//check result tag num
			switch (rule_result_list.tag_count)
			{
			case 0: //untag
				found = 1;//add to list
				break;
			case 1: //one tag
				//find for priority tag rules
				vid = 0xffff;
				//get result outer tag
				result_node = list_entry(rule_result_list.tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
				//check result_node to get vid
				switch (result_node->code)
				{
				case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER: //impossible
					vid = rule_fields->filter_outer.vid;
					break;
				case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
					vid = rule_fields->filter_inner.vid;
					break;
				case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER: //impossible
					if (rule_fields->treatment_outer.vid <= 4095)
					{
						vid = rule_fields->treatment_outer.vid;
					} else if (rule_fields->treatment_outer.vid == 4096) {
						vid = (rule_fields->filter_inner.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields->filter_inner.vid : 0);
					} else if (rule_fields->treatment_outer.vid == 4097) {
						vid = (rule_fields->filter_outer.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields->filter_outer.vid : 0);
					}
					break;
				case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
					if (rule_fields->treatment_inner.vid <= 4095)
					{
						vid = rule_fields->treatment_inner.vid;
					} else if (rule_fields->treatment_inner.vid == 4096) {
						vid = (rule_fields->filter_inner.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields->filter_inner.vid : 0);
					} else if (rule_fields->treatment_inner.vid == 4097) {
						vid = (rule_fields->filter_outer.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_15 ? rule_fields->filter_outer.vid : 0);
					}
					break;
				default:
					dbprintf(LOG_ERR, "result code error\n");
				}

				//result means priority tag rule, treat as reassign rule
				if (vid == 0) // priority tag
				{
					found = 1;
				}

				//find for 1tag + 1tag rule
				if (!found &&
					omciutil_vlan_get_rule_filter_tag_num(rule_fields) == 2 && 
					rule_fields->treatment_tag_to_remove == 1 &&
					omciutil_vlan_get_rule_treatment_tag_num(rule_fields) == 0) //2tag - 1tag
				{
					found = 1;
				}
				
				break;
			default: // >= two tags, impossible
				; //do nothing
			}					

			if (found)
			{
				//mark or copy to dest
				if (action == 0)
				{ //mark
					rule_fields->veip_flag = 1; //mark for veip bridge rules
				} else { //copy to dest
					rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
					*rule_fields_new = *rule_fields; //copy

					if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, rule_list_ds_dest) < 0)
					{
						free_safe(rule_fields_new);
						dbprintf(LOG_ERR, "reassign ds tagging rule error!!\n");
					}
				}
			}

			omciutil_vlan_release_rule_result(&rule_result_list);
		}
	}

	return 0;
}

static int 
omciutil_vlan_collect_copy_veip_bridge_rules(
	struct me_t *issue_bridge_port_me, //uni port
	struct me_t *issue_me,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds,
	unsigned char tag_mode)
{
	unsigned int found;
	struct meinfo_t *miptr=meinfo_util_miptr(45);
	struct meinfo_t *miptr_bridge_port=meinfo_util_miptr(47);
	struct me_t *bridge_me, *bridge_port_me;
	struct switch_port_info_t port_info;
	struct list_head rule_list_us_temp;
	struct list_head rule_list_ds_temp;

	if (issue_bridge_port_me == NULL ||
		rule_list_us == NULL ||
		rule_list_ds == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	INIT_LIST_HEAD(&rule_list_us_temp);
	INIT_LIST_HEAD(&rule_list_ds_temp);

	//fine the same bridge veip
	found = 0;
	list_for_each_entry(bridge_me, &miptr->me_instance_list, instance_node)
	{
		if (me_related(bridge_me, issue_bridge_port_me)) //belong to it
		{
			found = 1;
			break;
		}
	}

	if (found)
	{
		//collect info for each bridge port
		list_for_each_entry(bridge_port_me, &miptr_bridge_port->me_instance_list, instance_node)
		{
			if (me_related(bridge_me, bridge_port_me) &&
				switch_get_port_info_by_me(bridge_port_me, &port_info) == 0 &&
				port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU)
			{
				//collect veip 171 rules
				omciutil_vlan_collect_rules_by_port_for_171(ER_ATTR_GROUP_HW_OP_NONE,
					bridge_port_me,
					NULL,
					&rule_list_us_temp,
					&rule_list_ds_temp,
					tag_mode,
					NULL);

				//add untag/priority rules to us/ds list
				omciutil_vlan_collect_handle_veip_bridge_rules(1, &rule_list_us_temp, &rule_list_ds_temp, rule_list_us, rule_list_ds);

				//release veip 171 rules
				omciutil_vlan_release_rule_list(&rule_list_us_temp);
				omciutil_vlan_release_rule_list(&rule_list_ds_temp);
			}
		}
	}

	return 0;
}

static void
omciutil_vlan_collect_modify_rules_to_distinguish_iphost_ports(struct list_head *us_tagging_rule_list,
	struct list_head *ds_tagging_rule_list,
	unsigned char port_index,
	unsigned short dis_vlan_id)
{
	struct omciutil_vlan_rule_fields_t *rule_fields_pos;
	int filter_tag_num, treatment_tag_num;

	if (us_tagging_rule_list == NULL ||
		ds_tagging_rule_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	//check upstream rules
	list_for_each_entry(rule_fields_pos, us_tagging_rule_list, rule_node)
	{
		//check for rule must have no filter, then add a port-distinguished tag in filter for different iphost ports
		filter_tag_num = omciutil_vlan_get_rule_filter_tag_num(rule_fields_pos);
		if (filter_tag_num > 0) //not untag rule
		{
			dbprintf(LOG_ERR, "US filter error!! port_type_index=%u, entry_id=%u, tag_num=%u\n",
				port_index,
				rule_fields_pos->entry_id,
				filter_tag_num);
			continue;
		}

		//by dis_vlan_id to add the tag to inner filter's vid	
		rule_fields_pos->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
		rule_fields_pos->filter_inner.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
		rule_fields_pos->filter_inner.vid = dis_vlan_id;

		//remove this tag
		rule_fields_pos->treatment_tag_to_remove = 1;
	}

	//check downstream rules
	list_for_each_entry(rule_fields_pos, ds_tagging_rule_list, rule_node)
	{
		//check for rule result must have no filter, then add a port-distinguished tag in treatment for different cpu ports
		filter_tag_num = omciutil_vlan_get_rule_filter_tag_num(rule_fields_pos);
		treatment_tag_num = omciutil_vlan_get_rule_treatment_tag_num(rule_fields_pos);
		if (filter_tag_num - rule_fields_pos->treatment_tag_to_remove + treatment_tag_num > 0) //not untag rule
		{
			dbprintf(LOG_ERR, "DS result error!! port_type_index=%u, entry_id=%u, result_num=%u\n",
				port_index,
				rule_fields_pos->entry_id,
				filter_tag_num - rule_fields_pos->treatment_tag_to_remove + treatment_tag_num);
			continue;
		}

		//by dis_vlan_id to add the tag to inner treatment
		rule_fields_pos->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_0;
		rule_fields_pos->treatment_inner.tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010;
		rule_fields_pos->treatment_inner.vid = dis_vlan_id;
	}	

	return;
}

//calix proprietary
void
omciutil_vlan_collect_tlan_vid(struct omciutil_vlan_tlan_table_t *tlan_table)
{
	struct meinfo_t *miptr;
	struct me_t *me;
	struct attr_value_t attr_value;

	if (tlan_table == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	memset(tlan_table, 0x0, sizeof(struct omciutil_vlan_tlan_table_t));
	
	//collect
	if ((miptr = meinfo_util_miptr(65313)) == NULL) // 65313 CalixVlan
	{
		dbprintf(LOG_ERR, "get 65313 miptr error\n");
	} else {
		//get all tlan
		list_for_each_entry(me, &miptr->me_instance_list, instance_node)
		{
			//Tlan flag
			meinfo_me_attr_get(me, 4, &attr_value);
			if (attr_value.data == 1 && me->meid < 4096)
			{
				tlan_table->vid[tlan_table->total] = me->meid;
				tlan_table->total++;
			}
		}
	}
	
	return;
}

// calix, collect tlan vid in the same birdge,
// asume only apply on wan filter forward operation is 0x10
static void
omciutil_vlan_collect_tlan_vid_by_bridge(struct me_t *bridge_port_me, struct omciutil_vlan_tlan_table_t *tlan_table_bridge)
{
	unsigned char *bitmap;
	unsigned int i, j;
	unsigned short vlan_tci;
	struct omciutil_vlan_tlan_table_t tlan_table_all;
	struct meinfo_t *miptr_45, *miptr_47, *miptr_84;
	struct me_t *me_pos_45, *me_pos_47, *me_pos_84;
	struct switch_port_info_t port_info;
	struct attr_value_t attr_filter_list={0, NULL};
	struct attr_value_t attr_filter_op={0, NULL};
	struct attr_value_t attr_filter_entry_total={0, NULL};
	
	if (bridge_port_me == NULL ||
		tlan_table_bridge == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	//get all tlan table
	omciutil_vlan_collect_tlan_vid(&tlan_table_all);

	if (tlan_table_all.total == 0)
	{
		return; //skip this collection
	}

	//according the bridge port me to find wan filter in the same bridge, check with tlan vid
	if ((miptr_45 = meinfo_util_miptr(45)) == NULL) // 45 MAC bridge service profile
	{
		dbprintf(LOG_ERR, "get 45 miptr error\n");
		return;
	}

	if ((miptr_47 = meinfo_util_miptr(47)) == NULL) // 47 MAC bridge port configuration data
	{
		dbprintf(LOG_ERR, "get 47 miptr error\n");
		return;
	}

	if ((miptr_84 = meinfo_util_miptr(84)) == NULL) // 84 VLAN tagging filter data
	{
		dbprintf(LOG_ERR, "get 84 miptr error\n");
		return;
	}

	//check bridge
	list_for_each_entry(me_pos_45, &miptr_45->me_instance_list, instance_node)
	{
		if (me_related(me_pos_45, bridge_port_me) == 1)
		{
			//check wan port with the same bridge
			list_for_each_entry(me_pos_47, &miptr_47->me_instance_list, instance_node)
			{
				if ((me_pos_47 != bridge_port_me) &&
					(me_related(me_pos_45, me_pos_47) == 1) &&
					(switch_get_port_info_by_me(me_pos_47, &port_info) == 0) &&
					(port_info.port_type == ENV_BRIDGE_PORT_TYPE_WAN))
				{
					//find vlan filter
					list_for_each_entry(me_pos_84, &miptr_84->me_instance_list, instance_node)
					{
						if (me_related(me_pos_84, me_pos_47) == 1) //wan vlan filter
						{
							//get operation and vlan list
							meinfo_me_attr_get(me_pos_84, 1, &attr_filter_list);
							meinfo_me_attr_get(me_pos_84, 2, &attr_filter_op);
							meinfo_me_attr_get(me_pos_84, 3, &attr_filter_entry_total);

							if ((attr_filter_op.data == 0x10) &&
								(attr_filter_entry_total.data > 0) &&
								(bitmap=(unsigned char *) attr_filter_list.ptr)) //not null
							{
								for (i = 0; i < attr_filter_entry_total.data; i++)
								{
									vlan_tci = util_bitmap_get_value(bitmap, 24*8, i*16, 16);

									//compare with tlan_table_all
									for (j = 0; j < tlan_table_all.total; j++)
									{
										//set to tlan_table
										if ((vlan_tci & 0x0fff) == tlan_table_all.vid[j])
										{
											tlan_table_bridge->vid[tlan_table_bridge->total] = (vlan_tci & 0x0fff);
											tlan_table_bridge->total++;
										}
									}
								}
							}
						}
					}
				}
			}

			//belong to only one bridge
			break;
		}
	}

	return;
}

//tag mode: 0, collect all tagging rules. 1: ignore 2 tags rules.
int 
omciutil_vlan_collect_rules_by_port(int action,
	struct me_t *issue_bridge_port_me,
	struct me_t *issue_me,
	struct list_head *rule_list_us,
	struct list_head *rule_list_ds,
	unsigned char tag_mode)
{
	unsigned char wa= 0;
	unsigned int count;
	unsigned short dis_vlan_id;
	char *dot_ptr;
	struct meinfo_t *miptr_310, *miptr_47, *miptr_11, *miptr_78;
	struct me_t *me_pos_310,  *me_pos_47, *me_pos_11, *me_pos_78;
	struct attr_value_t attr_value1, attr_value2;
	struct me_t *me;
	struct list_head rule_list_ds_temp;
	struct list_head rule_list_ds_temp_1;
	struct switch_port_info_t port_info;
	struct omciutil_vlan_rule_fields_t *rule_fields_pos, *rule_fields_n, *rule_fields_new;
	struct meinfo_instance_def_t *instance_def_ptr = NULL;

	//init two list
	INIT_LIST_HEAD(rule_list_us);
	INIT_LIST_HEAD(rule_list_ds);
	INIT_LIST_HEAD(&rule_list_ds_temp);
	INIT_LIST_HEAD(&rule_list_ds_temp_1);

	//collect 9.3.2
	if ((miptr_310 = meinfo_util_miptr(310)) == NULL) // 310 Extend_multicast_operations_profile
	{
		dbprintf(LOG_ERR, "get 310 miptr error\n");
		return -1;
	}

	list_for_each_entry(me_pos_310, &miptr_310->me_instance_list, instance_node)
	{
		//bridge port configuration data, 47
		if ((me = meinfo_me_get_by_meid(47, me_pos_310->meid)) == NULL)
		{
			dbprintf(LOG_ERR, "get me error, classid=47, meid=%u, for class 310 related\n", me_pos_310->meid);
		} else {
			if (me == issue_bridge_port_me &&
				(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_310))
			{
				//merge all
				omciutil_vlan_collect_310_rules(issue_bridge_port_me, me_pos_310, &rule_list_ds_temp, tag_mode);
			}
		}
	}

	//zte proprietary
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ZTE)==0)
	{
		struct meinfo_t *miptr_65282;
		struct me_t *me_pos_65282;
		//collect 9.99.3
		if ((miptr_65282 = meinfo_util_miptr(65282)) == NULL) // 65282 Extend_multicast_operations_profile
		{
			dbprintf(LOG_ERR, "get 65282 miptr error\n");
			return -1;
		}

		list_for_each_entry(me_pos_65282, &miptr_65282->me_instance_list, instance_node)
		{
			//bridge port configuration data, 47
			if ((me = meinfo_me_get_by_meid(47, me_pos_65282->meid)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=47, meid=%u\n", me_pos_65282->meid);
			} else {
				if (me == issue_bridge_port_me &&
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_65282))
				{
					//merge all
					omciutil_vlan_collect_65282_rules(issue_bridge_port_me, me_pos_65282, rule_list_us, &rule_list_ds_temp, tag_mode);
				}
			}
		}
	}

	//huawei proprietary
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_HUAWEI)==0)
	{
		struct meinfo_t *miptr_367=meinfo_util_miptr(367);
		if (miptr_367 && !list_empty(&miptr_367->me_instance_list)) {
			struct me_t *me_pos_367 = meinfo_me_get_by_meid(367, 0);
			//collect 9.99.5
			if (me_pos_367) {
				if(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_367)
					omciutil_vlan_collect_367_rules(issue_bridge_port_me, me_pos_367, rule_list_us, &rule_list_ds_temp, tag_mode);
			}
		}
	}

	//calix proprietary
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0)
	{
		struct omciutil_vlan_tlan_table_t tlan_table_bridge; //collect tlan vid in the same bridge
	
		//reset tlan table
		memset(&tlan_table_bridge, 0x00, sizeof(struct omciutil_vlan_tlan_table_t));
		
		//get tlan_table of the same bridge
		omciutil_vlan_collect_tlan_vid_by_bridge(issue_bridge_port_me, &tlan_table_bridge);

		//by port to analyze tlan supported functions, in 171/249 rules collection:
		//1. transparent (add two tags pass rule)
		//2. any tags add one tag, untag + 1 tag condition
		//3. any tags add one tag, 1 tag + 1 tag condition
		//4. change tag
		
		//collect 9.3.13 
		if (omciutil_vlan_collect_rules_by_port_for_171(action,
			issue_bridge_port_me,
			issue_me,
			rule_list_us,
			&rule_list_ds_temp,
			tag_mode,
			&tlan_table_bridge) < 0) // input
		{
			return -1;
		}		

		//collect 9.99.6 
		if (omciutil_vlan_collect_rules_by_port_for_249(action,
			issue_bridge_port_me,
			issue_me,
			rule_list_us,
			&rule_list_ds_temp,
			tag_mode,
			&tlan_table_bridge) < 0) // input
		{
			return -1;
		}

		//according workaround flag in rules, to generate 65313 tlan rules
		//collect 9.99.16
		if (omciutil_vlan_collect_rules_by_port_for_65313(issue_bridge_port_me,
			rule_list_us,
			&rule_list_ds_temp) < 0)
		{
			return -1;
		}
	} else { //normal
		//collect 9.3.13 
		if (omciutil_vlan_collect_rules_by_port_for_171(action,
			issue_bridge_port_me,
			issue_me,
			rule_list_us,
			&rule_list_ds_temp,
			tag_mode,
			NULL) < 0)
		{
			return -1;
		}
	}
	
	//collect 9.3.12
	if ((miptr_78 = meinfo_util_miptr(78)) == NULL) // 78 VLAN_tagging_operation_configuration_data
	{
		dbprintf(LOG_ERR, "get 78 miptr error\n");
		return -1;
	}
	
	list_for_each_entry(me_pos_78, &miptr_78->me_instance_list, instance_node)
	{
		//check association type
		meinfo_me_attr_get(me_pos_78, 4, &attr_value1);
		//get association pointer meid
		meinfo_me_attr_get(me_pos_78, 5, &attr_value2);
		switch (attr_value1.data)
		{
		case 0:
			//implicit to pptp ethernet uni, 11
			if ((miptr_11 = meinfo_util_miptr(11)) == NULL)
			{
				dbprintf(LOG_ERR, "get 11 miptr error\n");
				continue;
			} else {
				list_for_each_entry(me_pos_11, &miptr_11->me_instance_list, instance_node)
				{
					if (me_pos_11->meid == me_pos_78->meid) //meid the same pointer to pptp ethernet uni
					{
						if (me_related(issue_bridge_port_me, me_pos_11) == 1 && //47->11
							(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_78))
						{
								//merge all
								omciutil_vlan_collect_78_rules(issue_bridge_port_me, me_pos_78, rule_list_us, &rule_list_ds_temp, tag_mode);
						}
					}
				}
			}
			break;
		case 1:
			//IP host config data, 134
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(134, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=134, meid=%u, for class 78 related\n", attr_value2);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->134
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_78))
				{
						//merge all
						omciutil_vlan_collect_78_rules(issue_bridge_port_me, me_pos_78, rule_list_us, &rule_list_ds_temp, tag_mode);
				}
			}
			break;
		case 2:
			//802.1p mapper service profile, 130
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(130, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=130, meid=%u, for class 78 related\n", attr_value2);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->130
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_78))
				{
						//merge all
						omciutil_vlan_collect_78_rules(issue_bridge_port_me, me_pos_78, rule_list_us, &rule_list_ds_temp, tag_mode);
				}
			}
			break;
		case 3:
			//bridge port configuration data, 47
			if ((me = meinfo_me_get_by_meid(47, attr_value2.data)) == NULL)
			{
				//??????????
				if ((miptr_47 = meinfo_util_miptr(47)) == NULL)
				{
					dbprintf(LOG_ERR, "get 47 miptr error\n");
					continue;
				} else {
					list_for_each_entry(me_pos_47, &miptr_47->me_instance_list, instance_node)
					{
						if (me_related(me_pos_78, me_pos_47) == 1) //78->47
						{
							if (me_pos_47 == issue_bridge_port_me &&
								(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_78))
							{
								//merge all
								omciutil_vlan_collect_78_rules(issue_bridge_port_me, me_pos_78, rule_list_us, &rule_list_ds_temp, tag_mode);
							}
						}
					}
				}
			} else {
				if (me == issue_bridge_port_me &&
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_78))
				{
					//merge all
					omciutil_vlan_collect_78_rules(issue_bridge_port_me, me_pos_78, rule_list_us, &rule_list_ds_temp, tag_mode);
				}
			}
			break;
		case 10:
			//pptp ethernet uni, 11
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(11, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=11, meid=%u, for class 78 related\n", attr_value2);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->11
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_78))
				{
						//merge all
						omciutil_vlan_collect_78_rules(issue_bridge_port_me, me_pos_78, rule_list_us, &rule_list_ds_temp, tag_mode);
				}
			}
			break;
		case 11:
			//VEIP, 329
			//check all bridge port configuration data mes
			if ((me = meinfo_me_get_by_meid(329, attr_value2.data)) == NULL)
			{
				dbprintf(LOG_ERR, "get me error, classid=329, meid=%u, for class 78 related\n", attr_value2);
			} else {
				if (me_related(issue_bridge_port_me, me) == 1 && //47->329
					(action != ER_ATTR_GROUP_HW_OP_DEL || issue_me != me_pos_78))
				{
						//merge all
						omciutil_vlan_collect_78_rules(issue_bridge_port_me, me_pos_78, rule_list_us, &rule_list_ds_temp, tag_mode);
				}
			}
			break;
		//others not supported
		default:
			dbprintf(LOG_ERR, "not supported classid=78, meid=%u association type=%u\n", me_pos_78->meid, attr_value1.data);
		}
	}

	//customization
	//collect CTC vlan translation
	omciutil_vlan_collect_ctc_rules(issue_bridge_port_me, rule_list_us, &rule_list_ds_temp);

	//cb to proprietary vlan rules collect
	if (omciutil_vlan_cb_g.omciutil_vlan_collect_rules_by_port != NULL &&
		omciutil_vlan_cb_g.omciutil_vlan_collect_rules_by_port(action,
			issue_bridge_port_me,
			issue_me,
			rule_list_us,
			&rule_list_ds_temp,
			tag_mode) < 0)
	{
		dbprintf(LOG_ERR, "proprietary vlan collect error, classid=%u, meid=%u\n", issue_me->classid, issue_me->meid);
		return -1;
	}

	//get port information
	if (switch_get_port_info_by_me(issue_bridge_port_me, &port_info) == 0)
	{
		//reassign veip(cpu) port tagging rules to uni ports, for all(sw/hw) part.
		if (omci_env_g.classf_veip_rules_reassign == 3 || //3: all MARK and COPY
			omci_env_g.classf_veip_rules_reassign == 4) //4: all only MARK
		{
			//check port type
			if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU)
			{
				//veip, remove 171 untag/priority tag rules
				omciutil_vlan_collect_handle_veip_bridge_rules(0, rule_list_us, &rule_list_ds_temp, NULL, NULL);
			} else if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI) {
				if (omci_env_g.classf_veip_rules_reassign == 3) //copy to uni ports
				{
					//uni port, add the same bridge veip port untag/priority tag rules
					omciutil_vlan_collect_copy_veip_bridge_rules(issue_bridge_port_me, issue_me, rule_list_us, &rule_list_ds_temp, tag_mode);
				} //otherwise(4), do nothing
			} // otherwise, do nothing
		}

		//check port type for iphost to modify untag rules
		//check: 1. tag_iphost_vid_map, 2. iphost me devname
		if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_IPHOST)
		{
			dis_vlan_id = 4096; //none
			
			//check: 1. iphost me devname
			instance_def_ptr = meinfo_instance_find_def(134, port_info.port_type_index);
			if (instance_def_ptr &&
				strlen(instance_def_ptr->devname)>0 &&
				(dot_ptr = strchr(instance_def_ptr->devname, '.')) != NULL)
			{
				dis_vlan_id = util_atoi(dot_ptr + 1);
			}

			//check: 2. tag_iphost_vid_map
			if (dis_vlan_id == 4096 && //not found ever
				omci_env_g.tag_iphost_vid_map[port_info.port_type_index] < 4096)
			{
				dis_vlan_id = (omci_env_g.tag_iphost_vid_map[port_info.port_type_index] & 0xfff);
			}

			if (dis_vlan_id < 4096) //modify rules
			{
				omciutil_vlan_collect_modify_rules_to_distinguish_iphost_ports(rule_list_us, &rule_list_ds_temp, port_info.port_type_index, dis_vlan_id);
			}
		}

		//DS workaround
		//pbit don't care workaround for ds
		if (!list_empty(&rule_list_ds_temp) &&
			(omci_env_g.tag_pbit_workaround != 0) &&
			((1 << port_info.logical_port_id) & omci_env_g.tag_pbit_portmask))
		{
			//case
			switch (omci_env_g.tag_pbit_workaround)
			{
				case 1:
				case 2:
					list_for_each_entry(rule_fields_pos, &rule_list_ds_temp, rule_node)
					{
						if (omci_env_g.tag_pbit_workaround == 1)
						{
							//for pbit don't care
							omciutil_vlan_pbit_dont_care_workaround(rule_fields_pos);
						} else {
							//for remap pbit
							omciutil_vlan_pbit_remap_workaround(rule_fields_pos);
						}
					}
					wa = 1;
					
					break;
				case 3: //add one more don't care rule
				case 4: //add one more transparent rule
					list_replace_init(&rule_list_ds_temp, &rule_list_ds_temp_1);
					list_for_each_entry_safe(rule_fields_pos, rule_fields_n, &rule_list_ds_temp_1, rule_node)
					{
						list_del(&rule_fields_pos->rule_node);
						
						rule_fields_new = malloc_safe(sizeof(struct omciutil_vlan_rule_fields_t));
						*rule_fields_new = *rule_fields_pos; //copy
						
						if (omci_env_g.tag_pbit_workaround == 3)
						{
							//modify for pbit don't care
							omciutil_vlan_pbit_dont_care_workaround(rule_fields_new);
						} else { 
							//modify for pbit transparent
							omciutil_vlan_pbit_dont_care_workaround_transparent(rule_fields_new);
						}

						//add original one to ds_temp
						if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_pos, &rule_list_ds_temp) < 0)
						{
							free_safe(rule_fields_pos);
							dbprintf(LOG_WARNING, "pbit workaround insert to list error, bridge port meid=%u!!\n", issue_bridge_port_me->meid);
						}
						//add new one to ds_temp
						if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_new, &rule_list_ds_temp) < 0)
						{
							free_safe(rule_fields_new);
							dbprintf(LOG_WARNING, "pbit workaround insert to list error, bridge port meid=%u!!\n", issue_bridge_port_me->meid);
						}
					}
					
					break;
				default:
					;
			}
		}

		if (wa == 1) // have workaround
		{
			//re-sort to rule_list_ds
			list_for_each_entry_safe(rule_fields_pos, rule_fields_n, &rule_list_ds_temp, rule_node)
			{
				list_del(&rule_fields_pos->rule_node);
				if (omciutil_vlan_insert_rule_fields_to_list(rule_fields_pos, rule_list_ds) < 0)
				{
					free_safe(rule_fields_pos);
					dbprintf(LOG_WARNING, "resort rule_list_ds error, bridge port meid=%u!!\n", issue_bridge_port_me->meid);
				}
			}
		} else {
			//replace to rule_list_ds
			list_replace(&rule_list_ds_temp, rule_list_ds);
		}
	} else {
		dbprintf(LOG_ERR, "can not get port info, meid=0x%.4x\n", issue_bridge_port_me->meid);
	}

	//assign seq num to each rule_field for us/ds list
	count = 0;
	list_for_each_entry(rule_fields_pos, rule_list_us, rule_node)
	{
		rule_fields_pos->entry_id = count++;
	}
	count = 0;
	list_for_each_entry(rule_fields_pos, rule_list_ds, rule_node)
	{
		rule_fields_pos->entry_id = count++;
	}

	return 0;
}

