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
 * File    : fwk_cond.h
 *
 ******************************************************************/

#ifndef __FWK_COND_H__
#define __FWK_COND_H__

#include "fwk_mutex.h"
#include <pthread.h>

#define FWK_COND_ERROR_BASE 0x5000

#define FWK_COND_ERROR_TIMEOUT -(FWK_COND_ERROR_BASE + 1)

struct fwk_cond_t {
	struct fwk_mutex_t *mutex_ptr;	//mutex structure for condition variable
	pthread_cond_t cond;	//platform depenedent condition variable structure 
};

int fwk_cond_create(struct fwk_cond_t *cond_ptr, struct fwk_mutex_t *mutex_ptr);
int fwk_cond_destroy(struct fwk_cond_t *cond_ptr);
int fwk_cond_signal(struct fwk_cond_t *cond_ptr);
int fwk_cond_timedwait(struct fwk_cond_t *cond_ptr, unsigned int timeout);
int fwk_cond_wait(struct fwk_cond_t *cond_ptr);

#endif
