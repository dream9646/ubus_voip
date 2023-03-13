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
 * File    : switch_hw_prx126_int.c
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
// Interrupt
// -------------------------------------------------------------------
int
switch_hw_prx126_int_ctrl_set(unsigned int type_idx, unsigned int enable)
{
	return 0;
}

int
switch_hw_prx126_int_ctrl_get(unsigned int type_idx, unsigned int *pEnable)
{
	return 0;
}

int
switch_hw_prx126_int_ctrl_print(int fd)
{
	return 0;
}

int
switch_hw_prx126_int_stat_set(unsigned int type_idx)
{
	return 0;
}

int
switch_hw_prx126_int_stat_get(unsigned int type_idx, unsigned int *result)
{
	return 0;
}

int
switch_hw_prx126_int_stat_print(int fd)
{
	return 0;
}

