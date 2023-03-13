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
 * File    : coretask.h
 *
 ******************************************************************/

#ifndef __CORETASK_H__
#define __CORETASK_H__

#include "list.h"
#include "omcimsg.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"

#define CORETASK_QUEUE_MAX_LEN FWK_MSGQ_MAX_QUEUE_SIZE

// some events in coretask timer handler
// this should use different value than omcimsg_timer_event_t in omcimsg.h
#define CORETASK_TIMER_EVENT_GCLIST		0x11
#define CORETASK_TIMER_EVENT_ERDIAG		0x12
#define CORETASK_TIMER_EVENT_TCONT_ALIGN	0x13
#define CORETASK_TIMER_EVENT_TAGGING		0x14
#define CORETASK_TIMER_EVENT_BATCHTAB		0x15
#define CORETASK_TIMER_EVENT_SWITCH_HISTORY	0x16
#define CORETASK_TIMER_EVENT_ONESECOND		0x17
#define CORETASK_TIMER_EVENT_ONEMINUTE		0x18

// check interval in coretask loop
#define CORETASK_GCLIST_CHECK_INTERVAL		5
// aging interval for me on gc list
#define CORETASK_GCLIST_AGING_INTERVAL		20
// idle interval before erdiag monitor auto gen output
#define CORETASK_ERDIAG_MON_INTERVAL		1
// tcont flush interval
#define CORETASK_TCONT_ALIGN_INTERVAL		5
// check interval for batchtab hw synn
#define CORETASK_BATCHTAB_CHECK_INTERVAL	1
// ensure omci has been idle for a while before start batchtab iteration
#define CORETASK_BATCHTAB_OMCI_IDLETIME		3
#define CORETASK_OMCI_PERMIT_CHECK_INTERVAL	1

enum coretask_cmd_t {
	CORETASK_CMD_PROCESS_MSG,
	CORETASK_CMD_SWITCH_HISTORY_INTERVAL_CHANGE,
	CORETASK_CMD_MIB_RESET,
	CORETASK_CMD_TERMINATE
};

struct coretask_msg_t {
	enum coretask_cmd_t cmd;
	unsigned char from; //0: from socket: send response, 1: from log: no send response
	unsigned int msglen;
	unsigned char *omci_data;
	struct list_head q_entry;
};

extern int coretask_hi_qid_g;
extern int coretask_lo_qid_g;
extern int coretask_timer_qid_g;

#define CORETASK_CLI_END_STR	"--end--\n"
#define CORETASK_CLI_END_LEN	8

/* coretask.c */
int coretask_get_o5_sequence(void);
int coretask_cmd(enum coretask_cmd_t cmd);
int coretask_init(void);
int coretask_start(void);
int coretask_shutdown(void);
int coretask_stop(void);
int coretask_send_tca_determination_pmtask(unsigned short classid, unsigned short meid);
int coretask_msg_dispatcher(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

#ifdef OMCI_AUTHORIZATION_PROTECT_ENABLE
void coretask_omci_set_time_sync_state(int state, struct timeval *tv);
#endif

#endif	/*  */
