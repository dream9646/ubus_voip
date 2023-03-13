/******************************************************************
 *
 * Copyright (C) 2013 5V Technologies Ltd.
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
 * File    : switch_hw_prx126_vacl.c
 *
 ******************************************************************/

#include <stdlib.h>

// @sdk/include

#include "chipdef.h"

#include "list.h"

#ifndef	ACLTEST
#include "util.h"
#else
#include "vacltest_util.h"
#endif

#include "util_inet.h"
#include "conv.h"
#include "vacl.h"
#include "vacl_util.h"
#include "vacl_diag_str.h"
#include "switch_hw_prx126.h"
#include "switch_hw_prx126_proto.h"

/*
 * Data Declaration
 */


////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
int
switch_hw_prx126_vacl_init(void)
{
	return 0;
}

int
switch_hw_prx126_vacl_shutdown(void)
{
	return 0;
}

////////////////////////////////////////////////////////////////////
int
switch_hw_prx126_vacl_port_enable_set(unsigned int port_id, unsigned int en)
{
	return 0;
}

int
switch_hw_prx126_vacl_port_permit_set(unsigned int port_id, unsigned int en)
{
	return 0;
}

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
int
switch_hw_prx126_vacl_meter_clear(int index)
{

	return 0;
}

int
switch_hw_prx126_vacl_clear(void)
{
	return 0;
}


int
switch_hw_prx126_vacl_rule_evaluate(struct vacl_user_node_t *rule_p)
{
	return 0;
}

////////////////////////////////////////////////////////////////////
int
switch_hw_prx126_vacl_rule_valid_get(int entry_index, int *valid_state)
{
	return 0;
}

int
switch_hw_prx126_vacl_commit(int *count)
{
	return 0;
}

////////////////////////////////////////////////////////////////////
int
switch_hw_prx126_vacl_port_enable_print(int fd)
{
	return 0;
}

int
switch_hw_prx126_vacl_port_permit_print(int fd)
{
	return 0;
}

////////////////////////////////////////////////////////////////////

// -------------------------------------------------------------------
// Print all or part of ACL entries by assigned start index and count
// -------------------------------------------------------------------
int
switch_hw_prx126_vacl_entry_print(int fd, unsigned int start_idx)
{
	return 0;
}

int
switch_hw_prx126_vacl_dump(int fd, int hw_order, int order)
{
	return 0;
}

////////////////////////////////////////////////////////////////////

