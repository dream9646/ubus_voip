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
 * File    : er_group_hw_149.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "voip_hw.h"
#include "er_group_hw.h"
#include "er_group_hw_util.h"
#include "batchtab.h"

// 149 SIP_config_portal

//149@1
int
er_group_hw_sip_config_portal(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	int ret;
	struct voip_hw_sip_config_portal_t scp;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	struct voip_hw_sip_config_portal_entry_t *scp_entry;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	memset(&scp, 0x00, sizeof(scp));

	//table
	INIT_LIST_HEAD(&scp.sip_config_portal_list);
	if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[0].attr_value.ptr) != NULL) {
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node) {
			if (table_entry_pos->entry_data_ptr != NULL) {
				scp_entry = malloc_safe(sizeof(struct voip_hw_sip_config_portal_entry_t));
				memcpy(scp_entry->configuration_text, (unsigned char *)table_entry_pos->entry_data_ptr, 29);
				list_add_tail(&scp_entry->sip_config_portal_node, &scp.sip_config_portal_list);
			}
		}
	} else {
		dbprintf(LOG_ERR, "attr value was not table\n");
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((ret = er_group_hw_voip_wrapper_sip_config_portal(VOIP_HW_ACTION_SET, VOIP_HW_GLOBAL_PORT_DEF, attr_mask, &scp)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			voip_hw_sip_config_portal_release_table(&scp);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			voip_hw_sip_config_portal_release_table(&scp);
			return -1;
		}
		//add
		if (er_group_hw_sip_config_portal(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			voip_hw_sip_config_portal_release_table(&scp);
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		voip_hw_sip_config_portal_release_table(&scp);
		memset(&scp, 0x00, sizeof(scp));
		INIT_LIST_HEAD(&scp.sip_config_portal_list);
		if ((ret = er_group_hw_voip_wrapper_sip_config_portal(VOIP_HW_ACTION_RELEASE, VOIP_HW_GLOBAL_PORT_DEF, attr_mask, &scp)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
		;
	}
	voip_hw_sip_config_portal_release_table(&scp);
	return 0;
}
