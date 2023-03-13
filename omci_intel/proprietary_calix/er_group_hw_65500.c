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
 * File    : er_group_hw_65500.c
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

#define STRING_LEN	256

static int
port2port_enable_env_set(unsigned char value)
{
	// exit if input value is unchanged
	if (omci_env_g.port2port_enable == value)
		return 0;
		
	omci_env_g.port2port_enable = value;

	// when port2port_enable==1, localbridging_default should also be enabled
	omci_env_g.localbridging_default = value;

	// Local_bridging is decided by P2P_enable
	if(switch_hw_g.local_bridging)
		switch_hw_g.local_bridging(value);
	
	return 0;	
}

static int
veip_reassign_enable_env_set(unsigned char value)
{
	int ret;
	char strbuf[STRING_LEN];
	
	omci_env_g.classf_veip_rules_reassign = value ? 3 : 0;
	snprintf(strbuf, STRING_LEN, "env classf_veip_rules_reassign %d", omci_env_g.classf_veip_rules_reassign);
	ret=omci_cli_input(1, strbuf);
	if( WEXITSTATUS(ret) != 0 ) {
		dbprintf(LOG_ERR, "omci env error ret=%d, value=%d\n", WEXITSTATUS(ret), value);
		return -1;
	}
	
	return 0;	
}

int
er_group_hw_full_bridge_enable(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	unsigned char value;

	if (attr_inst == NULL)
		return -1;

	value = (unsigned char)attr_inst->er_attr_instance[0].attr_value.data;

	// set port2port_enable flag
	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		port2port_enable_env_set(value==1?1:0);
		veip_reassign_enable_env_set(value==3?1:0);
		break;		
	case ER_ATTR_GROUP_HW_OP_UPDATE:		
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_full_bridge_enable(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		port2port_enable_env_set(value==1?1:0);
		veip_reassign_enable_env_set(value==3?1:0);
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}

	return 0;
}
