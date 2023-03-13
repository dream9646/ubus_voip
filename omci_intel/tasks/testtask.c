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
 * Module  : tasks
 * File    : testtask.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "omcimsg.h"
#include "testtask.h"
#include "tranxmod.h"
#include "meinfo.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"

int testtask_msg_qid_g = -1;
static int testtask_qsetid_g = -1;
static int testtask_loop_g = 0;
static struct fwk_thread_t testtask_thread_g;

static int
testtask_me_test(unsigned short classid, unsigned short meid, struct omcimsg_layout_t *req_msg) 
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	struct me_t *me=meinfo_me_get_by_meid(classid, meid);
	unsigned char result[TEST_RESULT_SIZE];
	int (*test_func)(struct me_t *me, unsigned char *req, unsigned char *result);


	if (miptr == NULL) {
		dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), miptr null?\n", classid, meid, meid);
		return TESTTASK_ERROR_UNKNOWN_ME;
	}
	if ((me = meinfo_me_get_by_meid(classid, meid)) == NULL) {
		dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), me ptr null?\n", classid, meid, meid);
		return TESTTASK_ERROR_UNKNOWN_ME_INSTANCE;
	}

	// select test_func ptr, hw is preferred over cb 
	test_func=miptr->hardware.test_hw;
	if (!test_func)
		test_func=miptr->callback.test_cb;		
	if (!test_func) {
		dbprintf(LOG_ERR, "classid=%u, meid=0x%x(%u), me test func ptr null?\n", classid, meid, meid);
		return TESTTASK_ERROR_CMD_NOT_SUPPORTED;
	}
	// the above error condition should not happen as they are checked before passing to test thread

	// issue test by me hardware callback
	memset(result, 0, TEST_RESULT_SIZE);
	test_func(me, req_msg->msg_contents, result);

	// generate and send result msg
	omcimsg_test_result_generator(classid, meid, omcimsg_header_get_transaction_id(req_msg), result);
	dbprintf(LOG_INFO, "test result msg sent for classid=%u, meid=%u\n", me->classid, me->meid);
	return TESTTASK_RW_OK;
}

static void *
testtask_func(void *ptr)
{
	int ret;
	
	struct list_head *entry;
	struct testtask_msg_t *testtask_msg = NULL;

	util_set_thread_name("omci-test");
	omci_env_g.taskname[ENV_TASK_NO_TEST] = "TEST";
	omci_env_g.taskid[ENV_TASK_NO_TEST] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_TEST] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_TEST] = 0;
	omci_env_g.taskts[ENV_TASK_NO_TEST] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_TEST] = 0; //idle


	while (testtask_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_TEST]++;
		omci_env_g.taskts[ENV_TASK_NO_TEST] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_TEST] = 0; //idle
		
		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_TEST)) {
			msleep(100);
			continue;
		}

		if ((ret = fwk_msgq_select(testtask_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) < 0) {
			dbprintf(LOG_ERR, "select error(%d)\n", ret);
			continue;
		}

		omci_env_g.taskstate[ENV_TASK_NO_TEST] = 1; //running
		
		if (fwk_msgq_set_isset(testtask_qsetid_g, testtask_msg_qid_g)) {
			if ((ret = fwk_msgq_recv(testtask_msg_qid_g, &entry)) < 0) {
				dbprintf(LOG_ERR, "recv error(%d)\n", ret);
				continue;
			}

			/* process data */
			testtask_msg = list_entry(entry, struct testtask_msg_t, q_entry);
			
			switch (testtask_msg->cmd) {
			case TESTTASK_CMD_TEST:
				dbprintf(LOG_DEBUG, "test classid=%u, meid=%u\n",
					testtask_msg->classid, testtask_msg->meid);
				testtask_me_test(testtask_msg->classid, testtask_msg->meid, testtask_msg->req_msg);
				break;
			case TESTTASK_CMD_TERMINATE:
				dbprintf(LOG_ERR, "task terminate\n");
				break;
			default:
				dbprintf(LOG_ERR, "get wrong data\n");
			}
			
			if (testtask_msg->req_msg)
				free_safe(testtask_msg->req_msg);
			free_safe(testtask_msg);
			testtask_msg = NULL;
		}
	} 

	omci_env_g.taskstate[ENV_TASK_NO_TEST] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_TEST] = 0;

	return 0;
}

int
testtask_init(void)
{
	int ret;
	
	/*allocate queue and set */
	if ((testtask_msg_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 20, "Q_TEST_CMD")) < 0) {
		dbprintf(LOG_ERR, "queue alloc error(%d)\n", testtask_msg_qid_g);
		return -1;
	}
	if ((testtask_qsetid_g = fwk_msgq_set_alloc()) < 0) {
		fwk_msgq_release(testtask_msg_qid_g);
		dbprintf(LOG_ERR, "queue set alloc error(%d)\n", testtask_qsetid_g);
		return -1;
	}

	/* set msgq set */
	if ((ret = fwk_msgq_set_zero(testtask_qsetid_g)) < 0) {
		fwk_msgq_set_release(testtask_qsetid_g);
		fwk_msgq_release(testtask_msg_qid_g);
		dbprintf(LOG_ERR, "queue set zero error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set(testtask_qsetid_g, testtask_msg_qid_g)) < 0) {
		fwk_msgq_set_release(testtask_qsetid_g);
		fwk_msgq_release(testtask_msg_qid_g);
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}

	return 0;
}

int
testtask_start(void)
{
	int ret;

	/*create pm task */
	testtask_thread_g.policy = FWK_THREAD_POLICY_OMCI_NOTIFY;
	testtask_thread_g.priority = FWK_THREAD_PRIO_OMCI_NOTIFY;
	testtask_thread_g.stack_size = 128*1024;
	testtask_thread_g.join = 1;
	sprintf(testtask_thread_g.name, "omci-TEST");
	testtask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&testtask_thread_g, testtask_func, NULL)) < 0) {
		dbprintf(LOG_ERR, "thread create error(%d)\n", ret);
		return -1;
	}
	return 0;
}

int
testtask_shutdown(void)
{
	fwk_msgq_set_release(testtask_qsetid_g);
	fwk_msgq_release(testtask_msg_qid_g);

	testtask_msg_qid_g = -1;
	testtask_qsetid_g = -1;
	
	return 0;
}

int
testtask_stop(void)
{
	int ret;
	struct testtask_msg_t *testtask_msg;
	testtask_loop_g = 0;	//stop loop

	testtask_msg = malloc_safe(sizeof (struct testtask_msg_t));

	INIT_LIST_NODE(&testtask_msg->q_entry);
	testtask_msg->cmd = TESTTASK_CMD_TERMINATE;
	if (fwk_msgq_send(testtask_msg_qid_g, &testtask_msg->q_entry) < 0)
	{
		free_safe(testtask_msg);
	}
	
	if ((ret = fwk_thread_join(&testtask_thread_g)) < 0) {
		dbprintf(LOG_ERR, "join error(%d)\n", ret);
	}

	return 0;
}
