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
 * File    : fwk_timer.c
 *
 ******************************************************************/

#include "fwk_thread.h"
#include "fwk_mutex.h"
#include "fwk_cond.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>

#define FWK_TIMER_WAIT_FOREVER_TIME 60*60*1000	//ms, for condition wait timeout while no active timer

/*global variables*/
int timer_queue_id_g = FWK_MSGQ_ALLOC_ID_UNASSIGN;
int timer_set_id_g = FWK_MSGQ_ALLOC_ID_UNASSIGN;
unsigned short timer_seq_g = 0;
struct fwk_mutex_t timer_seq_mutex_g;
struct fwk_mutex_t timer_mutex_g;
struct fwk_cond_t timer_cond_g;
struct fwk_thread_t timer_thread_g;

static void
timer_update_table(struct fwk_timer_table_t *table, unsigned int elapsed_time)
{
	int ret;
	struct fwk_timer_msg_t *timer_msg, *temp_timer_msg;

	dbprintf_fwk(LOG_DEBUG, "timer_update_table: elapsed_time(%d)\n", elapsed_time);

	if (table == NULL) {
		return;
	}

	/*traverse all node in the timer table */
	list_for_each_entry_safe(timer_msg, temp_timer_msg, &table->timer_list, node) {
		if (timer_msg->lefttimeout <= elapsed_time) {
			/*timeout, send back this one to original task queue, and delete from list */
			dbprintf_fwk(LOG_DEBUG,
				 "timer_update_table: timeout, id(%d), q_id(%d:%s), event(%d), timeout(%d)\n",
				 timer_msg->timer_id, timer_msg->q_id, fwk_msgq_name(timer_msg->q_id), 
				 timer_msg->event, timer_msg->timeout);

			timer_msg->lefttimeout = 0;
			timer_msg->op = FWK_TIMER_OP_TIMEOUT;
			list_del(&timer_msg->node);

			table->timer_count--;

			/*send */
			if ((ret = fwk_msgq_send(timer_msg->q_id, &timer_msg->node)) < 0) {
				dbprintf_fwk(LOG_ERR,
					 "timer_update_table: timeout send error(%d), id(%d), q_id(%d:%s), event(%d), timeout(%d)\n",
					 ret, timer_msg->timer_id, timer_msg->q_id, fwk_msgq_name(timer_msg->q_id), 
					 timer_msg->event, timer_msg->timeout);
				free_safe(timer_msg);
			}
		} else {
			/*update lefttimeout value */
			dbprintf_fwk(LOG_DEBUG,
				 "timer_update_table: update lefttimeout, id(%d), timeout(%d), lefttimeout(%d)\n",
				 timer_msg->timer_id, timer_msg->timeout, timer_msg->lefttimeout);

			timer_msg->lefttimeout -= elapsed_time;
		}
	}

	/*update finish */
	return;
}

static void
timer_process_msg(struct fwk_timer_table_t *table, struct fwk_timer_msg_t *timer_msg)
{
	int op_flag;
	int i;
	int ret;

	struct fwk_timer_msg_t *timer_msg_pos, *timer_msg_n;

	if (timer_msg == NULL) {
		return;
	}

	switch (timer_msg->op) {
	case FWK_TIMER_OP_ADD:
		dbprintf_fwk(LOG_DEBUG, "timer_process_msg: add, timer_id(%d)\n", timer_msg->timer_id);

		timer_msg->lefttimeout = timer_msg->timeout;

		/*sort and insert to list by timeout value */
		op_flag = 0;

		list_for_each_entry_safe(timer_msg_pos, timer_msg_n, &table->timer_list, node) {
			if (timer_msg_pos->lefttimeout > timer_msg->timeout) {
				list_add_tail(&timer_msg->node, &timer_msg_pos->node);
				op_flag = 1;
				break;
			}
		}

		/*no one lefttimeout greater than new timer's timeout value, add to tail */
		if (!op_flag) {
			list_add_tail(&timer_msg->node, &table->timer_list);
		}

		table->timer_count++;

		break;
	case FWK_TIMER_OP_DELETE:
		dbprintf_fwk(LOG_DEBUG, "timer_process_msg: delete, timer_id(%d)\n", timer_msg->timer_id);

		op_flag = 0;

		list_for_each_entry_safe(timer_msg_pos, timer_msg_n, &table->timer_list, node) {
			if (timer_msg_pos->timer_id == timer_msg->timer_id) {
				list_del(&timer_msg_pos->node);
				table->timer_count--;

				/*free the timer msg in timer table */
				free_safe(timer_msg_pos);

				op_flag = 1;

				break;
			}
		}

		if (!op_flag) {
			dbprintf_fwk(LOG_WARNING, "timer_process_msg: no one deleted, id(%d)\n", timer_msg->timer_id);
		}

		/*free the op timer msg */
		free_safe(timer_msg);

		break;
	case FWK_TIMER_OP_MODIFY:
		/*delete the old one */
		dbprintf_fwk(LOG_DEBUG, "timer_process_msg: modify, timer_id(%d)\n", timer_msg->timer_id);

		op_flag = 0;

		list_for_each_entry_safe(timer_msg_pos, timer_msg_n, &table->timer_list, node) {
			if (timer_msg_pos->timer_id == timer_msg->timer_id) {
				list_del(&timer_msg_pos->node);
				table->timer_count--;

				/*free the timer msg in timer table */
				free_safe(timer_msg_pos);

				op_flag = 1;

				break;
			}
		}

		if (!op_flag) {
			dbprintf_fwk(LOG_WARNING,
				 "timer_process_msg: no one deleted for modification, id(%d)\n",
				 timer_msg->timer_id);
		}

		/*add the new noe, sort and insert to list by timeout value */
		op_flag = 0;

		list_for_each_entry_safe(timer_msg_pos, timer_msg_n, &table->timer_list, node) {
			if (timer_msg_pos->lefttimeout > timer_msg->timeout) {
				list_add_tail(&timer_msg->node, &timer_msg_pos->node);
				op_flag = 1;
				break;
			}
		}

		/*no one lefttimeout greater than new timer's timeout value, add to tail */
		if (!op_flag) {
			list_add_tail(&timer_msg->node, &table->timer_list);
		}

		table->timer_count++;

		break;
	case FWK_TIMER_OP_DUMP:
		/*show all entries in table list */
		dbprintf_fwk(LOG_DEBUG, "timer_process_msg: dump all entries in table, num(%d)\n", table->timer_count);

		i = 0;

		list_for_each_entry_safe(timer_msg_pos, timer_msg_n, &table->timer_list, node) {
			dbprintf_fwk(LOG_ERR,
				 "timer_process_msg: dump(%d), id(%d), event(%d), q_id(%d), timeout(%d), lefttimeout(%d) \n",
				 i++, timer_msg_pos->timer_id, timer_msg_pos->event,
				 timer_msg_pos->q_id, timer_msg_pos->timeout, timer_msg_pos->lefttimeout);
		}

		break;
	case FWK_TIMER_OP_TERMINATE:
		table->loop = 0;

		/*free entry in table list */
		if (!list_empty(&table->timer_list)) {
			dbprintf_fwk(LOG_ERR,
				 "timer_process_msg: terminate with entries in table, num(%d)\n",
				 table->timer_count);

			list_for_each_entry_safe(timer_msg_pos, timer_msg_n, &table->timer_list, node) {
				timer_msg_pos->op = FWK_TIMER_OP_FAIL;

				list_del(&timer_msg_pos->node);
				table->timer_count--;

				/*free the timer msg in timer table */
				//free(timer_msg_pos);
				/*send */
				if ((ret = fwk_msgq_send(timer_msg_pos->q_id, &timer_msg_pos->node)) < 0) {
					dbprintf_fwk(LOG_ERR,
						 "timer_update_table: terminate send error(%d), id(%d), q_id(%d), event(%d), timeout(%d)\n",
						 ret, timer_msg_pos->timer_id, timer_msg_pos->q_id,
						 timer_msg_pos->event, timer_msg_pos->timeout);

					free_safe(timer_msg_pos);
				}
			}
		}

		dbprintf_fwk(LOG_DEBUG, "timer_process_msg: terminated\n");

		break;
	default:
		/*wrong op, free the timer msg */
		free_safe(timer_msg);
	}
}

static void *
timer_thread(void *ptr)
{
	int ret, sel_ret;
	unsigned int at_least_wait_timer;
	unsigned int elapsed_time;
	struct fwk_timer_table_t timer_table;
	struct fwk_timer_msg_t *timer_msg;
	struct timeval past_time={0}, curr_time={0};
	struct list_head *list_node;

	util_set_thread_name("omci-timer");
	omci_env_g.taskname[ENV_TASK_NO_TIMER] = "TIMER";
	omci_env_g.taskid[ENV_TASK_NO_TIMER] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_TIMER] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_TIMER] = 0;
	omci_env_g.taskts[ENV_TASK_NO_TIMER] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 0; //idle
	
	/*init timer table */
	timer_table.timer_count = 0;
	timer_table.loop = 1;
	INIT_LIST_HEAD(&timer_table.timer_list);

	/*timer loop */
	while (timer_table.loop) {
		omci_env_g.taskloopcount[ENV_TASK_NO_TIMER]++;
		omci_env_g.taskts[ENV_TASK_NO_TIMER] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 0; //idle

		/*lock condition's mutex */
		if ((ret = fwk_mutex_lock(&timer_mutex_g)) < 0) {
			dbprintf_fwk(LOG_ERR, "timer_thread: lock error(%d)\n", ret);
			
			fwk_thread_exit();
		}

		/*select with timeout 0, never wait */
		if ((sel_ret = fwk_msgq_select(timer_set_id_g, 0)) == FWK_MSGQ_ERROR_EMPTY) {
			omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 1; //running
			/*timer table is empty, wait on condition */
			if (timer_table.timer_count == 0)	//list_empty(&timer_table.timer_list)
			{
				at_least_wait_timer = FWK_TIMER_WAIT_FOREVER_TIME;
			} else {
				/*get the lefttimeout value from the first entry of timer table */
				timer_msg = list_entry(timer_table.timer_list.next, struct fwk_timer_msg_t, node);
				at_least_wait_timer = timer_msg->lefttimeout;
			}

			/*get past time */
			if ((ret = util_get_uptime(&past_time)) < 0) {
				dbprintf_fwk(LOG_ERR, "timer_thread: get uptime error(%d)\n", ret);
				omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 0; //idle
				fwk_thread_exit();
			}

			if ((ret = fwk_cond_timedwait(&timer_cond_g, at_least_wait_timer)) < 0) {
				/*error, except timeout, else continue */
				if (ret != FWK_COND_ERROR_TIMEOUT) {
					/*unlock condition's mutex and continue */
					if ((ret = fwk_mutex_unlock(&timer_mutex_g)) < 0) {
						dbprintf_fwk(LOG_ERR, "timer_thread: unlock error(%d)\n", ret);
						omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 0; //idle
						fwk_thread_exit();
					}

					continue;
				}
			}

			/*get current time */
			if ((ret = util_get_uptime(&curr_time)) < 0) {
				dbprintf_fwk(LOG_ERR, "timer_thread: get uptime error(%d)\n", ret);
				omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 0; //idle
				fwk_thread_exit();
			}

			/*unlock condition's mutex and continue */
			if ((ret = fwk_mutex_unlock(&timer_mutex_g)) < 0) {
				dbprintf_fwk(LOG_ERR, "timer_thread: unlock error(%d)\n", ret);
				omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 0; //idle
				fwk_thread_exit();
			}

			/*alculate time elapsed */
			elapsed_time = (curr_time.tv_sec - past_time.tv_sec) * 1000 +
			    (curr_time.tv_usec - past_time.tv_usec) / 1000;

			/*anyway, check timer table and update elapsed time */
			timer_update_table(&timer_table, elapsed_time);
		} else {
			omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 1; //running
			
			/*unlock condition's mutex */
			if ((ret = fwk_mutex_unlock(&timer_mutex_g)) < 0) {
				dbprintf_fwk(LOG_ERR, "timer_thread: unlock error(%d)\n", ret);
				omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 0; //idle
				fwk_thread_exit();
			}

			if (sel_ret > 0) {
				/*get msg from timer queue */
				if ((ret = fwk_msgq_recv(timer_queue_id_g, &list_node)) < 0) {
					dbprintf_fwk(LOG_ERR, "timer_thread: recv error(%d)\n", ret);

					continue;
				}

				timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);

				/*process the msg */
				timer_process_msg(&timer_table, timer_msg);
			} else {
				/*do nothing */
			}
		}
	}

	omci_env_g.taskstate[ENV_TASK_NO_TIMER] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_TIMER] = 0;

	return 0;
}

static int
fwk_timer_send(struct fwk_timer_msg_t *timer_msg)
{
	/*send timer msg to timer queue */
	if (fwk_msgq_send(timer_queue_id_g, &timer_msg->node) < 0) {
		return (-1);
	}

	/*lock timer condition's mutex */
	if (fwk_mutex_lock(&timer_mutex_g) < 0) {
		return (-1);
	}

	/*signal to timer condition */
	if (fwk_cond_signal(&timer_cond_g) < 0) {
		return (-1);
	}

	/*unlock timer condition's mutex */
	if (fwk_mutex_unlock(&timer_mutex_g) < 0) {
		return (-1);
	}

	return (0);
}

int
fwk_timer_create(unsigned int q_id, unsigned int event, unsigned int timeout, void *user_data)	//ms
{
	unsigned short timer_id = 0;
	struct fwk_timer_msg_t *timer_msg;

	timer_msg = malloc_safe(sizeof (struct fwk_timer_msg_t));
	INIT_LIST_NODE(&timer_msg->node);

	/*lock timer id sequence mutex */
	if (fwk_mutex_lock(&timer_seq_mutex_g) < 0) {
		free_safe(timer_msg);
		return (-1);
	}

	timer_seq_g++;
	if (timer_seq_g == 0)
	{
		timer_seq_g = 1;
		timer_id = 1;
	} else {
		timer_id = timer_seq_g;
	}

	/*unlock timer id sequence mutex */
	if (fwk_mutex_unlock(&timer_seq_mutex_g) < 0) {
		free_safe(timer_msg);
		return (-2);
	}

	timer_msg->timer_id = timer_id;
	timer_msg->op = FWK_TIMER_OP_ADD;
	timer_msg->q_id = q_id;
	timer_msg->event = event;
	timer_msg->timeout = timeout;
	timer_msg->user_data = user_data;

	if (fwk_timer_send(timer_msg) < 0) {
		free_safe(timer_msg);
		return (-3);
	}

	return (timer_id);
}

int
fwk_timer_delete(unsigned short timer_id)
{
	struct fwk_timer_msg_t *timer_msg;

	timer_msg = malloc_safe(sizeof (struct fwk_timer_msg_t));
	INIT_LIST_NODE(&timer_msg->node);

	timer_msg->timer_id = timer_id;
	timer_msg->op = FWK_TIMER_OP_DELETE;

	if (fwk_timer_send(timer_msg) < 0) {
		free_safe(timer_msg);
		return (-1);
	}

	return (0);
}

int
fwk_timer_modify(unsigned short timer_id, unsigned int q_id, unsigned int event, int timeout, void *user_data)
{
	struct fwk_timer_msg_t *timer_msg;

	timer_msg = malloc_safe(sizeof (struct fwk_timer_msg_t));
	INIT_LIST_NODE(&timer_msg->node);

	timer_msg->timer_id = timer_id;
	timer_msg->op = FWK_TIMER_OP_MODIFY;
	timer_msg->q_id = q_id;
	timer_msg->event = event;
	timer_msg->timeout = timeout;
	timer_msg->user_data = user_data;

	if (fwk_timer_send(timer_msg) < 0) {
		free_safe(timer_msg);
		return (-1);
	}

	return (0);
}

int
fwk_timer_dump(void)
{
	struct fwk_timer_msg_t *timer_msg;

	timer_msg = malloc_safe(sizeof (struct fwk_timer_msg_t));
	INIT_LIST_NODE(&timer_msg->node);

	timer_msg->op = FWK_TIMER_OP_DUMP;

	if (fwk_timer_send(timer_msg) < 0) {
		free_safe(timer_msg);
		return (-1);
	}

	return (0);
}

int
fwk_timer_terminate(void)
{
	struct fwk_timer_msg_t *timer_msg;

	timer_msg = malloc_safe(sizeof (struct fwk_timer_msg_t));
	INIT_LIST_NODE(&timer_msg->node);

	timer_msg->op = FWK_TIMER_OP_TERMINATE;

	if (fwk_timer_send(timer_msg) < 0) {
		free_safe(timer_msg);
		return (-1);
	}

	return (0);
}

int
fwk_timer_init(void)
{
	int ret;

	dbprintf_fwk(LOG_DEBUG, "fwk_timer_init\n");

	timer_seq_g = 0;

	/*create mutex and condition for sequence generator and task progress controlling */
	if ((ret = fwk_mutex_create(&timer_seq_mutex_g)) < 0) {
		dbprintf_fwk(LOG_ERR, "timer seq mutex create error(%d)\n", ret);

		return (-1);
	}
	if ((ret = fwk_mutex_create(&timer_mutex_g)) < 0) {
		dbprintf_fwk(LOG_ERR, "timer mutex create error(%d)\n", ret);

		fwk_mutex_destroy(&timer_seq_mutex_g);

		return (-1);
	}
	if ((ret = fwk_cond_create(&timer_cond_g, &timer_mutex_g)) < 0) {
		dbprintf_fwk(LOG_ERR, "timer condition create error(%d)\n", ret);

		fwk_mutex_destroy(&timer_seq_mutex_g);
		fwk_mutex_destroy(&timer_mutex_g);

		return (-1);
	}

	/*allocate message queue and set */
	if ((ret = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_TIMER")) < 0) {
		dbprintf_fwk(LOG_ERR, "queue allocate error(%d)\n", ret);

		fwk_mutex_destroy(&timer_seq_mutex_g);
		fwk_mutex_destroy(&timer_mutex_g);
		fwk_cond_destroy(&timer_cond_g);

		return (-1);
	} else {
		timer_queue_id_g = ret;
	}
	if ((ret = fwk_msgq_set_alloc()) < 0) {
		dbprintf_fwk(LOG_ERR, "set allocate error(%d)\n", ret);

		fwk_mutex_destroy(&timer_seq_mutex_g);
		fwk_mutex_destroy(&timer_mutex_g);
		fwk_cond_destroy(&timer_cond_g);
		fwk_msgq_release(timer_queue_id_g);

		return (-1);
	} else {
		timer_set_id_g = ret;
	}

	/*associate the queue with the msgq select set */
	if ((ret = fwk_msgq_set_set(timer_set_id_g, timer_queue_id_g)) < 0) {
		dbprintf_fwk(LOG_ERR, "set set error(%d)\n", ret);

		fwk_mutex_destroy(&timer_seq_mutex_g);
		fwk_mutex_destroy(&timer_mutex_g);
		fwk_cond_destroy(&timer_cond_g);
		fwk_msgq_release(timer_queue_id_g);
		fwk_msgq_set_release(timer_set_id_g);

		return (-1);
	}

	/*init success */
	return (0);
}

int
fwk_timer_shutdown(void)
{
	dbprintf_fwk(LOG_DEBUG, "fwk_timer_shutdown\n");

	fwk_mutex_destroy(&timer_seq_mutex_g);
	fwk_mutex_destroy(&timer_mutex_g);
	fwk_cond_destroy(&timer_cond_g);
	fwk_msgq_set_zero(timer_set_id_g);
	fwk_msgq_release(timer_queue_id_g);
	fwk_msgq_set_release(timer_set_id_g);

	return (0);
}

int
fwk_timer_start(void)
{
	int ret;

	dbprintf_fwk(LOG_DEBUG, "fwk_timer_start\n");

	/*create timer task */
	timer_thread_g.policy = FWK_THREAD_POLICY_OMCI_TIMER;
	timer_thread_g.priority = FWK_THREAD_PRIO_OMCI_TIMER;
	timer_thread_g.stack_size = 128*1024;
	timer_thread_g.join = 1;
	sprintf(timer_thread_g.name, "omci-TIMER");

	if ((ret = fwk_thread_create(&timer_thread_g, timer_thread, NULL)) < 0) {
		dbprintf_fwk(LOG_DEBUG, "error(%d)\n", ret);

		return (-1);
	}

	return (0);
}

int
fwk_timer_stop(void)
{
	int ret;

	/*send a terminate command to timer task loop */
	if ((ret = fwk_timer_terminate()) < 0) {
		dbprintf_fwk(LOG_DEBUG, "terminate error(%d)\n", ret);

		return (-1);
	}

	/*wait timer task terminate, join timer task */
	if ((ret = fwk_thread_join(&timer_thread_g)) < 0) {
		dbprintf_fwk(LOG_DEBUG, "join error(%d)\n", ret);

		return (-1);
	}

	return (0);
}

