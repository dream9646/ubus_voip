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
 * Module  : msgq
 * File    : fwk_timer.h
 *
 ******************************************************************/

#ifndef __FWK_TIMER_H__
#define __FWK_TIMER_H__

#include "list.h"

enum fwk_timer_op_t {
	FWK_TIMER_OP_ADD,	/*for user send to timer */
	FWK_TIMER_OP_DELETE,	/*for user send to timer */
	FWK_TIMER_OP_MODIFY,	/*for user send to timer */
	FWK_TIMER_OP_TIMEOUT,	/*for timer send to user */
	FWK_TIMER_OP_TERMINATE,	/*for user send to timer */
	FWK_TIMER_OP_DUMP,	/*for user send to timer */
	FWK_TIMER_OP_FAIL	/*for timer send to user */
};

struct fwk_timer_msg_t {
	unsigned short timer_id;	//an uniqe timer id
	enum fwk_timer_op_t op;	//timer operation
	unsigned int event;	//user specific
	unsigned int q_id;	//q_id, the queue that timeout send back
	unsigned int timeout;	//timeout value
	unsigned int lefttimeout;	//time left for timeout
	void *user_data;	//carry user's data for timeout handler, never malloc or free by timer
	struct list_head node;	/*for timer queue, timer table and application queue */
};

struct fwk_timer_table_t {
	unsigned int timer_count;
	int loop;		/*control timer thread loop */
	struct list_head timer_list;
};

int fwk_timer_init(void);
int fwk_timer_shutdown(void);
int fwk_timer_start(void);
int fwk_timer_stop(void);
int fwk_timer_create(unsigned int q_id, unsigned int event, unsigned int timeout, void *user_data);	//ms
int fwk_timer_delete(unsigned short timer_id);
int fwk_timer_modify(unsigned short timer_id, unsigned int q_id, unsigned int event, int timeout, void *user_data);
int fwk_timer_dump(void);
int fwk_timer_terminate(void);

#endif
