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
 * File    : fwk_cond.c
 *
 ******************************************************************/

#include "fwk_cond.h"
#include "fwk_thread.h"
#include "util.h"
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

int
fwk_cond_create(struct fwk_cond_t *cond_ptr, struct fwk_mutex_t *mutex_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*init condtion variable */
	status = pthread_cond_init(&cond_ptr->cond, NULL);

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	/*assign mutex struct for condition variable */
	cond_ptr->mutex_ptr = mutex_ptr;

	return (0);
}

int
fwk_cond_destroy(struct fwk_cond_t *cond_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* destroy condition variable */
	status = pthread_cond_destroy(&cond_ptr->cond);

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_cond_signal(struct fwk_cond_t *cond_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*unblock  thread  blocked on a condition */
	status = pthread_cond_signal(&cond_ptr->cond);

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_cond_timedwait(struct fwk_cond_t *cond_ptr, unsigned int timeout)
{
	int status;
	struct timespec cond_timeout;
	struct timeval currtime;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* Calculate the absolute time for the timeout */
	gettimeofday(&currtime, NULL);
	cond_timeout.tv_sec = currtime.tv_sec + (timeout / 1000);
	if ((currtime.tv_usec + (timeout % 1000) * 1000) >= 1000000) {
		cond_timeout.tv_sec++;
		cond_timeout.tv_nsec = ((currtime.tv_usec + (timeout % 1000) * 1000) % 1000000) * 1000;
	} else {
		cond_timeout.tv_nsec = (currtime.tv_usec + (timeout % 1000) * 1000) * 1000;
	}

	/* Blocking call thread on a condition with time */
	status = pthread_cond_timedwait(&cond_ptr->cond, &(cond_ptr->mutex_ptr->mutex), &cond_timeout);

	if (status != 0) {
		if (status == ETIMEDOUT) {
			dbprintf_fwk(LOG_INFO, "pid(%d), timeout(%d)\n", fwk_thread_gettid(), timeout);
			return (FWK_COND_ERROR_TIMEOUT);
		} else {
			dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
			return (-1);
		}
	}
	return (0);
}

int
fwk_cond_wait(struct fwk_cond_t *cond_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*block call thread on a condition without time */
	status = pthread_cond_wait(&cond_ptr->cond, &cond_ptr->mutex_ptr->mutex);

	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}
