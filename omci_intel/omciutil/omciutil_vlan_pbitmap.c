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
 * File    : omciutil_vlan_pbitmap.c
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

static int
omciutil_vlan_update_pbitmap_by_rule_list(char *pbitmap, struct omciutil_vlan_rule_fields_t *rule_fields)
{
	int tagnum_input=-1;
	int tagnum_output=-1;
	int pbit_input=-1;
	int pbit_output=-1;
	int is_default_rule=0, i;

	if (rule_fields==NULL)
		return -1;
		
	// match
	if (rule_fields->filter_outer.priority==15) {
		if (rule_fields->filter_inner.priority==15) {
			tagnum_input=0;
			is_default_rule=1;
		} else if (rule_fields->filter_inner.priority==14) {
			tagnum_input=1;
			is_default_rule=1;
		} else if (rule_fields->filter_inner.priority<=8) {
			tagnum_input=1;
		}
	} else if (rule_fields->filter_outer.priority==14 && rule_fields->filter_inner.priority==14) {
		tagnum_input=2;
		is_default_rule=2;
	} else if (rule_fields->filter_outer.priority<=8 && rule_fields->filter_inner.priority<=8) {
		tagnum_input=2;
	}
	if (tagnum_input<0)
		return -2;	// tagnum input can not be determined, invalid param?

	// treat
	if (rule_fields->treatment_tag_to_remove==3)	// drop
		return 1;	// output is drop
	tagnum_output=tagnum_input-rule_fields->treatment_tag_to_remove;
	if (tagnum_output<0)
		tagnum_output=0;
	if (rule_fields->treatment_outer.priority!=15) {
		if (rule_fields->treatment_inner.priority!=15)
			tagnum_output+=2;
		else
			tagnum_output++;
	} else {
		if (rule_fields->treatment_inner.priority!=15)
			tagnum_output++;
	}	
	if (tagnum_output==0)
		return 2;	// tagnum output is 0, no update to pbitmap


	// match outer pbit
	if (rule_fields->filter_outer.priority<=7)
		pbit_input=rule_fields->filter_outer.priority;
	// match inner pbit
	if (pbit_input==-1) {
		if (rule_fields->filter_inner.priority<=7)
			pbit_input=rule_fields->filter_inner.priority;
	}
	
	//treatment outer pbit
	if (rule_fields->treatment_outer.priority<=7) {
		pbit_output=rule_fields->treatment_outer.priority;
	} else if (rule_fields->treatment_outer.priority==8) {	// opri=ipri
		if (rule_fields->filter_inner.priority<=7)
			pbit_output=rule_fields->filter_inner.priority;
	} else if (rule_fields->treatment_outer.priority==9) {	// opri=opri
		if (rule_fields->filter_outer.priority<=7)
			pbit_output=rule_fields->filter_outer.priority;
	}
	//treatment inner pbit
	if (pbit_output==-1) {
		if (rule_fields->treatment_inner.priority<=7) {
			pbit_output=rule_fields->treatment_inner.priority;
		} else if (rule_fields->treatment_inner.priority==8) {	// ipri=ipri
			if (rule_fields->filter_inner.priority<=7)
				pbit_output=rule_fields->filter_inner.priority;
		} else if (rule_fields->treatment_inner.priority==9) {	// ipri=opri
			if (rule_fields->filter_outer.priority<=7)
				pbit_output=rule_fields->filter_outer.priority;
		}
	}
	
	if (pbit_output>=0 && pbit_output<8) {
		if (is_default_rule || pbit_input==-1) {
			dbprintf(LOG_WARNING, "pbit 0..7 -> %u\n", pbit_output);
			for (i=0; i<8; i++)
				pbitmap[i]=pbit_output;
		} else {
			dbprintf(LOG_WARNING, "pbit %u -> %u\n", pbit_input, pbit_output);
			pbitmap[pbit_input]=pbit_output;
		}
		return 0;
	}
	
	return 3;	// output pbit can not be determined
}

int
omciutil_vlan_pbitmap_ds_get(struct me_t *me_bport, char *pbitmap, unsigned char tag_mode)
{
	struct list_head rule_list_us, rule_list_ds;
	struct omciutil_vlan_rule_fields_t *rule_fields_pos;
	int i;
	
	// init pbitmap
	for (i=0; i<8; i++)
		pbitmap[i]=i;		

	if (omciutil_vlan_collect_rules_by_port(ER_ATTR_GROUP_HW_OP_NONE, me_bport, NULL, &rule_list_us, &rule_list_ds, tag_mode) < 0)
		return -1;

	// we traverse the rule list in reverse order so more specific rule will override the default rule
	list_for_each_entry_reverse(rule_fields_pos, &rule_list_ds, rule_node)
		omciutil_vlan_update_pbitmap_by_rule_list(pbitmap, rule_fields_pos);

	//release all in list
	omciutil_vlan_release_rule_list(&rule_list_us);
	omciutil_vlan_release_rule_list(&rule_list_ds);
	return 0;
}

