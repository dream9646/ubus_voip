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
 * File    : er_group_hw_47_84.c
 *
 ******************************************************************/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "util.h"
#include "env.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"
#include "batchtab.h"
#include "er_group_hw.h"
#include "proprietary_alu.h"

// 45 MAC_bridge_service_profile
// 47 MAC_bridge_port_configuration_data
// 84 VLAN_tagging_filter_dat

// l2s_filt_configure  /////////////////////////////////////////////////////////////////////

struct l2s_filt_create_vid_tci_pri_params_t
{
	struct tci_t tci[12];
	unsigned char entry_num;
};

static int 
get_l2s_filt_create_vid_tci_pri_parameter_from_attr_inst(struct l2s_filt_create_vid_tci_pri_params_t *params, struct er_attr_group_instance_t *attr_inst, char port_type)
{
	struct me_t *private_me=0;
	//unsigned char operation;
	unsigned char *bitmap, entry_num;
	int i;
	
	//0 47@2 Port_num
	//use attr's meid get internal resource
	if ((private_me=er_attr_group_util_attrinst2private_me(attr_inst, 1)) == NULL) {
       		dbprintf(LOG_ERR,"private me null?\n");
		return -1;
	}
	
	//1 84@1 VLAN_filter_list
	bitmap=attr_inst->er_attr_instance[2].attr_value.ptr;

	//2 84@2 Forward_operation
	//operation=attr_inst->er_attr_instance[3].attr_value.data;
	
	//3 84@3 Number of entries
	entry_num=attr_inst->er_attr_instance[4].attr_value.data;
	if(entry_num > 12){
       		dbprintf(LOG_ERR,"Number of entries should <= 12\n");
		entry_num=12;
	}
	params->entry_num=entry_num;

	for(i=0; i < entry_num; i++){
		params->tci[i].pbit=util_bitmap_get_value(bitmap, 24*8, i*16, 3);
		params->tci[i].de=util_bitmap_get_value(bitmap, 24*8, i*16+3, 1);
		params->tci[i].vid=util_bitmap_get_value(bitmap, 24*8, i*16+4, 12); 
       		dbprintf(LOG_DEBUG,"pbit=%d, de=%d, vid=%d\n", params->tci[i].pbit, params->tci[i].de, params->tci[i].vid);
	}
	
	return 0;
}

// gps l2s filt configure
// 47@2, 84@2
int 
er_group_hw_l2s_filt_configure(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	case ER_ATTR_GROUP_HW_OP_DEL:
                // the vfilter forward operation configuration is moved into batchtab classf        
	        batchtab_omci_update("classf");
	        batchtab_omci_update("tagging");
	        batchtab_omci_update("filtering");
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}
	return 0;
}

// l2s_filt_create_vid_tci_pri  /////////////////////////////////////////////////////////////////////

// gps l2s filt create-vid create-pri create-tci
//45@inst,47@2,84@1,84@2,84@3
int 
er_group_hw_l2s_filt_create_vid_tci_pri(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU)
		{
			struct l2s_filt_create_vid_tci_pri_params_t params;
			int ret;
			struct me_t *private_me;
			struct switch_port_info_t port_info;
			
			memset(&params,0x0, sizeof(struct l2s_filt_create_vid_tci_pri_params_t));
			
			if ((private_me=er_attr_group_util_attrinst2private_me(attr_inst, 1)) == NULL) {
				dbprintf(LOG_ERR, "\n[ext-filt] private me null?\n");
				return -1;
			}
			if (switch_get_port_info_by_me(private_me, &port_info) < 0) {
				dbprintf(LOG_ERR, "\n[ext-filt] get_port_info failed.\n");
				return -1;
			}

			if ( (ret=get_l2s_filt_create_vid_tci_pri_parameter_from_attr_inst(&params, attr_inst, port_info.port_type)) != 0 ){
				dbprintf(LOG_ERR,"get_l2s_filt_create_vid_tci_pri_parameter_from_attr_inst error\n");
				return -1;
			}
#if 0 // ALU decides to use VLAN=4085 for TLS with ALU 7342/7360 OLT
			// ALU TLS workaround
			if(proprietary_alu_get_olt_version() == 1) {
				int i;
				for(i=0; i<12; i++) {
					if(params.tci[i].pbit==0x7 && params.tci[i].vid==0xfff) { // Forward all traffic while tci=0xffff
						proprietary_alu_tls_mode(1);
						break;
					}
				}
			}
#endif
		}
		// the vfilter vid/tci/pri configuration is moved into batchtab classf
		batchtab_omci_update("classf");
		batchtab_omci_update("tagging");
		batchtab_omci_update("filtering");
		if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU))
		{
			batchtab_omci_update("alu_tlsv6");
		}
		if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ERICSSON))
		{
			batchtab_omci_update("ericsson_pvlan");
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		// the vfilter vid/tci/pri configuration is moved into batchtab classf
		batchtab_omci_update("classf");
		batchtab_omci_update("tagging");
		batchtab_omci_update("filtering");
		if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU))
		{
			batchtab_omci_update("alu_tlsv6");
		}
		if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ERICSSON))
		{
			batchtab_omci_update("ericsson_pvlan");
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
#if 0 // ALU decides to use VLAN=4085 for TLS with ALU 7342/7360 OLT
		if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) // ALU TLS workaround
		{
			proprietary_alu_tls_mode(0);
		}
#endif
		// the vfilter vid/tci/pri configuration is moved into batchtab classf
		batchtab_omci_update("classf");
		batchtab_omci_update("tagging");
		batchtab_omci_update("filtering");
		if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU))
		{
			batchtab_omci_update("alu_tlsv6");
		}
		if((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ERICSSON) == 0) &&
			(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ERICSSON))
		{
			batchtab_omci_update("ericsson_pvlan");
		}
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}
	return 0;
}

