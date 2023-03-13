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
 * File    : switch_hw_prx126_history.c
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

#include "util.h"
#include "list.h"
#include "switch_hw_prx126.h"
#include "switch.h"
#include "env.h"


int
switch_hw_prx126_history_init(void)
{
	return 0;
}


int
switch_hw_prx126_history_update(void)
{
	return 0;
}


int
switch_hw_prx126_history_add()
{
	return 0;
}

// 0:rxpkt, 1:txpkt, 2:fwdrop, 3:rxerr, 4:txerr, 5:rxrate, 6:txrate, 7:rxbyte, 8:txbyte
// 9:rxpktucast, 10:txpktucast, 11:rxpktmcast, 12:txpktmcast, 13:rxpktbcast, 14:txpktbcast
int
switch_hw_prx126_history_print(int fd, int type)
{
	return 0;
}
