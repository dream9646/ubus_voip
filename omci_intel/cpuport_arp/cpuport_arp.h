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
 * Module  : cpuport_arp
 * File    : cpuport_arp.h
 *
 ******************************************************************/

#ifndef __CPUPORT_ARP_H__
#define __CPUPORT_ARP_H__

struct cpuport_arper_t
{
	unsigned int arp_limit;
	unsigned int arp_total;
	unsigned int time;
};

enum arp_cmd_t {
	ARP_CMD_LIMIT_CHANGE,
	ARP_CMD_TERMINATE
};

struct arp_cmd_msg_t {
	enum arp_cmd_t cmd;
	unsigned int arp_limit;
	struct list_head q_entry;
};

/* cpuport_arp.c */
int cpuport_arp_pkt_filter(struct cpuport_info_t *pktinfo);
int cpuport_arp_init(void);
int cpuport_arp_shutdown(void);
int cpuport_arp_start(void);
int cpuport_arp_stop(void);

extern struct cpuport_arper_t cpuport_arper_g;
extern int arp_pkt_recv_qid_g;
extern int arp_cmd_qid_g;
extern int arp_qsetid_g;
#endif
