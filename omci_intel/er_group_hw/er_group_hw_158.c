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
 * File    : er_group_hw_158.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "util_run.h"
#include "er_group.h"

// 158 ONT_remote_debug

static char command[LINE_MAXSIZE];
static int cmd_length = 0;

//158@2
int
er_group_hw_remote_debug(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	int ret = -1, length = 0;

	if (attr_inst == NULL)
		return -1;

	// if command has not provisioned yet, do initialize the command string first
	if(!cmd_length) memset(command, 0, sizeof(command));
	
	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		length = strlen((char *) attr_inst->er_attr_instance[0].attr_value.ptr);
		strncat(command, attr_inst->er_attr_instance[0].attr_value.ptr, length);
		cmd_length += length;
		
		// if length==25, wait for the next command to concatenate, otherwise treat as last command and execute it
		if(length < 25) {
			dbprintf(LOG_INFO, "command=%s cmd_length=%d\n", command, cmd_length);
			ret = util_run_by_system(command);
			cmd_length = 0;
			if( WEXITSTATUS(ret) != 0 ) {
				dbprintf(LOG_ERR, "remote_debug_command error ret=%d, command=%s\n", WEXITSTATUS(ret), command);
				return -1;
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_remote_debug(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
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
		;
	}
	return 0;
}


