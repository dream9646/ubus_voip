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
 * File    : switch_hw_prx126_rate.c
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
#include "omciutil_misc.h"
#include "switch.h"
#include "env.h"


int
switch_hw_prx126_rate_storm_set(unsigned int port_id, unsigned int storm_type, unsigned int rate, unsigned int ifg_include)
{
	return 0;
}

int
switch_hw_prx126_rate_storm_get(unsigned int port_id, unsigned int storm_type, unsigned int *rate, unsigned int *ifg_include)
{
	return 0;
}

int
switch_hw_prx126_rate_storm_enable_set(unsigned int storm_type, unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_rate_storm_enable_get(unsigned int storm_type, unsigned int *enable)
{
	return 0;
}

int
switch_hw_prx126_rate_storm_port_enable_set(unsigned int port_id, unsigned int storm_type, unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_rate_storm_port_enable_get(unsigned int port_id, unsigned int storm_type, unsigned int *enable)
{
	return 0;
}

int
switch_hw_prx126_rate_storm_print(int fd, unsigned int port_id)
{
	return 0;
}

// -------------------------------------------------------------------
// Rate Meter
// -------------------------------------------------------------------

// also used by init.c
int
switch_hw_prx126_rate_meter_init(void)
{
	return 0;
}

int
switch_hw_prx126_rate_meter_add(unsigned int rate, unsigned int ifg_en, unsigned int *index)
{
	return 0;
}

int
switch_hw_prx126_rate_meter_del(unsigned int index)
{
	return 0;
}

int
switch_hw_prx126_rate_meter_set(unsigned int index, unsigned int rate, unsigned int ifg_en)
{
	return 0;
}

int
switch_hw_prx126_rate_meter_get(unsigned int index, unsigned int *rate, unsigned int *ifg_en)
{
	return 0;
}

int
switch_hw_prx126_rate_meter_print(int fd)
{
	return 0;
}

// -------------------------------------------------------------------
// Rate Share Meter Bucket
// -------------------------------------------------------------------
int
switch_hw_prx126_rate_meter_bucket_init(void)
{
	return 0;
}

int
switch_hw_prx126_rate_meter_bucket_add(unsigned int bucket_size, unsigned int *index)
{
	return 0;
}

int
switch_hw_prx126_rate_meter_bucket_del(unsigned int index)
{
	return 0;
}

int
switch_hw_prx126_rate_meter_bucket_set(unsigned int index, unsigned int bucket_size)
{
	return 0;
}

int
switch_hw_prx126_rate_meter_bucket_get(unsigned int index, unsigned int *bucket_size)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set rate control (port limit) configuration by port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_rate_control_set(unsigned int port_id, struct switch_rate_control_config_t rate_ctrl_cfg)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Get rate control (port limit) configuration by port
// ----------------------------------------------------------------------------
int
switch_hw_prx126_rate_control_get(unsigned int port_id, struct switch_rate_control_config_t * rate_cfg)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print rate control (port limit) configuration for all UNI ports
// ----------------------------------------------------------------------------
int
switch_hw_prx126_rate_control_print(int fd)
{
	return 0;
}
// ----------------------------------------------------------------------------
// Print rate status for all UNI ports
// ----------------------------------------------------------------------------
int
switch_hw_prx126_rate_status_print(int fd)
{
	return 0;
}
