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
 * File    : switch_hw_prx126_rldp.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// @sdk/include


#include "util.h"
#include "list.h"

#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "switch.h"
#include "env.h"

// -------------------------------------------------------------------
// RLDP
// -------------------------------------------------------------------
int
switch_hw_prx126_rldp_init(void)
{
	return 0;
}

int
switch_hw_prx126_rldp_enable_set(unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_rldp_enable_get(unsigned int *enable)
{
	return 0;
}

// interval unit:ms
int
switch_hw_prx126_rldp_time_period_set(unsigned int interval_check, unsigned int interval_loop)
{
	return 0;
}

int
switch_hw_prx126_rldp_time_period_get(unsigned int *interval_check, unsigned int *interval_loop)
{
	return 0;
}

int
switch_hw_prx126_rldp_portmask_set(unsigned int portmask)
{
	return 0;
}

int
switch_hw_prx126_rldp_portmask_get(unsigned int *portmask)
{
	return 0;
}

int
switch_hw_prx126_rldp_port_status_get(unsigned int portid, unsigned int *loop_status, unsigned int *loop_enter, unsigned int *loop_leave, unsigned int *looped_portid)
{
	return 0;
}

int
switch_hw_prx126_rldp_port_status_clear(unsigned int portid)
{
	return 0;
}

int
switch_hw_prx126_rldp_loopedportpair_get(unsigned int portid, unsigned int *is_looped, unsigned int *port_pair)
{
	return 0;
}

int
switch_hw_prx126_rldp_loopedportmask_get(unsigned int *loopedPortmask)
{
	return 0;
}

int
switch_hw_prx126_rldp_loop_print(int fd)
{
	return 0;
}

