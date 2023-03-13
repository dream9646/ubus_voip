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
 * File    : er_group_hw_47_148_329_65317.c
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "er_group.h"

// 9.12.4 148 Authentication_security_method
// 2: Username_1 (25byte,string,RW)
// 3: Password (25byte,string,RW)
// 5: Username_2 (25byte,string,RW)

//148@2,148@3,148@5,65317@meid
int
er_group_hw_calix_rg_config_pppauth(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short calix_rg_meid = (unsigned short)attr_inst->er_attr_instance[3].attr_value.data;
	struct me_t *calix_rg_me = meinfo_me_get_by_meid(65317, calix_rg_meid);	
	unsigned char calix_rg_attr_mask[2] = {0};
	int is_ppp = 0;

	if (calix_rg_me) {
		unsigned char mgmt_mode = (unsigned char)meinfo_util_me_attr_data(calix_rg_me, 1);	// 0: native (omci), 1: external (tr069 or other)
		unsigned char wan_proto = (unsigned char)meinfo_util_me_attr_data(calix_rg_me, 2);	// 0: dhcp, 1:static, 2:pppoe, 3: bridged
		unsigned short pppauth_meid = (unsigned short)meinfo_util_me_attr_data(calix_rg_me, 6);	
		// omci ctrl, native pppoe, the me that triggers the er_update is pppauth
		if (mgmt_mode == 0 && wan_proto == 2 && me->meid == pppauth_meid)
			is_ppp = 1;
	}
	if (! is_ppp)
		return 0;
	
	util_attr_mask_set_bit(calix_rg_attr_mask, 6);	// pointer from calix_rg_config me to pppauth me
	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		batchtab_cb_wanif_me_update(calix_rg_me, calix_rg_attr_mask);
		break;		
	case ER_ATTR_GROUP_HW_OP_UPDATE:		
		batchtab_cb_wanif_me_update(calix_rg_me, calix_rg_attr_mask);
		batchtab_omci_update("wanif");		
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}

	return 0;
}
