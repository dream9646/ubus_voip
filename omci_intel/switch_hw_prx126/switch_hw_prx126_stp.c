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
 * File    : switch_hw_prx126_stp.c
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

#include "bitmap.h"	//stp
#include "uid_stp.h"	//stp
#include "stp_in.h"	//stp

// -------------------------------------------------------------------
// STP state
// -------------------------------------------------------------------
int
switch_hw_prx126_stp_state_init(void)
{
	return 0;
}

int
switch_hw_prx126_stp_state_shutdown(void)
{
	return 0;
}

int
switch_hw_prx126_stp_designate_data_get(unsigned int port, struct stp_designate_data_t *designate_data)
{
	return 0;
}

int
switch_hw_prx126_stp_state_print(int fd)
{
	return 0;
}

int
switch_hw_prx126_stp_state_port_get(unsigned int port , unsigned int *stp_state)
{
	return 0;
}


int
switch_hw_prx126_stp_state_port_set(unsigned int port , unsigned int stp_state)
{
	return 0;
}
