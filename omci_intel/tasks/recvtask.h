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
 * Module  : tasks
 * File    : recvtask.h
 *
 ******************************************************************/

#ifndef __RECVTASK_H__
#define __RECVTASK_H__

#include "omcimsg.h"

#define RECVTASK_LOOPBACK_ADDR "127.0.0.1"
#define RECVTASK_LOOPBACK_PORT 2323
#define RECVTASK_BUFFER_SIZE 2048

#define RECVTASK_ERROR_BASE 0x3000
#define RECVTASK_SEND_ERROR_SEND_ERROR -(RECVTASK_ERROR_BASE + 1)
#define RECVTASK_SEND_ERROR_SEND_DATA -(RECVTASK_ERROR_BASE + 2)

#ifdef INTEL_OMCC_LAYER
int recvtask_init_intel_omcc(void);
int recvtask_shutdown_intel_omcc(void);

#else
int recvtask_init(void);
int recvtask_start(void);
int recvtask_shutdown(void);
int recvtask_stop(void);
#endif

int recvtask_util_send_omci_msg(struct omcimsg_layout_t *msg, int ms_diff, unsigned char from);

#endif
