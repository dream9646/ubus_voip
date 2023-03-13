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
 * File    : er_group_hw_65503.c
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
#include "cli.h"
#include "switch.h"
#include <sys/file.h>

#define RG_HARD_RESETORE        0x1
#define VOIP_HARD_RESTORE       0x2
#define RG_SOFT_RESTORE         0x4

static int isVoIPModel()
{
        struct me_t *me = meinfo_me_get_by_meid(256, 0x0);
        if (me == NULL) return 0;
        
        struct attr_value_t attr;
        attr.ptr= malloc_safe(15);
        meinfo_me_attr_get(me, 2, &attr);
        // 801Gv2 is the only model without VOIP known so far
        if (attr.ptr && strcmp(attr.ptr, "801Gv2") == 0)
        {
                free_safe(attr.ptr);
                return 0;
        }
        
        return 1;
}

int
er_group_hw_restore_to_default(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char value;
        unsigned short tasks = 0;
        struct attr_value_t attr;

	if (attr_inst == NULL)
		return -1;
        
        unsigned short old_status = meinfo_me_attr_get(me, 2, &attr);

	value = (unsigned short)attr_inst->er_attr_instance[0].attr_value.data;
	 
	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if(value & RG_HARD_RESETORE) tasks |= RG_HARD_RESETORE;  
                if(value & VOIP_HARD_RESTORE)
                {
                        if (isVoIPModel())
                                tasks |= VOIP_HARD_RESTORE;
                }                 
                if(value & RG_SOFT_RESTORE) tasks |= RG_SOFT_RESTORE;
                
                attr.data = tasks | old_status;    // value may contain bits not supported
                meinfo_me_attr_set(me, 2, &attr, 1);
                dbprintf(LOG_DEBUG, "action %x -> %x\n", old_status, tasks | old_status);
                attr.data = 0;
                meinfo_me_attr_set(me, 1, &attr, 0);
                
                tasks ^= old_status;    // only perform if status is dirty
                
                // perform actual action after status has been updated
                if (tasks & RG_HARD_RESETORE)
                {
			dbprintf(LOG_ERR, "Reset to RG default\n");
			util_run_by_system("/opt/wwwctrl/manage_config.sh default");
                }
                
                
                if (tasks & VOIP_HARD_RESTORE)
                {
                        // check if cwmp is running
			dbprintf(LOG_ERR, "Reset to VOIP default\n");
			util_run_by_system("/opt/wwwctrl/voip_config.sh reset");
                }
                
                if (tasks & RG_SOFT_RESTORE && (!(tasks & RG_HARD_RESETORE)))
                {
                        // only performed if hardware reset is not performed
			dbprintf(LOG_ERR, "Reset to RG default_user\n");
			util_run_by_system("/opt/wwwctrl/manage_config.sh default_user");
                }
                
                //if (tasks & (RG_HARD_RESETORE | RG_SOFT_RESTORE)) sleep(20);    // sleep until reboot
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
		if (er_group_hw_restore_to_default(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
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
