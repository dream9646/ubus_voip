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
 * File    : er_group_hw_47_130.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "switch.h"
#include "batchtab.h"

// 47 MAC_bridge_port_configuration_data
// 130 802.1p_mapper_service_profile

// gps l2s dscp port enable 
// 47@2, 130@10
int 
er_group_hw_l2s_dscp_port_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	        // the pbit value of an untag pkt can be either a default pbit value or mapped from dscp-to-pbit table
	        // this er group is to determine whether the default pbit vale will be referred or not.
	        // this is done in classe which can map untag upstream to related iwtp by dscp-to-internal_pbit table or by specific default pbit value
	        batchtab_omci_update("classf");
		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}

/*--------------------l2s_dscp_port_map-------------------------------- */ 

// gps l2s dscp port map 
// 47@2, 130@11
int 
er_group_hw_l2s_dscp_port_map(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		batchtab_omci_update("classf");
	        batchtab_omci_update("tagging");
		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}

/*--------------------l2s_dscp_port_default--------------------------- */ 

// gps l2s dscp port default 
// 47@2, 130@12
int 
er_group_hw_l2s_dscp_port_default(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	        // the pbit value of an untag pkt can be either a default pbit value or mapped from dscp-to-pbit table
	        // this er group is to determine the default pbit value 
	        // this is done in classe which maps untag upstream flow to the specific pbit related iwtp
	        batchtab_omci_update("classf");
		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}

