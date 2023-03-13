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
 * Module  : cfm
 * File    : cfm_switch.c
 *
 ******************************************************************/
 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "list.h"
#include "util.h"
#include "meinfo.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "cpuport_util.h"
#ifndef X86
#include "er_group_hw.h"
#endif

#include "cfm_switch.h"
#include "batchtab.h"

int 
cfm_switch_extract_set(int enable)
{
	int ret;
	if (switch_hw_g.cfm_extract_enable_set == NULL)
		return -1;

	ret = switch_hw_g.cfm_extract_enable_set(enable);

	//on tlan environment, trigger classf batchtab to generate a higher priority trap acl rule for cfm,
	//to avoid tlan rules pass throught cfm packets.
	if(strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX)==0)
	{
		batchtab_omci_update("classf");
	}

	return ret;
}

int 
cfm_switch_extract_get(int *enable)
{
	if (switch_hw_g.cfm_extract_enable_get == NULL)
		return -1;	
	return switch_hw_g.cfm_extract_enable_get(enable);
}
