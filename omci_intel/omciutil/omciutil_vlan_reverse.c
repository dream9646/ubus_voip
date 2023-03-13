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
 * File    : omciutil_vlan_reverse.c
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

int
omciutil_vlan_generate_rule_result(struct omciutil_vlan_rule_fields_t *rule_fields,
	struct omciutil_vlan_rule_result_list_t *rule_result_list)
{
	int i;
	struct omciutil_vlan_rule_result_node_t *result_node;

	//check input
	if (rule_fields == NULL || rule_result_list == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}
	
	//discard the frame
	if (rule_fields->treatment_tag_to_remove >= 3)
	{
		dbprintf(LOG_ERR, "discard the frame, tag_to_remove=%u\n", rule_fields->treatment_tag_to_remove);
		return -1;
	}

	//init entry_result_list
	memset(rule_result_list, 0x00, sizeof(struct omciutil_vlan_rule_result_list_t));
	INIT_LIST_HEAD(&rule_result_list->tag_list);

	//check filter inner
	if (rule_fields->filter_inner.priority <= OMCIUTIL_VLAN_FILTER_PRIORITY_8 ||
		rule_fields->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14)
	{
		//add a node to tag_list
		result_node = malloc_safe(sizeof(struct omciutil_vlan_rule_result_node_t));
		result_node->code = OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER;

		rule_result_list->tag_count++;
		//add to tail
		list_add_tail(&result_node->node, &rule_result_list->tag_list);
	}

	//check filter outer
	if (rule_fields->filter_outer.priority <= OMCIUTIL_VLAN_FILTER_PRIORITY_8 ||
		rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14)
	{
		//add a node to tag_list
		result_node = malloc_safe(sizeof(struct omciutil_vlan_rule_result_node_t));
		result_node->code = OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER;

		rule_result_list->tag_count++;
		//add to tail
		list_add_tail(&result_node->node, &rule_result_list->tag_list);
	}

	//check remove tag
	for (i = rule_fields->treatment_tag_to_remove; i > 0 && !list_empty(&rule_result_list->tag_list); i--)
	{
		result_node = list_entry(rule_result_list->tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
		rule_result_list->tag_count--;
		//remove tail one
		list_del(rule_result_list->tag_list.prev); //tail one first

		free_safe(result_node);
	}

	//check treatment inner
	if (rule_fields->treatment_inner.priority <= OMCIUTIL_VLAN_TREATMENT_PRIORITY_10)
	{
		//add a node to tag_list
		result_node = malloc_safe(sizeof(struct omciutil_vlan_rule_result_node_t));
		result_node->code = OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER;

		rule_result_list->tag_count++;
		//add to tail
		list_add_tail(&result_node->node, &rule_result_list->tag_list);
	}

	//check treatment outer
	if (rule_fields->treatment_outer.priority <= OMCIUTIL_VLAN_TREATMENT_PRIORITY_10)
	{
		//add a node to tag_list
		result_node = malloc_safe(sizeof(struct omciutil_vlan_rule_result_node_t));
		result_node->code = OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER;

		rule_result_list->tag_count++;
		//add to tail
		list_add_tail(&result_node->node, &rule_result_list->tag_list);
	}

	return 0;
}

int
omciutil_vlan_release_rule_result(struct omciutil_vlan_rule_result_list_t *rule_result_list)
{
	struct omciutil_vlan_rule_result_node_t *result_node_pos, *result_node_n;

	if (rule_result_list)
	{
		list_for_each_entry_safe(result_node_pos, result_node_n, &rule_result_list->tag_list, node)
		{
			list_del(&result_node_pos->node);
			free_safe(result_node_pos);
		}

		return 0;
	} else {
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}
}

static enum omciutil_vlan_tag_code_t
omciutil_vlan_check_many_to_many_vid(enum omciutil_vlan_tag_code_t code,
	struct omciutil_vlan_tag_fields_t *old_treatment_outer,
	struct omciutil_vlan_tag_fields_t *old_treatment_inner,
	struct omciutil_vlan_rule_result_list_t *rule_result_list)
{
	struct omciutil_vlan_rule_result_node_t *result_node;

	//check input
	if (old_treatment_outer == NULL || old_treatment_inner == NULL || rule_result_list ==  NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return OMCIUTIL_VLAN_TAG_CODE_INVALID;
	}

	switch (rule_result_list->tag_count)
	{
		case 0:
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
		case 1:
			//get last one of list
			result_node = list_entry(rule_result_list->tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_inner->vid == 4097) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_inner->vid == 4096))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER;
				}
			}
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
		default: // >= 2
			//get last one of list, outer one
			result_node = list_entry(rule_result_list->tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_outer->vid == 4097) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_outer->vid == 4096))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER;
				}
			} else if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER) {
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_outer->vid == 4097) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_outer->vid == 4096))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER;
				}
			}
			//get previous one, inner one
			result_node = list_entry(result_node->node.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_inner->vid == 4097) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_inner->vid == 4096))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER;
				}
			}
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
	}
}

static enum omciutil_vlan_tag_code_t
omciutil_vlan_check_many_to_many_priority(enum omciutil_vlan_tag_code_t code,
	struct omciutil_vlan_tag_fields_t *old_treatment_outer,
	struct omciutil_vlan_tag_fields_t *old_treatment_inner,
	struct omciutil_vlan_rule_result_list_t *rule_result_list)
{
	struct omciutil_vlan_rule_result_node_t *result_node;

	//check input
	if (old_treatment_outer == NULL || old_treatment_inner == NULL || rule_result_list ==  NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return OMCIUTIL_VLAN_TAG_CODE_INVALID;
	}

	switch (rule_result_list->tag_count)
	{
		case 0:
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
		case 1:
			//get last one of list
			result_node = list_entry(rule_result_list->tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_inner->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_9) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_inner->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_8))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER;
				}
			}
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
		default: // >= 2
			//get last one of list, outer one
			result_node = list_entry(rule_result_list->tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_outer->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_9) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_outer->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_8))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER;
				}
			} else if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER) {
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_outer->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_9) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_outer->priority== OMCIUTIL_VLAN_TREATMENT_PRIORITY_8))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER;
				}
			}
			//get previous one, inner one
			result_node = list_entry(result_node->node.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_inner->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_9) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_inner->priority == OMCIUTIL_VLAN_TREATMENT_PRIORITY_8))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER;
				}
			}
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
	}
}

static enum omciutil_vlan_tag_code_t
omciutil_vlan_check_many_to_many_tpid(enum omciutil_vlan_tag_code_t code,
	struct omciutil_vlan_tag_fields_t *old_treatment_outer,
	struct omciutil_vlan_tag_fields_t *old_treatment_inner,
	struct omciutil_vlan_rule_result_list_t *rule_result_list)
{
	struct omciutil_vlan_rule_result_node_t *result_node;

	//check input
	if (old_treatment_outer == NULL || old_treatment_inner == NULL || rule_result_list ==  NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return OMCIUTIL_VLAN_TAG_CODE_INVALID;
	}

	switch (rule_result_list->tag_count)
	{
		case 0:
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
		case 1:
			//get last one of list
			result_node = list_entry(rule_result_list->tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_inner->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_001) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_inner->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_000))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER;
				}
			}
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
		default: // >= 2
			//get last one of list, outer one
			result_node = list_entry(rule_result_list->tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_outer->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_001) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_outer->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_000))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER;
				}
			} else if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER) {
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_outer->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_001) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_outer->tpid_de== OMCIUTIL_VLAN_TREATMENT_TPID_DE_000))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER;
				}
			}
			//get previous one, inner one
			result_node = list_entry(result_node->node.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_inner->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_001) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_inner->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_000))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER;
				}
			}
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
	}
}

static enum omciutil_vlan_tag_code_t
omciutil_vlan_check_many_to_many_de(enum omciutil_vlan_tag_code_t code,
	struct omciutil_vlan_tag_fields_t *old_treatment_outer,
	struct omciutil_vlan_tag_fields_t *old_treatment_inner,
	struct omciutil_vlan_rule_result_list_t *rule_result_list)
{
	struct omciutil_vlan_rule_result_node_t *result_node;

	//check input
	if (old_treatment_outer == NULL || old_treatment_inner == NULL || rule_result_list ==  NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return OMCIUTIL_VLAN_TAG_CODE_INVALID;
	}

	switch (rule_result_list->tag_count)
	{
		case 0:
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
		case 1:
			//get last one of list
			result_node = list_entry(rule_result_list->tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_inner->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_011) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_inner->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_010))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER;
				}
			}
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
		default: // >= 2
			//get last one of list, outer one
			result_node = list_entry(rule_result_list->tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_outer->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_011) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_outer->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_010))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER;
				}
			} else if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER) {
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_outer->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_011) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_outer->tpid_de== OMCIUTIL_VLAN_TREATMENT_TPID_DE_010))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER;
				}
			}
			//get previous one, inner one
			result_node = list_entry(result_node->node.prev, struct omciutil_vlan_rule_result_node_t, node);
			if (result_node->code == OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER)
			{
				if ((code == OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER &&
					old_treatment_inner->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_011) ||
					(code == OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER&&
					old_treatment_inner->tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_010))
				{
					//only one filter
					return OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER;
				}
			}
			return OMCIUTIL_VLAN_TAG_CODE_INVALID;
	}
}

//watch!!, many-to-one case
static int
omciutil_vlan_translate_filter_to_treatment_tag(enum omciutil_vlan_tag_code_t code,
	struct omciutil_vlan_tag_fields_t *old_filter,
	struct omciutil_vlan_tag_fields_t *old_treatment_outer,
	struct omciutil_vlan_tag_fields_t *old_treatment_inner,
	struct omciutil_vlan_rule_result_list_t *rule_result_list,
	struct omciutil_vlan_tag_fields_t *new_treatment)
{
	//check input
	if (old_filter == NULL || old_treatment_outer == NULL || old_treatment_inner == NULL || rule_result_list == NULL || new_treatment == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//priority
	switch (old_filter->priority)
	{
	case OMCIUTIL_VLAN_FILTER_PRIORITY_0:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_1:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_2:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_3:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_4:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_5:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_6:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_7:
		new_treatment->priority = (omci_env_g.tag_ds_tci_dont_reverse_workaround == 0) ? old_filter->priority : OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
		break;
	case OMCIUTIL_VLAN_FILTER_PRIORITY_8:
	case OMCIUTIL_VLAN_FILTER_PRIORITY_14:
		//don't care, many-to-one case
		switch (omciutil_vlan_check_many_to_many_priority(code, old_treatment_outer, old_treatment_inner, rule_result_list))
		{
		//many-to-many
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
			new_treatment->priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_9;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
			new_treatment->priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
			break;
		//many-to-one
		default:
			new_treatment->priority = (omci_env_g.tag_ds_tci_dont_reverse_workaround == 0) ? OMCIUTIL_VLAN_TREATMENT_PRIORITY_0 : OMCIUTIL_VLAN_TREATMENT_PRIORITY_8;
		}
		break;
	case OMCIUTIL_VLAN_FILTER_PRIORITY_15:
		new_treatment->priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		break;
	default:
		;//do nothing
	}

	//vid
	if (old_filter->vid <= 4095)
	{
		new_treatment->vid = old_filter->vid;
	} else if (old_filter->vid == 4096) {
		//don't care, many-to-one case
		switch (omciutil_vlan_check_many_to_many_vid(code, old_treatment_outer, old_treatment_inner, rule_result_list))
		{
		//many-to-many
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
			new_treatment->vid = 4097;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
			new_treatment->vid = 4096;
			break;
		//many-to-one
		default:
			new_treatment->vid = (omci_env_g.tag_ds_tci_dont_reverse_workaround == 0) ? 0 : old_filter->vid;
		}
	}

	//tpid/de
	switch (old_filter->tpid_de)
	{
	case OMCIUTIL_VLAN_FILTER_TPID_DE_000:
		//don't care, many-to-one case
		switch (omciutil_vlan_check_many_to_many_tpid(code, old_treatment_outer, old_treatment_inner, rule_result_list))
		{
		//many-to-many
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
			new_treatment->tpid_de= OMCIUTIL_VLAN_TREATMENT_TPID_DE_001;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
			new_treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
			break;
		//many-to-one
		default:
			new_treatment->tpid_de = (omci_env_g.tag_ds_tci_dont_reverse_workaround == 0) ? OMCIUTIL_VLAN_TREATMENT_TPID_DE_100 : old_filter->tpid_de;
		}
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_100:
		// tpid = 0x8100
		new_treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_100;
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_101:
		//de don't care, many-to-one case
		switch (omciutil_vlan_check_many_to_many_de(code, old_treatment_outer, old_treatment_inner, rule_result_list))
		{
		//many-to-many
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
			new_treatment->tpid_de= OMCIUTIL_VLAN_TREATMENT_TPID_DE_1101;
			break;
		case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
			new_treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1100;
			break;
		//many-to-one
		default:
			new_treatment->tpid_de = (omci_env_g.tag_ds_tci_dont_reverse_workaround == 0) ? OMCIUTIL_VLAN_TREATMENT_TPID_DE_1000 : OMCIUTIL_VLAN_TREATMENT_TPID_DE_000;
		}
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_110:
		new_treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1000;
		break;
	case OMCIUTIL_VLAN_FILTER_TPID_DE_111:
		new_treatment->tpid_de = OMCIUTIL_VLAN_TREATMENT_TPID_DE_1001;
		break;
	default:
		new_treatment->tpid_de = old_filter->tpid_de;
		break;
	}

	return 0;
}

#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
static int
omciutil_vlan_translate_treatment_to_fileter_tag(struct omciutil_vlan_tag_fields_t *old_treatment,
	struct omciutil_vlan_tag_fields_t *old_filter_outer,
	struct omciutil_vlan_tag_fields_t *old_filter_inner,
	unsigned char *filter_ethertype,
	struct omciutil_vlan_tag_fields_t *new_filter,
	omciutil_vlan_direct dir)
#else
static int
omciutil_vlan_translate_treatment_to_fileter_tag(struct omciutil_vlan_tag_fields_t *old_treatment,
	struct omciutil_vlan_tag_fields_t *old_filter_outer,
	struct omciutil_vlan_tag_fields_t *old_filter_inner,
	unsigned char *filter_ethertype,
	struct omciutil_vlan_tag_fields_t *new_filter)
#endif
{
	//check input
	if (old_treatment == NULL || old_filter_outer == NULL || old_filter_inner == NULL || new_filter == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//priority
	switch (old_treatment->priority)
	{
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_0:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_1:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_2:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_3:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_4:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_5:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_6:
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_7:
#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
		if( OMCIUTIL_VLAN_DIRECTION_DS == dir )
		{
			// ALU-CHT workaround for MSER (downstream ARP packet always carries pbit=7)
			new_filter->priority = (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU && *filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_ARP) ? OMCIUTIL_VLAN_TREATMENT_PRIORITY_8 : old_treatment->priority;
		}
		else
		{
			new_filter->priority = old_treatment->priority;
		}
#else
		// ALU-CHT workaround for MSER (downstream ARP packet always carries pbit=7)
		new_filter->priority = (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU && *filter_ethertype==OMCIUTIL_VLAN_FILTER_ET_ARP) ? OMCIUTIL_VLAN_TREATMENT_PRIORITY_8 : old_treatment->priority;
#endif
		break;
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_8:
		//from inner
		switch (old_filter_inner->priority)
		{
		case OMCIUTIL_VLAN_FILTER_PRIORITY_0:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_1:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_2:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_3:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_4:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_5:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_6:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_7:
			new_filter->priority = old_filter_inner->priority;
			break;
		case OMCIUTIL_VLAN_FILTER_PRIORITY_8:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_14:
			new_filter->priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
			break;
		default:
			new_filter->priority = OMCIUTIL_VLAN_FILTER_PRIORITY_0;
		}
		break;
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_9:
		//from outer
		switch (old_filter_outer->priority)
		{
		case OMCIUTIL_VLAN_FILTER_PRIORITY_0:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_1:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_2:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_3:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_4:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_5:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_6:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_7:
			new_filter->priority = old_filter_outer->priority;
			break;
		case OMCIUTIL_VLAN_FILTER_PRIORITY_8:
		case OMCIUTIL_VLAN_FILTER_PRIORITY_14:
			new_filter->priority = OMCIUTIL_VLAN_FILTER_PRIORITY_8;
			break;
		default:
			new_filter->priority = OMCIUTIL_VLAN_FILTER_PRIORITY_0;
		}
		break;
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_10: //dscp-2-pbit
		new_filter->priority = OMCIUTIL_VLAN_FILTER_PRIORITY_13;
		break;
	case OMCIUTIL_VLAN_TREATMENT_PRIORITY_15:
		new_filter->priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		break;
	default:
		;
	}

	//vid
	if (old_treatment->vid <= 4095)
	{
		new_filter->vid = old_treatment->vid;
	} else if (old_treatment->vid == 4096) {
		//from inner
		new_filter->vid = old_filter_inner->vid;
	} else if (old_treatment->vid == 4097) {
		//from outer
		new_filter->vid = old_filter_outer->vid;
	} else {
		new_filter->vid = 4096; //don't care
	}

	//tpid/de
	switch (old_treatment->tpid_de)
	{
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_000:
		//Copy TPID (and DE, if present) from inner tag of received frame.
		new_filter->tpid_de = old_filter_inner->tpid_de;
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_001:
		//Copy TPID (and DE, if present) from outer tag of received frame.
		new_filter->tpid_de = old_filter_outer->tpid_de;
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_010:
		//Set TPID = output TPID attribute value, copy DE bit from inner tag of received frame.
		switch (old_filter_inner->tpid_de)
		{
		case OMCIUTIL_VLAN_FILTER_TPID_DE_110:
			//de = 0;
			new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1001;
			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_111:
			//de = 1;
			new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1010;
			break;
		default:
			//don't care
			new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1000;
		}
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_011:
		//Set TPID = output TPID, copy DE from outer tag of received frame.
		switch (old_filter_outer->tpid_de)
		{
		case OMCIUTIL_VLAN_FILTER_TPID_DE_110:
			//de = 0;
			new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1001;
			break;
		case OMCIUTIL_VLAN_FILTER_TPID_DE_111:
			//de = 1;
			new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1010;
			break;
		default:
			//don't care
			new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1000;
		}
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_100:
		//Set TPID = 0x8100.
		new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_100;
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_110:
		//Set TPID = output TPID, DE=0.
		new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1001;
		break;
	case OMCIUTIL_VLAN_TREATMENT_TPID_DE_111:
		//Set TPID = output TPID, DE=1.
		new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_1010;
		break;
	default:
		//don't care
		new_filter->tpid_de = OMCIUTIL_VLAN_FILTER_TPID_DE_000;
	}

	return 0;
}

#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
int
omciutil_vlan_generate_reverse_filter(struct omciutil_vlan_rule_fields_t *rule_fields,
	enum omciutil_vlan_tag_code_t code, 
	struct omciutil_vlan_tag_fields_t *new_filter,
	omciutil_vlan_direct dir)
#else
int
omciutil_vlan_generate_reverse_filter(struct omciutil_vlan_rule_fields_t *rule_fields,
	enum omciutil_vlan_tag_code_t code, 
	struct omciutil_vlan_tag_fields_t *new_filter)
#endif
{
	//check input
	if (rule_fields == NULL || new_filter == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	switch (code)
	{
	case OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER:
		*new_filter = rule_fields->filter_outer;
		break;
	case OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER:
		*new_filter = rule_fields->filter_inner;
		break;
	case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_OUTER:
#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
		if (omciutil_vlan_translate_treatment_to_fileter_tag(&rule_fields->treatment_outer, 
			&rule_fields->filter_outer,
			&rule_fields->filter_inner,
			&rule_fields->filter_ethertype,
			new_filter,
			dir) < 0)
#else
		if (omciutil_vlan_translate_treatment_to_fileter_tag(&rule_fields->treatment_outer, 
			&rule_fields->filter_outer,
			&rule_fields->filter_inner,
			&rule_fields->filter_ethertype,
			new_filter) < 0)
#endif
		{
			dbprintf(LOG_ERR, "translate treatment outer to filter inner error\n");
			return -1;
		} 
		break;
	case OMCIUTIL_VLAN_TAG_CODE_TREATMENT_INNER:
#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
		if (omciutil_vlan_translate_treatment_to_fileter_tag(&rule_fields->treatment_inner, 
			&rule_fields->filter_outer,
			&rule_fields->filter_inner,
			&rule_fields->filter_ethertype,
			new_filter,
			dir) < 0)
#else
		if (omciutil_vlan_translate_treatment_to_fileter_tag(&rule_fields->treatment_inner, 
			&rule_fields->filter_outer,
			&rule_fields->filter_inner,
			&rule_fields->filter_ethertype,
			new_filter) < 0)
#endif
		{
			dbprintf(LOG_ERR, "translate treatment inner to filter inner error\n");
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR, "result code error\n");
		return -1;
	}

	return 0;
}

static int
omciutil_vlan_copy_rule_fields(struct omciutil_vlan_rule_fields_t *des_rule_fields,
	struct omciutil_vlan_rule_fields_t *src_rule_fields)
{
	//check input
	if (des_rule_fields == NULL || src_rule_fields == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	memcpy(des_rule_fields, src_rule_fields, sizeof(struct omciutil_vlan_rule_fields_t));
	//RE-init list head
	INIT_LIST_NODE(&des_rule_fields->rule_node);

	return 0;
}

int
omciutil_vlan_reverse_rule_generate(struct omciutil_vlan_rule_fields_t *rule_fields,
	struct omciutil_vlan_rule_fields_t *rule_fields_reverse)
{
	struct omciutil_vlan_rule_result_list_t rule_result_list;
	struct omciutil_vlan_rule_result_node_t *result_node;

	//check input
	if (rule_fields == NULL || rule_fields_reverse == NULL)
	{
		dbprintf(LOG_ERR, "input error\n");
		return -1;
	}

	//discard frame, reverse is the same
	if (rule_fields->treatment_tag_to_remove >= 3)
	{
		if (omci_env_g.tag_discard_reversing)
		{
			//copy to reverse
			omciutil_vlan_copy_rule_fields(rule_fields_reverse, rule_fields);
			return 0;
		} else {
			return -1;
		}
	}

	//generate result
	if (omciutil_vlan_generate_rule_result(rule_fields, &rule_result_list) < 0)
	{
		dbprintf(LOG_ERR, "could not get rule result\n");
		return -1;
	}

	//according the result tag count, reverse the rule to filter
	switch (rule_result_list.tag_count)
	{
	case 0:
		//fill the default rule for no tag
		rule_fields_reverse->filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		rule_fields_reverse->filter_inner.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;
		rule_fields_reverse->treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		rule_fields_reverse->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
		break;
	case 1:
		//pop-up the top-most one tag for filter inner
		result_node = list_entry(rule_result_list.tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);
#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
		if (omciutil_vlan_generate_reverse_filter(rule_fields, result_node->code, &rule_fields_reverse->filter_inner, 1) < 0)
#else
		if (omciutil_vlan_generate_reverse_filter(rule_fields, result_node->code, &rule_fields_reverse->filter_inner) < 0)
#endif
		{
			//release the result
			omciutil_vlan_release_rule_result(&rule_result_list);
			return -1;
		}

		//filter outer
		rule_fields_reverse->filter_outer.priority = OMCIUTIL_VLAN_FILTER_PRIORITY_15;

		break;
	default: // >= 2
		//pop-up the top-most two tag for filter
		result_node = list_entry(rule_result_list.tag_list.prev, struct omciutil_vlan_rule_result_node_t, node);

		//fill filter outer
#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
		if (omciutil_vlan_generate_reverse_filter(rule_fields, result_node->code, &rule_fields_reverse->filter_outer, 1) < 0)
#else
		if (omciutil_vlan_generate_reverse_filter(rule_fields, result_node->code, &rule_fields_reverse->filter_outer) < 0)
#endif
		{
			//release the result
			omciutil_vlan_release_rule_result(&rule_result_list);
			return -1;
		}

		//get prev one from current result node
		result_node = list_entry(result_node->node.prev, struct omciutil_vlan_rule_result_node_t, node);

		//fill filter inner
#if defined (CONFIG_SUBTYPE_RTL9601C) || defined (CONFIG_SUBTYPE_RTL9601D)
		if (omciutil_vlan_generate_reverse_filter(rule_fields, result_node->code, &rule_fields_reverse->filter_inner, 1) < 0)
#else
		if (omciutil_vlan_generate_reverse_filter(rule_fields, result_node->code, &rule_fields_reverse->filter_inner) < 0)
#endif
		{
			//release the result
			omciutil_vlan_release_rule_result(&rule_result_list);
			return -1;
		}
	}

	//copy the ether type field
	rule_fields_reverse->filter_ethertype = rule_fields->filter_ethertype;

	//reverse tags to remove
	if (rule_fields->treatment_tag_to_remove == 3)
	{
		rule_fields_reverse->treatment_tag_to_remove = 3;
	} else {
		if (rule_fields->treatment_inner.priority != OMCIUTIL_VLAN_TREATMENT_PRIORITY_15)
		{
			rule_fields_reverse->treatment_tag_to_remove++;
		}
		if (rule_fields->treatment_outer.priority != OMCIUTIL_VLAN_TREATMENT_PRIORITY_15)
		{
			rule_fields_reverse->treatment_tag_to_remove++;
		}
	}
 
	//set default treatment first
	rule_fields_reverse->treatment_outer.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;
	rule_fields_reverse->treatment_inner.priority = OMCIUTIL_VLAN_TREATMENT_PRIORITY_15;

	switch (rule_fields->treatment_tag_to_remove)
	{
	case 1:
		//add a tag translate
		if (rule_fields->filter_outer.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15)
		{
			//translate filter outer to treatment inner
			if (omciutil_vlan_translate_filter_to_treatment_tag(OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER,
				&rule_fields->filter_outer,
				&rule_fields->treatment_outer,
				&rule_fields->treatment_inner,
				&rule_result_list,
				&rule_fields_reverse->treatment_inner) < 0)
			{
				dbprintf(LOG_ERR, "translate filter outer to treatment inner error\n");
				//release the result
				omciutil_vlan_release_rule_result(&rule_result_list);
				return -1;
			}
		} else if (rule_fields->filter_inner.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15) {
			//translate filter inner to treatment inner
			if (omciutil_vlan_translate_filter_to_treatment_tag(OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER,
				&rule_fields->filter_inner,
				&rule_fields->treatment_outer,
				&rule_fields->treatment_inner,
				&rule_result_list,
				&rule_fields_reverse->treatment_inner) < 0)
			{
				dbprintf(LOG_ERR, "translate filter inner to treatment inner error\n");
				//release the result
				omciutil_vlan_release_rule_result(&rule_result_list);
				return -1;
			}
		} else {
			//rule error
			dbprintf(LOG_ERR, "translate treatment error\n");
			//release the result
			omciutil_vlan_release_rule_result(&rule_result_list);
			return -1;
		}
		break;
	case 2:
		//add a tag translate
		if (rule_fields->filter_outer.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15)
		{
			//translate filter outer to treatment outer
			if (omciutil_vlan_translate_filter_to_treatment_tag(OMCIUTIL_VLAN_TAG_CODE_FILTER_OUTER,
				&rule_fields->filter_outer,
				&rule_fields->treatment_outer,
				&rule_fields->treatment_inner,
				&rule_result_list,
				&rule_fields_reverse->treatment_outer) < 0)
			{
				dbprintf(LOG_ERR, "translate filter outer to treatment outer error\n");
				//release the result
				omciutil_vlan_release_rule_result(&rule_result_list);
				return -1;
			}
		} else {
			//rule error
			dbprintf(LOG_ERR, "translate treatment error\n");
			//release the result
			omciutil_vlan_release_rule_result(&rule_result_list);
			return -1;
		}
		if (rule_fields->filter_inner.priority != OMCIUTIL_VLAN_FILTER_PRIORITY_15) {
			//translate filter inner to treatment inner
			if (omciutil_vlan_translate_filter_to_treatment_tag(OMCIUTIL_VLAN_TAG_CODE_FILTER_INNER,
				&rule_fields->filter_inner,
				&rule_fields->treatment_outer,
				&rule_fields->treatment_inner,
				&rule_result_list,
				&rule_fields_reverse->treatment_inner) < 0)
			{
				dbprintf(LOG_ERR, "translate filter outer to treatment outer error\n");
				//release the result
				omciutil_vlan_release_rule_result(&rule_result_list);
				return -1;
			}
		} else {
			//rule error
			dbprintf(LOG_ERR, "translate treatment error\n");
			//release the result
			omciutil_vlan_release_rule_result(&rule_result_list);
			return -1;
		}
		break;
	default:
		;//do nothing
	}

	//copy the tpid
	rule_fields_reverse->input_tpid = rule_fields->input_tpid;
	rule_fields_reverse->output_tpid = rule_fields->output_tpid;

	//release the result
	omciutil_vlan_release_rule_result(&rule_result_list);

	return 0;
}

