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
 * Module  : proprietary_calix
 * File    : er_group_hw_45_47_49_79_251.c
 *
 ******************************************************************/
#include <string.h>
#include "util.h"
#include "er_group.h"
#include "me_related.h"
#include "switch.h"
#include "batchtab.h"
#include "proprietary_calix.h"


static void
rma_forward_mask_set(unsigned long long l2cp_mask, unsigned char bmi, unsigned char bpdu, unsigned char garp, unsigned char all_lans)
{
	unsigned long long rma_forward_mask = omci_env_g.rma_forward_mask;
	int i = 0;
	
	// 1. Forward all: ME79 forward. ME49 empty. ME251 forward.
	// 2. Deny all: ME79 filter. ME49 empty. ME251 filter.
	// 3. Forward all, deny some: ME79 forward. ME49 has filter table. ME251 don't care.
	// 4. Deny all, forward some: ME79 filter. ME49 has forward table. ME251 don't care.
	
	if(l2cp_mask != 0xffffffffffffffffULL) { // me49 not empty case, do not check me251
		if(bmi) { // if me79 is forwarded, check whether me49 is filter table
			rma_forward_mask |= 0xffffffffffffffffULL;
			rma_forward_mask &= l2cp_mask;
		} else { // if me79 is filtered, check whether me49 is forward table
			rma_forward_mask &= ~0xffffffffffffffffULL;
			rma_forward_mask |= l2cp_mask;
		}
	} else { // me49 empty case, check me251 (upstream only)
		rma_forward_mask = (bmi) ? rma_forward_mask | 0xffffffffffffffffULL : rma_forward_mask & ~0xffffffffffffffffULL;
		//rma_forward_mask = (bpdu) ? rma_forward_mask | 0xffffULL : rma_forward_mask & ~0xffffULL;
		//rma_forward_mask = (garp) ? rma_forward_mask | 0xffff00000000ULL : rma_forward_mask & ~0xffff00000000ULL;
		//rma_forward_mask = (all_lans) ? rma_forward_mask | 0x10000ULL : rma_forward_mask & ~0x10000ULL;
	}
	//rma_forward_mask &= ~0x2ULL; // disable flow control bit by default (01:80:c2:00:00:01)
	
	omci_env_g.rma_forward_mask = rma_forward_mask;
	for(i=0; i<64; i++) {
		if(switch_hw_g.rma_action_set != NULL)
			switch_hw_g.rma_action_set(i, (1ULL<<i & omci_env_g.rma_forward_mask) ? 0 : 1); // 0: forward, 1: drop
	}
}

// 45@meid
// 47@3
// 49@1
// 79@8
// 251@meid,10,11,12
int
er_group_hw_l2cp_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char bmi = 1, bpdu = 1, garp = 1, all_lans = 1;
	unsigned long long l2cp_mask = 0xffffffffffffffffULL;
	struct attr_table_header_t *mac_filter_table_header = NULL;
	struct attr_table_entry_t *table_entry_pos = NULL;
	struct me_t *bridge_me = meinfo_me_get_by_meid(45, (unsigned short)attr_inst->er_attr_instance[0].attr_value.data);
	struct me_t *me251 = meinfo_me_get_by_meid(251, (unsigned short)attr_inst->er_attr_instance[4].attr_value.data);
	unsigned char tp_type = (unsigned char)attr_inst->er_attr_instance[1].attr_value.data;
	
	if (attr_inst == NULL)
		return -1;
	
	if(bridge_me == NULL || me251 == NULL)
		return -2;
	
	if(!me_related_251_045(me251, bridge_me))
		return -3;
	
	mac_filter_table_header = (struct attr_table_header_t *)attr_inst->er_attr_instance[2].attr_value.ptr;
	bmi = (unsigned char)attr_inst->er_attr_instance[3].attr_value.data ? 0 : 1; // 0: forward, 1: filter
	bpdu = (unsigned char)attr_inst->er_attr_instance[5].attr_value.data; // 1: forward, 0: filter
	garp = (unsigned char)attr_inst->er_attr_instance[6].attr_value.data; // 1: forward, 0: filter
	all_lans = (unsigned char)attr_inst->er_attr_instance[7].attr_value.data; // 1: forward, 0: filter
	if(bmi==0 && l2cp_mask==0xffffffffffffffffULL) l2cp_mask = 0; // initial l2cp_mask according to bmi (filter or forward)
	if(mac_filter_table_header) {
		list_for_each_entry(table_entry_pos, &mac_filter_table_header->entry_list, entry_node) {
			if (table_entry_pos->entry_data_ptr != NULL) {
				unsigned char add = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 8, 2);
				unsigned char dir = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 14, 1);
				unsigned char filter = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 15, 1);
				unsigned char i, mac_addr[6], l2cp_addr[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x00};
				unsigned long long mask = (bmi==1)?0xffffffffffffffffULL:0;
				for (i=0; i<6; i++) mac_addr[i] = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 16+i*8, 8);
				if(memcmp(l2cp_addr, mac_addr, 5) == 0) { // l2cp
					mask = ((add==1) & (filter==1) & (dir==0)) ? mask & ~(1ULL << mac_addr[5]) : mask | (1ULL << mac_addr[5]);
					l2cp_mask = (bmi==1) ? (l2cp_mask&mask) : (l2cp_mask|mask);
				}
			}
		}
	}

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(tp_type == 1) // for egress uni mac bridge port, set global env rma_forward_mask for downstream ingress filter
			rma_forward_mask_set(l2cp_mask, bmi, bpdu, garp, all_lans);
		else if(tp_type == 3 || tp_type == 5) // for egress ani mac bridge port, use per-uni port rma_forward_mask for upstream ingress filter
			batchtab_omci_update("calix_l2cp");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:		
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_l2cp_enable(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}

	return 0;
}
