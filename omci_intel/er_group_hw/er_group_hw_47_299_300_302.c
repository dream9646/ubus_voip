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
 * File    : er_group_hw_47_299_300_302.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "cpuport.h"
#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_switch.h"
#include "er_group_hw.h"
#include "proprietary_alu.h"

//9.3.4 47 MAC_bridge_port_configuration_data
//9.3.19 299 Dot1ag_maintenance_domain
//9.3.20 300 Dot1ag_maintenance_association
//9.3.22 302 Dot1ag_MEP

int
er_group_hw_new_mep_by_vlan(int action, struct er_attr_group_instance_t *attr_inst)
{
	// ME47->ME171->VLAN
	cfm_config_t *cfm_config = NULL;
	unsigned short meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
	unsigned char md_level = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
	unsigned short pri_vlan = (unsigned short) util_bitmap_get_value(attr_inst->er_attr_instance[19].attr_value.ptr ,2*8, 0, 16);
	unsigned char ma_mhf_creation = (unsigned char) attr_inst->er_attr_instance[14].attr_value.data;
	struct me_t *meptr_47 = meinfo_me_get_by_meid(47, meid);
	unsigned char tp_type = meinfo_util_me_attr_data(meptr_47, 3);
	struct me_t *ibridgeport_me = hwresource_public2private(meptr_47);
	unsigned char port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
	unsigned char port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
	
	if(((tp_type == 1) || (tp_type == 11)) && (ma_mhf_creation==MA_MHF_CREATION_NONE)) { // (PPTP_ETH_UNI || VEIP) && MEP (not MIP)
		struct list_head rule_list_us, rule_list_ds;
		struct omciutil_vlan_rule_fields_t *rule_fields_pos;
		
		if (omciutil_vlan_collect_rules_by_port(ER_ATTR_GROUP_HW_OP_NONE, meptr_47, NULL, &rule_list_us, &rule_list_ds, 0) < 0)
			return -1;
		
		list_for_each_entry(rule_fields_pos, &rule_list_us, rule_node) {
			if(rule_fields_pos->treatment_inner.vid == 0 || rule_fields_pos->treatment_inner.vid == 4096)
				continue;
			else if(rule_fields_pos->treatment_inner.vid == pri_vlan)
				continue;
			
			switch(action) {
				case ER_ATTR_GROUP_HW_OP_ADD:
					// Check if MEP already exists
					if((cfm_config = cfm_mp_find(md_level, rule_fields_pos->treatment_inner.vid, port_type, port_type_index)) != NULL)
						break;
					// Add additional MEP by different VLAN
					if((cfm_config = cfm_mp_add(md_level, rule_fields_pos->treatment_inner.vid, port_type, port_type_index)) != NULL) {
						int i;
						cfm_config->port_type = port_type;
						cfm_config->port_type_index = port_type_index;
						cfm_config->logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
						
						// From 9.3.19 Dot1ag maintenance domain (ME 299, MD)
						cfm_config->md_meid = (unsigned short) attr_inst->er_attr_instance[1].attr_value.data;
						cfm_config->md_level = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
						cfm_config->md_format = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;
						memcpy(cfm_config->md_name_1, (unsigned char *) attr_inst->er_attr_instance[4].attr_value.ptr, sizeof(cfm_config->md_name_1));
						memcpy(cfm_config->md_name_2, (unsigned char *) attr_inst->er_attr_instance[5].attr_value.ptr, sizeof(cfm_config->md_name_2));
						cfm_config->md_mhf_creation = (unsigned char) attr_inst->er_attr_instance[6].attr_value.data;
						cfm_config->md_sender_id_permission = (unsigned char) attr_inst->er_attr_instance[7].attr_value.data;
						
						// From 9.3.20 Dot1ag maintenance association (ME 300, MA)
						cfm_config->ma_meid = (unsigned short) attr_inst->er_attr_instance[8].attr_value.data;
						cfm_config->ma_format = (unsigned char) attr_inst->er_attr_instance[9].attr_value.data;
						memcpy(cfm_config->ma_name_1, (unsigned char *) attr_inst->er_attr_instance[10].attr_value.ptr, sizeof(cfm_config->ma_name_1));
						memcpy(cfm_config->ma_name_2, (unsigned char *) attr_inst->er_attr_instance[11].attr_value.ptr, sizeof(cfm_config->ma_name_2));
						cfm_config->ccm_interval = (unsigned char) attr_inst->er_attr_instance[12].attr_value.data;
						for(i=0; i<sizeof(cfm_config->assoc_vlans)/sizeof(unsigned short); i++)
							cfm_config->assoc_vlans[i] = (unsigned short) util_bitmap_get_value(attr_inst->er_attr_instance[13].attr_value.ptr ,12*16, i*16, 16);
						cfm_config->ma_mhf_creation = (unsigned char) attr_inst->er_attr_instance[14].attr_value.data;
						cfm_config->ma_sender_id_permission = (unsigned char) attr_inst->er_attr_instance[15].attr_value.data;
						
						// From 9.3.22 Dot1ag MEP (ME 302, MEP)
						cfm_config->mep_meid = (unsigned short) attr_inst->er_attr_instance[16].attr_value.data;
						cfm_config->mep_id = (unsigned short) rule_fields_pos->treatment_inner.vid;
						cfm_config->mep_control = (unsigned char) util_bitmap_get_value(attr_inst->er_attr_instance[18].attr_value.ptr ,1*8, 0, 8);
						cfm_config->pri_vlan = (unsigned short) rule_fields_pos->treatment_inner.vid;
						cfm_config->priority = (unsigned char) rule_fields_pos->treatment_inner.priority;
						memcpy(cfm_config->egress_id, (unsigned char *) attr_inst->er_attr_instance[21].attr_value.ptr, sizeof(cfm_config->egress_id));
						cfm_config->eth_ais_control = (unsigned char) util_bitmap_get_value(attr_inst->er_attr_instance[22].attr_value.ptr ,1*8, 0, 8);
						
					}
					break;
				case ER_ATTR_GROUP_HW_OP_DEL:
					// Delete additional MEP by different VLAN
					cfm_mp_del(md_level, rule_fields_pos->treatment_inner.vid, port_type, port_type_index);
					break;
				case ER_ATTR_GROUP_HW_OP_UPDATE:
					// Update additional MEP by different VLAN
					if((cfm_config = cfm_mp_find(md_level, rule_fields_pos->treatment_inner.vid, port_type, port_type_index)) != NULL) {
						int i;
						cfm_config->port_type = port_type;
						cfm_config->port_type_index = port_type_index;
						cfm_config->logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
						
						// From 9.3.19 Dot1ag maintenance domain (ME 299, MD)
						cfm_config->md_meid = (unsigned short) attr_inst->er_attr_instance[1].attr_value.data;
						cfm_config->md_level = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
						cfm_config->md_format = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;
						memcpy(cfm_config->md_name_1, (unsigned char *) attr_inst->er_attr_instance[4].attr_value.ptr, sizeof(cfm_config->md_name_1));
						memcpy(cfm_config->md_name_2, (unsigned char *) attr_inst->er_attr_instance[5].attr_value.ptr, sizeof(cfm_config->md_name_2));
						cfm_config->md_mhf_creation = (unsigned char) attr_inst->er_attr_instance[6].attr_value.data;
						cfm_config->md_sender_id_permission = (unsigned char) attr_inst->er_attr_instance[7].attr_value.data;
						
						// From 9.3.20 Dot1ag maintenance association (ME 300, MA)
						cfm_config->ma_meid = (unsigned short) attr_inst->er_attr_instance[8].attr_value.data;
						cfm_config->ma_format = (unsigned char) attr_inst->er_attr_instance[9].attr_value.data;
						memcpy(cfm_config->ma_name_1, (unsigned char *) attr_inst->er_attr_instance[10].attr_value.ptr, sizeof(cfm_config->ma_name_1));
						memcpy(cfm_config->ma_name_2, (unsigned char *) attr_inst->er_attr_instance[11].attr_value.ptr, sizeof(cfm_config->ma_name_2));
						cfm_config->ccm_interval = (unsigned char) attr_inst->er_attr_instance[12].attr_value.data;
						for(i=0; i<sizeof(cfm_config->assoc_vlans)/sizeof(unsigned short); i++)
							cfm_config->assoc_vlans[i] = (unsigned short) util_bitmap_get_value(attr_inst->er_attr_instance[13].attr_value.ptr ,12*16, i*16, 16);
						cfm_config->ma_mhf_creation = (unsigned char) attr_inst->er_attr_instance[14].attr_value.data;
						cfm_config->ma_sender_id_permission = (unsigned char) attr_inst->er_attr_instance[15].attr_value.data;
						
						// From 9.3.22 Dot1ag MEP (ME 302, MEP)
						cfm_config->mep_meid = (unsigned short) attr_inst->er_attr_instance[16].attr_value.data;
						cfm_config->mep_id = (unsigned short) rule_fields_pos->treatment_inner.vid;
						cfm_config->mep_control = (unsigned char) util_bitmap_get_value(attr_inst->er_attr_instance[18].attr_value.ptr ,1*8, 0, 8);
						cfm_config->pri_vlan = (unsigned short) rule_fields_pos->treatment_inner.vid;
						cfm_config->priority = (unsigned char) rule_fields_pos->treatment_inner.priority;
						memcpy(cfm_config->egress_id, (unsigned char *) attr_inst->er_attr_instance[21].attr_value.ptr, sizeof(cfm_config->egress_id));
						cfm_config->eth_ais_control = (unsigned char) util_bitmap_get_value(attr_inst->er_attr_instance[22].attr_value.ptr ,1*8, 0, 8);
						
					} else 
						er_group_hw_new_mep_by_vlan(ER_ATTR_GROUP_HW_OP_ADD, attr_inst);
					break;
			}
		}
		omciutil_vlan_release_rule_list(&rule_list_us);
		omciutil_vlan_release_rule_list(&rule_list_ds);
	}
	return 0;
}

unsigned short
er_group_hw_find_uni_bp(unsigned short bridge_ptr)
{
	struct meinfo_t *miptr = meinfo_util_miptr(47);
	struct me_t *meptr_47 = NULL;
	
	list_for_each_entry(meptr_47, &miptr->me_instance_list, instance_node) {
		if((meinfo_util_me_attr_data(meptr_47, 1) == bridge_ptr) &&
		   (meinfo_util_me_attr_data(meptr_47, 3) == 1)) { // PPTP_ETH_UNI
			return meptr_47->meid;
		}
	}
	return 0;
}

// 47@meid 299@meid,1,2,3,4,5,6 300@meid,2,3,4,5,6,7,8 302@meid,4,5,6,8,9,11
int 
er_group_hw_cfm_bridge_info(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char md_level;
	unsigned short meid, mepid;
	unsigned char port_type, port_type_index;
	struct er_attr_group_instance_t *current_attr_inst;
	struct me_t *meptr = NULL, *ibridgeport_me = NULL;
	cfm_config_t *cfm_config = NULL;
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_ERR,"cfm_enable==0\n");
		return -1;
	}
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}
	meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
	md_level = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
	mepid = (unsigned short) attr_inst->er_attr_instance[17].attr_value.data;
	
	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((meptr = meinfo_me_get_by_meid(47, meid)) == NULL) {
			dbprintf(LOG_ERR, "could not get bridge port me, classid=47, meid=%u\n", meid);
			return -1;
		}
		ibridgeport_me = hwresource_public2private(meptr);
		if(ibridgeport_me && (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) { // Occupied
			port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
			port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
			if((cfm_config = cfm_mp_add(md_level, mepid, port_type, port_type_index)) != NULL) {
				int i;
				cfm_config->port_type = port_type;
				cfm_config->port_type_index = port_type_index;
				cfm_config->logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
				
				// From 9.3.19 Dot1ag maintenance domain (ME 299, MD)
				cfm_config->md_meid = (unsigned short) attr_inst->er_attr_instance[1].attr_value.data;
				cfm_config->md_level = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
				cfm_config->md_format = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;
				memcpy(cfm_config->md_name_1, (unsigned char *) attr_inst->er_attr_instance[4].attr_value.ptr, sizeof(cfm_config->md_name_1));
				memcpy(cfm_config->md_name_2, (unsigned char *) attr_inst->er_attr_instance[5].attr_value.ptr, sizeof(cfm_config->md_name_2));
				cfm_config->md_mhf_creation = (unsigned char) attr_inst->er_attr_instance[6].attr_value.data;
				cfm_config->md_sender_id_permission = (unsigned char) attr_inst->er_attr_instance[7].attr_value.data;
				
				// From 9.3.20 Dot1ag maintenance association (ME 300, MA)
				cfm_config->ma_meid = (unsigned short) attr_inst->er_attr_instance[8].attr_value.data;
				cfm_config->ma_format = (unsigned char) attr_inst->er_attr_instance[9].attr_value.data;
				memcpy(cfm_config->ma_name_1, (unsigned char *) attr_inst->er_attr_instance[10].attr_value.ptr, sizeof(cfm_config->ma_name_1));
				memcpy(cfm_config->ma_name_2, (unsigned char *) attr_inst->er_attr_instance[11].attr_value.ptr, sizeof(cfm_config->ma_name_2));
				cfm_config->ccm_interval = (unsigned char) attr_inst->er_attr_instance[12].attr_value.data;
				for(i=0; i<sizeof(cfm_config->assoc_vlans)/sizeof(unsigned short); i++)
					cfm_config->assoc_vlans[i] = (unsigned short) util_bitmap_get_value(attr_inst->er_attr_instance[13].attr_value.ptr ,12*16, i*16, 16);
				cfm_config->ma_mhf_creation = (unsigned char) attr_inst->er_attr_instance[14].attr_value.data;
				cfm_config->ma_sender_id_permission = (unsigned char) attr_inst->er_attr_instance[15].attr_value.data;
				// From 9.3.22 Dot1ag MEP (ME 302, MEP)
				cfm_config->mep_meid = (unsigned short) attr_inst->er_attr_instance[16].attr_value.data;
				cfm_config->mep_id = (unsigned short) attr_inst->er_attr_instance[17].attr_value.data;
				cfm_config->mep_control = (unsigned char) util_bitmap_get_value(attr_inst->er_attr_instance[18].attr_value.ptr ,1*8, 0, 8);
				cfm_config->pri_vlan = (unsigned short) util_bitmap_get_value(attr_inst->er_attr_instance[19].attr_value.ptr ,2*8, 0, 16);
				cfm_config->priority = (unsigned char) attr_inst->er_attr_instance[20].attr_value.data;
				memcpy(cfm_config->egress_id, (unsigned char *) attr_inst->er_attr_instance[21].attr_value.ptr, sizeof(cfm_config->egress_id));
				cfm_config->eth_ais_control = (unsigned char) util_bitmap_get_value(attr_inst->er_attr_instance[22].attr_value.ptr ,1*8, 0, 8);
				
				// For ALU-CHT case with FTTU 7342 OLT workaround (Support 8 MEP under 4 UNI)
				if( (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0) &&
				    (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) &&
				    (proprietary_alu_get_olt_version() == 0) ) {
					er_group_hw_new_mep_by_vlan(action, attr_inst);
				}
			}
		} else {
			dbprintf(LOG_ERR, "could not get internal bridge port me, classid=%u, meid=%u\n", 
				meptr->classid, meptr->meid);
			return -1;
		}
		break;
		
	case ER_ATTR_GROUP_HW_OP_DEL:
		if ((meptr = meinfo_me_get_by_meid(47, meid)) == NULL) {
			dbprintf(LOG_ERR, "could not get bridge port me, classid=47, meid=%u\n", meid);
			return -1;
		}
		ibridgeport_me = hwresource_public2private(meptr);
		if(ibridgeport_me) {
			port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
			port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
			cfm_mp_del(md_level, mepid, port_type, port_type_index);
			
			// For ALU-CHT case with FTTU 7342 OLT workaround (Support 8 MEP under 4 UNI)
			if( (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0) &&
			    (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) &&
			    (proprietary_alu_get_olt_version() == 0) ) {
				er_group_hw_new_mep_by_vlan(action, attr_inst);
			}
		} else {
			dbprintf(LOG_ERR, "could not get internal bridge port me, classid=%u, meid=%u\n", 
				meptr->classid, meptr->meid);
			return -1;
		}
		break;
		
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		if ((meptr = meinfo_me_get_by_meid(47, meid)) == NULL) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "could not get bridge port me, classid=47, meid=%u\n", meid);
			return -1;
		}
		ibridgeport_me = hwresource_public2private(meptr);
		if(ibridgeport_me) {
			port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
			port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
			if((cfm_config = cfm_mp_find(md_level, mepid, port_type, port_type_index)) != NULL) {
				int i;
				cfm_config->port_type = port_type;
				cfm_config->port_type_index = port_type_index;
				cfm_config->logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
				
				// From 9.3.19 Dot1ag maintenance domain (ME 299, MD)
				cfm_config->md_meid = (unsigned short) current_attr_inst->er_attr_instance[1].attr_value.data;
				cfm_config->md_level = (unsigned char) current_attr_inst->er_attr_instance[2].attr_value.data;
				cfm_config->md_format = (unsigned char) current_attr_inst->er_attr_instance[3].attr_value.data;
				memcpy(cfm_config->md_name_1, (unsigned char *) current_attr_inst->er_attr_instance[4].attr_value.ptr, sizeof(cfm_config->md_name_1));
				memcpy(cfm_config->md_name_2, (unsigned char *) current_attr_inst->er_attr_instance[5].attr_value.ptr, sizeof(cfm_config->md_name_2));
				cfm_config->md_mhf_creation = (unsigned char) current_attr_inst->er_attr_instance[6].attr_value.data;
				cfm_config->md_sender_id_permission = (unsigned char) current_attr_inst->er_attr_instance[7].attr_value.data;
				
				// From 9.3.20 Dot1ag maintenance association (ME 300, MA)
				cfm_config->ma_meid = (unsigned short) current_attr_inst->er_attr_instance[8].attr_value.data;
				cfm_config->ma_format = (unsigned char) current_attr_inst->er_attr_instance[9].attr_value.data;
				memcpy(cfm_config->ma_name_1, (unsigned char *) current_attr_inst->er_attr_instance[10].attr_value.ptr, sizeof(cfm_config->ma_name_1));
				memcpy(cfm_config->ma_name_2, (unsigned char *) current_attr_inst->er_attr_instance[11].attr_value.ptr, sizeof(cfm_config->ma_name_2));
				cfm_config->ccm_interval = (unsigned char) current_attr_inst->er_attr_instance[12].attr_value.data;
				for(i=0; i<sizeof(cfm_config->assoc_vlans)/sizeof(unsigned short); i++)
					cfm_config->assoc_vlans[i] = (unsigned short) util_bitmap_get_value(current_attr_inst->er_attr_instance[13].attr_value.ptr ,12*16, i*16, 16);
				cfm_config->ma_mhf_creation = (unsigned char) current_attr_inst->er_attr_instance[14].attr_value.data;
				cfm_config->ma_sender_id_permission = (unsigned char) current_attr_inst->er_attr_instance[15].attr_value.data;
				
				// From 9.3.22 Dot1ag MEP (ME 302, MEP)
				cfm_config->mep_meid = (unsigned short) current_attr_inst->er_attr_instance[16].attr_value.data;
				cfm_config->mep_id = (unsigned short) current_attr_inst->er_attr_instance[17].attr_value.data;
				cfm_config->mep_control = (unsigned char) util_bitmap_get_value(current_attr_inst->er_attr_instance[18].attr_value.ptr ,1*8, 0, 8);
				cfm_config->pri_vlan = (unsigned short) util_bitmap_get_value(current_attr_inst->er_attr_instance[19].attr_value.ptr ,2*8, 0, 16);
				cfm_config->priority = (unsigned char) current_attr_inst->er_attr_instance[20].attr_value.data;
				memcpy(cfm_config->egress_id, (unsigned char *) current_attr_inst->er_attr_instance[21].attr_value.ptr, sizeof(cfm_config->egress_id));
				cfm_config->eth_ais_control = (unsigned char) util_bitmap_get_value(current_attr_inst->er_attr_instance[22].attr_value.ptr ,1*8, 0, 8);
				
				// For ALU-CHT case with FTTU 7342 OLT workaround (Support 8 MEP under 4 UNI)
				if( (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU)==0) &&
				    (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) &&
				    (proprietary_alu_get_olt_version() == 0) ) {
					er_group_hw_new_mep_by_vlan(action, attr_inst);
				}
			}
		} else {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "could not get internal bridge port me, classid=%u, meid=%u\n", 
				meptr->classid, meptr->meid);
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;

	default:
       		dbprintf(LOG_ERR,"Unknown operator\n");
		break;
	}
	return 0;
}
