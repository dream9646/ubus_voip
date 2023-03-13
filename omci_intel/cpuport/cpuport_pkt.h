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
 * Module  : cpuport_pkt
 * File    : cpuport_pkt.h
 *
 ******************************************************************/

#ifndef __CPUPORT_PKT_H__
#define __CPUPORT_PKT_H__

enum pkt_cmd_t {
	PKT_CMD_TERMINATE
};

struct pkt_cmd_msg_t {
	enum pkt_cmd_t cmd;
	struct list_head q_entry;
};

/* cpuport_pkt.c */
int cpuport_pkt_is_rma_forward(struct cpuport_info_t *pktinfo);
int cpuport_pkt_init(void);
int cpuport_pkt_shutdown(void);
int cpuport_pkt_start(void);
int cpuport_pkt_stop(void);

extern int pkt_recv_qid_g;
extern int pkt_cmd_qid_g;
extern int pkt_qsetid_g;
#endif
