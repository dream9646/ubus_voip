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
 * File    : fwk_rwlock.h
 *
 ******************************************************************/

#ifndef __FWK_RWLOCK_H__
#define __FWK_RWLOCK_H__

#include <pthread.h>

struct fwk_rwlock_t {
	pthread_rwlock_t rwlock;	//platform dependent rwlock structure
};

int fwk_rwlock_create(struct fwk_rwlock_t *rwlock_ptr);
int fwk_rwlock_destroy(struct fwk_rwlock_t *rwlock_ptr);
int fwk_rwlock_rdlock(struct fwk_rwlock_t *rwlock_ptr);
int fwk_rwlock_wrlock(struct fwk_rwlock_t *rwlock_ptr);
int fwk_rwlock_unlock(struct fwk_rwlock_t *rwlock_ptr);

#endif
