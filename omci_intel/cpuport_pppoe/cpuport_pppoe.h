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
 * Module  : cpuport_pppoe
 * File    : cpuport_pppoe.h
 *
 ******************************************************************/

#ifndef __CPUPORT_PPPOE_H__
#define __CPUPORT_PPPOE_H__

enum pppoe_cmd_t {
	PPPOE_CMD_LIMIT_CHANGE,
	PPPOE_CMD_TERMINATE
};

struct pppoe_cmd_msg_t {
	enum pppoe_cmd_t cmd;
	unsigned int pppoe_limit;
	struct list_head q_entry;
};

extern int pppoe_pkt_recv_qid_g;
extern int pppoe_cmd_qid_g;
extern int pppoe_qsetid_g;

/* cpuport_pppoe.c */
int cpuport_pppoe_pkt_filter(struct cpuport_info_t *pktinfo);
int cpuport_pppoe_init(void);
int cpuport_pppoe_shutdown(void);
int cpuport_pppoe_start(void);
int cpuport_pppoe_stop(void);
int cpuport_pppoe_count_reset(int fd);
int cpuport_pppoe_count_get(int fd);

#endif
