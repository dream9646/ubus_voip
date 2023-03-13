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

#ifndef __CPUPORT_HISTORY_H__
#define __CPUPORT_HISTORY_H__

struct cpuport_history_find_t {
	unsigned char mac_addr[6];
	unsigned char mac_mask[6];
	unsigned short eth_proto;
	unsigned char ip_proto;
	unsigned char ip_addr[4];
	unsigned short tcpudp_port;
	unsigned rx_logical_port_mask;
};

/* cpuport_history.c */
void cpuport_history_init(void);
void cpuport_history_shutdown(void);
int cpuport_history_clear(int size);
int cpuport_history_add(struct cpuport_info_t *pktinfo);
int cpuport_history_sync(struct cpuport_info_t *pktinfo);
int cpuport_history_summary(int fd);
int cpuport_history_list(int fd, int start, int end, char *keyword);
int cpuport_history_listp(int fd, int start, int end, char *keyword);
int cpuport_history_show(int fd, int start, int end, char *keyword);
int cpuport_history_defer(int fd, unsigned long defer_us);
int cpuport_history_run(int fd, int start, int end, char *keyword);
int cpuport_history_save(int fd, char *filename, int start, int end, char *keyword);
int cpuport_history_load(int fd, char *filename);
int cpuport_history_prepare_find(int fd, struct cpuport_history_find_t *find, int argc, char *argv[]);
int cpuport_history_list_find(int fd, struct cpuport_history_find_t *find);

#endif

