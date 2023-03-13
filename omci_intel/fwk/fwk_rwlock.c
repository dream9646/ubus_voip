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
 * File    : fwk_rwlock.c
 *
 ******************************************************************/

#include "fwk_rwlock.h"
#include "fwk_thread.h"
#include "util.h"
#include <sys/types.h>
#include <unistd.h>

int
fwk_rwlock_create(struct fwk_rwlock_t *rwlock_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*init and create of rwlock */
	status = pthread_rwlock_init(&(rwlock_ptr->rwlock), NULL);

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_rwlock_destroy(struct fwk_rwlock_t *rwlock_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*destroy rwlock */
	status = pthread_rwlock_destroy(&(rwlock_ptr->rwlock));

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_rwlock_rdlock(struct fwk_rwlock_t *rwlock_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* lock rwlock */
	status = pthread_rwlock_rdlock(&(rwlock_ptr->rwlock));

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_rwlock_wrlock(struct fwk_rwlock_t *rwlock_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* lock rwlock */
	status = pthread_rwlock_wrlock(&(rwlock_ptr->rwlock));

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_rwlock_unlock(struct fwk_rwlock_t *rwlock_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* release rwlock */
	status = pthread_rwlock_unlock(&(rwlock_ptr->rwlock));

	if (status == -1) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}
