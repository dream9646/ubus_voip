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
 * File    : er_group_hw_65297.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include "util.h"
#include "meinfo.h"
#include "meinfo_hw_util.h"
#include "me_related.h"
#include "omcimsg.h"
#include "switch.h"
#include "er_group.h"
#include "er_group_hw.h"

void
er_group_hw_send_lan_los_alarm(struct me_t *me)
{
	struct me_t *pptp_ethernet_uni_me=meinfo_me_get_by_meid(11, me->meid);
	
	if (me_related(me, pptp_ethernet_uni_me)) {
		struct switch_port_info_t port_info;
		if (!meinfo_me_is_admin_locked(pptp_ethernet_uni_me) && switch_get_port_info_by_me(pptp_ethernet_uni_me, &port_info)>=0) {
			// pptp on mglc
			if (switch_hw_g.port_status_get) {
				struct switch_port_mac_config_t port_mac_status;
				switch_hw_g.port_status_get(port_info.logical_port_id, &port_mac_status);
				if (port_mac_status.link)
					util_alarm_mask_clear_bit(pptp_ethernet_uni_me->alarm_result_mask, 0);
				else
					util_alarm_mask_set_bit(pptp_ethernet_uni_me->alarm_result_mask, 0);
				omcimsg_alarm_generator(pptp_ethernet_uni_me->classid, pptp_ethernet_uni_me->meid, pptp_ethernet_uni_me->alarm_result_mask);
			}
		}
	}
}

// 65297@1
int 
er_group_hw_downstream_tag_handling_mode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char value = ((unsigned char)attr_inst->er_attr_instance[0].attr_value.data==1)? 2 : 0;
	
	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		// 1: strip tag, classf_ds_unmatch=2
		// 2: tag thru, classf_ds_unmatch=0
		omci_env_g.classf_ds_unmatch = value;
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_downstream_tag_handling_mode(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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

// 65297@2
int 
er_group_hw_igmp_access_control_method(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *port_me = NULL, *uni0_privateme = NULL, *veip_me=NULL;
	struct meinfo_t *port_miptr = meinfo_util_miptr(422);
	struct switch_port_info_t port_info;

	
	list_for_each_entry(port_me, &port_miptr->me_instance_list, instance_node) {
		 if (switch_get_port_info_by_me(port_me, &port_info) < 0)
		 	continue;
		 if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_CPU ) {
		 	veip_me = port_me;
		 	break;
		 }
	}
	list_for_each_entry(port_me, &port_miptr->me_instance_list, instance_node) {
		 if (switch_get_port_info_by_me(port_me, &port_info) < 0)
		 	continue;
		 if (port_info.port_type == ENV_BRIDGE_PORT_TYPE_UNI) {
		 	uni0_privateme = port_me;
		 	break;
		 }
	}

	// Workaround for ALU OLT: Force to send LAN_LOS alarm first
	if ((omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) && (meinfo_hw_util_get_ontstate() == 5))
		er_group_hw_send_lan_los_alarm(me);
	
	if ( (veip_me && me->meid == meinfo_util_me_attr_data(veip_me , 2)) ||
	     (uni0_privateme && me->meid == meinfo_util_me_attr_data(uni0_privateme, 2)) ) {
		
		switch (action) {
		case ER_ATTR_GROUP_HW_OP_ADD:	
		case ER_ATTR_GROUP_HW_OP_UPDATE:
		case ER_ATTR_GROUP_HW_OP_DEL:
			omci_env_g.olt_alu_mcast_support = meinfo_util_me_attr_data(me,2);
			break;
		default:
			dbprintf(LOG_ERR,"Unknown operator\n");
		}
	}
	
	return 0;
}
