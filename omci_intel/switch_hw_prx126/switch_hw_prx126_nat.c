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
 * File    : switch_hw_prx126_nat.c
 *
 ******************************************************************/
#include "util.h"

int
switch_hw_prx126_nat_session_hw_print(int fd)
{
	return 0;	
}

int
switch_hw_prx126_nat_session_sw_print(int fd)
{
	util_fdprintf(fd, "TBD and implement!\n");
	return 0;
}

