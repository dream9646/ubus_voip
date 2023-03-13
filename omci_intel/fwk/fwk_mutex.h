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
 * File    : fwk_mutex.h
 *
 ******************************************************************/

#ifndef __FWK_MUTEX_H__
#define __FWK_MUTEX_H__

#include <pthread.h>

struct fwk_mutex_t {
	pthread_mutex_t mutex;	//platform dependent mutex structure
};

int fwk_mutex_create(struct fwk_mutex_t *mutex_ptr);
int fwk_mutex_destroy(struct fwk_mutex_t *mutex_ptr);
int fwk_mutex_lock(struct fwk_mutex_t *mutex_ptr);
int fwk_mutex_unlock(struct fwk_mutex_t *mutex_ptr);

#endif
