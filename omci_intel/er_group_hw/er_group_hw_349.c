/******************************************************************
 *
 * Copyright (C) 2082 5V Technologies Ltd.
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
 * File    : er_group_hw_349.c
 *
 ******************************************************************/
#include <string.h>

#include "util.h"
#include "switch.h"
#include "meinfo_hw_poe.h"
#include "er_group_hw.h"

// 349 PoE_Control

//349@6
int
er_group_hw_poe_power_priority(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char priority;
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int phy_portid, ext_portid;
	struct switch_port_info_t port_info;
	struct me_t *pptp_uni_me= meinfo_me_get_by_meid(11, me->meid);

    	if (pptp_uni_me && switch_get_port_info_by_me(pptp_uni_me, &port_info)<0) {
		dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", pptp_uni_me->meid);	
		return -1;
	}
	
	// port logical->physical
	switch_hw_g.portid_logical_to_private(port_info.logical_port_id, &phy_portid, &ext_portid);
	
	priority = attr_inst->er_attr_instance[0].attr_value.data;
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			//get PsE_Control to check management capability
			extern int poe_mgt_cap;
			if (poe_mgt_cap == -1)
				meinfo_hw_poe_get_management_capability(&poe_mgt_cap);
			if (poe_mgt_cap == 0) {
				meinfo_hw_poe_set_priority(phy_portid, priority);
				if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
					// write poe class to default config because calix don't want to power off poe client during mib reset
					char str[4] = {0};
					memset(str, 0, sizeof(str));
					sprintf(str, "%d", (unsigned char) attr_inst->er_attr_instance[0].attr_value.data);
					meinfo_config_set_attr_by_str(me->classid, me->instance_num, 6, str);
				}
			}
		} else
			meinfo_hw_poe_set_priority(phy_portid, priority);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_poe_power_priority(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

//349@12
int
er_group_hw_poe_class_control(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned char classify;
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned int phy_portid, ext_portid;
	struct switch_port_info_t port_info;
	struct me_t *pptp_uni_me= meinfo_me_get_by_meid(11, me->meid);

    	if (pptp_uni_me && switch_get_port_info_by_me(pptp_uni_me, &port_info)<0) {
		dbprintf(LOG_ERR,"get port_info by pptp_uni_me %d err?\n", pptp_uni_me->meid);	
		return -1;
	}
	
	// port logical->physical
	switch_hw_g.portid_logical_to_private(port_info.logical_port_id, &phy_portid, &ext_portid);
	
	classify = attr_inst->er_attr_instance[0].attr_value.data;
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0) {
			//get PsE_Control to check management capability
			extern int poe_mgt_cap;
			if (poe_mgt_cap == -1)
				meinfo_hw_poe_get_management_capability(&poe_mgt_cap);
			if (poe_mgt_cap == 0) {
				meinfo_hw_poe_set_class(phy_portid, classify);
				if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
					// write poe class to default config because calix don't want to power off poe client during mib reset
					char str[4] = {0};
					memset(str, 0, sizeof(str));
					sprintf(str, "%d", (unsigned char) attr_inst->er_attr_instance[0].attr_value.data);
					meinfo_config_set_attr_by_str(me->classid, me->instance_num, 12, str);
				}
			}
		} else
			meinfo_hw_poe_set_class(phy_portid, classify);
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_poe_class_control(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}
