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
 * File    : er_group_hw_65288_65289_65292.c
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
#include "cfm_send.h"
#include "er_group_hw.h"
#include "omciutil_misc.h"

//9.99.26 65288 OamMeg
//9.99.27 65289 OamMep
//9.99.30 65292 OamLoopback

// 65288@3 [0]
// 65289@1,2,4,5,8 [1-5]
// 65292@1,2,3,4,5,6,7,8 [6-13]
int 
er_group_hw_oam_mep_loopback(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char md_level, tp_type;
	unsigned short meid, mepid;
	unsigned char port_type, port_type_index;
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
	md_level = (unsigned char) attr_inst->er_attr_instance[0].attr_value.data;
	mepid = (unsigned short) attr_inst->er_attr_instance[2].attr_value.data;
	tp_type = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;
	meid = (unsigned short) attr_inst->er_attr_instance[4].attr_value.data;
	
	switch(tp_type) {
		case 1: // pptp eth uni
			if ((meptr = meinfo_me_get_by_meid(11, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=11, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				//return -1;
			} else {
				struct me_t *ipptp_me = hwresource_public2private(meptr);
				if(ipptp_me) {
					port_type = ENV_BRIDGE_PORT_TYPE_UNI;
					port_type_index = meptr->instance_num;
				} else {
					port_type = 0xff;
					port_type_index = 0xff;
				}
			}
			//omci_env_g.cfm_reverse_mep_direction = 0;
			break;
		case 3: // 802.1p mapper service
			if ((meptr = meinfo_me_get_by_meid(130, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=130, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				//return -1;
			} else {
				struct me_t *bridgeport_me = omciutil_misc_find_me_related_bport(meptr);
				struct me_t *ibridgeport_me = hwresource_public2private(bridgeport_me);
				if(ibridgeport_me) {
					port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
					port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
				} else {
					port_type = 0xff;
					port_type_index = 0xff;
				}
			}
			//omci_env_g.cfm_reverse_mep_direction = 1;
			break;
		case 4: // ip host
			if ((meptr = meinfo_me_get_by_meid(134, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=134, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				//return -1;
			} else {
				port_type = ENV_BRIDGE_PORT_TYPE_IPHOST;
				port_type_index = meptr->instance_num;
			}
			//omci_env_g.cfm_reverse_mep_direction = 0;
			break;
		case 5: // gem iwtp
			if ((meptr = meinfo_me_get_by_meid(266, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=266, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				//return -1;
			} else {
				struct me_t *bridgeport_me = omciutil_misc_find_me_related_bport(meptr);
				struct me_t *ibridgeport_me = hwresource_public2private(bridgeport_me);
				if(ibridgeport_me) {
					port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
					port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
				} else {
					port_type = 0xff;
					port_type_index = 0xff;
				}
			}
			//omci_env_g.cfm_reverse_mep_direction = 1;
			break;
		case 6: // gem mcast iwtp
			if ((meptr = meinfo_me_get_by_meid(281, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=281, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				//return -1;
			} else {
				struct me_t *bridgeport_me = omciutil_misc_find_me_related_bport(meptr);
				struct me_t *ibridgeport_me = hwresource_public2private(bridgeport_me);
				if(ibridgeport_me) {
					port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
					port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
				} else {
					port_type = 0xff;
					port_type_index = 0xff;
				}
			}
			//omci_env_g.cfm_reverse_mep_direction = 1;
			break;
		case 11: // veip
			if ((meptr = meinfo_me_get_by_meid(329, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=329, meid=%u\n", meid);
				port_type = 0xff;
				port_type_index = 0xff;
				//return -1;
			} else {
				port_type = ENV_BRIDGE_PORT_TYPE_CPU;
				port_type_index = meptr->instance_num;
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
		if((cfm_config = cfm_mp_find(md_level, mepid, port_type, port_type_index)) != NULL) {
			unsigned short to_mepid = (unsigned short) attr_inst->er_attr_instance[6].attr_value.data;
			unsigned short counts = (unsigned short) attr_inst->er_attr_instance[8].attr_value.data;
			unsigned char pbit = (unsigned char) attr_inst->er_attr_instance[9].attr_value.data;
			unsigned char dei = (unsigned char) attr_inst->er_attr_instance[10].attr_value.data;
			unsigned char start = (unsigned char) attr_inst->er_attr_instance[13].attr_value.data;
			unsigned char to_mac[IFHWADDRLEN], null_mac[IFHWADDRLEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			unsigned char bcast_mac[IFHWADDRLEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
			memcpy(to_mac, (unsigned char *) attr_inst->er_attr_instance[7].attr_value.ptr, sizeof(to_mac));
			cfm_config->tlv_data_length = (unsigned short) attr_inst->er_attr_instance[11].attr_value.data;
			cfm_config->payload_pattern = (unsigned int) attr_inst->er_attr_instance[12].attr_value.data;
			cfm_config->payload_format = OPTION_TLV_DATA;

			if(memcmp(to_mac, null_mac, IFHWADDRLEN) == 0) { // Find MAC by MEP_ID
				cfm_pkt_rmep_entry_t *entry = NULL;
				list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
					if(entry->rmep_id == to_mepid) {
						memcpy(to_mac, entry->rmep_mac, IFHWADDRLEN);
						break;
					}
				}
			} else if(memcmp(to_mac, bcast_mac, IFHWADDRLEN) == 0) { // Multicast LBM
				unsigned char mcast_mac[IFHWADDRLEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x30};
				mcast_mac[5] |= md_level;
				memcpy(to_mac, mcast_mac, IFHWADDRLEN);
			}

			if(start) {
				int i;
				for(i=0; i<counts; i++) {
					struct timeval time;
					time.tv_sec = omci_env_g.cfm_send_timeout;
					time.tv_usec = 0;
					cfm_send_lbm(cfm_config, to_mac, pbit, dei);
					select(0, NULL, NULL, NULL, &time);
				}
			}
		}
		break;
		
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
		
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_oam_mep_loopback(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		if (er_group_hw_oam_mep_loopback(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
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

