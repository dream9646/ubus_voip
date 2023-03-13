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
 * File    : er_group_hw_47_249.c
 *
 ******************************************************************/

#include <net/if.h>
#include "util.h"
#include "util_run.h"
#include "er_group.h"
#include "batchtab.h"

// 47 MAC_bridge_port_configuration_data
// 249 ExtL2VlanTagConfigData
#if 0
int
er_group_hw_ifsw_proto_br_path(int action, struct attr_table_header_t *tab_header)
{
	struct meinfo_t *miptr = meinfo_util_miptr(249);
	struct me_t *meptr = NULL;
	char cmd[128];
	
	if (tab_header == NULL) {
       		dbprintf(LOG_ERR,"tab_header NULL\n");
		return -1;
	}
	
	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
		struct attr_table_entry_t *entry = NULL;
		list_for_each_entry(entry, &tab_header->entry_list, entry_node) {
			unsigned char mac[IFHWADDRLEN], mask[IFHWADDRLEN], pbit;
			unsigned int vlan, i;
			for(i=0; i<IFHWADDRLEN; i++) {
				mac[i] = (unsigned char) util_bitmap_get_value(entry->entry_data_ptr, 20*8, 64+8*i, 8);
				mask[i] = (unsigned char) util_bitmap_get_value(entry->entry_data_ptr, 20*8, 16+8*i, 8);
			}
			vlan = (unsigned int) util_bitmap_get_value(entry->entry_data_ptr, 20*8, 115, 13);
			pbit = (unsigned char) util_bitmap_get_value(entry->entry_data_ptr, 20*8, 144, 4);
			// protocol classify by mac
			sprintf(cmd, "echo proto %s 2 %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x > /proc/ifsw", 
				(action==ER_ATTR_GROUP_HW_OP_ADD) ? "add" : "del",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mask[0], mask[1], mask[2], mask[3], mask[4], mask[5]);
			util_run_by_system(cmd);
			/*// protocol classify by vlan
			sprintf(cmd, "echo proto %s 4 %d 0xfff > /proc/ifsw", 
				(action==ER_ATTR_GROUP_HW_OP_ADD) ? "add" : "del", vlan);
			util_run_by_system(cmd);
			// protocol classify by pbit
			sprintf(cmd, "echo proto %s 6 %d 0x7 > /proc/ifsw", 
				(action==ER_ATTR_GROUP_HW_OP_ADD) ? "add" : "del", pbit);
			util_run_by_system(cmd);
			// protocol classify by tci
			sprintf(cmd, "echo proto %s 10 %d 0xffff > /proc/ifsw", 
				(action==ER_ATTR_GROUP_HW_OP_ADD) ? "add" : "del", (pbit<<13|vlan));
			util_run_by_system(cmd);*/
		}
	}
	
	return 0;
}
#endif
//47@2,249@2,3,5
int
er_group_hw_l2s_l2_tags_create_add012_bridge(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
		batchtab_omci_update("classf");
		batchtab_omci_update("tagging");
		batchtab_omci_update("calix_apas");
		//er_group_hw_ifsw_proto_br_path(action, (struct attr_table_header_t *) attr_inst->er_attr_instance[3].attr_value.ptr); // move to batchtab
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_l2s_l2_tags_create_add012_bridge(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			//er_attr_group_util_release_attrinst(attr_inst);
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			//return -1;
		}
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_l2s_l2_tags_create_add012_bridge(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
       		dbprintf(LOG_ERR,"Unknown operator\n");
	}
	return 0;
}

