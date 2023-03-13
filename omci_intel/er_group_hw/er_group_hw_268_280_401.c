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
 * File    : er_group_hw_268_280_401.c
 *
 ******************************************************************/

#include <stdio.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "batchtab.h"
#include "proprietary_alu.h"

// 268 GEM_port_network_CTP
// 280 GEM_traffic_descriptor
// 401 Downstream pseudo obj

//268@1,268@9,280@1,280@2,280@3,280@4,280@5,280@6,280@7,280@8
int
er_group_hw_mac_traffic_control_td_ds_gem(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
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
	        // ds gemport td configuration is handled by batch_cb "td" by setting ACL meter
	        batchtab_omci_update("td");
	        batchtab_omci_update("tm");
		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}

