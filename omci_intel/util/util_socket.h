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
 * Module  : util
 * File    : util_socket.h
 *
 ******************************************************************/

#ifndef __UTIL_SOCKET_H__
#define __UTIL_SOCKET_H__

int util_create_raw_socket(char *ifname, struct sockaddr_ll *sockaddr);
int util_create_udp_socket(unsigned int ip, unsigned short port, struct sockaddr_in *sockaddr);
int util_create_tcp_socket(unsigned int ip, unsigned short port, struct sockaddr_in *sockaddr);

#endif