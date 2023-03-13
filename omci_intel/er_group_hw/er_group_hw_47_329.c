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
 * File    : er_group_hw_47_329.c
 *
 ******************************************************************/

#include "util.h"
#include "er_group_hw.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "switch.h"

int
sync_admin_state_to_uni_under_same_bridge(unsigned short bridge_meid, unsigned char admin_state)
{
	struct meinfo_t *miptr_47 = meinfo_util_miptr(47);
	struct me_t *meptr_47;
	int found = 0;

	list_for_each_entry(meptr_47, &miptr_47->me_instance_list, instance_node) {
		// find bport uni within the same bridge id
		if ((unsigned short)meinfo_util_me_attr_data(meptr_47, 1) == bridge_meid) {
			if (miptr_47->callback.get_pointer_classid_cb(meptr_47, 4) == 11) {	// pptp-uni
				struct me_t *meptr_11 = meinfo_me_get_by_meid(11, (unsigned short)meinfo_util_me_attr_data(meptr_47, 4));
				struct attr_value_t attr = { 0, NULL };
				struct switch_port_info_t port_info;

				attr.data = admin_state;	// set admin_state to pptp-uni
				meinfo_me_attr_set(meptr_11, 5, &attr, 0);
				if (switch_get_port_info_by_me(meptr_11, &port_info)<0)
					continue;
				if (switch_hw_g.port_enable_set)
					switch_hw_g.port_enable_set(port_info.logical_port_id, !admin_state); // port down(0)/up(1)
				found++;
			}
		}
	}
	return found;
}

// 47 MAC_bridge_port_configuration_data
// 329 Virtual_Ethernet_interface_point

// veip_admin_state
//47@1,329@1
int
er_group_hw_sync_veip_autouni_admin_state(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me,
					  unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned short bridge_meid = attr_inst->er_attr_instance[0].attr_value.data;
	unsigned char veip_admin_state = attr_inst->er_attr_instance[1].attr_value.data;

	if (omci_env_g.autouni_enable == 0)
		return 0;

	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr_inst NULL\n");
		return -1;
	}

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if (me->classid == 329) {
			batchtab_cb_wanif_me_update(me, attr_mask);
			batchtab_omci_update("wanif");
			batchtab_omci_update("autoveip");
		}			
		// sync veip admin_state to pptp
		sync_admin_state_to_uni_under_same_bridge(bridge_meid, veip_admin_state);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		if (me->classid == 329) {
			batchtab_cb_wanif_me_update(me, attr_mask);
			batchtab_omci_update("wanif");
			batchtab_omci_update("autoveip");
		}			
		// set admin unlock to pptp eth uni
		sync_admin_state_to_uni_under_same_bridge(bridge_meid, 0);	
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if (er_group_hw_sync_veip_autouni_admin_state(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0) {
			dbprintf(LOG_ERR, "fail to update for deleting\n");
		 	return -1;
		}
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//allocate
		if (er_group_hw_sync_veip_autouni_admin_state(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	}

	return 0;
}
