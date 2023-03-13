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
 * File    : testtask.h
 *
 ******************************************************************/

#ifndef __TESTTASK_H__
#define __TESTTASK_H__

#include "list.h"
#include "omcimsg.h"
#include "fwk_timer.h"

#define TEST_RESULT_SIZE			32

extern int testtask_msg_qid_g;

/* TESTTASK function return value */
#define TESTTASK_RW_OK				0
#define TESTTASK_ERROR_BASE			0x4000
#define TESTTASK_ERROR_UNKNOWN_ME		-(TESTTASK_ERROR_BASE+1)
#define TESTTASK_ERROR_UNKNOWN_ME_INSTANCE	-(TESTTASK_ERROR_BASE+2)
#define TESTTASK_ERROR_CMD_NOT_SUPPORTED	-(TESTTASK_ERROR_BASE+3)

enum testtask_cmd_t {
	TESTTASK_CMD_TEST,
	TESTTASK_CMD_TERMINATE
};

enum testtask_timer_event_t {
	TESTTASK_INTERVAL_REFRESH_TIMEOUT
};

struct testtask_msg_t {
	enum testtask_cmd_t cmd;
	unsigned short classid;
	unsigned short meid;
	struct omcimsg_layout_t *req_msg;
	struct list_head q_entry;
};

int testtask_init(void);
int testtask_start(void);
int testtask_shutdown(void);
int testtask_stop(void);

#endif

