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
 * File    : omciutil_vlan.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "util.h"
#include "meinfo.h"
#include "hwresource.h"
#include "er_group.h"
#include "omciutil_vlan.h"
#include "me_related.h"
#include "batchtab.h"
#include "batchtab_cb.h"

struct omciutil_vlan_cb_t omciutil_vlan_cb_g;

int
omciutil_vlan_release_rule_list(struct list_head *rule_list)
{
	struct omciutil_vlan_rule_fields_t *rule_fields_pos, *rule_fields_n;
	list_for_each_entry_safe(rule_fields_pos, rule_fields_n, rule_list, rule_node) {
		list_del(&rule_fields_pos->rule_node);		
		free_safe(rule_fields_pos);
	}
	return 0;
}

int
omciutil_vlan_get_rule_filter_tag_num(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	int rule_tag_num=-1;

	if (rule_fields==NULL)
		return -1;		

	// match 
	if (rule_fields->filter_outer.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_15) {
		if (rule_fields->filter_inner.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_15) {	// untag, default
			rule_tag_num=0;
		} else if (rule_fields->filter_inner.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_14) {	// 1tag, default
			rule_tag_num=1;
		} else if (rule_fields->filter_inner.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_13 || //dscp-2-pbit reversing temp rule
			rule_fields->filter_inner.priority<=OMCIUTIL_VLAN_FILTER_PRIORITY_8) { // 1tag
			rule_tag_num=1;
		}
	} else if (rule_fields->filter_outer.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_14) {
		if (rule_fields->filter_inner.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_14 || 	// 2tag, default
			rule_fields->filter_inner.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_13 || //dscp-2-pbit reversing temp rule
			rule_fields->filter_inner.priority<=OMCIUTIL_VLAN_FILTER_PRIORITY_8)
			rule_tag_num=2;
	} else if (rule_fields->filter_outer.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_13) {
		if (rule_fields->filter_inner.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_14 ||
			rule_fields->filter_inner.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_13 || //dscp-2-pbit reversing temp rule
			rule_fields->filter_inner.priority<=OMCIUTIL_VLAN_FILTER_PRIORITY_8)	// 2tag, default
			rule_tag_num=2;
	} else if (rule_fields->filter_outer.priority<=OMCIUTIL_VLAN_FILTER_PRIORITY_8) {
		if (rule_fields->filter_inner.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_14 ||
			rule_fields->filter_inner.priority==OMCIUTIL_VLAN_FILTER_PRIORITY_13 || //dscp-2-pbit reversing temp rule
			rule_fields->filter_inner.priority<=OMCIUTIL_VLAN_FILTER_PRIORITY_8)	// 2tag
			rule_tag_num=2;
	}

	return rule_tag_num;
}

int
omciutil_vlan_get_rule_treatment_tag_num(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	int rule_tag_num=-1;

	if (rule_fields==NULL)
		return -1;		

	if (rule_fields->treatment_inner.priority==OMCIUTIL_VLAN_TREATMENT_PRIORITY_15) {
		rule_tag_num = 0;
	} else {
		if (rule_fields->treatment_outer.priority==OMCIUTIL_VLAN_TREATMENT_PRIORITY_15) {
			rule_tag_num = 1;
		} else {
			rule_tag_num = 2;
		}
	}

	return rule_tag_num;

}

int
omciutil_vlan_analyse_dscp2pbit_bitmap(char *dscp2pbit_bitmap, unsigned char *dscp2pbit_table, unsigned char *pbit_mask)
{
	unsigned int i;
	unsigned char pbit;

	if (dscp2pbit_bitmap == NULL || 
		dscp2pbit_table == NULL ||
		pbit_mask == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	*pbit_mask = 0;
	for (i = 0; i < 64; i++) //64 dscp value
	{
		pbit = util_bitmap_get_value((unsigned char *)dscp2pbit_bitmap, 8*24, i*3, 3);
		dscp2pbit_table[i] = pbit;
		*pbit_mask |= (0x1 << pbit);
	}

	return 0;
}

int
omciutil_vlan_get_171_dscp2pbit_info(struct me_t *me, struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info, unsigned char force_flag)
{
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	struct omciutil_vlan_rule_fields_t rule_fields;
	struct attr_value_t attr_dscp2pbit_bitmap={0, NULL};

	if (me == NULL || me->classid != 171 ||
		dscp2pbit_info == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//get tagging table
	if ((table_header = (struct attr_table_header_t *) me->attr[6].value.ptr) == NULL)
	{
		dbprintf(LOG_ERR, "attr value was not table\n");
		return -1;
	}

	//check dscp2pbit enable
	list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
		{
			memset(&rule_fields, 0x00, sizeof(rule_fields));

			//fill to entry fields
			omciutil_vlan_collect_rule_fields_from_table_entry((unsigned char *)table_entry_pos->entry_data_ptr, 0, 0, &rule_fields);

			//calix, overwrite pbit treatment to dscp-2-pbit in 1-filter, 1-removement, 1-treatment rules. 
			if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0 &&
				//port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU && //determining by wan vid
				omciutil_vlan_get_rule_filter_tag_num(&rule_fields) == 1 &&
				rule_fields.treatment_tag_to_remove == 1 &&
				omciutil_vlan_get_rule_treatment_tag_num(&rule_fields) == 1 &&
				rule_fields.treatment_inner.priority != OMCIUTIL_VLAN_TREATMENT_PRIORITY_10) // calix
			{
				int i;
				struct batchtab_cb_wand2p_t *t = batchtab_table_data_get("wand2p");

				if (t != NULL)
				{
					fwk_mutex_lock(&t->mutex);
					
					for (i = 0; i < WANIF_INDEX_TOTAL; i++)
					{
						if (t->wand2p[i].enable == 1 &&
							t->wand2p[i].vid == rule_fields.filter_inner.vid &&
							t->wand2p[i].dscp2pbit_enable == 1)
						{
							//set to dscp2pbit enable if it is not dscp2pbit
							if (rule_fields.treatment_inner.priority != OMCIUTIL_VLAN_TREATMENT_PRIORITY_10)
							{
								dscp2pbit_info->enable = 1; //dscp-2-pbit
							}
						}
					}
					
					fwk_mutex_unlock(&t->mutex);
					batchtab_table_data_put("wand2p");
				}
			} else {	
				if (rule_fields.treatment_outer.priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_10 ||
					rule_fields.treatment_inner.priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_10)
				{
					dscp2pbit_info->enable = 1; //dscp-2-pbit
					break;
				}
			}
		}
	}

	//force to get dscp-2-pbit table
	if (force_flag)
	{
		dscp2pbit_info->enable = 1;
	}

	//get dscp2pbit table;
	if (dscp2pbit_info->enable == 1) //enable
	{
		meinfo_me_attr_get(me, 8, &attr_dscp2pbit_bitmap); //ptr
		omciutil_vlan_analyse_dscp2pbit_bitmap(attr_dscp2pbit_bitmap.ptr, dscp2pbit_info->table, &dscp2pbit_info->pbit_mask);
		meinfo_me_attr_clear_value(130, 11, &attr_dscp2pbit_bitmap);
		dscp2pbit_info->from_me = me;
	}

	return 0;
}

int omciutil_vlan_cb_register(struct omciutil_vlan_cb_t *omciutil_vlan_cb)
{
	if (omciutil_vlan_cb == NULL)
	{
		return -1;
	}

	omciutil_vlan_cb_g = *omciutil_vlan_cb;

	return 0;
}

int omciutil_vlan_cb_unregister()
{
	memset(&omciutil_vlan_cb_g, 0x00, sizeof(struct omciutil_vlan_cb_t));

	return 0;
}
