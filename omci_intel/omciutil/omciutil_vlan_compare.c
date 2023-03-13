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
 * File    : omciutil_vlan_compare.c
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

//return value: 0: not, 1: yes
static int
omciutil_vlan_is_default_rule(struct omciutil_vlan_rule_fields_t *rule_fields)
{
	if ((rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15 &&
		rule_fields->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15 &&
		rule_fields->filter_ethertype == OMCIUTIL_VLAN_FILTER_ET_IGNORE) || //no tags, Ethertype don't care
		(rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_15 &&
		rule_fields->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14 &&
		rule_fields->filter_ethertype == OMCIUTIL_VLAN_FILTER_ET_IGNORE) || //1 tag, Ethertype don't care
		(rule_fields->filter_outer.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14 &&
		rule_fields->filter_inner.priority == OMCIUTIL_VLAN_FILTER_PRIORITY_14 &&
		rule_fields->filter_ethertype == OMCIUTIL_VLAN_FILTER_ET_IGNORE)) //2 tags, Ethertype don't care
	{
		return 1;
	} else {
		return 0;
	}
}

//return value: -1: rule_fields1 < rule_fields2, 0: NOT one and only one is default rule, 1: rule_fields1 > rule_fields2
static int
omciutil_vlan_compare_rule_fields_default_rule(struct omciutil_vlan_rule_fields_t *rule_fields1,
	struct omciutil_vlan_rule_fields_t *rule_fields2)
{
	if ((omciutil_vlan_is_default_rule(rule_fields1) ^
		omciutil_vlan_is_default_rule(rule_fields2)) == 1)
	{
		if (omciutil_vlan_is_default_rule(rule_fields1))
		{
			return 1;
		} else {
			return -1;
		}
	} else {
		return 0;
	}
}

//return value: -1: rule_fields1 < rule_fields2, 0: rule_fields1 == rule_fields2, 1: rule_fields1 > rule_fields2
int
omciutil_vlan_compare_rule_fields(struct omciutil_vlan_rule_fields_t *rule_fields1,
	struct omciutil_vlan_rule_fields_t *rule_fields2)
{
	int ret;

	//check priority first
	if (rule_fields1->sort_priority != rule_fields2->sort_priority)
	{
		if (rule_fields1->sort_priority > rule_fields2->sort_priority)
		{
			return -1;
		} else {
			return 1;
		}
	}

	if (rule_fields1->l2_filter.index != rule_fields2->l2_filter.index)
	{
		if (rule_fields1->l2_filter.index < rule_fields2->l2_filter.index)
		{
			return -1;
		} else {
			return 1;
		}
	}

	if ((ret = omciutil_vlan_compare_rule_fields_default_rule(rule_fields1, rule_fields2)) != 0)
	{
		return ret;
	}

	//filter outer
	if (rule_fields1->filter_outer.priority != rule_fields2->filter_outer.priority)
	{
		if (rule_fields1->filter_outer.priority < rule_fields2->filter_outer.priority)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_fields1->filter_outer.vid != rule_fields2->filter_outer.vid)
	{
		if (rule_fields1->filter_outer.vid < rule_fields2->filter_outer.vid)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_fields1->filter_outer.tpid_de != rule_fields2->filter_outer.tpid_de)
	{
		//greater code value is more specific
		if (rule_fields1->filter_outer.tpid_de< rule_fields2->filter_outer.tpid_de)
		{
			return 1;
		} else {
			return -1;
		}
	} else {
		if (rule_fields1->filter_outer.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_101 ||
			rule_fields1->filter_outer.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_110 ||
			rule_fields1->filter_outer.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_111)
		{
			//compare input_tpid
			if (rule_fields1->input_tpid != rule_fields2->input_tpid)
			{
				if (rule_fields1->input_tpid < rule_fields2->input_tpid)
				{
					return -1;
				} else {
					return 1;
				}
			}
		}
		if (rule_fields1->filter_outer.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_1000 ||
			rule_fields1->filter_outer.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_1001 ||
			rule_fields1->filter_outer.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_1010)
		{
			//compare output_tpid
			if (rule_fields1->output_tpid != rule_fields2->output_tpid)
			{
				if (rule_fields1->output_tpid < rule_fields2->output_tpid)
				{
					return -1;
				} else {
					return 1;
				}
			}
		}
	}

	//filter inner
	if (rule_fields1->filter_inner.priority != rule_fields2->filter_inner.priority)
	{
		if (rule_fields1->filter_inner.priority < rule_fields2->filter_inner.priority)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_fields1->filter_inner.vid != rule_fields2->filter_inner.vid)
	{
		if (rule_fields1->filter_inner.vid < rule_fields2->filter_inner.vid)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_fields1->filter_inner.tpid_de != rule_fields2->filter_inner.tpid_de)
	{
		//greater code value is more specific
		if (rule_fields1->filter_inner.tpid_de< rule_fields2->filter_inner.tpid_de)
		{
			return 1;
		} else {
			return -1;
		}
	} else {
		if (rule_fields1->filter_inner.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_101 ||
			rule_fields1->filter_inner.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_110 ||
			rule_fields1->filter_inner.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_111)
		{
			//compare input_tpid
			if (rule_fields1->input_tpid != rule_fields2->input_tpid)
			{
				if (rule_fields1->input_tpid < rule_fields2->input_tpid)
				{
					return -1;
				} else {
					return 1;
				}
			}
		}
		if (rule_fields1->filter_inner.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_1000 ||
			rule_fields1->filter_inner.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_1001 ||
			rule_fields1->filter_inner.tpid_de == OMCIUTIL_VLAN_FILTER_TPID_DE_1010)
		{
			//compare output_tpid
			if (rule_fields1->output_tpid != rule_fields2->output_tpid)
			{
				if (rule_fields1->output_tpid < rule_fields2->output_tpid)
				{
					return -1;
				} else {
					return 1;
				}
			}
		}
	}

	//ethertype
	if (rule_fields1->filter_ethertype != rule_fields2->filter_ethertype)
	{
		//greater code value is more specific
		if (rule_fields1->filter_ethertype < rule_fields2->filter_ethertype)
		{
			return 1;
		} else {
			return -1;
		}
	}

	//tags to remove
	if (rule_fields1->treatment_tag_to_remove != rule_fields2->treatment_tag_to_remove)
	{
		if (rule_fields1->treatment_tag_to_remove < rule_fields2->treatment_tag_to_remove)
		{
			return -1;
		} else {
			return 1;
		}
	}

	//treatment outer
	if (rule_fields1->treatment_outer.priority != rule_fields2->treatment_outer.priority)
	{
		if (rule_fields1->treatment_outer.priority < rule_fields2->treatment_outer.priority)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_fields1->treatment_outer.vid != rule_fields2->treatment_outer.vid)
	{
		if (rule_fields1->treatment_outer.vid < rule_fields2->treatment_outer.vid)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_fields1->treatment_outer.tpid_de != rule_fields2->treatment_outer.tpid_de)
	{
		if (rule_fields1->treatment_outer.tpid_de< rule_fields2->treatment_outer.tpid_de)
		{
			return -1;
		} else {
			return 1;
		}
	} else {
		if (rule_fields1->treatment_outer.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_010 ||
			rule_fields1->treatment_outer.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_011 ||
			rule_fields1->treatment_outer.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_110 ||
			rule_fields1->treatment_outer.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_111)
		{
			//compare input_tpid
			if (rule_fields1->output_tpid != rule_fields2->output_tpid)
			{
				if (rule_fields1->output_tpid < rule_fields2->output_tpid)
				{
					return -1;
				} else {
					return 1;
				}
			}
		}
		if (rule_fields1->treatment_outer.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_1000 ||
			rule_fields1->treatment_outer.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_1001)
		{
			//compare output_tpid
			if (rule_fields1->input_tpid != rule_fields2->input_tpid)
			{
				if (rule_fields1->input_tpid < rule_fields2->input_tpid)
				{
					return -1;
				} else {
					return 1;
				}
			}
		}
	}

	//treatment inner
	if (rule_fields1->treatment_inner.priority != rule_fields2->treatment_inner.priority)
	{
		if (rule_fields1->treatment_inner.priority < rule_fields2->treatment_inner.priority)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_fields1->treatment_inner.vid != rule_fields2->treatment_inner.vid)
	{
		if (rule_fields1->treatment_inner.vid < rule_fields2->treatment_inner.vid)
		{
			return -1;
		} else {
			return 1;
		}
	}
	if (rule_fields1->treatment_inner.tpid_de != rule_fields2->treatment_inner.tpid_de)
	{
		if (rule_fields1->treatment_inner.tpid_de< rule_fields2->treatment_inner.tpid_de)
		{
			return -1;
		} else {
			return 1;
		}
	} else {
		if (rule_fields1->treatment_inner.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_010 ||
			rule_fields1->treatment_inner.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_011 ||
			rule_fields1->treatment_inner.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_110 ||
			rule_fields1->treatment_inner.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_111)
		{
			//compare input_tpid
			if (rule_fields1->output_tpid != rule_fields2->output_tpid)
			{
				if (rule_fields1->output_tpid < rule_fields2->output_tpid)
				{
					return -1;
				} else {
					return 1;
				}
			}
		}
		if (rule_fields1->treatment_inner.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_1000 ||
			rule_fields1->treatment_inner.tpid_de == OMCIUTIL_VLAN_TREATMENT_TPID_DE_1001)
		{
			//compare output_tpid
			if (rule_fields1->input_tpid != rule_fields2->input_tpid)
			{
				if (rule_fields1->input_tpid < rule_fields2->input_tpid)
				{
					return -1;
				} else {
					return 1;
				}
			}
		}
	}

	return 0;
}

