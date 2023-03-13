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
 * Module  : proprietary_alu
 * File    : batchtab_cb_alu_ds_mcastvlan.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "meinfo.h"
#include "me_related.h"
#include "list.h"
#include "util.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "proprietary_alu.h"

static int alu_ds_mcastvlan_total_g = 0;

////////////////////////////////////////////////////

static int
check_default_rule_exists(void)
{
	struct meinfo_t *miptr = meinfo_util_miptr(171);
	struct me_t *meptr = NULL;
	struct attr_table_header_t *table_header = NULL;	
	struct attr_table_entry_t *table_entry_pos;
	//unsigned short filter_inner_vid = 0xFFFF;
	unsigned short treatment_inner_vid = 0xFFFF;
	//unsigned char filter_inner_pbit = 0xFF, treatment_inner_pbit = 0xFF;
	
	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
		if ((table_header = (struct attr_table_header_t *) meinfo_util_me_attr_ptr(meptr, 6)) == NULL)
			continue;
		
		if(meinfo_util_me_attr_data(meptr, 1) != 10) // Only check VEIP related
			continue;
		
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
		{
			if (table_entry_pos->entry_data_ptr != NULL)
			{
				//filter_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 36, 13);
				//filter_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 32, 4);
				treatment_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 112, 13);
				//treatment_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 108, 4);
				
				if(treatment_inner_vid == 4095) { // Default rule VID
					return 1;
				}
			}
		}
	}
	return 0;
}

static int
batchtab_cb_alu_ds_mcastvlan_init(void)
{
	alu_ds_mcastvlan_total_g = 0;
	return 0;
}

static int
batchtab_cb_alu_ds_mcastvlan_finish(void)
{
	alu_ds_mcastvlan_total_g = 0;
	return 0;
}

static int
batchtab_cb_alu_ds_mcastvlan_gen(void **table_data)
{
	struct meinfo_t *miptr = NULL;
   	struct me_t *meptr = NULL;
	
	if(omci_env_g.olt_workaround_enable != ENV_OLT_WORKAROUND_ALU)
		return 0;
	
	// Workaround for ALU OLT which cannot remove DS multicast tag on VEIP
	if(check_default_rule_exists()==0) {
		// 329->47->310->309
		miptr = meinfo_util_miptr(329);
	   	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
	   		struct meinfo_t *miptr_47 = meinfo_util_miptr(47);
	   		struct me_t *meptr_47 = NULL;
	   		list_for_each_entry(meptr_47, &miptr_47->me_instance_list, instance_node) {
	   			if(me_related(meptr_47, meptr)) {
					struct meinfo_t *miptr_310 = meinfo_util_miptr(310);
				   	struct me_t *meptr_310 = NULL;
				   	list_for_each_entry(meptr_310, &miptr_310->me_instance_list, instance_node) {
			   			if(me_related(meptr_310, meptr_47)) {
							struct meinfo_t *miptr_309 = meinfo_util_miptr(309);
						   	struct me_t *meptr_309 = NULL;
						   	list_for_each_entry(meptr_309, &miptr_309->me_instance_list, instance_node) {
					   			if(me_related(meptr_309, meptr_310)) {
						   			struct attr_value_t attr = {0, NULL};
								   	unsigned char attr_mask[2]={0, 0};
									meinfo_me_attr_get(meptr_309, 16, &attr);
									util_bitmap_set_value(attr.ptr, 3*8, 0, 1*8, 1); // 1: Remove tag
									meinfo_me_attr_set(meptr_309, 16, &attr, 0);
									util_attr_mask_set_bit(attr_mask, 16);
									meinfo_me_flush(meptr_309, attr_mask);
									alu_ds_mcastvlan_total_g++;
									break;
								}
							}
							break;
						}
					}
					break;
				}
			}
		}
	}
	
	// Workaround for ALU OLT 7342 R4.8 only, remove DS multicast tag on UNI
	if(proprietary_alu_get_olt_version() == 0) {
		// 11->47->310->309
		miptr = meinfo_util_miptr(11);
	   	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
	   		struct meinfo_t *miptr_65297 = meinfo_util_miptr(65297);
	   		struct me_t *meptr_65297 = NULL;
	   		struct meinfo_t *miptr_47 = meinfo_util_miptr(47);
	   		struct me_t *meptr_47 = NULL;
	   		unsigned char ds_tag_mode = 0;
	   		
	   		// Find ME65297 to check ds_tag_mode
			list_for_each_entry(meptr_65297, &miptr_65297->me_instance_list, instance_node) {
				if(me_related(meptr_65297, meptr)) {
					ds_tag_mode = meinfo_util_me_attr_data(meptr_65297, 1);
					break;
				}
			}
	   		
	   		list_for_each_entry(meptr_47, &miptr_47->me_instance_list, instance_node) {
	   			if(me_related(meptr_47, meptr)) {
					struct meinfo_t *miptr_310 = meinfo_util_miptr(310);
				   	struct me_t *meptr_310 = NULL;
				   	list_for_each_entry(meptr_310, &miptr_310->me_instance_list, instance_node) {
			   			if(me_related(meptr_310, meptr_47)) {
							struct meinfo_t *miptr_309 = meinfo_util_miptr(309);
						   	struct me_t *meptr_309 = NULL;
						   	list_for_each_entry(meptr_309, &miptr_309->me_instance_list, instance_node) {
					   			if(me_related(meptr_309, meptr_310)) {
						   			if(ds_tag_mode == 1) { // 1: Strip tag
										struct attr_value_t attr = {0, NULL};
										unsigned char attr_mask[2]={0, 0};
										meinfo_me_attr_get(meptr_309, 16, &attr);
										util_bitmap_set_value(attr.ptr, 3*8, 0, 1*8, 1); // 1: Remove tag
										meinfo_me_attr_set(meptr_309, 16, &attr, 0);
										util_attr_mask_set_bit(attr_mask, 16);
										meinfo_me_flush(meptr_309, attr_mask);
										alu_ds_mcastvlan_total_g++;
									}
									break;
								}
							}
							break;
						}
					}
					break;
				}
			}
		}
	}
	
	*table_data = &alu_ds_mcastvlan_total_g;
	return 0;
}

static int
batchtab_cb_alu_ds_mcastvlan_release(void *table_data)
{
	return 0;
}

static int
batchtab_cb_alu_ds_mcastvlan_dump(int fd, void *table_data)
{
	int *total = table_data;
	
	util_fdprintf(fd, "[alu_ds_mcastvlan] found = %d\n", (table_data) ? *total : 0);
	return 0;
}

static int
batchtab_cb_alu_ds_mcastvlan_hw_sync(void *table_data)
{
	return 0;
}

// public register function /////////////////////////////////////
int
batchtab_cb_alu_ds_mcastvlan_register(void)
{
	return batchtab_register("alu_ds_mcastvlan", BATCHTAB_OMCI_UPDATE_NOAUTO, 3,
		batchtab_cb_alu_ds_mcastvlan_init,
		batchtab_cb_alu_ds_mcastvlan_finish,
		batchtab_cb_alu_ds_mcastvlan_gen,
		batchtab_cb_alu_ds_mcastvlan_release,
		batchtab_cb_alu_ds_mcastvlan_dump,
		batchtab_cb_alu_ds_mcastvlan_hw_sync);
}

