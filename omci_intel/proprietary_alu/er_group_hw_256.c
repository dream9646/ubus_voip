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
 * File    : er_group_hw_256.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "util_run.h"
#include "meinfo.h"
#include "er_group.h"
#include "switch.h"
#include "metacfg_adapter.h"
#include "batchtab.h"
#include "er_group_hw_util.h"

#define METAFILE_PATH	"/etc/wwwctrl/metafile.dat"

// 256 ONT_G

//256@6
int
er_group_hw_change_ont_mode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	// CHT workaround to switch "ont_mode" via ME256 attribute #6 "Battery_backup"
	if(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		if (attr_inst == NULL)
			return -1;			
		switch (action) {
		case ER_ATTR_GROUP_HW_OP_ADD:
			batchtab_omci_update("alu_ont_mode");
			break;
		case ER_ATTR_GROUP_HW_OP_UPDATE:		
			batchtab_omci_update("alu_ont_mode");
			break;
		case ER_ATTR_GROUP_HW_OP_DEL:
			break;
		default:
			dbprintf(LOG_ERR,"Unknown operator\n");
		}
	}

	return 0;
}
