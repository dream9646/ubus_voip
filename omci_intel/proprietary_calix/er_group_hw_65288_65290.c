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
 * File    : er_group_hw_65288_65290.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"
#include "cpuport.h"
#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_switch.h"
#include "er_group_hw.h"
#include "omciutil_misc.h"

//9.99.26 65288 OamMeg
//9.99.28 65290 OamMip

// 65288@meid,1,2,3,4,5,6,7,8 [0-8]
// 65290@meid,1,2,3,4 [9-13]
int 
er_group_hw_oam_mip(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char md_level, tp_type;
	unsigned short meid, mepid;
	unsigned char port_type, port_type_index, logical_port_id;
	struct er_attr_group_instance_t *current_attr_inst;
	struct me_t *meptr = NULL;
	cfm_config_t *cfm_config = NULL;
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_INFO,"cfm_enable==0\n");
		return 0;
	}
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}
	md_level = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;
	mepid = (unsigned short) attr_inst->er_attr_instance[11].attr_value.data;
	tp_type = (unsigned char) attr_inst->er_attr_instance[12].attr_value.data;
	meid = (unsigned short) attr_inst->er_attr_instance[13].attr_value.data;
	
	switch(tp_type) {
		case 1: // pptp eth uni
			if ((meptr = meinfo_me_get_by_meid(11, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=11, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				logical_port_id = 0;
				//return -1;
			} else {
				struct me_t *ipptp_me = hwresource_public2private(meptr);
				if(ipptp_me) {
					port_type = ENV_BRIDGE_PORT_TYPE_UNI;
					port_type_index = meptr->instance_num;
					logical_port_id = meinfo_util_me_attr_data(ipptp_me, 4);
				} else {
					port_type = 0xff;
					port_type_index = 0xff;
					logical_port_id = 0;
				}
			}
			//omci_env_g.cfm_reverse_mep_direction = 0;
			break;
		case 3: // 802.1p mapper service
			if ((meptr = meinfo_me_get_by_meid(130, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=130, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				logical_port_id = switch_get_wan_logical_portid();
				//return -1;
			} else {
				struct me_t *bridgeport_me = omciutil_misc_find_me_related_bport(meptr);
				struct me_t *ibridgeport_me = hwresource_public2private(bridgeport_me);
				if(ibridgeport_me) {
					port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
					port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
					logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
				} else {
					port_type = 0xff;
					port_type_index = 0xff;
					logical_port_id = switch_get_wan_logical_portid();
				}
			}
			//omci_env_g.cfm_reverse_mep_direction = 1;
			break;
		case 4: // ip host
			if ((meptr = meinfo_me_get_by_meid(134, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=134, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				logical_port_id = switch_get_cpu_logical_portid(0);
				//return -1;
			} else {
				port_type = ENV_BRIDGE_PORT_TYPE_IPHOST;
				port_type_index = meptr->instance_num;
				logical_port_id = switch_get_cpu_logical_portid(0);
			}
			//omci_env_g.cfm_reverse_mep_direction = 0;
			break;
		case 5: // gem iwtp
			if ((meptr = meinfo_me_get_by_meid(266, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=266, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				logical_port_id = switch_get_wan_logical_portid();
				//return -1;
			} else {
				struct me_t *bridgeport_me = omciutil_misc_find_me_related_bport(meptr);
				struct me_t *ibridgeport_me = hwresource_public2private(bridgeport_me);
				if(ibridgeport_me) {
					port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
					port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
					logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
				} else {
					port_type = 0xff;
					port_type_index = 0xff;
					logical_port_id = switch_get_wan_logical_portid();
				}
			}
			//omci_env_g.cfm_reverse_mep_direction = 1;
			break;
		case 6: // gem mcast iwtp
			if ((meptr = meinfo_me_get_by_meid(281, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=281, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				logical_port_id = switch_get_wan_logical_portid();
				//return -1;
			} else {
				struct me_t *bridgeport_me = omciutil_misc_find_me_related_bport(meptr);
				struct me_t *ibridgeport_me = hwresource_public2private(bridgeport_me);
				if(ibridgeport_me) {
					port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
					port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
					logical_port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
				} else {
					port_type = 0xff;
					port_type_index = 0xff;
					logical_port_id = switch_get_wan_logical_portid();
				}
			}
			//omci_env_g.cfm_reverse_mep_direction = 1;
			break;
		case 11: // veip
			if ((meptr = meinfo_me_get_by_meid(329, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=329, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				logical_port_id = switch_get_cpu_logical_portid(0);
				//return -1;
			} else {
				port_type = ENV_BRIDGE_PORT_TYPE_CPU;
				port_type_index = meptr->instance_num;
				logical_port_id = switch_get_cpu_logical_portid(0);
			}
			//omci_env_g.cfm_reverse_mep_direction = 0;
			break;
		default:
			dbprintf(LOG_ERR, "unknown bridge port me, type=%d, meid=%u\n", tp_type, meid);
			return -1;
	}
	
	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		if((cfm_config = cfm_mp_add(md_level, mepid, port_type, port_type_index)) != NULL) {
			unsigned char large_string[376], index = 0;
			struct attr_table_header_t *tab_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[8].attr_value.ptr;
			struct attr_table_entry_t *entry = NULL;
			cfm_config->port_type = port_type;
			cfm_config->port_type_index = port_type_index;
			cfm_config->logical_port_id = logical_port_id;
			
			// From 9.3.19 Dot1ag maintenance domain (ME 299, MD)
			cfm_config->md_meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
			cfm_config->md_level = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;
			cfm_config->md_format = MD_NAME_FORMAT_NONE;
			cfm_config->md_mhf_creation = MA_MHF_CREATION_DEFAULT;
			
			// From 9.3.20 Dot1ag maintenance association (ME 300, MA)
			cfm_config->ma_meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
			cfm_config->ma_format = (unsigned char) attr_inst->er_attr_instance[4].attr_value.data ? MA_NAME_FORMAT_STRING : MA_NAME_FORMAT_ICC;
			if ((meptr = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[2].attr_value.data)) != NULL) {
				memset(large_string, 0, sizeof(large_string));
				meinfo_me_copy_from_large_string_me(meptr, large_string, sizeof(large_string));
				strncpy(cfm_config->ma_name_1, large_string, sizeof(cfm_config->ma_name_1));
				strncpy(cfm_config->ma_name_2, large_string+sizeof(cfm_config->ma_name_1), sizeof(cfm_config->ma_name_2));
			}
			cfm_config->ccm_interval = (unsigned char) attr_inst->er_attr_instance[5].attr_value.data;
			cfm_config->ma_mhf_creation = MA_MHF_CREATION_DEFAULT;
			cfm_config->auto_discovery = (unsigned char) attr_inst->er_attr_instance[6].attr_value.data;
			omci_env_g.cfm_rmep_holdtime_before_active = (cfm_config->auto_discovery) 
				? (unsigned char) attr_inst->er_attr_instance[7].attr_value.data*100 : 10000;
			for(index=0; index<12; index++) cfm_config->peer_mep_ids[index] = 0;
			if(!cfm_config->auto_discovery) {
				index = 0;
				list_for_each_entry(entry, &tab_header->entry_list, entry_node) {
					unsigned char op = (unsigned char) util_bitmap_get_value(entry->entry_data_ptr, 3*8, 16, 8);
					if (entry->entry_data_ptr != NULL && op == 1)
						cfm_config->peer_mep_ids[index++] = (unsigned short) util_bitmap_get_value(entry->entry_data_ptr, 3*8, 0, 16);
				}
				cfm_mp_add_peer_mep_ids_to_rmep_list(cfm_config);
			}
			
			// From 9.3.22 Dot1ag MEP (ME 302, MEP)
			cfm_config->mep_meid = (unsigned short) attr_inst->er_attr_instance[9].attr_value.data;
			cfm_config->mep_id = (unsigned short) attr_inst->er_attr_instance[11].attr_value.data;
			cfm_config->pri_vlan = ((unsigned short) attr_inst->er_attr_instance[1].attr_value.data) & 0xFFF;
			cfm_config->pri_pbit = (((unsigned short) attr_inst->er_attr_instance[1].attr_value.data) >> 12) & 0x7;
			cfm_config->pbit_filter = (((unsigned short) attr_inst->er_attr_instance[1].attr_value.data) >> 15) & 0x1;
			cfm_config->admin_state = 0;

			// From 9.99.25 OamCfg (ME65287, MEG)
			if ((meptr = meinfo_me_get_by_instance_num(65287, 0)) != NULL) {
				cfm_config->admin_state = ((unsigned char) meinfo_util_me_attr_data(meptr, 1)==0) ? 1 : cfm_config->admin_state;
				cfm_config->md_sender_id_permission = (unsigned char) meinfo_util_me_attr_data(meptr, 2)+1;
				cfm_config->ma_sender_id_permission = (unsigned char) meinfo_util_me_attr_data(meptr, 2)+1;
				cfm_config->payload_format = ((unsigned char) cfm_config->ma_sender_id_permission<=1) ? 0 : 1;
				cfm_config->send_cc_senderid = (unsigned char) meinfo_util_me_attr_data(meptr, 3);
				cfm_config->send_cc_port_status = (unsigned char) meinfo_util_me_attr_data(meptr, 4);
				cfm_config->send_cc_interface_status = (unsigned char) meinfo_util_me_attr_data(meptr, 5);
				cfm_config->send_cc_data_status = (unsigned char) meinfo_util_me_attr_data(meptr, 6);
				cfm_config->send_lb_senderid = (unsigned char) meinfo_util_me_attr_data(meptr, 7);
				cfm_config->send_lb_port_status = (unsigned char) meinfo_util_me_attr_data(meptr, 8);
				cfm_config->send_lb_interface_status = (unsigned char) meinfo_util_me_attr_data(meptr, 9);
				cfm_config->send_lb_data_status = (unsigned char) meinfo_util_me_attr_data(meptr, 10);
				cfm_config->send_lt_senderid = (unsigned char) meinfo_util_me_attr_data(meptr, 11);
				cfm_config->send_lt_port_status = (unsigned char) meinfo_util_me_attr_data(meptr, 12);
				cfm_config->send_lt_interface_status = (unsigned char) meinfo_util_me_attr_data(meptr, 13);
				cfm_config->send_lt_data_status = (unsigned char) meinfo_util_me_attr_data(meptr, 14);
			}
		}
		break;
		
	case ER_ATTR_GROUP_HW_OP_DEL:
		cfm_mp_del(md_level, mepid, port_type, port_type_index);
		break;
		
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_oam_mip(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		if (er_group_hw_oam_mip(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
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
