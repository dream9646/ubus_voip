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
 * File    : fwk_thread.h
 *
 ******************************************************************/

#ifndef __FWK_THREAD_H__
#define __FWK_THREAD_H__

#include <pthread.h>
#include <sched.h>

#define FWK_THREAD_NAME_LEN 32

/*difine schedule policy*/
enum fwk_thread_sp_t {
	FWK_THREAD_SP_NORMAL,	/*normal task */
	FWK_THREAD_SP_FIFO,	/*realtime task, schedule as first-in-first-out */
	FWK_THREAD_SP_RR	/*realtime task, schedule as round-robin */
};

// policy
#define FWK_THREAD_POLICY_OMCI_TIMER		FWK_THREAD_SP_RR
#define FWK_THREAD_POLICY_OMCI_CORE		FWK_THREAD_SP_RR
#define FWK_THREAD_POLICY_CPUPORT_HANDLER	FWK_THREAD_SP_RR
#define FWK_THREAD_POLICY_OMCI_NOTIFY		FWK_THREAD_SP_NORMAL
#define FWK_THREAD_POLICY_OMCI_MISC		FWK_THREAD_SP_NORMAL

// priority
// for pthread FIFO/RR: realtime, prio is 1(low)..99(high)
// for pthread NORMAL: time sharing, prio should be 0
#define FWK_THREAD_PRIO_OMCI_TIMER		56
#define FWK_THREAD_PRIO_OMCI_CORE		55
#define FWK_THREAD_PRIO_CPUPORT_HI		54
#define FWK_THREAD_PRIO_CPUPORT_RECV		53
#define FWK_THREAD_PRIO_CPUPORT_MED		52
#define FWK_THREAD_PRIO_CPUPORT_LOW		51
#define FWK_THREAD_PRIO_OMCI_NOTIFY		0
#define FWK_THREAD_PRIO_OMCI_MISC		0

// whether to wait child thread complete
#define FWK_THREAD_RUN_FUNC_ASYNC		0
#define FWK_THREAD_RUN_FUNC_SYNC		1

struct fwk_thread_t {
	enum fwk_thread_sp_t policy;
	int priority;		/* priority of thread */
	unsigned int stack_size;	/* stack size of thread */
	char join;		/*0: not join, 1: default join */
	char name[FWK_THREAD_NAME_LEN];	/* thread name */
	pthread_t thread_id;
};

/* fwk_thread.c */
int fwk_thread_create(struct fwk_thread_t *thread_ptr, void *(*thread_entry)(void *), void *arg);
void fwk_thread_exit(void);
int fwk_thread_join(struct fwk_thread_t *thread_ptr);
int fwk_thread_cancel(struct fwk_thread_t *thread_ptr);
int fwk_thread_kill(struct fwk_thread_t *thread_ptr, int signal);
int fwk_thread_yield(void);
int fwk_thread_gettid(void);
int fwk_thread_set_cancellable(void);
int fwk_thread_get_priority(int pid, enum fwk_thread_sp_t *policy, int *priority);
int fwk_thread_set_priority(int pid, enum fwk_thread_sp_t policy, int priority);
int fwk_thread_run_function(void *(*func)(void *), void *data, int is_sync, enum fwk_thread_sp_t policy, unsigned priority);

#endif
