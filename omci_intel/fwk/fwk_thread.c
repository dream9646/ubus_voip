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
 * File    : fwk_thread.c
 *
 ******************************************************************/

#include "fwk_thread.h"
#include "util.h"
#include <errno.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

int
fwk_thread_create(struct fwk_thread_t *thread_ptr, void *(*thread_entry) (void *), void *arg)
{
	int status, i;
	pthread_attr_t thread_attr;	/*store pthread attributes */
	struct sched_param sch_param_t;	/*set pthread schedule policy */
	int policy;		/*schedule policy */
	sigset_t sigmask;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*init the attributes to default value */
	pthread_attr_init(&thread_attr);

	if (thread_ptr->join == 0) {
		/*not join */
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	} else {
		/*do nothing, default: join */
	}

	/*set stack size */
	pthread_attr_setstacksize(&thread_attr, thread_ptr->stack_size);

	/*set schedule policy */
	// policy FIFO/RR: realtime with prio 1(low)..99(high)
	// policy OTHER: timesharing round-robin with proi 0 
	switch (thread_ptr->policy) {
	case FWK_THREAD_SP_FIFO:
		policy = SCHED_FIFO;
		break;
	case FWK_THREAD_SP_RR:
		policy = SCHED_RR;
		break;
	case FWK_THREAD_SP_NORMAL:
	default:
		policy = SCHED_OTHER;
	}
	pthread_attr_setschedpolicy(&thread_attr, policy);

	/*set priority */
	sch_param_t.sched_priority = (policy == SCHED_OTHER)?0:thread_ptr->priority;

	/*set param to attr */
	pthread_attr_setschedparam(&thread_attr, &sch_param_t);

	/* specify explicit scheduling */
	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);

	/* Thread creation and Initialization of its attributes */
	for(i=0; i < 3; i++) {
		status = pthread_create((pthread_t *) & (thread_ptr->thread_id), &thread_attr, thread_entry, arg);
		if(status == 0 || errno != EAGAIN)
			break;
	}

	/* mask signal SIGALRM off by default */
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGALRM);
	pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

	/*error handle */
	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);	//error
	}

	return (0);		//success
}

void
fwk_thread_exit(void)
{
	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	pthread_exit(NULL);
}

int
fwk_thread_join(struct fwk_thread_t *thread_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* block thread */
	status = pthread_join((pthread_t) thread_ptr->thread_id, NULL);

	/*error handle */
	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_thread_cancel(struct fwk_thread_t *thread_ptr)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* cancel thread */
	status = pthread_cancel((pthread_t) thread_ptr->thread_id);

	/*error handle */
	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_thread_kill(struct fwk_thread_t *thread_ptr, int signal)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/* signal to thread */
	status = pthread_kill((pthread_t) thread_ptr->thread_id, signal);

	/*error handle */
	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_thread_yield(void)
{
	int status;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	status = sched_yield();

	/*error handle */
	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return (0);
}

int
fwk_thread_gettid(void)
{
	return syscall(__NR_gettid);
}

int
fwk_thread_set_cancellable(void)
{
	int status;
	int old_state, old_type;

	status = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);

	/*error handle */
	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	status = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_type);

	/*error handle */
	if (status != 0) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), status);
		return (-1);
	}

	return 0;
}

// policy FIFO/RR: realtime with prio 1(low)..99(high)
// policy OTHER: timesharing round-robin with proi 0 
int
fwk_thread_get_priority(int pid, enum fwk_thread_sp_t *policy, int *priority)
{
	struct sched_param sp;
	int sched_policy = sched_getscheduler(pid);
	
	switch (sched_policy) {
	case SCHED_FIFO:
		*policy = FWK_THREAD_SP_FIFO;
		break;
	case SCHED_RR:
		*policy = FWK_THREAD_SP_RR;
		break;
	case SCHED_OTHER:
	default:
		*policy = FWK_THREAD_SP_NORMAL;
	}
	sched_getparam(pid, &sp);
	*priority = sp.sched_priority;

	return 0;
}

// policy FIFO/RR: realtime with prio 1(low)..99(high)
// policy OTHER: timesharing round-robin with proi 0 
int
fwk_thread_set_priority(int pid, enum fwk_thread_sp_t policy, int priority)
{
	struct sched_param sp;

	sched_getparam(pid, &sp);
	switch(policy)
	{
	case FWK_THREAD_SP_NORMAL:
		sp.sched_priority = 0;
		sched_setscheduler(pid, SCHED_OTHER, &sp);
		break;
	case FWK_THREAD_SP_FIFO:
		sp.sched_priority = priority;
		sched_setscheduler(pid, SCHED_FIFO, &sp);
		break;
	case FWK_THREAD_SP_RR:
		sp.sched_priority = priority;
		sched_setscheduler(pid, SCHED_RR, &sp);
		break;
	default:
		sched_setscheduler(pid, SCHED_OTHER, &sp);
	}

	return 0;
}

int
fwk_thread_run_function(void *(*func)(void *), void *data, int is_sync, enum fwk_thread_sp_t policy, unsigned priority)
{
	struct fwk_thread_t t;
	int ret;
	t.policy = policy;
	t.priority = priority;
	t.stack_size = 128*1024;
	t.join = is_sync ? 1 : 0;
	sprintf(t.name, "omci-runfunc");
	if ((ret = fwk_thread_create(&t, func, data)) < 0) {
		dbprintf_fwk(LOG_ERR, "thread create error(%d)\n", ret);
		return ret;
	}
	if (is_sync) {
		if ((ret = fwk_thread_join(&t)) < 0) {
			dbprintf_fwk(LOG_DEBUG, "join error(%d)\n", ret);
			return ret;
		}
	}
	return ret;
}

