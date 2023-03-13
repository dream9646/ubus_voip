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
 * File    : fwk_sem.c
 *
 ******************************************************************/

#include "fwk_sem.h"
#include "fwk_thread.h"
#include "util.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

int
fwk_sem_create(struct fwk_sem_t *sem_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*init and create of semaphore */
	status = sem_init((sem_t *) & (sem_ptr->sem), 0, sem_ptr->value);

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), errno);
		return (-1);
	}

	return (0);
}

int
fwk_sem_destroy(struct fwk_sem_t *sem_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*delete semaphore */
	status = sem_destroy((sem_t *) & (sem_ptr->sem));

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), errno);
		return (-1);
	}

	return (0);
}

int
fwk_sem_lock(struct fwk_sem_t *sem_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* lock semaphore */
	status = sem_wait((sem_t *) & (sem_ptr->sem));

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), errno);
		return (-1);
	}

	return (0);
}

int
fwk_sem_unlock(struct fwk_sem_t *sem_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* release semaphore */
	status = sem_post((sem_t *) & (sem_ptr->sem));

	if (status == -1) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), errno);
		return (-1);
	}

	return (0);
}
