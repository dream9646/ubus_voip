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
 * File    : avctask.h
 *
 ******************************************************************/

#ifndef __AVCTASK_H__
#define __AVCTASK_H__

#include "list.h"
#include "omcimsg.h"
#include "fwk_timer.h"

#define AVC_VALUE_SIZE			30

/* AVCTASK function return value */
#define AVCTASK_RW_OK			0
#define AVCTASK_ERROR_BASE		0x4000
#define AVCTASK_ERROR_UNKNOWN_ME	-(AVCTASK_ERROR_BASE+1)
#define AVCTASK_ERROR_TIMER_ERROR 	-(AVCTASK_ERROR_BASE+2)

enum avctask_cmd_t {
	AVCTASK_CMD_CHECKVALUE,
	AVCTASK_CMD_TERMINATE
};

enum avctask_timer_event_t {
	AVCTASK_INTERVAL_REFRESH_TIMEOUT
};

struct avctask_msg_t {
	enum avctask_cmd_t cmd;
	unsigned short classid;
	unsigned short instance_num;	// if classid==0 && instance_num==0, check all me
	struct list_head q_entry;
};

extern int avctask_msg_qid_g;

int avctask_init(void);
int avctask_start(void);
int avctask_shutdown(void);
int avctask_stop(void);

#endif	/*  */

