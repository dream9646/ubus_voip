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
 * File    : switch_hw_prx126_switch.c
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
#include "util_run.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "omciutil_misc.h"
#include "switch.h"
#include "env.h"

int
switch_hw_prx126_hw_version_get(unsigned int *family, unsigned int *version, unsigned int *rev, unsigned int *subtype)
{
	return 0;
}

// ------------------------------------------------------------------------------
// local bridging
//
// Note: dstport_bitmap contains physical ports(b7~0) and extension ports(b15~8)
// ------------------------------------------------------------------------------

// to get the real asci configuration, no matter in rg or not, we always use sdk api to get isolation status
int
switch_hw_prx126_port_isolation_set(unsigned int srcport, unsigned int dstport_bitmap)
{
	return 0;
}

int
switch_hw_prx126_port_isolation_get(unsigned int srcport, unsigned int *dstport_bitmap)
{
	return 0;
}

// to get the real asci configuration, no matter in rg or not, we always use sdk api to get isolation status
int
switch_hw_prx126_port_isolation_print(int fd, int mode)
{
	return 0;
}


int
switch_hw_prx126_port_allowed(int src_port, unsigned int dst_port_bitmap)
{
	return 0;
}

int
switch_hw_prx126_local_bridging(unsigned int en)
{
	return 0;
}

int
switch_hw_prx126_test_mode_set (unsigned int port_id, unsigned int mode) 
{
	return 0;
}

int
switch_hw_prx126_test_mode_get (unsigned int port_id, unsigned int *mode) 
{
	return 0;
}

int
switch_hw_prx126_unknown_packet_action_set(int portid, unsigned int pkt_type, unsigned int action)
{
	return 0;
}

int
switch_hw_prx126_unknown_packet_action_get(int portid, unsigned int pkt_type, unsigned int *action) 
{
	return 0;
}

int
switch_hw_prx126_unknown_packet_flood_mask_set(int portid, unsigned int pkt_type, unsigned int action)
{
	return 0;
}

int
switch_hw_prx126_unknown_packet_flood_mask_get(int portid, unsigned int pkt_type, unsigned int *action) 
{
	return 0;
}

int
switch_hw_prx126_unknown_sa_action_set(int portid, unsigned int action)
{
	return 0;
}

int
switch_hw_prx126_unknown_sa_action_get(int portid, unsigned int *action) 
{
	return 0;
}

int
switch_hw_prx126_unknown_mcast_drop_set(unsigned int drop)
{

	return 0;
}

int
switch_hw_prx126_over_maclimit_action_set(int portid, unsigned int action)
{
	return 0;
}

int
switch_hw_prx126_over_maclimit_action_get(int portid, unsigned int *action) 
{
	return 0;
}
