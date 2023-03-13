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
 * File    : pmtask.h
 *
 ******************************************************************/

#ifndef __PMTASK_H__
#define __PMTASK_H__

#include <time.h>
#include "list.h"
#include "omcimsg.h"
#include "fwk_timer.h"

extern int pmtask_qid_g;

/* PMTASK function return value */
#define PMTASK_RW_OK				0
#define PMTASK_ERROR_BASE			0x4000
#define PMTASK_ERROR_CMD_ERROR			-(PMTASK_ERROR_BASE + 0x1)
#define PMTASK_ERROR_CMD_NOT_SUPPORTED		-(PMTASK_ERROR_BASE + 0x2)
#define PMTASK_ERROR_PARM_ERROR			-(PMTASK_ERROR_BASE + 0x3)
#define PMTASK_ERROR_UNKNOWN_ME			-(PMTASK_ERROR_BASE + 0x4)
#define PMTASK_ERROR_UNKNOWN_ME_INSTANCE	-(PMTASK_ERROR_BASE + 0x5)
#define PMTASK_ERROR_DEVICE_BUSY		-(PMTASK_ERROR_BASE + 0x6)
#define PMTASK_ERROR_INSTANCE_EXISTS		-(PMTASK_ERROR_BASE + 0x7)
#define PMTASK_ERROR_ATTR_FAILED_UNKNOWN	-(PMTASK_ERROR_BASE + 0x9)

#define PMTASK_ERROR_NOMEM 			-(PMTASK_ERROR_BASE + 0x10)
#define PMTASK_ERROR_START_DUPLICATED 		-(PMTASK_ERROR_BASE + 0x11)
#define PMTASK_ERROR_CMD_NUM_WRONG 		-(PMTASK_ERROR_BASE + 0x12)
#define PMTASK_ERROR_UPLOAD_COMPLETED 		-(PMTASK_ERROR_BASE + 0x13)
#define PMTASK_ERROR_NOT_IN_PROGRESS 		-(PMTASK_ERROR_BASE + 0x14)
#define PMTASK_ERROR_TIMER_ERROR 		-(PMTASK_ERROR_BASE + 0x15)

enum pmtask_cmd_t {
	PMTASK_CMD_SYNC_TIME,
	PMTASK_CMD_TCA_DETERMINATION,
	PMTASK_CMD_TERMINATE
};

enum pmtask_timer_event_t {
	PMTASK_INTERVAL_REFRESH_TIMEOUT
};

struct pmtask_msg_t {
	enum pmtask_cmd_t cmd;
	unsigned short classid;
	unsigned short meid;
	struct list_head q_entry;
};

int pmtask_init(void);
int pmtask_start(void);
int pmtask_shutdown(void);
int pmtask_stop(void);
void pmtask_get_history_time(struct tm *start_time, struct tm *end_time);

#endif	/*  */

