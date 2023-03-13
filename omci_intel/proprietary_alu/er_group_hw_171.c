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
 * Module  : er_group_hw
 * File    : er_group_hw_171.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "switch.h"
#include "tm.h"
#include "vacl.h"
#include "batchtab.h"
#include "proprietary_alu.h"

#define VACL_METER_ORDER_IPOE	29
#define VACL_METER_ORDER_PPPOE	30

// 171 Extended_VLAN_tagging_operation_configuration_data

static int
er_group_hw_vlan_to_td(unsigned int vid, unsigned char pbit, unsigned int *us_pir, unsigned int *ds_pir, struct me_t **ustd_meptr, struct me_t **dstd_meptr)
{
	// 171->84->47->(130)->266->268->280
	struct meinfo_t *miptr = meinfo_util_miptr(84);
	struct me_t *meptr_84 = NULL;
	int is_found = 0;
	
	// 171->84
	list_for_each_entry(meptr_84, &miptr->me_instance_list, instance_node) {
		unsigned char *bitmap = meinfo_util_me_attr_ptr(meptr_84, 1);
		if((meinfo_util_me_attr_data(meptr_84, 3) == 1) &&
		   (vid == util_bitmap_get_value(bitmap, 24*8, 4, 12))) {
			is_found = 1;
			//dbprintf(LOG_ERR,"84=0x%04x\n",meptr_84->meid);
			break;
		}
	}
	if(is_found) {
		// 84->47
		struct me_t *meptr_47 = meinfo_me_get_by_meid(47, meptr_84->meid);
		unsigned char tp_type = 0;
		if(meptr_47) {
			tp_type = meinfo_util_me_attr_data(meptr_47, 3);
			//dbprintf(LOG_ERR,"47=0x%04x tp_type=%d\n",meptr_47->meid, tp_type);
		}
		if(meptr_47 && tp_type == 3) { // 802.1p
			// 47->130
			struct me_t *meptr_130 = meinfo_me_get_by_meid(130, meinfo_util_me_attr_data(meptr_47, 4));
			if(meptr_130) {
				//dbprintf(LOG_ERR,"130=0x%04x\n",meptr_130->meid);
					// 130->266
				struct me_t *meptr_266 = meinfo_me_get_by_meid(266, meinfo_util_me_attr_data(meptr_130 , pbit+2));
				if(meptr_266) {
					// 266->268
					struct me_t *meptr_268 = meinfo_me_get_by_meid(268, meinfo_util_me_attr_data(meptr_266 , 1));
					//dbprintf(LOG_ERR,"266=0x%04x\n",meptr_266->meid);
					if(meptr_268) {
						// 268->280
						struct me_t *td_us = meinfo_me_get_by_meid(280, meinfo_util_me_attr_data(meptr_268, 5));
						struct me_t *td_ds = meinfo_me_get_by_meid(280, meinfo_util_me_attr_data(meptr_268, 9));
						//dbprintf(LOG_ERR,"268=0x%04x\n",meptr_268->meid);
						if(td_us != NULL) {
							//dbprintf(LOG_ERR,"us 280=0x%04x\n",td_us->meid);
							*us_pir = meinfo_util_me_attr_data(td_us, 2);
							*ustd_meptr = td_us;
						}
						if(td_ds != NULL) {
							//dbprintf(LOG_ERR,"ds 280=0x%04x\n",td_ds->meid);
							*ds_pir = meinfo_util_me_attr_data(td_ds, 2);
							*dstd_meptr = td_ds;
						}
					}
				}
			}
		} else if (meptr_47 && tp_type == 5) { // GEM IWTP
			// 47->266
			struct me_t *meptr_266 = meinfo_me_get_by_meid(266, meinfo_util_me_attr_data(meptr_47 , 4));
			if(meptr_266) {
				// 266->268
				struct me_t *meptr_268 = meinfo_me_get_by_meid(268, meinfo_util_me_attr_data(meptr_266 , 1));
				//dbprintf(LOG_ERR,"266=0x%04x\n",meptr_266->meid);
				if(meptr_268) {
					// 268->280
					struct me_t *td_us = meinfo_me_get_by_meid(280, meinfo_util_me_attr_data(meptr_268, 5));
					struct me_t *td_ds = meinfo_me_get_by_meid(280, meinfo_util_me_attr_data(meptr_268, 9));
					//dbprintf(LOG_ERR,"268=0x%04x\n",meptr_268->meid);
					if(td_us != NULL) {
						//dbprintf(LOG_ERR,"us 280=0x%04x\n",td_us->meid);
						*us_pir = meinfo_util_me_attr_data(td_us, 2);
						*ustd_meptr = td_us;
					}
					if(td_ds != NULL) {
						//dbprintf(LOG_ERR,"ds 280=0x%04x\n",td_ds->meid);
						*ds_pir = meinfo_util_me_attr_data(td_ds, 2);
						*dstd_meptr = td_ds;
					}
				}
			}
		}
	}
	return 0;
}

static struct me_t*
er_group_hw_vlan_to_gem(unsigned int vid, unsigned char pbit, int *gem_port)
{
	// 171->84->47->(130)->266->268
	struct meinfo_t *miptr = meinfo_util_miptr(84);
	struct me_t *meptr_84 = NULL, *ret = NULL;
	int is_found = 0;
	
	// 171->84
	list_for_each_entry(meptr_84, &miptr->me_instance_list, instance_node) {
		unsigned char *bitmap = meinfo_util_me_attr_ptr(meptr_84, 1);
		if((meinfo_util_me_attr_data(meptr_84, 3) == 1) &&
		    (vid == util_bitmap_get_value(bitmap, 24*8, 4, 12))) {
			is_found = 1;
			//dbprintf(LOG_ERR,"84=0x%04x\n",meptr_84->meid);
			break;
		}
	}
	if(is_found) {
		// 84->47
		struct me_t *meptr_47 = meinfo_me_get_by_meid(47, meptr_84->meid);
		unsigned char tp_type = 0;
		if(meptr_47) {
			tp_type = meinfo_util_me_attr_data(meptr_47, 3);
			//dbprintf(LOG_ERR,"47=0x%04x tp_type=%d\n",meptr_47->meid, tp_type);
		}
		if(meptr_47 && tp_type == 3) { // 802.1p
			// 47->130
			struct me_t *meptr_130 = meinfo_me_get_by_meid(130, meinfo_util_me_attr_data(meptr_47, 4));
			if(meptr_130) {
				//dbprintf(LOG_ERR,"130=0x%04x\n",meptr_130->meid);
				// 130->266
				struct me_t *meptr_266 = meinfo_me_get_by_meid(266, meinfo_util_me_attr_data(meptr_130 , 2+pbit));
				if(meptr_266) {
					// 266->268
					struct me_t *meptr_268 = meinfo_me_get_by_meid(268, meinfo_util_me_attr_data(meptr_266 , 1));
					//dbprintf(LOG_ERR,"266=0x%04x\n",meptr_266->meid);
					if(meptr_268) {
						*gem_port = meinfo_util_me_attr_data(meptr_268, 1);
						ret = meptr_268;
					}
				}
			}
		} else if (meptr_47 && tp_type == 5) { // GEM IWTP
			// 47->266
			struct me_t *meptr_266 = meinfo_me_get_by_meid(266, meinfo_util_me_attr_data(meptr_47 , 4));
			if(meptr_266) {
				// 266->268
				struct me_t *meptr_268 = meinfo_me_get_by_meid(268, meinfo_util_me_attr_data(meptr_266 , 1));
				//dbprintf(LOG_ERR,"266=0x%04x\n",meptr_266->meid);
				if(meptr_268) {
					*gem_port = meinfo_util_me_attr_data(meptr_268, 1);
					ret = meptr_268;
				}
			}
		}
	}
	return ret;
}

static int
er_group_hw_change_td(int enable, struct me_t *td_us_ipoe, struct me_t *td_ds_ipoe, struct me_t *td_us_pppoe, struct me_t *td_ds_pppoe)
{
	struct attr_value_t attr;
	
	meinfo_me_attr_get(td_us_pppoe, 2, &attr);
	if(enable)
		attr.data += (unsigned int) meinfo_util_me_attr_data(td_us_ipoe, 2);
	else
		attr.data -= (unsigned int) meinfo_util_me_attr_data(td_us_ipoe, 2);
	meinfo_me_attr_set(td_us_pppoe, 2, &attr, 0);
	
	meinfo_me_attr_get(td_ds_pppoe, 2, &attr);
	if(enable)
		attr.data += (unsigned int) meinfo_util_me_attr_data(td_ds_ipoe, 2);
	else
		attr.data -= (unsigned int) meinfo_util_me_attr_data(td_ds_ipoe, 2);
	meinfo_me_attr_set(td_ds_pppoe, 2, &attr, 0);

	return 0;
}

static int
er_group_hw_change_ts_pq(int enable)
{
	struct me_t *me;
	struct attr_value_t attr = {0, NULL};
	
	// Change TS 0x8001 to SP
	me=meinfo_me_get_by_meid(278, 0x8001);
	if(me) {
		attr.data = (enable) ? 1 : 2; // SP or WRR
		meinfo_me_attr_set(me, 3, &attr, 0);
	}
	// Change PQ 0x8002 to priority=1
	me=meinfo_me_get_by_meid(277, 0x8002);
	if(me) {
		meinfo_me_attr_get(me, 6, &attr);
		util_bitmap_set_value(attr.ptr, 32, 16, 16, (enable) ? 1 : 0);
		meinfo_me_attr_set(me, 6, &attr, 0);
	}
	// Change PQ 0x8003 to priority=2
	me=meinfo_me_get_by_meid(277, 0x8003);
	if(me) {
		meinfo_me_attr_get(me, 6, &attr);
		util_bitmap_set_value(attr.ptr, 32, 16, 16, (enable) ? 2 : 0);
		meinfo_me_attr_set(me, 6, &attr, 0);
	}
	return 0;
}

// 171@6
int
er_group_hw_bundle_rate(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned int us_pir_ipoe = 0, us_pir_pppoe = 0, ds_pir_ipoe = 0, ds_pir_pppoe = 0;
	unsigned short filter_inner_vid = 0xFFFF, treatment_inner_vid = 0xFFFF;
	int bundle_rate = 0, gem_port_ipoe = 0, gem_port_pppoe = 0;
	//unsigned char filter_inner_pbit = 0xFF;
	unsigned char treatment_inner_pbit = 0xFF;
	struct attr_table_header_t *table_header = NULL;	
	struct attr_table_entry_t *table_entry_pos;
	struct er_attr_group_instance_t *current_attr_inst=NULL;
	struct me_t *td_us_ipoe = NULL, *td_ds_ipoe = NULL;
	struct me_t *td_us_pppoe = NULL, *td_ds_pppoe = NULL;
	struct me_t *meptr = meinfo_me_get_by_meid(65302, 0);
	
	if(omci_env_g.olt_workaround_enable != ENV_OLT_WORKAROUND_ALU)
		return 0;
	
	if(meptr && meinfo_util_me_attr_data(meptr, 1) != 1)
		return 0;
	
	if(me->instance_num != 0) // Bundle rate workaround only apply on ME171 inst=0
		return 0;
	
	if(attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}
	
	if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[0].attr_value.ptr) == NULL) {
		dbprintf(LOG_ERR, "vlan tagging table is empty\n");
		return -1;
	}
	
	list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node)
	{
		if (table_entry_pos->entry_data_ptr != NULL)
		{
			filter_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 36, 13);
			//filter_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 32, 4);
			treatment_inner_vid = (unsigned short)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 112, 13);
			treatment_inner_pbit = (unsigned char)util_bitmap_get_value(table_entry_pos->entry_data_ptr, 16*8, 108, 4);
			
			if(filter_inner_vid == 4081) { // IPoE VID
				er_group_hw_vlan_to_td(treatment_inner_vid, treatment_inner_pbit, &us_pir_ipoe, &ds_pir_ipoe, &td_us_ipoe, &td_ds_ipoe);
				er_group_hw_vlan_to_gem(treatment_inner_vid, treatment_inner_pbit, &gem_port_ipoe);
			} else if(filter_inner_vid == 4082) { // PPPoE VID
				er_group_hw_vlan_to_td(treatment_inner_vid, treatment_inner_pbit, &us_pir_pppoe, &ds_pir_pppoe, &td_us_pppoe, &td_ds_pppoe);
				er_group_hw_vlan_to_gem(treatment_inner_vid, treatment_inner_pbit, &gem_port_pppoe);
			} else if(filter_inner_vid == 4086) { // Bundle rate VID
				bundle_rate = 1;
			}
		}
	}
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:		
		if(bundle_rate && td_us_ipoe && td_ds_ipoe && td_us_pppoe && td_ds_pppoe) {
			// Change PPPoE TD
			er_group_hw_change_td(1, td_us_ipoe, td_ds_ipoe, td_us_pppoe, td_ds_pppoe);
			// Change TS/PQ to SP
			er_group_hw_change_ts_pq(1);
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		if(bundle_rate && td_us_ipoe && td_ds_ipoe && td_us_pppoe && td_ds_pppoe) {
			er_group_hw_change_td(0, td_us_ipoe, td_ds_ipoe, td_us_pppoe, td_ds_pppoe);
			er_group_hw_change_ts_pq(0);
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_bundle_rate(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_bundle_rate(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}

	// for ds gem td
	batchtab_omci_update("td");

	return 0;
}
