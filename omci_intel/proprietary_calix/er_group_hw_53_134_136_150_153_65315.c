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
 * Module  : proprietary_calix
 * File    : er_group_hw_53_134_136_150_153_65315.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "meinfo_hw_util.h"
#include "er_group.h"
#include "batchtab.h"

int
system_cmd_calix_fqdn(int voip_instance, char *fqdn)
{
#ifdef OMCI_CMD_DISPATCHER_ENABLE
	char cmd[128];	
	snprintf(cmd, 127, "/etc/init.d/cmd_dispatcher.sh set voip_ep%d_domain_name=%s", voip_instance, fqdn);
	util_run_by_thread(cmd, 1);
#endif
	return 0;
}

//53@instid,134@meid,65315@1
int
er_group_hw_calix_voip_fqdn(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned short MAYBE_UNUSED pots_instid, iphost_meid, large_string_meid;
	struct me_t *iphost_me, *large_string_me;
	unsigned char large_string_fqdn[376];

	if (attr_inst == NULL)
		return -1;

	pots_instid = (unsigned short)attr_inst->er_attr_instance[0].attr_value.data;	
	iphost_meid = (unsigned short)attr_inst->er_attr_instance[1].attr_value.data;
	large_string_meid = (unsigned short)attr_inst->er_attr_instance[2].attr_value.data;
	
	iphost_me=meinfo_me_get_by_meid(134, iphost_meid);	
	large_string_me=meinfo_me_get_by_meid(157, large_string_meid);

	if (iphost_me == NULL || large_string_me == NULL)
		return -1;
	
	memset(large_string_fqdn, 0x00, sizeof(large_string_fqdn));
	meinfo_me_copy_from_large_string_me(large_string_me, large_string_fqdn, 375);
	
	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		system_cmd_calix_fqdn(pots_instid, large_string_fqdn);
		break;		
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_calix_voip_fqdn(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}

	return 0;
}
