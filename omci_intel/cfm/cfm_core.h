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
 * Module  : cfm
 * File    : cfm_core.h
 *
 ******************************************************************/
 
#ifndef __CFM_CORE_H__
#define __CFM_CORE_H__

#include "fwk_thread.h"

#define CFM_CCM_TIMER_INTERVAL		500	// unit ms
#define CFM_ALARM_CHECK_INTERVAL	1000	// unit ms

#define CFM_TIMER_EVENT_ALARM_CHECK			0x100
#define CFM_TIMER_EVENT_ETH_LM_DUAL_STOP		1
#define CFM_TIMER_EVENT_ETH_LM_SINGLE_STOP		2
#define CFM_TIMER_EVENT_ETH_LM_SINGLE_START_CHECK	3
#define CFM_TIMER_EVENT_ETH_LM_SINGLE_STOP_CHECK	4
#define CFM_TIMER_EVENT_ETH_SLM_DUAL_STOP_CHECK		5
#define CFM_TIMER_EVENT_ETH_SLM_DUAL_SENDER_LOOP	6
#define CFM_TIMER_EVENT_ETH_SLM_SINGLE_SENDER_LOOP	7
#define CFM_TIMER_EVENT_ETH_SLM_SINGLE_SENDER_STOP	8
#define CFM_TIMER_EVENT_ETH_CC_PERIODIC_CHECK		9
#define CFM_TIMER_EVENT_ETH_LM_SINGLE_RESULT		10

enum cfm_cmd_t {
	CFM_CMD_ME_ADD_MSG,
	CFM_CMD_ME_MODIFY_MSG,
	CFM_CMD_ME_DEL_MSG,
	CFM_CMD_ETH_LM_DUAL_START,
	CFM_CMD_ETH_LM_SINGLE_START,
	CFM_CMD_ETH_SLM_DUAL_START,
	CFM_CMD_ETH_SLM_SINGLE_START,
	CFM_CMD_CCM_INTERVAL_UPDATE,
	CFM_CMD_TERMINATE
};

typedef struct {
	enum cfm_cmd_t cmd;
	unsigned int msglen;
	unsigned char *usr_data;
	unsigned short classid;
	unsigned short meid;
	unsigned short mdlevel;
	unsigned short rmepid;
	unsigned short repeat;
	unsigned short interval;
	struct list_head q_entry;
} cfm_cmd_msg_t;

extern int cfm_task_loop_g;
extern int cfm_cmd_qid_g;
extern int cfm_timer_qid_g;

/* cfm_core.c */
int cfm_init(void);
int cfm_start(void);
int cfm_stop(void);
int cfm_shutdown(void);

#endif
