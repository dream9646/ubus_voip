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
 * File    : fwk_sem.h
 *
 ******************************************************************/

#ifndef __FWK_SEM_H__
#define __FWK_SEM_H__

#include <semaphore.h>

struct fwk_sem_t {
	int value;		//initial value for semaphore
	sem_t sem;		//platform dependent semaphore structure
};

int fwk_sem_create(struct fwk_sem_t *sem_ptr);
int fwk_sem_delete(struct fwk_sem_t *sem_ptr);
int fwk_sem_lock(struct fwk_sem_t *sem_ptr);
int fwk_sem_unlock(struct fwk_sem_t *sem_ptr);
#endif
