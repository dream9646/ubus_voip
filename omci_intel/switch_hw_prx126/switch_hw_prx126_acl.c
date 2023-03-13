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
 * File    : switch_hw_prx126_acl.c
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
#include "vacl_util.h"


// -------------------------------------------------------------------
// Set an ACL entry at specified index id
// -------------------------------------------------------------------
// -------------------------------------------------------------------
// Delete acl entry by index
// -------------------------------------------------------------------
int
switch_hw_prx126_acl_entry_delete(unsigned int index)
{
	return 0;
}

// -------------------------------------------------------------------
// flush acl entry table
// -------------------------------------------------------------------
int
switch_hw_prx126_acl_entry_flush(void)
{
	return 0;
}

// -------------------------------------------------------------------
// enable/disable acl entry by index id
// -------------------------------------------------------------------
int
switch_hw_prx126_acl_entry_enable_set(unsigned int idx, unsigned int en)
{
	return 0;
}

////////////////////////////////////////////////////////////////////
// --------------------------------------------------------------------------------------
// Get free entry count and the first free ACL entry index from top or bottom
// in specified table range based on starting index (*entry_idx) and ending_idx
// --------------------------------------------------------------------------------------
// INPUT:
//   *entry_idx: starting searching index
//   ending_idx: ending searching index
//   (these two indexes decide the searching range and direction (from top or bottom)
// RETURN:
//   *entry_idx: first index of the free entry in the range.
//               HAL_MAX_NUM_OF_ACL_RULE_ENTRY() is returned if no free entry found
//   *count: total count of free entry in the range
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_acl_entry_get_free_info(unsigned int *entry_idx, unsigned int ending_idx, unsigned int *count)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// the following functios are for user-defined acl fields(FS_08~14) and template(2)
// if contents in template 2(FS_00~07) and FS_08~14 in template 3 can be overwritten
// [ie. reserve default template 0,1 and FS_15(valid tag) in template 3]
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Set user-defined filter field template using fixed template 2
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_acl_user_template_set(unsigned int *field_typeid)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Get user-defined filter field template using fixed template 2
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_acl_user_template_get(unsigned int *field_typeid)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Set user-defined filter field selector by index (0~6-->FS_08~14)
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_acl_user_field_set(unsigned int index, unsigned int field_format, unsigned int offset)
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Get user-defined filter field template using fixed template 2
// --------------------------------------------------------------------------------------
int
switch_hw_prx126_acl_user_field_get(unsigned int index, unsigned int *field_format, unsigned int *offset)
{
	return 0;
}
