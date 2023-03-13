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
 * File    : er_group_hw_277_280_65323.c
 *
 ******************************************************************/

#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "gpon_sw.h"
#if defined(CONFIG_TARGET_PRX126) || defined CONFIG_SDK_PRX120 || defined CONFIG_SDK_PRX321
#include "gpon_hw_prx126.h"
#endif
#include "tm.h"
#include "proprietary_alu.h"
#include "batchtab.h"

// 277 Priority_queue_G
// 280 GEM_traffic_descriptor
// 65323 Priority_Queue-G_Supplemental_1

//277@inst,280@1,280@2,280@3,280@4,280@5,280@6,280@7,280@8
int
er_group_hw_mac_traffic_control_td_us_pq(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct me_t *pq_me;

	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	if ((pq_me = er_attr_group_util_attrinst2me(attr_inst, 0)) == NULL) {
		dbprintf(LOG_ERR, "could not get pq me, classid=%u, meid=%u\n", 
			attr_inst->er_attr_group_definition_ptr->er_attr[0].classid, attr_inst->er_attr_instance[0].meid);
		return -1;
	}
	
	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
	case ER_ATTR_GROUP_HW_OP_DEL:
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//batchtab_omci_update("td");
		batchtab_omci_update("tm");
		break;
	default:
       		dbprintf(LOG_ERR,"Unknown operator\n");
	}
	return 0;
}

