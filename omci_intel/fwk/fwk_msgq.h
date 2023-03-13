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
 * File    : fwk_msgq.h
 *
 ******************************************************************/

#ifndef __FWK_MSGQ_H__
#define __FWK_MSGQ_H__

#include "fwk_cond.h"
#include "fwk_mutex.h"
#include "list.h"

#define FWK_MSGQ_ERROR_BASE 0x6000
#define FWK_MSGQ_ERROR_FULL -(FWK_MSGQ_ERROR_BASE + 1)
#define FWK_MSGQ_ERROR_EMPTY -(FWK_MSGQ_ERROR_BASE + 2)
#define FWK_MSGQ_ERROR_OUTRANGE -(FWK_MSGQ_ERROR_BASE + 3)
#define FWK_MSGQ_ERROR_OCCUPIED -(FWK_MSGQ_ERROR_BASE + 4)
#define FWK_MSGQ_ERROR_UNALLOC -(FWK_MSGQ_ERROR_BASE + 5)

#define FWK_MSGQ_NANE_SIZE 32	
#define FWK_MSGQ_MAX_QUEUE 64	//multiple of 32
#define FWK_MSGQ_MAX_QUEUE_SIZE 1024
#define FWK_MSGQ_ALLOC_ID_UNASSIGN (-1)
#define FWK_MSGQ_WAIT_INFINITE (-1)

struct fwk_msgq_t {
	char q_name[FWK_MSGQ_NANE_SIZE];
	unsigned char is_used;	//0: not used, 1: used
	unsigned int q_len;
	unsigned int q_size;
	// statistics
	unsigned int q_len_max;	// the max len that ever happen since last dump
	unsigned int q_add;	// the add that ...
	unsigned int q_del;	// the del that ...
	unsigned int q_drop;	// the drop because of q full that ...
	struct fwk_mutex_t mutex;
	struct list_head cond_list;	//store the condition variables waiting on this queue
	struct list_head q;
};

struct fwk_msgq_set_t {
	unsigned char is_used;	//0: not used, 1: used
	struct fwk_cond_t cond;
	struct fwk_mutex_t mutex;
	unsigned int msgq_bitmap[FWK_MSGQ_MAX_QUEUE / 32];
	unsigned int msgq_bitmap_result[FWK_MSGQ_MAX_QUEUE / 32];
};

struct fwk_msgq_cond_node_t {
	unsigned int cond_num;
	struct list_head cond_list;
};

int fwk_msgq_module_init(void);
int fwk_msgq_module_shutdown(void);
int fwk_msgq_alloc(int q_id, unsigned int q_size, char *q_name);
int fwk_msgq_release(unsigned int q_id);
int fwk_msgq_set_alloc(void);
int fwk_msgq_set_set(unsigned int set_id, unsigned int q_id);
int fwk_msgq_set_clear(unsigned int set_id, unsigned int q_id);
int fwk_msgq_set_zero(unsigned int set_id);
int fwk_msgq_set_release(unsigned int set_id);
int fwk_msgq_set_isset(unsigned int set_id, unsigned int q_id);
int fwk_msgq_select(unsigned int set_id, int timeout);
int fwk_msgq_recv(unsigned int q_id, struct list_head **node);
int fwk_msgq_send(unsigned int q_id, struct list_head *node);
int fwk_msgq_change_size(unsigned int q_id, unsigned int q_size);
int fwk_msgq_len(unsigned int q_id);
char *fwk_msgq_name(unsigned int q_id);
void fwk_msgq_dump(int fd);

#endif
