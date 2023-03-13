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
 * File    : er_group_hw_47_84_299_300.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "cpuport.h"
#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_switch.h"
#include "er_group_hw.h"

//9.3.4 47 MAC_bridge_port_configuration_data
//9.3.11 84 VLAN_tagging_filter_data
//9.3.19 299 Dot1ag_maintenance_domain
//9.3.20 300 Dot1ag_maintenance_association

int
er_group_hw_get_mac_addr(unsigned char *mac, char *devname)
{
	int fd;
	struct ifreq itf;
	
	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	
	strncpy(itf.ifr_name, devname, IFNAMSIZ-1);
	if(ioctl(fd, SIOCGIFHWADDR, &itf) < 0) {
		close(fd);
		return -1;
	} else
		memcpy(mac, itf.ifr_hwaddr.sa_data, IFHWADDRLEN);
	close(fd);
	return 0;
}

// 47@meid 299@meid,1,2,3,4,5,6 300@meid,2,3,4,5,6,7,8
int 
er_group_hw_cfm_mip_info(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char md_level;
	unsigned short meid, mepid[12];
	unsigned char port_type, port_type_index;
	struct er_attr_group_instance_t *current_attr_inst = NULL;
	struct me_t *meptr = NULL, *ibridgeport_me = NULL;
	cfm_config_t *cfm_config = NULL;
	int j;
	
	if (omci_env_g.cfm_enable == 0) {
       		dbprintf(LOG_ERR,"cfm_enable==0\n");
		return -1;
	}
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}
	// MHF==0 or 1 means no MHF function is needed
	if((action == ER_ATTR_GROUP_HW_OP_ADD) &&
	  ((((unsigned char) attr_inst->er_attr_instance[14].attr_value.data) == 0) ||
	  (((unsigned char) attr_inst->er_attr_instance[14].attr_value.data) == 1)))
		return 0;
	
	if(action == ER_ATTR_GROUP_HW_OP_UPDATE) {
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		
		meid = (unsigned short) current_attr_inst->er_attr_instance[0].attr_value.data;
		md_level = (unsigned char) current_attr_inst->er_attr_instance[2].attr_value.data;
	} else {
		meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
		md_level = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
	}
	
	if ((meptr = meinfo_me_get_by_meid(47, meid)) == NULL) {
		dbprintf(LOG_ERR, "could not get bridge port me, classid=47, meid=%u\n", meid);
		return -1;
	}
	ibridgeport_me = hwresource_public2private(meptr);
	if(ibridgeport_me && (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) { // Occupied
		port_type = meinfo_util_me_attr_data(ibridgeport_me, 4);
		port_type_index = meinfo_util_me_attr_data(ibridgeport_me, 5);
	} else {
		dbprintf(LOG_ERR, "could not get ibridge port me, classid=422, meid=%u\n", meid);
		return -1;
	}
	
	for(j=0; j<12; j++) {
		// Use VID as MEP ID
		if(action == ER_ATTR_GROUP_HW_OP_UPDATE) {
			if(current_attr_inst->er_attr_instance[13].attr_value.ptr == NULL) {
				//er_attr_group_util_release_attrinst(current_attr_inst);
				return 0;
			}
			mepid[j] = (unsigned short) util_bitmap_get_value(current_attr_inst->er_attr_instance[13].attr_value.ptr ,12*16, j*16, 16);
			
			// MHF==0 or 1 means no MHF function is needed
			if((((unsigned char) current_attr_inst->er_attr_instance[14].attr_value.data) == 0)
			|| (((unsigned char) current_attr_inst->er_attr_instance[14].attr_value.data) == 1)) {
				if ((meptr = meinfo_me_get_by_meid(47, meid)) == NULL) {
					dbprintf(LOG_ERR, "could not get bridge port me, classid=47, meid=%u\n", meid);
					return -1;
				}
				ibridgeport_me = hwresource_public2private(meptr);
				if(ibridgeport_me && (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) { // Occupied
					cfm_config = cfm_mp_find(md_level, mepid[j], port_type, port_type_index);
					if(cfm_config && (cfm_config->ma_mhf_creation == MA_MHF_CREATION_DEFAULT || cfm_config->ma_mhf_creation == MA_MHF_CREATION_EXPLICIT)) // MIP
						cfm_mp_del(md_level, mepid[j], port_type, port_type_index); // Delete existed MIP
					er_attr_group_util_release_attrinst(current_attr_inst);
				}
				return 0;
			}
		} else
			mepid[j] = (unsigned short) util_bitmap_get_value(attr_inst->er_attr_instance[13].attr_value.ptr ,12*16, j*16, 16);
		// Skip VID==0
		if(mepid[j] == 0) continue;
		
		switch(action)
		{
		case ER_ATTR_GROUP_HW_OP_ADD:
			if ((meptr = meinfo_me_get_by_meid(47, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=47, meid=%u\n", meid);
				return -1;
			}
			ibridgeport_me = hwresource_public2private(meptr);
			if(ibridgeport_me && (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) { // Occupied
				// Check if MIP only works at UNI/WAN ports
				if((meinfo_util_me_attr_data(ibridgeport_me, 4) != 1) &&
				   (meinfo_util_me_attr_data(ibridgeport_me, 4) != 2))
				   	return 0;
				if((cfm_config = cfm_mp_add(md_level, mepid[j], port_type, port_type_index)) != NULL) {
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
					cfm_config->mep_id = mepid[j];
					cfm_config->mep_control = 0;
					cfm_config->pri_vlan = (mepid[j] & 0x0FFF);
					cfm_config->priority = (mepid[j] & 0xE000) >> 13;
					memset(cfm_config->egress_id, 0, sizeof(cfm_config->egress_id));
					er_group_hw_get_mac_addr(cfm_config->egress_id, "ifcpuuni0");
					cfm_config->eth_ais_control = 0;
				}
			} else {
				dbprintf(LOG_ERR, "could not get internal bridge port me, classid=%u, meid=%u\n", 
					meptr->classid, meptr->meid);
				return -1;
			}
			break;
			
		case ER_ATTR_GROUP_HW_OP_DEL:
			cfm_mp_del(md_level, mepid[j], port_type, port_type_index);
			break;
			
		case ER_ATTR_GROUP_HW_OP_UPDATE:
			if ((meptr = meinfo_me_get_by_meid(47, meid)) == NULL) {
				dbprintf(LOG_ERR, "could not get bridge port me, classid=47, meid=%u\n", meid);
				er_attr_group_util_release_attrinst(current_attr_inst);
				return -1;
			}
			ibridgeport_me = hwresource_public2private(meptr);
			if(ibridgeport_me) {
				// Check if MIP only works at UNI/WAN ports
				if((meinfo_util_me_attr_data(ibridgeport_me, 4) != 1) &&
				   (meinfo_util_me_attr_data(ibridgeport_me, 4) != 2)) {
				   	er_attr_group_util_release_attrinst(current_attr_inst);
				   	return 0;
				}
				if((cfm_config = cfm_mp_find(md_level, mepid[j], port_type, port_type_index)) != NULL) {
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
					cfm_config->mep_id = mepid[j];
					cfm_config->mep_control = 0;
					cfm_config->pri_vlan = (mepid[j] & 0x0FFF);
					cfm_config->priority = (mepid[j] & 0xE000) >> 13;
					memset(cfm_config->egress_id, 0, sizeof(cfm_config->egress_id));
					er_group_hw_get_mac_addr(cfm_config->egress_id, "ifcpuuni0");
					cfm_config->eth_ais_control = 0;
				} else {
					md_level = (unsigned char) attr_inst->er_attr_instance[2].attr_value.data;
					mepid[j] = (unsigned short) util_bitmap_get_value(attr_inst->er_attr_instance[13].attr_value.ptr ,12*16, j*16, 16);
					cfm_mp_del(md_level, mepid[j], port_type, port_type_index);
					md_level = (unsigned char) current_attr_inst->er_attr_instance[2].attr_value.data;
					mepid[j] = (unsigned short) util_bitmap_get_value(current_attr_inst->er_attr_instance[13].attr_value.ptr ,12*16, j*16, 16);
					if((cfm_config = cfm_mp_add(md_level, mepid[j], port_type, port_type_index)) != NULL) {
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
						cfm_config->mep_id = mepid[j];
						cfm_config->mep_control = 0;
						cfm_config->pri_vlan = (mepid[j] & 0x0FFF);
						cfm_config->priority = (mepid[j] & 0xE000) >> 13;
						memset(cfm_config->egress_id, 0, sizeof(cfm_config->egress_id));
						er_group_hw_get_mac_addr(cfm_config->egress_id, "ifcpuuni0");
						cfm_config->eth_ais_control = 0;
					}
				}
			} else {
				dbprintf(LOG_ERR, "could not get internal bridge port me, classid=%u, meid=%u\n", 
					meptr->classid, meptr->meid);
				er_attr_group_util_release_attrinst(current_attr_inst);
				return -1;
			}
			er_attr_group_util_release_attrinst(current_attr_inst);
			break;
	
		default:
	       		dbprintf(LOG_ERR,"Unknown operator\n");
			break;
		}
	}
	return 0;
}
