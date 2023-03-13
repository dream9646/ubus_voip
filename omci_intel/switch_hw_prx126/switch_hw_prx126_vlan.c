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
 * File    : switch_hw_prx126_vlan.c
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

// -------------------------------------------------------------------
// Enable/disable VLAN function
// -------------------------------------------------------------------
int
switch_hw_prx126_vlan_enable(unsigned int enable)
{
	return 0;
}

// -------------------------------------------------------------------
// Tag/untag reserved VLAN function
// -------------------------------------------------------------------
int
switch_hw_prx126_vlan_reserved_action(unsigned int is_tag)
{
	return 0;
}

// -------------------------------------------------------------------
// Print VLAN entry table
// -------------------------------------------------------------------
int
switch_hw_prx126_vlan_entry_print_by_vid(int fd, unsigned int vid)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Set default VLAN group for untagging traffic (only one default group in system)
// --------------------------------------------------------------------------------------
// 1. Get current default entry based on current uplink port PVID
// 2. Set new default VLAN entry for untagged traffic based on assigned vid
// 3. Update old default VLAN entry for tagged traffic based on current uplink port PVID
// 4. Update new port PVID to all ports based on assigned vid
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_vlan_entry_default_set(unsigned int vid)
{
	return 0;
}

// -------------------------------------------------------------------------------------------
// Set VLAN egress tag and keep modes
// -------------------------------------------------------------------------------------------
// port:       egress port  	
// mode:       0- VLAN_TAG_MODE_ORIGINAL 
// 		       1- VLAN_TAG_MODE_KEEP_FORMAT
//		       2- VLAN_TAG_MODE_PRI
// egkeep_msk: ingress portmask for keeping egress content unchanged
// -------------------------------------------------------------------------------------------
int
switch_hw_prx126_vlan_egress_mode_set(unsigned int port, unsigned int mode, unsigned int egkeep_msk)
{
	return 0;
}

// -------------------------------------------------------------------------------------------
int
switch_hw_prx126_vlan_egress_mode_print(int fd)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Get port members for existing VLAN entry by vid
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_vlan_entry_get(unsigned int vid, unsigned int *mbr_msk, unsigned int *utg_msk)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Set port members for existing VLAN entry by vid, member portmask and untag portmask
// --------------------------------------------------------------------------------------
// vlan group can be created and set if the entry is not found in the table
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_vlan_entry_set(unsigned int vid, unsigned int mbr_msk, unsigned int utg_msk)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Delete existing VLAN entry by vid
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_vlan_entry_delete(unsigned int vid)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Flush VLAN entry table
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_vlan_entry_flush(unsigned int keep_default)
{
	return 0;
}

// ------------------------------------------------------------------------------------
// Enable/disable default VLAN group member for untagging traffic
// ------------------------------------------------------------------------------------
int
switch_hw_prx126_vlan_entry_untagged_op(unsigned int port_id, unsigned int fwd_op)
{
	return 0;
}


