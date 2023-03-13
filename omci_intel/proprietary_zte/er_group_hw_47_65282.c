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
 * Module  : proprietary_zte
 * File    : er_group_hw_47_65282.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "omciutil_vlan.h"
#include "batchtab.h"

extern int er_group_hw_util_vlan_tagging_manager(int action, struct me_t *issue_bridge_port_me, struct me_t *issue_me);

//47@2,65282@1,65282@2,65282@3
int
er_group_hw_l2s_tags_create_add01_multicast(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
		dbprintf(LOG_ERR, "attr instance error\n");
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
	case ER_ATTR_GROUP_HW_OP_DEL:
	        batchtab_omci_update("classf");
		break;
	default:
		dbprintf(LOG_ERR, "unsupported er_group_hw method: %d\n", action);
		return -1;
	}
	return 0;
}

