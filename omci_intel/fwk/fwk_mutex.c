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
 * File    : fwk_mutex.c
 *
 ******************************************************************/

#include "fwk_mutex.h"
#include "fwk_thread.h"
#include "util.h"
#include <sys/types.h>
#include <unistd.h>

int
fwk_mutex_create(struct fwk_mutex_t *mutex_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*init and create of mutex */
	status = pthread_mutex_init(&(mutex_ptr->mutex), NULL);

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_mutex_destroy(struct fwk_mutex_t *mutex_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*destroy mutex */
	status = pthread_mutex_destroy(&(mutex_ptr->mutex));

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_mutex_lock(struct fwk_mutex_t *mutex_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* lock mutex */
	status = pthread_mutex_lock(&(mutex_ptr->mutex));

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_mutex_unlock(struct fwk_mutex_t *mutex_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* release mutex */
	status = pthread_mutex_unlock(&(mutex_ptr->mutex));

	if (status == -1) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}
