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
 * Module  : switch_hw_prx126
 * File    : switch_hw_prx126_trunk.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// @sdk/include

#include "util.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "switch.h"
#include "env.h"


int
switch_hw_prx126_trunk_port_set(unsigned int logical_portmask)
{
	return 0;
}

int
switch_hw_prx126_trunk_port_get(unsigned int *logical_portmask)
{
	return 0;
}

int
switch_hw_prx126_trunk_dumbmode_set(int enable)
{
	return 0;
}

int
switch_hw_prx126_trunk_dumbmode_get(int *enable)
{
	return 0;
}

int
switch_hw_prx126_trunk_algorithm_set(unsigned int algo_bitmask)
{
	return 0;
}

int
switch_hw_prx126_trunk_algorithm_get(unsigned int *algo_bitmask)
{
	return 0;
}

// integrated function of the above
int
switch_hw_prx126_trunk_portmask_config(unsigned int trunk_portmask)
{
	return 0;
}
