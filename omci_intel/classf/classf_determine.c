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
 * Module  : classf
 * File    : classf_determine.c
 *
 ******************************************************************/

#include <string.h>

#include "classf.h"
#include "util.h"
#include "meinfo.h"

struct classf_bridge_info_wan_port_t dummy_wan_port_g = {0};
unsigned int rule_count_g;

static struct classf_rule_t *
classf_alloc_rule(void)
{
	struct classf_rule_t *rule;

	rule = malloc_safe(sizeof(struct classf_rule_t));
	rule->hw_stream_id = -1;
	rule->hw_acl_sub_order = -1;
	rule->hw_stat_id = -1;
	INIT_LIST_HEAD(&rule->acl_entry_list);
	INIT_LIST_HEAD(&rule->classify_entry_list);

	return rule;
}

int
classf_release_rule(struct classf_rule_t *rule)
{
	struct classf_entry_node_t *entry_node, *entry_node_n;

	if (rule == NULL)
	{
		return -1;
	}

	list_for_each_entry_safe(entry_node, entry_node_n, &rule->acl_entry_list, e_node)
	{
		list_del(&entry_node->e_node);
		free_safe(entry_node);
	}

	list_for_each_entry_safe(entry_node, entry_node_n, &rule->classify_entry_list, e_node)
	{
		list_del(&entry_node->e_node);
		free_safe(entry_node);
	}

	return 0;
}

//return value: -1: rule_fields1 < rule_fields2, 0: rule_fields1 == rule_fields2, 1: rule_fields1 > rule_fields2
static int
classf_compare_rule(struct classf_rule_t *rule1,
	struct classf_rule_t *rule2)
{
	unsigned int ret;
	struct classf_pvlan_acl_care_bit_t care_bit = {0};

	//check lan port
	if (rule1->lan_port->port_type_sort != rule2->lan_port->port_type_sort)
	{
		if (rule1->lan_port->port_type_sort < rule2->lan_port->port_type_sort)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule1->lan_port->port_type_index_sort != rule2->lan_port->port_type_index_sort)
	{
		if (rule1->lan_port->port_type_index_sort < rule2->lan_port->port_type_index_sort)
		{
			return -1;
		} else {
			return 1;
		}
	}

	if (rule1->wan_port->port_filter.low_priority != rule2->wan_port->port_filter.low_priority)
	{
		if (rule1->wan_port->port_filter.low_priority < rule2->wan_port->port_filter.low_priority)
		{
			return -1;
		} else {
			return 1;
		}
	}

	//compare pvlan acl, according the order value
	if (rule1->pvlan_acl != rule2->pvlan_acl)
	{
		if (rule1->pvlan_acl == NULL)
		{
			return 1;
		}
		if (rule2->pvlan_acl == NULL)
		{
			return -1;
		}
	}

	//all pvlan_acl not NULL
	if (rule1->pvlan_acl != NULL &&
		rule2->pvlan_acl != NULL)
	{	
		if (memcmp(&rule1->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) == 0 &&
			memcmp(&rule2->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) != 0)
		{
			return 1;
		} else if (memcmp(&rule1->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) != 0 &&
			memcmp(&rule2->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) == 0) {
			return -1;
		} else if (memcmp(&rule1->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) != 0 &&
			memcmp(&rule2->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) != 0) {
			if (rule1->pvlan_acl->order != rule2->pvlan_acl->order)
			{
				if (rule1->pvlan_acl->order < rule2->pvlan_acl->order)
				{
					return -1;
				} else {
					return 1;
				}
			} else {
				return 1; //according original list order
			}
		} else if (memcmp(&rule1->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) == 0 &&
			memcmp(&rule2->pvlan_acl->care_bit, &care_bit, sizeof(struct classf_pvlan_acl_care_bit_t)) == 0) {
			if (rule1->pvlan_acl->order != rule2->pvlan_acl->order)
			{
				if (rule1->pvlan_acl->order < rule2->pvlan_acl->order)
				{
					return -1;
				} else {
					return 1;
				}
			} else {
				return 1; //according original list order
			}
		}
	}

	//check rule,
	if ((ret = omciutil_vlan_compare_rule_fields(&rule1->rule_fields, &rule2->rule_fields)) != 0)
	{
		return ret;
	}

	//check wan port, and filter null
	if (rule1->wan_port->port_type_index != rule2->wan_port->port_type_index)
	{
		if (rule1->wan_port->port_filter.filter_me != NULL &&
			rule2->wan_port->port_filter.filter_me == NULL)
		{
			return -1;
		}	
		if (rule1->wan_port->port_filter.filter_me == NULL &&
			rule2->wan_port->port_filter.filter_me != NULL)
		{
			return 1;
		}
	
		if (rule1->wan_port->port_type_index < rule2->wan_port->port_type_index)
		{
			return -1;
		} else {
			return 1;
		}
	}

	//check gem port, by index??
	if (rule1->gem_index != rule2->gem_index)
	{
		if (rule1->gem_index < rule2->gem_index)
		{
			return -1;
		} else {
			return 1;
		}
	}

	return 0;
}

//insert rule to list by increasing order bridge lan port, then rule field, reject the same one
static int
classf_insert_rule_to_list(struct classf_rule_t *rule,
	struct list_head *rule_list)
{
	int ret;
	struct classf_rule_t *rule_pos;
	struct list_head *insert_point;
	
	insert_point = rule_list;

	list_for_each_entry(rule_pos, rule_list, r_node)
	{
		if ((ret = classf_compare_rule(rule, rule_pos)) == 0)
		{
			//the same, no need to insert
			return -1;
		} else {
			if (ret < 0) {
				break;
			}
			//otherwise, check next one
		}
		insert_point = &rule_pos->r_node;
	}

	//insert;
	list_add(&rule->r_node, insert_point);

	return 0;
}

//insert rule to list by increasing order bridge lan port, then rule field, reject the same one
int
classf_insert_rule_to_list_for_cpu(struct classf_rule_t *rule,
	struct list_head *rule_list)
{
	int ret;
	struct classf_rule_t *rule_pos;
	struct list_head *insert_point;
	
	insert_point = rule_list;

	list_for_each_entry(rule_pos, rule_list, r_node_cpu)
	{
		if ((ret = classf_compare_rule(rule, rule_pos)) == 0)
		{
			//the same, no need to insert
			return -1;
		} else {
			if (ret < 0) {
				break;
			}
			//otherwise, check next one
		}
		insert_point = &rule_pos->r_node_cpu;
	}

	//insert;
	list_add(&rule->r_node_cpu, insert_point);

	return 0;
}

static int
classf_rule_filter_convert2_mask(struct omciutil_vlan_tag_fields_t *filter, unsigned short *tag_value, unsigned short *tag_mask)
{
	unsigned short value;

	if (filter == NULL || tag_value == NULL || tag_mask == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	*tag_value = *tag_mask = 0;

	//pbit
	if (filter->priority < OMCIUTIL_VLAN_FILTER_PRIORITY_8)
	{
		value = filter->priority & 0x7;
		*tag_value |= (value << 13);
		*tag_mask |= 0xe000;
	}

	//vid
	if (filter->vid <= 4095)
	{
		value = filter->vid & 0xfff;
		*tag_value |= value;
		*tag_mask |= 0x0fff;
	}

	//de bit
	switch(filter->tpid_de)
	{
	//de=0
	case OMCIUTIL_VLAN_FILTER_TPID_DE_110:
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1001:
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1011:
	case OMCIUTIL_VLAN_FILTER_TPID_DE_10000:
		value = (0 & 0x1);
		*tag_value |= (value << 12);
		*tag_mask |= 0x1000;		
		break;
	//de=1
	case OMCIUTIL_VLAN_FILTER_TPID_DE_111:
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1010:
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1100:
	case OMCIUTIL_VLAN_FILTER_TPID_DE_10001:
		value = (1 & 0x1);
		*tag_value |= (value << 12);
		*tag_mask |= 0x1000;		
		break;
	default:
		; //do nothing
	}

	return 0;
}

static int
classf_value_override_rule_filter(struct omciutil_vlan_tag_fields_t *filter, unsigned short tag_value, unsigned short tag_mask)
{
	if (filter == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//check pbit
	if (tag_mask & 0xe000)
	{
		if (filter->priority < OMCIUTIL_VLAN_FILTER_PRIORITY_8)
		{
			if (filter->priority != (tag_value & 0xe000) >> 13)
			{
				//show error message
				dbprintf(LOG_ERR, "restore rule filter pbit error, filter pbit=%u!!\n", filter->priority);
				return -1;
			}
		} else if (filter->priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8 ||
			filter->priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14) {
			filter->priority = (tag_value & 0xe000) >> 13;
		} else {
			//show error message
			dbprintf(LOG_ERR, "restore rule filter pbit error, filter pbit=%u!!\n", filter->priority);
			return -1;
		}
	}

	if (tag_mask & 0x0fff)
	{
		if (filter->vid <= 4095)
		{
			if (filter->vid != (tag_value & 0x0fff))
			{
				//show error message
				dbprintf(LOG_ERR, "restore rule filter vid error, filter vid=%u!!\n", filter->vid);
				return -1;
			}
		} else if (filter->vid == 4096) {
			filter->vid = (tag_value & 0x0fff);
		} else {
			//show error message
			dbprintf(LOG_ERR, "restore rule filter vid error, filter vid=%u!!\n", filter->vid);
			return -1;
		}
	}

	if (tag_mask & 0x1000)
	{
		switch (filter->tpid_de)
		{
		case OMCIUTIL_VLAN_FILTER_TPID_DE_000: //de bit don't care 
			if (tag_value & 0x1000)
			{
				filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1100;
			} else {
				filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1011;			
			}
			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_100: //de bit don't care
			if (tag_value & 0x1000)
			{
				filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_10001;
			} else {
				filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_10000;			
			}
		case OMCIUTIL_VLAN_FILTER_TPID_DE_101: //de bit don't care
			if (tag_value & 0x1000)
			{
				filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_111;
			} else {
				filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_110;			
			}
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1000: //de bit don't care
			if (tag_value & 0x1000)
			{
				filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1100;
			} else {
				filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1011;			
			}
			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_110: //de = 0
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1001: //de = 0 
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1011: //de = 0
		case OMCIUTIL_VLAN_FILTER_TPID_DE_10000: //de = 0 
			if ((tag_value & 0x1000) != 0)
			{
				//show error message
				dbprintf(LOG_ERR, "restore rule filter de error, filter de=%u!!\n", filter->tpid_de);
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_111: //de = 1
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1010: //de = 1
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1100: //de = 1
		case OMCIUTIL_VLAN_FILTER_TPID_DE_10001: //de = 1
			if ((tag_value & 0x1000) == 0)
			{
				//show error message
				dbprintf(LOG_ERR, "restore rule filter de error, filter de=%u!!\n", filter->tpid_de);
				return -1;
			}
			break;
		default:
			//show error message
			dbprintf(LOG_ERR, "restore rule filter de error, filter de=%u!!\n", filter->tpid_de);
			return -1;
		}
	}

	return 0;
}

static int
classf_filter_convert2_mask(unsigned char inspection_type, unsigned short *tag_mask)
{
	if (tag_mask == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	switch(inspection_type)
	{
	case CLASSF_FILTER_INSPECTION_TYPE_VID:
		*tag_mask = 0x0fff;
		break;
	case CLASSF_FILTER_INSPECTION_TYPE_PRIORITY:
		*tag_mask = 0xe000;
		break;
	case CLASSF_FILTER_INSPECTION_TYPE_TCI:
		*tag_mask = 0xffff;
		break;
	default:
		*tag_mask = 0;
	}

	return 0;
}

static int
classf_rule_modify_rule_fields_treatment_pbit(unsigned char priority, struct omciutil_vlan_rule_fields_t *rule_fields, unsigned char inner) //0:outer, 1inner
{
	struct omciutil_vlan_tag_fields_t *treatment;

	if (rule_fields == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	if (inner)
	{
		treatment = &rule_fields->treatment_inner;
	} else {
		treatment = &rule_fields->treatment_outer;
	}

	if (priority < OMCIUTIL_VLAN_FILTER_PRIORITY_8)
	{
		if (treatment->priority < OMCIUTIL_VLAN_TREATMENT_PRIORITY_8)
		{
			//should equal with filter->priority
		} else if (treatment->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_8) { //copy from inner
			if (rule_fields->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8 ||
				rule_fields->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14)  //do not care
			{
				rule_fields->filter_inner.priority = priority;
			}
		} else if (treatment->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_9) { //copy from outer
			if (rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8 ||
				rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14)  //do not care
			{
				rule_fields->filter_outer.priority = priority;
			}
		} else if (treatment->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_10) { //from dscp
			//modify this option to the priority, indicate hw to add a criteria like internal priority.
			treatment->priority = priority;
		}
	}

	// else {
		//priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8 //don't care
		// even for dscp-2-pbit mean all pbit possible, keep the dscp2bit option, complete the setting in hw.
	//}

	return 0;
}

static int
classf_rule_modify_rule_fields_treatment_vid(unsigned short vid, struct omciutil_vlan_rule_fields_t *rule_fields, unsigned char inner) //0:outer, 1inner
{
	struct omciutil_vlan_tag_fields_t *treatment;

	if (rule_fields == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	if (inner)
	{
		treatment = &rule_fields->treatment_inner;
	} else {
		treatment = &rule_fields->treatment_outer;
	}

	if (vid <= 4095) 
	{
		if (treatment->vid <= 4095)
		{
			//should equal with filter->vid
		} else if (treatment->vid == 4096) { //copy from inner
			if (rule_fields->filter_inner.vid == 4096)  //do not care
			{
				rule_fields->filter_inner.vid = vid;
			}
		} else if (treatment->vid == 4097) { //copy from outer
			if (rule_fields->filter_outer.vid == 4096)  //do not care
			{
				rule_fields->filter_outer.vid = vid;
			}
		}
	}

	return 0;
}

static int
classf_rule_modify_rule_fields_filter_de(struct omciutil_vlan_tag_fields_t *filter, unsigned char de)
{
	if (filter == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	switch (filter->tpid_de)
	{
	case OMCIUTIL_VLAN_FILTER_TPID_DE_000:
		if (de)
		{
			filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1100; //tpid = don't care, de = 1
		} else {
			filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1011; //tpid = don't care, de = 0
		}
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_100:
		if (de)
		{
			filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_10001; //tpid = 8100, de = 1
		} else {
			filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_10000; //tpid = 8100, de = 0
		}
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_101:
		if (de)
		{
			filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_111; //tpid = input tpid, de = 1
		} else {
			filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_110; //tpid = input tpid, de = 0
		}
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_110:
		if (de)
		{
			dbprintf(LOG_ERR, "set tpid de error for filter inner\n");
			return -1;
		} else {
			//do nothing
		}
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_111:
		if (de)
		{
			//do nothing
		} else {
			dbprintf(LOG_ERR, "set tpid de error for filter inner\n");
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "set tpid de error for filter inner\n");
		return -1;
	}

	return 0;
}

static int
classf_rule_modify_rule_fields_treatment_de(unsigned char tpid_de, struct omciutil_vlan_rule_fields_t *rule_fields, unsigned char inner) //0:outer, 1inner
{
	struct omciutil_vlan_tag_fields_t *treatment;

	if (rule_fields == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	if (inner)
	{
		treatment = &rule_fields->treatment_inner;
	} else {
		treatment = &rule_fields->treatment_outer;
	}

	switch(tpid_de)
	{
	case OMCIUTIL_VLAN_FILTER_TPID_DE_000: //de bit don't care 
	case OMCIUTIL_VLAN_FILTER_TPID_DE_100: //de bit don't care
	case OMCIUTIL_VLAN_FILTER_TPID_DE_101: //de bit don't care
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1000: //de bit don't care
		//do notthing
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_110: //de = 0
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1001: //de = 0 
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1011: //de = 0
	case OMCIUTIL_VLAN_FILTER_TPID_DE_10000: //de = 0 
		switch (treatment->tpid_de)
		{
		//de bit don't care
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_000:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_inner, 0) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_001:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_outer, 0) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_inner, 0) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_011:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_outer, 0) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_100:
			treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010; //tpid = 8100, de = 0
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1100:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_inner, 0) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1101:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_outer, 0) < 0)
			{
				return -1;
			}
			break;
		//de = 0
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_110:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1000:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10000:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10010:
			//do notthing
			break;
		//de = 1
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_101: //reserved
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_111:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1001:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10001:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10011:
		default:
			//imposible
			dbprintf(LOG_ERR, "set tpid de error for outer\n");
			return -1;
		}
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_111: //de = 1
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1010: //de = 1
	case OMCIUTIL_VLAN_FILTER_TPID_DE_1100: //de = 1
	case OMCIUTIL_VLAN_FILTER_TPID_DE_10001: //de = 1
		switch (treatment->tpid_de)
		{
		//de bit don't care
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_000:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_inner, 1) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_001:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_outer, 1) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_inner, 1) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_011:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_outer, 1) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_100:
			treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1001; //tpid = 8100, de = 1
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1100:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_inner, 1) < 0)
			{
				return -1;
			}
			break;
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1101:
			if (classf_rule_modify_rule_fields_filter_de(&rule_fields->filter_outer, 1) < 0)
			{
				return -1;
			}
			break;
		//de = 1	
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_111:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1001:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1011:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1111:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10001:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10011:
			//do notthing
			break;
		//de = 0
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_101: //reserved
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_110:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1000:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1010:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_1110:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10000:
		case OMCIUTIL_VLAN_TREATMENT_TPID_DE_10010:
		default:
			//imposible
			dbprintf(LOG_ERR, "set tpid de error for outer\n");
			return -1;
		}
		break;
	default:
		//imposible
		dbprintf(LOG_ERR, "filter tpid de error\n");
		return -1;
	}

	return 0;
}

static int
classf_rule_modify_rule_fields_by_filter(enum omciutil_vlan_tag_code_t code, struct omciutil_vlan_rule_fields_t *rule_fields, struct omciutil_vlan_tag_fields_t *filter)
{
	if (rule_fields == NULL || filter == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//according code , restore filter to tagging rule
	switch (code)
	{
	case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
		rule_fields->filter_outer = *filter;
		break;
	case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
		rule_fields->filter_inner = *filter;
		break;
	case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER:
		//check pbit
		if (classf_rule_modify_rule_fields_treatment_pbit(filter->priority, rule_fields, 0) < 0)
		{
			dbprintf(LOG_ERR, "modify rule_fields outer treatment pbit error!!\n");
			return -1;
		}
		//check vid
		if (classf_rule_modify_rule_fields_treatment_vid(filter->vid, rule_fields, 0) < 0)
		{
			dbprintf(LOG_ERR, "modify rule_fields outer treatment vid error!!\n");
			return -1;
		}
		//check de
		if (classf_rule_modify_rule_fields_treatment_de(filter->tpid_de, rule_fields, 0) < 0)
		{
			dbprintf(LOG_ERR, "modify rule_fields outer treatment de error!!\n");
			return -1;
		}		
		break;
	case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
		//check pbit
		if (classf_rule_modify_rule_fields_treatment_pbit(filter->priority, rule_fields, 1) < 0)
		{
			dbprintf(LOG_ERR, "modify rule_fields inner treatment pbit error!!\n");
			return -1;
		}
		//check vid
		if (classf_rule_modify_rule_fields_treatment_vid(filter->vid, rule_fields, 1) < 0)
		{
			dbprintf(LOG_ERR, "modify rule_fields inner treatment vid error!!\n");
			return -1;
		}
		//check de
		if (classf_rule_modify_rule_fields_treatment_de(filter->tpid_de, rule_fields, 1) < 0)
		{
			dbprintf(LOG_ERR, "modify rule_fields inner treatment de error!!\n");
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "result code error!!\n");
		return -1;
	}
	
	return 0;
}

static int
classf_generate_rule_by_filter_result(struct list_head *rule_list,
	struct classf_rule_candi_t *rule_candi,
	struct classf_filter_result_t *filter_result,
	enum omciutil_vlan_tag_code_t code,
	unsigned char tlan_flag)
{
	struct classf_rule_t *rule;
	struct omciutil_vlan_rule_fields_t rule_fields;
	struct classf_filter_node_t *filter_node;
	unsigned int count = 0;

	if (rule_list == NULL || rule_candi == NULL || filter_result == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//generate expected rule
	list_for_each_entry(filter_node, &filter_result->expected_filter_list, f_node)
	{
		rule_fields = *rule_candi->rule_fields;

		if (classf_rule_modify_rule_fields_by_filter(code, &rule_fields, &filter_node->filter) < 0)
		{
			dbprintf(LOG_ERR, "modify rule_fields error!!, rule_candi entry_id=%u\n", rule_candi->entry_id);
			continue;
		}

		rule = classf_alloc_rule();
		//lan
		rule->lan_port = rule_candi->lan_port;
		rule->rule_fields = rule_fields;
		rule->dscp2pbit_enable = rule_candi->dscp2pbit_enable;
		rule->pvlan_acl = rule_candi->pvlan_acl;
		//wan
		rule->wan_port = rule_candi->wan_port;
		rule->gem_index = rule_candi->gem_index;
		rule->rule_candi_entry_id = rule_candi->entry_id;
		//colour
		rule->colour = filter_node->colour;
		rule->tlan_info.flag = tlan_flag;

		count++;

		if (classf_insert_rule_to_list(rule, rule_list) < 0)
		{
			dbprintf(LOG_WARNING, "the same rule, drop it!!\n");
			free_safe(rule);
			count--;
		}
	}

	//generate unexpected rule
	list_for_each_entry(filter_node, &filter_result->unexpected_filter_list, f_node)
	{
		rule_fields = *rule_candi->rule_fields;

		if (classf_rule_modify_rule_fields_by_filter(code, &rule_fields, &filter_node->filter) < 0)
		{
			dbprintf(LOG_ERR, "modify rule_fields error!!, rule_candi entry_id=%u\n", rule_candi->entry_id);
			continue;
		}

		rule = classf_alloc_rule();
		//lan
		rule->lan_port = rule_candi->lan_port;
		rule_fields.treatment_tag_to_remove += 4; // 4: remove 0 tag then drop, 5: remove 1 tag then drop, 6: remove 2 tag then drop
		rule_fields.sort_priority = OMCIUTIL_VLAN_SORT_PRIORITY_HIGHEST; //these rules have the highest priority, them would match first before other rules.
		rule->rule_fields = rule_fields;
		rule->dscp2pbit_enable = rule_candi->dscp2pbit_enable;
		rule->pvlan_acl = rule_candi->pvlan_acl;
		//wan
		rule->wan_port = rule_candi->wan_port;
		rule->gem_index = rule_candi->gem_index;
		rule->rule_candi_entry_id = rule_candi->entry_id;

		count++;

		if (classf_insert_rule_to_list(rule, rule_list) < 0)
		{
			dbprintf(LOG_WARNING, "the same rule, drop it!!\n");
			free_safe(rule);
			count--;
		}
	}

	//set valid
	if (count > 0)
	{
		rule_candi->reason = CLASSF_REASON_OK;
	} else {
		rule_candi->reason = CLASSF_REASON_DUPLICATE;
	}

	return 0;
}


static void
classf_release_filter_list(struct list_head *filter_list)
{
	struct classf_filter_node_t *filter_node_pos, *filter_node_n;

	if (filter_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return;
	}

	list_for_each_entry_safe(filter_node_pos, filter_node_n, filter_list, f_node)
	{
		list_del(&filter_node_pos->f_node);
		free_safe(filter_node_pos);
	}

	return;
}

//class reason code
static unsigned int
classf_check_untag_filter(unsigned char filter_type, unsigned char iswan) //0: lan, other: wan
{
	unsigned int reason = CLASSF_REASON_OK;

	switch (filter_type)
	{
	case CLASSF_FILTER_OP_TYPE_EXT_UNTAG_PASS:
		break;
	case CLASSF_FILTER_OP_TYPE_EXT_UNTAG_DISCARD:
		if (iswan)
		{
			reason = CLASSF_REASON_WF_UNTAG_DISCARD;
		} else {
			reason = CLASSF_REASON_LF_UNTAG_DISCARD;
		}
		break;
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_PASS:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_DISCARD:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_1:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_2:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_3:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_4:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_5:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_6:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_7:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_8:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_9:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_10:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_11:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE:
		if (iswan)
		{
			reason = CLASSF_REASON_WF_TYPE_ERROR;
		} else {
			reason = CLASSF_REASON_LF_TYPE_ERROR;
		}
		break;
	default:
		reason = CLASSF_REASON_INTERNAL_ERROR;
	}

	return reason;
}

//class reason code
static unsigned int
classf_check_and_modify_tag_filter(unsigned char filter_type, struct classf_port_filter_t *port_filter, struct omciutil_vlan_tag_fields_t *filter, unsigned char iswan) //0: lan, other: wan
{
	unsigned int reason = CLASSF_REASON_OK;

	if (port_filter == NULL || filter == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return 0; //discard
	}

	switch(filter_type)
	{
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_PASS:
		break;
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_DISCARD:
		if (iswan)
		{
			reason = CLASSF_REASON_WF_TAG_DISCARD;
		} else {
			reason = CLASSF_REASON_LF_TAG_DISCARD;
		}
		break;
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_0:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_1:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_2:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_3:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_4:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_5:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_6:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_7:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_8:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_9:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_10:
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_POSITIVE_11:
		//check filter_type first
		if (!(port_filter->vlan_tci_num > (filter_type - 4)))
		{
			reason = CLASSF_REASON_INTERNAL_ERROR;
		}
		
		//check with filter pbit
		if (reason == CLASSF_REASON_OK)
		{
			if (filter->priority < OMCIUTIL_VLAN_FILTER_PRIORITY_8)
			{
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_PRIORITY ||
					port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					if (((port_filter->vlan_tci[filter_type - 4] & 0xe000) >> 13) !=
						filter->priority)
					{
						if (iswan)
						{
							reason = CLASSF_REASON_WF_PRI_UNMATCH;
						} else {
							reason = CLASSF_REASON_LF_PRI_UNMATCH;
						}
					}
				}
			} else if (filter->priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8 ||
				filter->priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14) {
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_PRIORITY ||
					port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					filter->priority = (port_filter->vlan_tci[filter_type - 4] & 0xe000) >> 13;
				}
			} else {
				//impossible
				reason = CLASSF_REASON_INTERNAL_ERROR;
			}
		}

		//check with filter vid
		if (reason == CLASSF_REASON_OK)
		{
			if (filter->vid <= 4095)
			{
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_VID ||
					port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					if ((port_filter->vlan_tci[filter_type - 4] & 0x0fff) !=
						filter->vid)
					{
						if (iswan)
						{
							reason = CLASSF_REASON_WF_VID_UNMATCH;
						} else {
							reason = CLASSF_REASON_LF_VID_UNMATCH;
						}
					}
				}
			} else if (filter->vid == 4096) {
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_VID ||
					port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					filter->vid = port_filter->vlan_tci[filter_type - 4] & 0x0fff;
				}
			} else {
				//impossible
				reason = CLASSF_REASON_INTERNAL_ERROR;
			}
		}

		//check with filter de bit
		if (reason == CLASSF_REASON_OK)
		{
			switch (filter->tpid_de)
			{
			case OMCIUTIL_VLAN_FILTER_TPID_DE_000: //de bit don't care
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					if ((port_filter->vlan_tci[filter_type - 4] & 0x1000) >> 12)
					{
						filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1100; //tpid = don't care, de = 1
					} else {
						filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1011; //tpid = don't care, de = 0
					}
				}
				break;
			case OMCIUTIL_VLAN_FILTER_TPID_DE_100: //de bit don't care
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					if ((port_filter->vlan_tci[filter_type - 4] & 0x1000) >> 12)
					{
						filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_10001; //tpid = 8100, de = 1
					} else {
						filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_10000; //tpid = 8100, de = 0
					}
				}
				break;
			case OMCIUTIL_VLAN_FILTER_TPID_DE_101: //de bit don't care
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					if ((port_filter->vlan_tci[filter_type - 4] & 0x1000) >> 12)
					{
						filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_111; //tpid = input tpid, de = 1
					} else {
						filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_110; //tpid = input tpid, de = 0
					}
				}
				break;
			case OMCIUTIL_VLAN_FILTER_TPID_DE_1000: //de bit don't care
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					if ((port_filter->vlan_tci[filter_type - 4] & 0x1000) >> 12)
					{
						filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1010; //tpid = output tpid, de = 1
					} else {
						filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1001; //tpid = output tpid, de = 0
					}
				}
				break;
			case OMCIUTIL_VLAN_FILTER_TPID_DE_110: //de = 0
			case OMCIUTIL_VLAN_FILTER_TPID_DE_1001: //de = 0 
			case OMCIUTIL_VLAN_FILTER_TPID_DE_1011: //de = 0
			case OMCIUTIL_VLAN_FILTER_TPID_DE_10000: //de = 0 
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					if ((port_filter->vlan_tci[filter_type - 4] & 0x1000) >> 12)
					{
						if (iswan)
						{
							reason = CLASSF_REASON_WF_DE_UNMATCH;
						} else {
							reason = CLASSF_REASON_LF_DE_UNMATCH;
						}
					}
				}
				break;
			case OMCIUTIL_VLAN_FILTER_TPID_DE_111: //de = 1
			case OMCIUTIL_VLAN_FILTER_TPID_DE_1010: //de = 1
			case OMCIUTIL_VLAN_FILTER_TPID_DE_1100: //de = 1
			case OMCIUTIL_VLAN_FILTER_TPID_DE_10001: //de = 1
				if (port_filter->vlan_inspection_type == CLASSF_FILTER_INSPECTION_TYPE_TCI)
				{
					if (((port_filter->vlan_tci[filter_type - 4] & 0x1000) >> 12) == 0)
					{
						if (iswan)
						{
							reason = CLASSF_REASON_WF_DE_UNMATCH;
						} else {
							reason = CLASSF_REASON_LF_DE_UNMATCH;
						}
					}
				}
				break;
			default:
				reason = CLASSF_REASON_INTERNAL_ERROR;
			}
		}
		break;
	case CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE:
		//set pass = 1, then determine it after gem port checking
		reason = CLASSF_REASON_OK;
		break;
	case CLASSF_FILTER_OP_TYPE_EXT_UNTAG_PASS:
	case CLASSF_FILTER_OP_TYPE_EXT_UNTAG_DISCARD:
		if (iswan)
		{
			reason = CLASSF_REASON_WF_TYPE_ERROR;
		} else {
			reason = CLASSF_REASON_LF_TYPE_ERROR;
		}
		break;
	default: //include untag
		reason = CLASSF_REASON_INTERNAL_ERROR;
	}

	return reason;
}

static unsigned int
classf_check_negative_tag_filter(struct classf_port_filter_t *port_filter, struct classf_filter_result_t *filter_result, unsigned char iswan) //0: lan, other: wan
{
	struct list_head unexpected_filter_list_temp;
	struct classf_filter_node_t *expected_filter_node, *expected_filter_node_n;
	struct classf_filter_node_t *unexpected_filter_node;
	struct omciutil_vlan_tag_fields_t new_filter;
	unsigned int flag, i;
	unsigned short expected_tag_value;
	unsigned short expected_tag_mask;
	unsigned short unexpected_tag_value;
	unsigned short unexpected_tag_mask;

	if (port_filter == NULL || filter_result == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return 0; //discard
	}

	//get unexpected tag mask
	classf_filter_convert2_mask(port_filter->vlan_inspection_type, &unexpected_tag_mask);

	list_for_each_entry_safe(expected_filter_node, expected_filter_node_n, &filter_result->expected_filter_list, f_node)
	{
		INIT_LIST_HEAD(&unexpected_filter_list_temp);
		expected_tag_value = expected_tag_mask = unexpected_tag_value = 0;

		//get expected filter tag value and mask
		classf_rule_filter_convert2_mask(&expected_filter_node->filter, &expected_tag_value, &expected_tag_mask);
		
		flag = 1;
		for (i = 0; i < port_filter->vlan_tci_num; i++)
		{
			unexpected_tag_value = port_filter->vlan_tci[i];

			if ((expected_tag_mask & unexpected_tag_mask) == unexpected_tag_mask) // mean expected is equal(subet) of unexpected
			{
				if ((expected_tag_value & unexpected_tag_mask) == (unexpected_tag_value & unexpected_tag_mask))
				{
					//can not pass, remove this expected filter
					list_del(&expected_filter_node->f_node);
					free_safe(expected_filter_node);
					flag = 0;
					break;
				} else {
					//this tci does not drop any packet from expected filter
					continue; //check next
				}
			} else {
				if ((expected_tag_mask & unexpected_tag_mask) != 0 &&
					(expected_tag_value & (expected_tag_mask & unexpected_tag_mask)) != (unexpected_tag_value & (expected_tag_mask & unexpected_tag_mask)))
				{
					//this tci does not drop any packet from expected filter
					continue; //check next
				} else {
					//extract unexpected rule, add this tci filter to unexpected filter list temp
					new_filter = expected_filter_node->filter;
					if (classf_value_override_rule_filter(&new_filter, unexpected_tag_value, unexpected_tag_mask) == 0)
					{
						unexpected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
						unexpected_filter_node->filter = new_filter;
						list_add_tail(&unexpected_filter_node->f_node, &unexpected_filter_list_temp);
					}
				}
			}
		}
		if (flag)
		{
			//merge unexpected_filter_list_temp to filter_result unexpected filter list
			list_splice(&unexpected_filter_list_temp, filter_result->unexpected_filter_list.prev);
		} else {
			//release unexpected_filter_list_temp
			classf_release_filter_list(&unexpected_filter_list_temp);
		}
	}
	if (list_empty(&filter_result->expected_filter_list))
	{
		if (iswan)
		{
			return CLASSF_REASON_WF_NEGATIVE_DISCARD;
		} else {
			return CLASSF_REASON_LF_NEGATIVE_DISCARD;
		}
	} else {
		return CLASSF_REASON_OK;
	}
}

//definition pcp by colour scheme
unsigned char classf_colour_pcp[8] =
{
	0x00,
	0x00,
	0x00,
	0x00, //8P0D
	0x10, //7P1D
	0x14, //6P2D
	0x15, //5P3D
	0x00
};

static int
classf_rule_gen_colour_filter(struct classf_filter_node_t *filter_node,
	unsigned char colour_marking,
	struct list_head *expected_filter_list)
{
	unsigned int i;
	struct classf_filter_node_t *expected_filter_node;
	
	if (filter_node == NULL || expected_filter_list == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}
	
	//check colour marking scheme
	if (colour_marking == 2) //DEI
	{
		switch(filter_node->filter.tpid_de)
		{
		case OMCIUTIL_VLAN_FILTER_TPID_DE_000:
			//seperate to two filters, green
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->filter.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1011; //green, tpid = don't care, de = 0
			expected_filter_node->colour = 2; //de green
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);
			//yellow
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->filter.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1100; //yellow, tpid = don't care, de = 1
			expected_filter_node->colour = 3; //de yellow
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);

			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_100:
			//seperate to two filters, green
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->filter.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_10000; //tpid = 8100, de = 0
			expected_filter_node->colour = 2; //de green
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);
			//yellow
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->filter.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_10001; //tpid = 8100, de = 1
			expected_filter_node->colour = 3; //de yellow
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);
			
			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_101:
			//seperate to two filters, green
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->filter.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_110;
			expected_filter_node->colour = 2; //de green
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);
			//yellow
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->filter.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_111;
			expected_filter_node->colour = 3; //de yellow
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);
			
			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_110:
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1001:
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1011:
		case OMCIUTIL_VLAN_FILTER_TPID_DE_10000:
			//gen green filter
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->colour = 2; //de green
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);

			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_111:
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1010:
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1100:
		case OMCIUTIL_VLAN_FILTER_TPID_DE_10001:
			//gen yellow filter
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->colour = 3; //de yellow
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);

			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_1000:
			//seperate to two filters, green
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->filter.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1001; //green, tpid = output tpid, de = 0
			expected_filter_node->colour = 2; //de green
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);
			//yellow
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->filter.tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1010; //yellow, tpid = output tpid, de = 1
			expected_filter_node->colour = 3; //de yellow
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);

			break;
		default:
			//add default one
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);
		}
	} else {
		//3: 8P0D, 4: 7P1D, 5: 6P2D, 6: 5P3D
		if (filter_node->filter.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_8)
		{
			// decise green or yellow
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			expected_filter_node->colour = (classf_colour_pcp[colour_marking] & (1 << filter_node->filter.priority)) ? 1 : 0;
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);
		} else if (filter_node->filter.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8 ||
			filter_node->filter.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14)
		{
			//seperate green and yellow part for 8 pbits
			for (i = 0; i < 8; i++)
			{
				expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
				expected_filter_node->filter = filter_node->filter;
				expected_filter_node->filter.priority = i;
				expected_filter_node->colour = (classf_colour_pcp[colour_marking] & (1 << i)) ? 1 : 0;
				list_add_tail(&expected_filter_node->f_node, expected_filter_list);
			}
		} else {
			//add default one
			expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
			expected_filter_node->filter = filter_node->filter;
			list_add_tail(&expected_filter_node->f_node, expected_filter_list);
		}
	}

	return 0;
}

//0: non-TLS rule, 1: TLS rule
static int
classf_check_tlan(struct omciutil_vlan_tlan_table_t *tlan_table, struct omciutil_vlan_tag_fields_t *filter)
{
	int i;
	
	if (tlan_table == NULL || filter == NULL)
	{
		return 0;
	}

	for (i = 0; i < tlan_table->total && i < CLASSF_TLAN_TABLE_MAX; i++)
	{
		//excatly match, treat "don't care" in filter was not equal
		if (tlan_table->vid[i] == filter->vid)
		{
			//TLS(tlan)
			return 1;
		}
	}

	return 0;
}

static int
classf_rule_determine_us(struct classf_bridge_info_t *bridge_info,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info,
	struct omciutil_vlan_tlan_table_t *tlan_table)
{
	struct classf_rule_candi_t *rule_candi;
	struct classf_rule_t *rule;
	struct omciutil_vlan_rule_result_list_t rule_result_list;
	struct omciutil_vlan_rule_result_node_t *result_node;
	struct omciutil_vlan_tag_fields_t filter;
	struct classf_filter_result_t filter_result;
	struct classf_filter_node_t *filter_node, *expected_filter_node;
	struct list_head filter_list;
	unsigned int i;
	unsigned char dscp2pbit_mask;
	unsigned int result_ok_flag;
	unsigned char tlan_flag;

	if (bridge_info == NULL || dscp2pbit_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	list_for_each_entry(rule_candi, &bridge_info->rule_candi_list_us, rc_node)
	{
		rule_candi->reason = CLASSF_REASON_OK;
		tlan_flag = 0; //reset

		//check drop first
		if (rule_candi->rule_fields->treatment_tag_to_remove == 3) //dircard
		{
			//add a rule to list
			rule = classf_alloc_rule();
			//lan
			rule->lan_port = rule_candi->lan_port;
			rule->rule_fields = *(rule_candi->rule_fields);
			rule->pvlan_acl = rule_candi->pvlan_acl;

			//wan
			rule->wan_port = &dummy_wan_port_g;
			rule->gem_index = 0;
			rule->rule_candi_entry_id = rule_candi->entry_id;

			if (classf_insert_rule_to_list(rule, &bridge_info->rule_list_us) < 0)
			{
				dbprintf(LOG_WARNING, "the same rule, drop it!!\n");
				free_safe(rule);
				rule_candi->reason = CLASSF_REASON_DUPLICATE;
			}

			continue;
		}

		//get rule result, and translate to filter
		if (omciutil_vlan_generate_rule_result(rule_candi->rule_fields, &rule_result_list) < 0)
		{
			dbprintf(LOG_ERR, "could not get rule result\n");
			rule_candi->reason = CLASSF_REASON_TAGGING_ERROR;

			continue;
		}

		//according the result tag count, reverse the rule to filter
		switch (rule_result_list.tag_count)
		{
		case 0:
			//untag, check lan filter
			if (rule_candi->lan_filter_valid != 0)
			{
				rule_candi->reason = classf_check_untag_filter(rule_candi->lan_filter_type, 0);
			}

			//untag, check wan filter
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				if (rule_candi->wan_filter_valid != 0)
				{
					rule_candi->reason = classf_check_untag_filter(rule_candi->wan_filter_type, 1);
				}
			}

			//untag, check gem
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				if (!(rule_candi->wan_port->port_gem.gem_index_total > rule_candi->gem_index &&
					rule_candi->wan_port->port_gem.gem_tagged[rule_candi->gem_index] == 0))
				{
					rule_candi->reason = CLASSF_REASON_GEM_UNTAG_DISCARD;
				}
			}

			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				//add a rule to list
				rule = classf_alloc_rule();
				//lan
				rule->lan_port = rule_candi->lan_port;
				rule->rule_fields = *(rule_candi->rule_fields);
				rule->pvlan_acl = rule_candi->pvlan_acl;
				//wan
				rule->wan_port = rule_candi->wan_port;
				rule->gem_index = rule_candi->gem_index;
				rule->rule_candi_entry_id = rule_candi->entry_id;

				if (classf_insert_rule_to_list(rule, &bridge_info->rule_list_us) < 0)
				{
					dbprintf(LOG_WARNING, "the same rule, drop it!!\n");
					//release
					free_safe(rule);
					rule_candi->reason = CLASSF_REASON_DUPLICATE;
				}
			}

			break;
		case 1:
		case 2:
			//tag
			INIT_LIST_HEAD(&filter_list);

			//pop-up the top-most one tag for outer tag
			result_node = list_entry(rule_result_list.tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
			if (omciutil_vlan_generate_reverse_filter(rule_candi->rule_fields, result_node->code, &filter, 0) < 0)
#else
			if (omciutil_vlan_generate_reverse_filter(rule_candi->rule_fields, result_node->code, &filter) < 0)
#endif
			{
				rule_candi->reason = CLASSF_REASON_TAGGING_ERROR;
				continue;
			}

			if (filter.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_13) //from dscp-2-pbit
			{
				switch(result_node->code)
				{
				case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER:
					rule_candi->dscp2pbit_enable = 2; //dscp2pbit from outer treatment
					break;
				case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
					rule_candi->dscp2pbit_enable = 1; //dscp2pbit from inner treatment
					break;
				default:
					//impossible
					dbprintf(LOG_ERR, "dscp2pbit from wrong treatment, rule_candi entry_id=%u!!\n", rule_candi->entry_id);
				}
				//get dscp2pbit mask
				if (omci_env_g.classf_dscp2pbit_mode == 0)
				{
					if (rule_candi->lan_port->port_tagging.dscp2pbit_info.enable == 0)
					{
						dbprintf(LOG_ERR, "dscp2pbit did not enable, rule_candi entry_id=%u!!\n", rule_candi->entry_id);
					}
					dscp2pbit_mask = rule_candi->lan_port->port_tagging.dscp2pbit_info.pbit_mask;
				} else {
					if (dscp2pbit_info->enable == 0)
					{
						dbprintf(LOG_ERR, "dscp2pbit did not enable, rule_candi entry_id=%u!!\n", rule_candi->entry_id);
					}
					dscp2pbit_mask = dscp2pbit_info->pbit_mask;
				}

				//generate filter node by dscp2pbit mask
				if (dscp2pbit_mask == 0xff || dscp2pbit_mask == 0x00) //all pbit
				{
					filter.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8; //don't care
					//allocate a filter node
					filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
					filter_node->filter = filter;
					list_add_tail(&filter_node->f_node, &filter_list);
				} else {
					//for non ip packets, assign default pbit 0 to match internal pbit anyway;
					dscp2pbit_mask |= 0x1;
					
					for (i = 0; i < 8; i++)
					{
						if (dscp2pbit_mask & (0x1 << i))
						{
							filter.priority = i; //assign pbit
							//allocate a filter node
							filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
							filter_node->filter = filter;
							list_add_tail(&filter_node->f_node, &filter_list);
						}
					}
				}
			} else {
				//allocate a filter node
				filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
				filter_node->filter = filter;
				list_add_tail(&filter_node->f_node, &filter_list);
			}

			//check each filter_node to generate classf rules
			result_ok_flag = 0;
			list_for_each_entry(filter_node, &filter_list, f_node)
			{
				//init reason to CLASSF_REASON_OK
				rule_candi->reason = CLASSF_REASON_OK;
				memset(&filter_result, 0x00, sizeof(struct classf_filter_result_t));
				INIT_LIST_HEAD(&filter_result.expected_filter_list);
				INIT_LIST_HEAD(&filter_result.unexpected_filter_list);

				//check lan filter
				if (rule_candi->lan_filter_valid != 0)
				{
					rule_candi->reason = classf_check_and_modify_tag_filter(rule_candi->lan_filter_type, &rule_candi->lan_port->port_filter, &filter_node->filter, 0);
				}

				//check wan filter
				if (rule_candi->reason == CLASSF_REASON_OK)
				{
					if (rule_candi->wan_filter_valid != 0)
					{
						rule_candi->reason = classf_check_and_modify_tag_filter(rule_candi->wan_filter_type, &rule_candi->wan_port->port_filter, &filter_node->filter, 1);
					}
				}

				//check for tlan(TLS), calix proprietary
				if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 &&
					rule_candi->reason == CLASSF_REASON_OK &&
					rule_candi->rule_fields->owner_me != NULL &&
					(rule_candi->rule_fields->owner_me->classid == 171 ||
					rule_candi->rule_fields->owner_me->classid == 249 ||
					rule_candi->rule_fields->owner_me->classid == 65313) &&
					tlan_table != NULL)
				{
					tlan_flag = classf_check_tlan(tlan_table, &filter_node->filter);
				}

				//check gem pbit, and generated expected filter list
				if (rule_candi->reason == CLASSF_REASON_OK)
				{
					if (rule_candi->wan_port->port_gem.gem_index_total > rule_candi->gem_index &&
						rule_candi->wan_port->port_gem.gem_tagged[rule_candi->gem_index] == 1)
					{
						unsigned char pbit_mask = 0;

						if (filter_node->filter.priority < OMCIUTIL_VLAN_FILTER_PRIORITY_8)
						{		
							pbit_mask = (0x1 << filter_node->filter.priority);
							//check pbit mask
							if ((pbit_mask & rule_candi->wan_port->port_gem.pbit_mask[rule_candi->gem_index]) > 0)
							{
								if (rule_candi->wan_port->port_gem.colour_marking_us[rule_candi->gem_index] >= 2 &&
									rule_candi->wan_port->port_gem.colour_marking_us[rule_candi->gem_index] <= 6)
								{
									//enable colour marking, generate filters to distinguish the green/yellow flows
									classf_rule_gen_colour_filter(filter_node,
										rule_candi->wan_port->port_gem.colour_marking_us[rule_candi->gem_index],
										&filter_result.expected_filter_list);
								} else {
									//add this filter to expected filter list
									expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
									expected_filter_node->filter = filter_node->filter;
									list_add_tail(&expected_filter_node->f_node, &filter_result.expected_filter_list);
								}
							} else {
								rule_candi->reason = CLASSF_REASON_GEM_TAG_PBIT_DISCARD;
							}
						} else if (filter_node->filter.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_8 ||
							filter_node->filter.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14) {
							//pass for some pbit
							if (rule_candi->wan_port->port_gem.pbit_mask[rule_candi->gem_index] == 0xff) //all pbit pass
							{
								if (rule_candi->wan_port->port_gem.colour_marking_us[rule_candi->gem_index] >= 2 &&
									rule_candi->wan_port->port_gem.colour_marking_us[rule_candi->gem_index] <= 6)
								{
									//enable colour marking, generate filters to distinguish the green/yellow flows
									classf_rule_gen_colour_filter(filter_node,
										rule_candi->wan_port->port_gem.colour_marking_us[rule_candi->gem_index],
										&filter_result.expected_filter_list);									
								} else {
									//add this filter to expected filter list
									expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
									expected_filter_node->filter = filter_node->filter;
									list_add_tail(&expected_filter_node->f_node, &filter_result.expected_filter_list);
								}
							} else {
								//add this to expected filter list by allowed pbit
								for (i = 0; i < 8; i++)
								{
									pbit_mask = (1 << i);
									if (pbit_mask & rule_candi->wan_port->port_gem.pbit_mask[rule_candi->gem_index])
									{
										if (rule_candi->wan_port->port_gem.colour_marking_us[rule_candi->gem_index] >= 2 &&
											rule_candi->wan_port->port_gem.colour_marking_us[rule_candi->gem_index] <= 6)
										{
											//set priority
											filter_node->filter.priority = i;
											
											//enable colour marking, generate filters to distinguish the green/yellow flows
											classf_rule_gen_colour_filter(filter_node,
												rule_candi->wan_port->port_gem.colour_marking_us[rule_candi->gem_index],
												&filter_result.expected_filter_list);									
										} else {
											expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
											expected_filter_node->filter = filter_node->filter;
											expected_filter_node->filter.priority = i;
											list_add_tail(&expected_filter_node->f_node, &filter_result.expected_filter_list);
										}
									}
								}
							}
						} else {
							rule_candi->reason = CLASSF_REASON_INTERNAL_ERROR;
						}				
					} else {
						rule_candi->reason = CLASSF_REASON_GEM_TAG_DISCARD;
					}
				}

				//check negative, lan, generate unexpected filter list
				if (rule_candi->reason == CLASSF_REASON_OK)
				{
					if (rule_candi->lan_filter_type == CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE)
					{
						rule_candi->reason = classf_check_negative_tag_filter(&rule_candi->lan_port->port_filter, &filter_result, 0);
					}	
				}

				//check negative, wan, generate unexpected filter list
				if (rule_candi->reason == CLASSF_REASON_OK)
				{
					if (rule_candi->wan_filter_type == CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE)
					{
						rule_candi->reason = classf_check_negative_tag_filter(&rule_candi->wan_port->port_filter, &filter_result, 1);
					}
				}

				//according pass result to generate rule and set result_ok_flag
				if (rule_candi->reason == CLASSF_REASON_OK)
				{
					classf_generate_rule_by_filter_result(&bridge_info->rule_list_us, rule_candi, &filter_result, result_node->code, tlan_flag);
					result_ok_flag = 1;
				}

				//release filter_result
				classf_release_filter_list(&filter_result.expected_filter_list);
				classf_release_filter_list(&filter_result.unexpected_filter_list);
			}

			//if any one filter in filter_list was CLASSF_REASON_OK to generate real rules, set rule_candi->reason to CLASSF_REASON_OK
			if (result_ok_flag == 1)
			{
				rule_candi->reason = CLASSF_REASON_OK;
			}

			//release filter list
			classf_release_filter_list(&filter_list);

			break;
		default:
			;//somethng wrong
		}

		//release the result
		omciutil_vlan_release_rule_result(&rule_result_list);
	}

	return 0;
}

static int
classf_rule_determine_ds_check_pre_condition(struct classf_rule_candi_t *rule_candi)
{
	if (rule_candi == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	} 

	if (omci_env_g.tag_mc_ds_wildcard != 0) //check only ds multicast gem to 310 rules
	{
		if ((rule_candi->wan_port != NULL &&
			rule_candi->wan_port->port_gem.gem_type == 1) && //ds multicast gem
			(rule_candi->rule_fields == NULL ||
			rule_candi->rule_fields->owner_me == NULL ||
			(rule_candi->rule_fields != NULL &&
			rule_candi->rule_fields->owner_me != NULL &&
			rule_candi->rule_fields->owner_me->classid != 310))) //NOT multicast subscriber config info
		{
			rule_candi->reason = CLASSF_REASON_MC_TAGGING_ERROR;
		}
		if ((rule_candi->wan_port != NULL &&
			rule_candi->wan_port->port_gem.gem_type != 1) && //NOT ds multicast gem
			(rule_candi->rule_fields != NULL &&
			rule_candi->rule_fields->owner_me != NULL &&
			rule_candi->rule_fields->owner_me->classid == 310)) //multicast subscriber config info
		{
			rule_candi->reason = CLASSF_REASON_MC_GEM_ERROR;
		}
	}

	return 0;
}

static int
classf_rule_determine_ds(struct classf_bridge_info_t *bridge_info, struct omciutil_vlan_tlan_table_t *tlan_table)
{
	struct classf_rule_candi_t *rule_candi;
	struct classf_rule_t *rule;
	struct omciutil_vlan_tag_fields_t filter;
	struct classf_filter_result_t filter_result;
	struct classf_filter_node_t *expected_filter_node;
	int tag_num;
	unsigned char tlan_flag;

	if (bridge_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	list_for_each_entry(rule_candi, &bridge_info->rule_candi_list_ds, rc_node)
	{
		rule_candi->reason = CLASSF_REASON_OK;
		tlan_flag = 0; //reset

		//check pre-condition
		if (classf_rule_determine_ds_check_pre_condition(rule_candi) == 0 &&
			rule_candi->reason != CLASSF_REASON_OK)
		{
			continue;
		}

		tag_num = omciutil_vlan_get_rule_filter_tag_num(rule_candi->rule_fields);

		//according the tag filter num, check untag or tag
		switch (tag_num)
		{
		case 0:
			//untag, check lan filter
			if (rule_candi->lan_filter_valid != 0)
			{
				rule_candi->reason = classf_check_untag_filter(rule_candi->lan_filter_type, 0);
			}

			//untag, check wan filter
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				if (rule_candi->wan_filter_valid != 0)
				{
					rule_candi->reason = classf_check_untag_filter(rule_candi->wan_filter_type, 1);
				}
			}

#if 0
			//untag, check gem
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				if (!(rule_candi->wan_port->port_gem.gem_index_total > rule_candi->gem_index &&
					rule_candi->wan_port->port_gem.gem_tagged[rule_candi->gem_index] == 0))
				{
					rule_candi->reason = CLASSF_REASON_GEM_UNTAG_DISCARD;
				}
			}
#endif

			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				//add a rule to list
				rule = classf_alloc_rule();
				//lan
				rule->lan_port = rule_candi->lan_port;
				rule->rule_fields = *(rule_candi->rule_fields);
				rule->pvlan_acl = rule_candi->pvlan_acl;
				//wan
				rule->wan_port = rule_candi->wan_port;
				rule->gem_index = rule_candi->gem_index;
				rule->rule_candi_entry_id = rule_candi->entry_id;

				if (classf_insert_rule_to_list(rule, &bridge_info->rule_list_ds) < 0)
				{
					dbprintf(LOG_WARNING, "the same rule, drop it!!\n");
					//release
					free_safe(rule);
					rule_candi->reason = CLASSF_REASON_DUPLICATE;
				}
			}

			break;
		case 1:
		case 2:
			//tag
			memset(&filter_result, 0x00, sizeof(struct classf_filter_result_t));
			INIT_LIST_HEAD(&filter_result.expected_filter_list);
			INIT_LIST_HEAD(&filter_result.unexpected_filter_list);

			if (tag_num == 1)
			{
				filter = rule_candi->rule_fields->filter_inner;
			} else {
				filter = rule_candi->rule_fields->filter_outer;
			}

			//check lan filter
			if (rule_candi->lan_filter_valid != 0)
			{
				rule_candi->reason = classf_check_and_modify_tag_filter(rule_candi->lan_filter_type, &rule_candi->lan_port->port_filter, &filter, 0);
			}

			//check wan filter
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				if (rule_candi->wan_filter_valid != 0)
				{
					rule_candi->reason = classf_check_and_modify_tag_filter(rule_candi->wan_filter_type, &rule_candi->wan_port->port_filter, &filter, 1);
				}
			}

			//check for tlan(TLS), calix proprietary
			if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0 &&
				rule_candi->reason == CLASSF_REASON_OK &&
				rule_candi->rule_fields->owner_me != NULL &&
				(rule_candi->rule_fields->owner_me->classid == 171 ||
				rule_candi->rule_fields->owner_me->classid == 249 ||
				rule_candi->rule_fields->owner_me->classid == 65313) &&
				tlan_table != NULL)
			{
				tlan_flag = classf_check_tlan(tlan_table, &filter);
			}

			//check gem pbit mask for tag ??
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				if (!(rule_candi->wan_port->port_gem.gem_index_total > rule_candi->gem_index &&
					rule_candi->wan_port->port_gem.gem_tagged[rule_candi->gem_index] == 1))
				{
					rule_candi->reason = CLASSF_REASON_GEM_TAG_DISCARD;
				}
			}

			//add this filter to expected filter list
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				expected_filter_node = malloc_safe(sizeof(struct classf_filter_node_t));
				expected_filter_node->filter = filter;
				list_add_tail(&expected_filter_node->f_node, &filter_result.expected_filter_list);
			}

			//check negative, lan, generate unexpected filter list
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				if (rule_candi->lan_filter_type == CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE)
				{
					rule_candi->reason = classf_check_negative_tag_filter(&rule_candi->lan_port->port_filter, &filter_result, 0);
				}
			}

			//check negative, wan, generate unexpected filter list
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				if (rule_candi->wan_filter_type == CLASSF_FILTER_OP_TYPE_EXT_TAG_NEGATIVE)
				{
					rule_candi->reason = classf_check_negative_tag_filter(&rule_candi->wan_port->port_filter, &filter_result, 1);
				}
			}

			//according pass result to generate rule
			if (rule_candi->reason == CLASSF_REASON_OK)
			{
				if (tag_num == 1)
				{
					classf_generate_rule_by_filter_result(&bridge_info->rule_list_ds, rule_candi, &filter_result, OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER, tlan_flag);
				} else {
					classf_generate_rule_by_filter_result(&bridge_info->rule_list_ds, rule_candi, &filter_result, OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER, tlan_flag);
				}
			}

			//release filter_result
			classf_release_filter_list(&filter_result.expected_filter_list);
			classf_release_filter_list(&filter_result.unexpected_filter_list);

			break;
		default:
			dbprintf(LOG_ERR, "error tag_num=%d\n", tag_num);//somethng wrong
		}
	}

	return 0;
}

int
classf_bridge_rule_determine(struct classf_bridge_info_t *bridge_info,
	struct omciutil_vlan_dscp2pbit_info_t *dscp2pbit_info,
	struct omciutil_vlan_tlan_table_t *tlan_table)
{
	if (bridge_info == NULL || dscp2pbit_info == NULL)
	{
		dbprintf(LOG_ERR, "param error!!\n");
		return -1;
	}

	//upstream rule determine
	if (classf_rule_determine_us(bridge_info, dscp2pbit_info, tlan_table) < 0)
	{
		dbprintf(LOG_ERR, "bridge rule determine error for upstream!!, bridge meid=0x%.4x\n", bridge_info->me_bridge->meid);
	}

	//downstream rule determine
	if (classf_rule_determine_ds(bridge_info, tlan_table) < 0)
	{
		dbprintf(LOG_ERR, "bridge rule determine error for downstream!!, bridge meid=0x%.4x\n", bridge_info->me_bridge->meid);
	}

	return 0;
}

