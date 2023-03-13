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
 * Module  : cpuport
 * File    : cpuport_recv.h
 *
 ******************************************************************/

#ifndef __CPUPORT_RECV_H__
#define __CPUPORT_RECV_H__

/* cpuport_recv.c */
int cpuport_recv_dispatch(struct cpuport_info_t *cpuport_info);
int cpuport_recv_init(void);
int cpuport_recv_shutdown(void);
int cpuport_recv_start(void);
int cpuport_recv_stop(void);

#endif
