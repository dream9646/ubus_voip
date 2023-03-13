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
 * Module  : voipclient
 * File    : voipclient_comm.h
 *
 ******************************************************************/

#ifndef COMMON_H_
#define COMMON_H_
#include <netinet/in.h>

#ifndef NULL
#define NULL  ((void *)0)
#endif

#define BUFFER_SIZE 4096
#define UDP_TIMEOUT 2		/* 2 s */
#define SINGLE_PKT_RESP	1	/* the response always in only 1 UDP */

typedef struct voip_connect_t {
	int socket;
	struct sockaddr_in remoteaddr;
	struct sockaddr_in localaddr;
} voipclient_comm_t;

extern struct voip_connect_t *voipclient_comm_create();
extern int voipclient_comm_connect(struct voip_connect_t *connect_p, char *ip, int port);
extern int voipclient_comm_bind(struct voip_connect_t *connect_p, char *ip, int port);
extern void voipclient_comm_destroy(struct voip_connect_t *connect_p);
extern int voipclient_comm_cli(const char *command, char *response, const int response_len);
extern int voipclient_comm_set_timeout(int timeout);
extern int voipclient_comm_check_ipphone_exist(void);
extern int voipclient_comm_check_proxy_exist(void);

#endif
