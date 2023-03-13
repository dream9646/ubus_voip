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
 * File    : er_group_hw_6_47_48.c
 *
 ******************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "omciutil_stp.h"
#include "switch.h"
#include "er_group.h"
#include "hwresource.h"

// 6 Circuit_pack
// 47 MAC_bridge_port_configuration_data
// 48 MAC_bridge_port_designation_data

//6@6,47@2,48@2
int
er_group_hw_l2s_bridging_stp_portstate(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	//params
	unsigned char is_circuitpack_locked;
	unsigned char stp_portstate;
	int ret = 0;
	struct me_t *bport_me;
	struct er_attr_group_instance_t *current_attr_inst;
	struct switch_port_info_t port_info;
	
	if (attr_inst == NULL)
	{
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	is_circuitpack_locked = attr_inst->er_attr_instance[0].attr_value.data;

	//get bridge port me
	bport_me=er_attr_group_util_attrinst2me(attr_inst, 1);
	if(bport_me == NULL)
		return -1;

	if (switch_get_port_info_by_me(bport_me, &port_info) < 0) {
		if(bport_me != NULL)
			dbprintf(LOG_ERR, "get port info error:classid=%u, meid=%u\n", bport_me->classid, bport_me->meid);
		return -1;
	}
	
	if (port_info.port_type != ENV_BRIDGE_PORT_TYPE_UNI) {
		dbprintf(LOG_INFO, "STP skipped for non uni port:classid=%u, meid=%u\n", bport_me->classid, bport_me->meid);
		return 0;
	}
 
	/*0: Disable
	  1: Listening
	  2: Learning
	  3: Forwarding
	  4: Blocking
	  5: Linkdown
	  6: Stp_off */
	stp_portstate = attr_inst->er_attr_instance[2].attr_value.data;

	// In case circuit is locked, either uni side  or ani side,
	// the traffic for same side bridge port should be stoped
	// we implemented this by putting bridge port to stp disable state
	if (is_circuitpack_locked) 
		stp_portstate = STP_DISABLE;

	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (switch_hw_g.stp_state_port_set == NULL 
				|| switch_hw_g.stp_state_port_set(port_info.logical_port_id, stp_portstate) < 0) {
			dbprintf(LOG_ERR, "fail to set stp port state port=%u\n", port_info.logical_port_id);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		// basically, below will set stpstate to STP DISABLE for uni/wan, STP_OFF for cpu/iphost
		stp_portstate=omciutil_stp_state_by_porttype(omci_env_g.stp_conf_before_omci, port_info.port_type);
                if (switch_hw_g.stp_state_port_set == NULL || 
                    (ret = switch_hw_g.stp_state_port_set(port_info.logical_port_id, stp_portstate ) < 0))
                {
			dbprintf(LOG_ERR, "fail to set stp port state, port=%u\n", port_info.logical_port_id);
			return -1;
		} 
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_l2s_bridging_stp_portstate(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
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
		if (er_group_hw_l2s_bridging_stp_portstate(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		// FIXME, KEVIN, why this is require?
		//if (switch_hw_g.mac_table_flush)
		//	switch_hw_g.mac_table_flush(0x1f, 0);

		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}

	return 0;
}

