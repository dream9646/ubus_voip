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
 * File    : switch_hw_prx126_init.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

// @sdk/include

#include "util.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "omciutil_vlan.h"
#include "switch.h"
#include "env.h"


int
switch_hw_prx126_init_vlan(void)
{			
	return 0;
}

// -------------------------------------------------------------------
// init switch for omci
// -------------------------------------------------------------------
int
switch_hw_prx126_init(void)
{
	return 0;
}

// -------------------------------------------------------------------
// reset switch to init state when omci is shutdown
// -------------------------------------------------------------------
int
switch_hw_prx126_shutdown(void)
{
	return 0;
}
