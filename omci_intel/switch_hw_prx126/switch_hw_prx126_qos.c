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
 * File    : switch_hw_prx126_qos.c
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

// FIXME! STEVE - currently group=0 and drop_presedence=0 are supported 
// for 1p-remap and dscp-remap functions 
// ----------------------------------------------------------------------------
// Set remapped internal-priority by 1p-priority
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_dot1p2pri_set_by_pri(unsigned int org_pri, unsigned int rmp_pri)
{
	return 0;
}


// ----------------------------------------------------------------------------
// Get remapped internal-priority by 1p-priority
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_dot1p2pri_get_by_pri(unsigned int org_pri, unsigned int *rmp_pri)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print QoS dscp-priority remapping
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_dot1p2pri_print(int fd)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set remapped internal-priority by dscp
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_dscp2pri_set_by_pri(unsigned int dscp, unsigned int rmp_pri)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Get remapped internal-priority by dscp
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_dscp2pri_get_by_pri(unsigned int dscp, unsigned int *rmp_pri)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print QoS dscp-priority remapping
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_dscp2pri_print(int fd)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set QoS queue id for specified group id and priority (set one mapped qid)
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_p2q_set_by_pri(unsigned int group_id, unsigned int pri, unsigned int qid)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set QoS pri-to-queue array by group id (set mapped qid array)
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_p2q_set_by_group(unsigned int group_id, unsigned int *qid)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set QoS pri-to-queue group binding by port id 
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_p2q_group_set_by_port(unsigned int port_id, unsigned int group_id)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print QoS priority to queue id mapping info
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_p2q_print(int fd)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print port-based QoS configurations
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_portcfg_print(int fd)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set QoS port-based priority
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_port_priority_set(unsigned int port_id,  unsigned int pri)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set QoS port-based scheduled weight by queue-id
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_port_scheduled_weight_set_by_qid(unsigned int port_id, unsigned int qid, unsigned int weight)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print QoS queue id to meter id mapping info
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_meter_print(int fd)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set QoS queue id to meter id mapping
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_meter_set_by_qid(unsigned int port_id, unsigned int q_id, unsigned int meter_id)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set QoS priority selection weight by port id and selection type
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_prisel_weight_set(unsigned int port_id, unsigned int sel_type, unsigned int weight)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Print QoS priority selection
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_prisel_weight_print(int fd)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Get port-based mapped internal_queue_id from pbit
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_ds_pbit_pq_map_get(unsigned int port_id, unsigned int pbit, unsigned int *pq_id)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Set port-based mapped internal_queue_id from pbit
// ----------------------------------------------------------------------------
int
switch_hw_prx126_qos_ds_pbit_pq_map_set(unsigned int port_id, unsigned int pbit, unsigned int pq_id)
{
	return 0;
}


