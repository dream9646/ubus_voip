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
 * File    : switch_hw_prx126_svlan.c
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


#include "chipdef.h"
#include "util.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "omciutil_misc.h"
#include "switch.h"
#include "env.h"

// --------------------------------------------------------------------------------------
// Enable/Disable SVLAN service port by portid
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_svlan_service_port_set(unsigned int port_id, unsigned int en)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Set SVLAN tpid value
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_svlan_tpid_set(unsigned int tpid)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Set SVLAN port-based svid
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_svlan_pvid_set(unsigned int port_id, unsigned int svid)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Get port members for existing SVLAN entry by svid
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_svlan_entry_member_get(unsigned int svid, unsigned int *mbr_msk, unsigned int *utg_msk)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Set port members for existing SVLAN entry by svid, member portmask and untag portmask
// --------------------------------------------------------------------------------------
// vlan group can be created and set if the entry is not found in the table
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_svlan_entry_member_set(unsigned int svid, unsigned int mbr_msk, unsigned int utg_msk)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Delete existing SVLAN entry by svid
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_svlan_entry_delete(unsigned int svid)
{
	return 0;
}

// -------------------------------------------------------------------
// Get SVLAN entry config (pri, fid, fid_en, efid, efid_en) by svid
// -------------------------------------------------------------------
int
switch_hw_prx126_svlan_entry_config_get(unsigned int svid, unsigned int *pri, unsigned int *fid_en, unsigned int *fid, unsigned int *efid_en, unsigned int *efid)
{
	return 0;
}

// -------------------------------------------------------------------
// Set SVLAN entry config (pri, fid, fid_en, efid, efid_en) by svid
// -------------------------------------------------------------------
int
switch_hw_prx126_svlan_entry_config_set(unsigned int svid, unsigned int pri, unsigned int fid_en, unsigned int fid, unsigned int efid_en, unsigned int efid)
{
	return 0;
}

// -------------------------------------------------------------------
// Print SVLAN entry table
// -------------------------------------------------------------------
int
switch_hw_prx126_svlan_entry_print(int fd)
{
	return 0;
}

