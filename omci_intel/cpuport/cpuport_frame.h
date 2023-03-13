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
 * File    : cpuport_frame.h
 *
 ******************************************************************/

#ifndef __CPUPORT_FRAME_H__
#define __CPUPORT_FRAME_H__

/* cpuport_frame.c */
int cpuport_frame_is_available(struct timeval *timeout);
int cpuport_frame_recv(struct cpuport_info_t *pktinfo, unsigned short buf_len);
int cpuport_frame_recv_abort_select(void);
int cpuport_frame_recv_abort_loop(void);
int cpuport_frame_send(struct cpuport_info_t *pktinfo);

#endif
