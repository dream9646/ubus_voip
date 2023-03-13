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
 * File    : fwk_msgq.c
 *
 ******************************************************************/

#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "env.h"
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

struct fwk_msgq_pool_t {
	struct fwk_mutex_t mutex;
	struct fwk_msgq_t queue[FWK_MSGQ_MAX_QUEUE];
};

struct fwk_msgq_pool_t q_pool_g;

struct fwk_msgq_set_pool_t {
	struct fwk_mutex_t mutex;
	struct fwk_msgq_set_t set[FWK_MSGQ_MAX_QUEUE];
};

struct fwk_msgq_set_pool_t msgq_set_pool_g;

int
fwk_msgq_module_init(void)
{
	int i;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*init queue pool */
	memset(&q_pool_g, 0x00, sizeof (q_pool_g));

	fwk_mutex_create(&q_pool_g.mutex);

	for (i = 0; i < FWK_MSGQ_MAX_QUEUE; i++) {
		INIT_LIST_HEAD(&q_pool_g.queue[i].cond_list);
		INIT_LIST_HEAD(&q_pool_g.queue[i].q);
	}

	/*init msgq set pool */
	memset(&msgq_set_pool_g, 0x00, sizeof (msgq_set_pool_g));

	fwk_mutex_create(&msgq_set_pool_g.mutex);

	return 0;
}

int
fwk_msgq_module_shutdown(void)
{
	int i;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	/*clear queue pool */
	memset(&q_pool_g, 0x00, sizeof (q_pool_g));

	fwk_mutex_destroy(&q_pool_g.mutex);

	for (i = 0; i < FWK_MSGQ_MAX_QUEUE; i++) {
		INIT_LIST_HEAD(&q_pool_g.queue[i].cond_list);
		INIT_LIST_HEAD(&q_pool_g.queue[i].q);
	}

	/*clear msgq set pool */
	memset(&msgq_set_pool_g, 0x00, sizeof (msgq_set_pool_g));

	fwk_mutex_destroy(&msgq_set_pool_g.mutex);

	return 0;
}

/*return a queue id*/
int
fwk_msgq_alloc(int q_id, unsigned int q_size, char *q_name)
{
	int i;
	int ret_id = FWK_MSGQ_ERROR_FULL;

	dbprintf_fwk(LOG_DEBUG, "pid(%d), q_id(%d:), q_size(%d), name=%s\n", fwk_thread_gettid(), q_id, q_size, q_name);

	if (q_id == FWK_MSGQ_ALLOC_ID_UNASSIGN) {
		/*id assigned automatically */
		fwk_mutex_lock(&q_pool_g.mutex);

		for (i = 0; i < FWK_MSGQ_MAX_QUEUE; i++) {
			if (q_pool_g.queue[i].is_used == 0) {
				ret_id = i;
				q_pool_g.queue[i].is_used = 1;
				q_pool_g.queue[i].q_size = q_size;
				q_pool_g.queue[i].q_len = q_pool_g.queue[i].q_len_max = 0;
				q_pool_g.queue[i].q_add = q_pool_g.queue[i].q_del = q_pool_g.queue[i].q_drop = 0;
				fwk_mutex_create(&q_pool_g.queue[i].mutex);
				INIT_LIST_HEAD(&q_pool_g.queue[i].cond_list);
				INIT_LIST_HEAD(&q_pool_g.queue[i].q);
				strncpy(q_pool_g.queue[i].q_name, q_name, FWK_MSGQ_NANE_SIZE);
				q_pool_g.queue[i].q_name[FWK_MSGQ_NANE_SIZE-1]=0;

				break;
			}
		}

		fwk_mutex_unlock(&q_pool_g.mutex);

		if (i == FWK_MSGQ_MAX_QUEUE) {
			dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_FULL);

			ret_id = FWK_MSGQ_ERROR_FULL;
		}
	} else {
		/*attempt to get a queue id */
		if (q_id < 0 || q_id >= FWK_MSGQ_MAX_QUEUE) {
			dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

			ret_id = FWK_MSGQ_ERROR_OUTRANGE;
		} else {
			fwk_mutex_lock(&q_pool_g.mutex);

			if (q_pool_g.queue[q_id].is_used == 0) {
				ret_id = q_id;
				q_pool_g.queue[q_id].is_used = 1;
				q_pool_g.queue[q_id].q_size = q_size;
				q_pool_g.queue[q_id].q_len = q_pool_g.queue[q_id].q_len_max = 0;
				q_pool_g.queue[q_id].q_add = q_pool_g.queue[q_id].q_del = q_pool_g.queue[q_id].q_drop = 0;
				fwk_mutex_create(&q_pool_g.queue[q_id].mutex);
				INIT_LIST_HEAD(&q_pool_g.queue[q_id].cond_list);
				INIT_LIST_HEAD(&q_pool_g.queue[q_id].q);
				strncpy(q_pool_g.queue[q_id].q_name, q_name, FWK_MSGQ_NANE_SIZE);
				q_pool_g.queue[q_id].q_name[FWK_MSGQ_NANE_SIZE-1]=0;
			} else {
				dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n",
					 fwk_thread_gettid(), FWK_MSGQ_ERROR_OCCUPIED);

				ret_id = FWK_MSGQ_ERROR_OCCUPIED;
			}

			fwk_mutex_unlock(&q_pool_g.mutex);
		}
	}

	dbprintf_fwk(LOG_DEBUG, "pid(%d), q_id(%d)\n", fwk_thread_gettid(), ret_id);

	return ret_id;
}

int
fwk_msgq_release(unsigned int q_id)
{
	dbprintf_fwk(LOG_DEBUG, "pid(%d), q_id(%d)\n", fwk_thread_gettid(), q_id);

	/*release queue */
	if (q_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), q_id(%d), error(%d)\n",
			 fwk_thread_gettid(), q_id, FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	fwk_mutex_lock(&q_pool_g.mutex);

	fwk_mutex_lock(&q_pool_g.queue[q_id].mutex);
	
	/*check q */
	if (q_pool_g.queue[q_id].q_len > 0 || !list_empty(&q_pool_g.queue[q_id].q)) {
		/*could not free, show warning message */
		dbprintf_fwk(LOG_WARNING, "pid(%d), q_id(%d), not empty(%d)\n",
			 fwk_thread_gettid(), q_id, q_pool_g.queue[q_id].q_len);
	}

	/*free queue list*/
	

	/*check cond_list */
	if (!list_empty(&q_pool_g.queue[q_id].cond_list)) {
		struct fwk_msgq_cond_node_t *cond_node_ptr, *temp_ptr;

		/*show warning message, free each nodes */
		dbprintf_fwk(LOG_WARNING, "pid(%d), q_id(%d), cond_list not empty\n", fwk_thread_gettid(), q_id);

		/*delete a node from queue cond list */
		list_for_each_entry_safe(cond_node_ptr, temp_ptr, &q_pool_g.queue[q_id].cond_list, cond_list) {
			list_del(&cond_node_ptr->cond_list);
			free_safe(cond_node_ptr);
		}
	}

	q_pool_g.queue[q_id].is_used = 0;
	q_pool_g.queue[q_id].q_len = 0;
	INIT_LIST_HEAD(&q_pool_g.queue[q_id].cond_list);
	INIT_LIST_HEAD(&q_pool_g.queue[q_id].q);
	
	fwk_mutex_unlock(&q_pool_g.queue[q_id].mutex);

	fwk_mutex_destroy(&q_pool_g.queue[q_id].mutex);

	fwk_mutex_unlock(&q_pool_g.mutex);

	return 0;
}

/*return a msgq set id*/
int
fwk_msgq_set_alloc(void)
{
	int i;
	int ret_id = FWK_MSGQ_ERROR_FULL;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	fwk_mutex_lock(&msgq_set_pool_g.mutex);

	for (i = 0; i < FWK_MSGQ_MAX_QUEUE; i++) {
		if (msgq_set_pool_g.set[i].is_used == 0) {
			msgq_set_pool_g.set[i].is_used = 1;
			fwk_mutex_create(&msgq_set_pool_g.set[i].mutex);
			fwk_cond_create(&msgq_set_pool_g.set[i].cond, &msgq_set_pool_g.set[i].mutex);
			memset(msgq_set_pool_g.set[i].msgq_bitmap, 0x00, sizeof (msgq_set_pool_g.set[i].msgq_bitmap));

			ret_id = i;

			break;
		}
	}

	fwk_mutex_unlock(&msgq_set_pool_g.mutex);

	if (i == FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_MAX_QUEUE);

		ret_id = FWK_MSGQ_ERROR_FULL;
	}

	dbprintf_fwk(LOG_DEBUG, "pid(%d), set_id(%d)\n", fwk_thread_gettid(), ret_id);

	return ret_id;
}

int
fwk_msgq_set_set(unsigned int set_id, unsigned int q_id)
{
	dbprintf_fwk(LOG_DEBUG, "pid(%d), set_id(%d), q_id(%d)\n", fwk_thread_gettid(), set_id, q_id);

	if (set_id >= FWK_MSGQ_MAX_QUEUE || q_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	fwk_mutex_lock(&msgq_set_pool_g.mutex);
	fwk_mutex_lock(&q_pool_g.mutex);

	if (msgq_set_pool_g.set[set_id].is_used == 1 && q_pool_g.queue[q_id].is_used == 1) {
		struct fwk_msgq_cond_node_t *cond_node_ptr;
		/*set bitmap of msgq set for the queue id */
		msgq_set_pool_g.set[set_id].msgq_bitmap[q_id / 32] |= ((int) 1 << (q_id % 32));	//(__FDS_BITS (s)[__FDELT(d)] |= __FDMASK(d)

		/*add a node to queue cond list */
		cond_node_ptr = (struct fwk_msgq_cond_node_t *)malloc_safe(sizeof (struct fwk_msgq_cond_node_t));

		cond_node_ptr->cond_num = set_id;
		INIT_LIST_HEAD(&cond_node_ptr->cond_list);

		fwk_mutex_lock(&q_pool_g.queue[q_id].mutex);
		list_add_tail(&cond_node_ptr->cond_list, &q_pool_g.queue[q_id].cond_list);
		fwk_mutex_unlock(&q_pool_g.queue[q_id].mutex);
	} else {
		int taskid = fwk_thread_gettid();

		fwk_mutex_unlock(&q_pool_g.mutex);
		fwk_mutex_unlock(&msgq_set_pool_g.mutex);

		dbprintf_fwk(LOG_ERR, "pid(%d:%s), qid(%d:%s), error(%d)\n", taskid, env_get_taskname_by_id(taskid), q_id, fwk_msgq_name(q_id), FWK_MSGQ_ERROR_UNALLOC);
		return FWK_MSGQ_ERROR_UNALLOC;
	}

	fwk_mutex_unlock(&q_pool_g.mutex);
	fwk_mutex_unlock(&msgq_set_pool_g.mutex);

	return 0;
}

int
fwk_msgq_set_clear(unsigned int set_id, unsigned int q_id)
{
	dbprintf_fwk(LOG_DEBUG, "pid(%d), set_id(%d), q_id(%d)\n", fwk_thread_gettid(), set_id, q_id);

	if (set_id >= FWK_MSGQ_MAX_QUEUE || q_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	fwk_mutex_lock(&msgq_set_pool_g.mutex);
	fwk_mutex_lock(&q_pool_g.mutex);

	if (msgq_set_pool_g.set[set_id].is_used == 1 && q_pool_g.queue[q_id].is_used == 1) {
		struct fwk_msgq_cond_node_t *cond_node_ptr, *temp_ptr;

		/*clear bitmap of msgq set for the queue id */
		msgq_set_pool_g.set[set_id].msgq_bitmap[q_id / 32] &= ~((int) 1 << (q_id % 32));	//(__FDS_BITS (s)[__FDELT(d)] |= __FDMASK(d)

		fwk_mutex_lock(&q_pool_g.queue[q_id].mutex);
		/*delete a node from queue cond list */
		list_for_each_entry_safe(cond_node_ptr, temp_ptr, &q_pool_g.queue[q_id].cond_list, cond_list) {
			if (cond_node_ptr->cond_num == set_id) {
				list_del(&cond_node_ptr->cond_list);
				free_safe(cond_node_ptr);

				break;
			}
		}
		fwk_mutex_unlock(&q_pool_g.queue[q_id].mutex);
	} else {
		int taskid = fwk_thread_gettid();

		fwk_mutex_unlock(&q_pool_g.mutex);
		fwk_mutex_unlock(&msgq_set_pool_g.mutex);

		dbprintf_fwk(LOG_ERR, "pid(%d:%s), qid(%d:%s), error(%d)\n", taskid, env_get_taskname_by_id(taskid), q_id, fwk_msgq_name(q_id), FWK_MSGQ_ERROR_UNALLOC);
		return FWK_MSGQ_ERROR_UNALLOC;
	}

	fwk_mutex_unlock(&q_pool_g.mutex);
	fwk_mutex_unlock(&msgq_set_pool_g.mutex);

	return 0;
}

int
fwk_msgq_set_zero(unsigned int set_id)
{
	int i;

	dbprintf_fwk(LOG_DEBUG, "pid(%d), set_id(%d)\n", fwk_thread_gettid(), set_id);

	if (set_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	fwk_mutex_lock(&msgq_set_pool_g.mutex);
	fwk_mutex_lock(&q_pool_g.mutex);

	if (msgq_set_pool_g.set[set_id].is_used == 1) {
		for (i = 0; i < FWK_MSGQ_MAX_QUEUE; i++) {
			if ((msgq_set_pool_g.set[set_id].msgq_bitmap[i / 32] & ((int) 1 << (i % 32))) != 0) {
				struct fwk_msgq_cond_node_t *cond_node_ptr, *temp_ptr;

				fwk_mutex_lock(&q_pool_g.queue[i].mutex);
				/*delete the node from queue cond list */
				list_for_each_entry_safe(cond_node_ptr, temp_ptr,
							 &q_pool_g.queue[i].cond_list, cond_list) {
					if (cond_node_ptr->cond_num == set_id) {
						list_del(&cond_node_ptr->cond_list);
						free_safe(cond_node_ptr);

						break;
					}
				}
				fwk_mutex_unlock(&q_pool_g.queue[i].mutex);
			}
		}

		memset(msgq_set_pool_g.set[set_id].msgq_bitmap, 0x00, sizeof (msgq_set_pool_g.set[set_id].msgq_bitmap));
	} else {
		int taskid = fwk_thread_gettid();

		fwk_mutex_unlock(&q_pool_g.mutex);
		fwk_mutex_unlock(&msgq_set_pool_g.mutex);

		dbprintf_fwk(LOG_ERR, "pid(%d:%s), set_id(%d), error(%d)\n", taskid, env_get_taskname_by_id(taskid), set_id, FWK_MSGQ_ERROR_UNALLOC);
		return FWK_MSGQ_ERROR_UNALLOC;
	}

	fwk_mutex_unlock(&q_pool_g.mutex);
	fwk_mutex_unlock(&msgq_set_pool_g.mutex);

	return 0;
}

int
fwk_msgq_set_release(unsigned int set_id)
{
	int i;

	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	if (set_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	fwk_mutex_lock(&msgq_set_pool_g.mutex);
	fwk_mutex_lock(&q_pool_g.mutex);

	if (msgq_set_pool_g.set[set_id].is_used == 1) {
		for (i = 0; i < FWK_MSGQ_MAX_QUEUE; i++) {
			if ((msgq_set_pool_g.set[set_id].msgq_bitmap[i / 32] & ((int) 1 << (i % 32))) != 0) {
				struct fwk_msgq_cond_node_t *cond_node_ptr, *temp_ptr;

				fwk_mutex_lock(&q_pool_g.queue[i].mutex);
				/*delete the node from queue cond list */
				list_for_each_entry_safe(cond_node_ptr, temp_ptr,
							 &q_pool_g.queue[i].cond_list, cond_list) {
					if (cond_node_ptr->cond_num == set_id) {
						list_del(&cond_node_ptr->cond_list);
						free_safe(cond_node_ptr);

						break;
					}
				}
				fwk_mutex_unlock(&q_pool_g.queue[i].mutex);
			}
		}

		memset(msgq_set_pool_g.set[set_id].msgq_bitmap, 0x00, sizeof (msgq_set_pool_g.set[set_id].msgq_bitmap));
		fwk_cond_destroy(&msgq_set_pool_g.set[set_id].cond);
		fwk_mutex_destroy(&msgq_set_pool_g.set[set_id].mutex);
		msgq_set_pool_g.set[set_id].is_used = 0;
	} else {
		int taskid = fwk_thread_gettid();

		fwk_mutex_unlock(&q_pool_g.mutex);
		fwk_mutex_unlock(&msgq_set_pool_g.mutex);

		dbprintf_fwk(LOG_ERR, "pid(%d:%s), set_id(%d), error(%d)\n", taskid, env_get_taskname_by_id(taskid), set_id, FWK_MSGQ_ERROR_UNALLOC);
		return FWK_MSGQ_ERROR_UNALLOC;
	}

	fwk_mutex_unlock(&q_pool_g.mutex);
	fwk_mutex_unlock(&msgq_set_pool_g.mutex);

	return 0;
}

int
fwk_msgq_set_isset(unsigned int set_id, unsigned int q_id)
{
	dbprintf_fwk(LOG_DEBUG, "pid(%d)\n", fwk_thread_gettid());

	if (set_id >= FWK_MSGQ_MAX_QUEUE || q_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	if (msgq_set_pool_g.set[set_id].is_used == 1 && q_pool_g.queue[q_id].is_used == 1) {
		/*check bitmap of msgq set for the queue id */
		return ((msgq_set_pool_g.set[set_id].msgq_bitmap_result[q_id / 32] & ((int) 1 << (q_id % 32))) != 0);	//(__FDS_BITS (s)[__FDELT(d)] |= __FDMASK(d)
	} else {
		int taskid = fwk_thread_gettid();
		dbprintf_fwk(LOG_ERR, "pid(%d:%s), qid(%d:%s), error(%d)\n", taskid, env_get_taskname_by_id(taskid), q_id, fwk_msgq_name(q_id), FWK_MSGQ_ERROR_UNALLOC);
		return FWK_MSGQ_ERROR_UNALLOC;
	}
}

/*timeout: ms, or FWK_MSGQ_WAIT_INFINITE*/
int
fwk_msgq_select(unsigned int set_id, int timeout)
{
	int i;
	int ret;

	dbprintf_fwk(LOG_DEBUG, "pid(%d), set_id(%d), timeout(%d)\n", fwk_thread_gettid(), set_id, timeout);

	if (set_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	if (msgq_set_pool_g.set[set_id].is_used == 1) {
		/*clear result bit map */
		memset(msgq_set_pool_g.set[set_id].msgq_bitmap_result, 0x00,
		       sizeof (msgq_set_pool_g.set[set_id].msgq_bitmap_result));

		/*use the mutex associated with the cond var to lock for protecting queues length checking */
		fwk_mutex_lock(&msgq_set_pool_g.set[set_id].mutex);

		do {
			ret = 0;

			for (i = 0; i < FWK_MSGQ_MAX_QUEUE; i++) {
				if (((msgq_set_pool_g.set[set_id].
				      msgq_bitmap[i / 32] & ((unsigned int) 1 << (i % 32))) !=
				     0) && q_pool_g.queue[i].is_used == 1) {
					if (q_pool_g.queue[i].q_len > 0) {
						msgq_set_pool_g.set[set_id].msgq_bitmap_result[i / 32] |= ((int) 1 << (i % 32));	//(__FDS_BITS (s)[__FDELT(d)] |= __FDMASK(d)
						ret++;
					}
				}
			}

			/*if all queses' length were zero, wait on set's condition variable */
			if (!ret) {
				if (timeout == 0) {
					ret = FWK_MSGQ_ERROR_EMPTY;

					break;
				} else {
					if (timeout == FWK_MSGQ_WAIT_INFINITE) {
						ret = fwk_cond_wait(&msgq_set_pool_g.set[set_id].cond);
					} else {
						ret = fwk_cond_timedwait(&msgq_set_pool_g.set[set_id].cond, timeout);
					}

					if (ret < 0) {
						break;
					}
				}
			}
		}
		while (!ret);

		/*unlock */
		fwk_mutex_unlock(&msgq_set_pool_g.set[set_id].mutex);

		dbprintf_fwk(LOG_DEBUG, "pid(%d), ret(%d)\n", fwk_thread_gettid(), ret);

		return ret;
	} else {
		int taskid = fwk_thread_gettid();
		dbprintf_fwk(LOG_ERR, "pid(%d:%s), set_id(%d), error(%d)\n", taskid, env_get_taskname_by_id(taskid), set_id, FWK_MSGQ_ERROR_UNALLOC);
		return FWK_MSGQ_ERROR_UNALLOC;
	}

}

int
fwk_msgq_recv(unsigned int q_id, struct list_head **node)
{
	int ret = 0;

	dbprintf_fwk(LOG_DEBUG, "pid(%d), q_id(%d)\n", fwk_thread_gettid(), q_id);

	if (q_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	fwk_mutex_lock(&q_pool_g.queue[q_id].mutex);
	
	if (q_pool_g.queue[q_id].is_used == 1) {
		struct fwk_msgq_cond_node_t *cond_node_ptr;

		/*lock */
		list_for_each_entry(cond_node_ptr, &q_pool_g.queue[q_id].cond_list, cond_list) {
			if (msgq_set_pool_g.set[cond_node_ptr->cond_num].is_used == 1) {
				fwk_mutex_lock(&msgq_set_pool_g.set[cond_node_ptr->cond_num].mutex);
			}
		}

		/*check length */
		if (q_pool_g.queue[q_id].q_len > 0) {
			*node = q_pool_g.queue[q_id].q.prev;
			list_del(q_pool_g.queue[q_id].q.prev);
			q_pool_g.queue[q_id].q_len--;
			q_pool_g.queue[q_id].q_del++;
		} else {
			ret = FWK_MSGQ_ERROR_EMPTY;
		}

		/*unlock */
		list_for_each_entry_reverse(cond_node_ptr, &q_pool_g.queue[q_id].cond_list, cond_list) {
			if (msgq_set_pool_g.set[cond_node_ptr->cond_num].is_used == 1) {
				fwk_mutex_unlock(&msgq_set_pool_g.set[cond_node_ptr->cond_num].mutex);
			}
		}

		dbprintf_fwk(LOG_DEBUG, "pid(%d), ret(%d)\n", fwk_thread_gettid(), ret);

		fwk_mutex_unlock(&q_pool_g.queue[q_id].mutex);

		return ret;
	} else {
		int taskid = fwk_thread_gettid();
		dbprintf_fwk(LOG_ERR, "pid(%d:%s), qid(%d:%s), error(%d)\n", taskid, env_get_taskname_by_id(taskid), q_id, fwk_msgq_name(q_id), FWK_MSGQ_ERROR_UNALLOC);
		
		fwk_mutex_unlock(&q_pool_g.queue[q_id].mutex);
		
		return FWK_MSGQ_ERROR_UNALLOC;
	}
}

int
fwk_msgq_send(unsigned int q_id, struct list_head *node)
{
	int ret = 0;

	dbprintf_fwk(LOG_DEBUG, "pid(%d), q_id(%d)\n", fwk_thread_gettid(), q_id);

	if (q_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	fwk_mutex_lock(&q_pool_g.queue[q_id].mutex);
	
	if (q_pool_g.queue[q_id].is_used == 1) {
		struct fwk_msgq_cond_node_t *cond_node_ptr;

		/*lock */
		list_for_each_entry(cond_node_ptr, &q_pool_g.queue[q_id].cond_list, cond_list) {
			if (msgq_set_pool_g.set[cond_node_ptr->cond_num].is_used == 1) {
				fwk_mutex_lock(&msgq_set_pool_g.set[cond_node_ptr->cond_num].mutex);
			}
		}

		/*check length */
		if (q_pool_g.queue[q_id].q_len < q_pool_g.queue[q_id].q_size) {
			q_pool_g.queue[q_id].q_len++;
			q_pool_g.queue[q_id].q_add++;
			if (q_pool_g.queue[q_id].q_len_max <  q_pool_g.queue[q_id].q_len)
				q_pool_g.queue[q_id].q_len_max =  q_pool_g.queue[q_id].q_len;
			list_add(node, &q_pool_g.queue[q_id].q);

			/*signal all condition */
			list_for_each_entry(cond_node_ptr, &q_pool_g.queue[q_id].cond_list, cond_list) {
				if (msgq_set_pool_g.set[cond_node_ptr->cond_num].is_used == 1) {
					fwk_cond_signal(&msgq_set_pool_g.set[cond_node_ptr->cond_num].cond);
				}
			}
		} else {
			int taskid = fwk_thread_gettid();
			q_pool_g.queue[q_id].q_drop++;
			ret = FWK_MSGQ_ERROR_FULL;
			dbprintf_fwk(LOG_ERR, "pid(%d:%s), qid(%d:%s), ret(%d), msgq full!!\n", 
				taskid, env_get_taskname_by_id(taskid), q_id, fwk_msgq_name(q_id), ret);
		}

		/*unlock */
		list_for_each_entry_reverse(cond_node_ptr, &q_pool_g.queue[q_id].cond_list, cond_list) {
			if (msgq_set_pool_g.set[cond_node_ptr->cond_num].is_used == 1) {
				fwk_mutex_unlock(&msgq_set_pool_g.set[cond_node_ptr->cond_num].mutex);
			}
		}

		dbprintf_fwk(LOG_DEBUG, "pid(%d), ret(%d)\n", fwk_thread_gettid(), ret);

		fwk_mutex_unlock(&q_pool_g.queue[q_id].mutex);

		return ret;
	} else {
		int taskid = fwk_thread_gettid();
		dbprintf_fwk(LOG_ERR, "pid(%d:%s), qid(%d:%s), error(%d)\n", taskid, env_get_taskname_by_id(taskid), q_id, fwk_msgq_name(q_id), FWK_MSGQ_ERROR_UNALLOC);

		fwk_mutex_unlock(&q_pool_g.queue[q_id].mutex);
		
		return FWK_MSGQ_ERROR_UNALLOC;
	}
}

int
fwk_msgq_change_size(unsigned int q_id, unsigned int q_size)
{
	int ret = 0;

	dbprintf_fwk(LOG_DEBUG, "pid(%d), q_id(%d), size(%d)\n", fwk_thread_gettid(), q_id, q_size);

	if (q_id >= FWK_MSGQ_MAX_QUEUE) {
		dbprintf_fwk(LOG_ERR, "pid(%d), error(%d)\n", fwk_thread_gettid(), FWK_MSGQ_ERROR_OUTRANGE);

		return FWK_MSGQ_ERROR_OUTRANGE;
	}

	fwk_mutex_lock(&q_pool_g.queue[q_id].mutex);
	
	if (q_pool_g.queue[q_id].is_used == 1) {
		struct fwk_msgq_cond_node_t *cond_node_ptr;

		/*lock */
		list_for_each_entry(cond_node_ptr, &q_pool_g.queue[q_id].cond_list, cond_list) {
			if (msgq_set_pool_g.set[cond_node_ptr->cond_num].is_used == 1) {
				fwk_mutex_lock(&msgq_set_pool_g.set[cond_node_ptr->cond_num].mutex);
			}
		}

		/*change len */
		q_pool_g.queue[q_id].q_size = q_size;

		/*unlock */
		list_for_each_entry_reverse(cond_node_ptr, &q_pool_g.queue[q_id].cond_list, cond_list) {
			if (msgq_set_pool_g.set[cond_node_ptr->cond_num].is_used == 1) {
				fwk_mutex_unlock(&msgq_set_pool_g.set[cond_node_ptr->cond_num].mutex);
			}
		}

		dbprintf_fwk(LOG_DEBUG, "pid(%d), ret(%d)\n", fwk_thread_gettid(), ret);

		fwk_mutex_unlock(&q_pool_g.queue[q_id].mutex);

		return ret;
	} else {
		int taskid = fwk_thread_gettid();
		dbprintf_fwk(LOG_ERR, "pid(%d:%s), qid(%d:%s), error(%d)\n", taskid, env_get_taskname_by_id(taskid), q_id, fwk_msgq_name(q_id), FWK_MSGQ_ERROR_UNALLOC);

		fwk_mutex_unlock(&q_pool_g.queue[q_id].mutex);
		
		return FWK_MSGQ_ERROR_UNALLOC;
	}
}

int
fwk_msgq_len(unsigned int q_id)
{
	if ( q_id < FWK_MSGQ_MAX_QUEUE)
		return q_pool_g.queue[q_id].q_len;
	return 0;
}

char *
fwk_msgq_name(unsigned int q_id)
{
	if ( q_id < FWK_MSGQ_MAX_QUEUE)
		return q_pool_g.queue[q_id].q_name;
	return "?";
}

void
fwk_msgq_dump(int fd)
{
	int i;	
	for (i=0; i<FWK_MSGQ_MAX_QUEUE; i++) {
		if (q_pool_g.queue[i].is_used) {
			if (!q_pool_g.queue[i].q_len_max && !q_pool_g.queue[i].q_add && !q_pool_g.queue[i].q_del && !q_pool_g.queue[i].q_drop) {
				util_fdprintf(fd, "%2d: %-32s, size=%4d, len=%d\n", 
					i, q_pool_g.queue[i].q_name, q_pool_g.queue[i].q_size, q_pool_g.queue[i].q_len);
			} else {
				util_fdprintf(fd, "%2d: %-32s, size=%4d, len=%d (len_max=%d, add=%d, del=%d, drop=%d)\n", 
					i, q_pool_g.queue[i].q_name, q_pool_g.queue[i].q_size, 
					q_pool_g.queue[i].q_len, q_pool_g.queue[i].q_len_max, 
					q_pool_g.queue[i].q_add, q_pool_g.queue[i].q_del, q_pool_g.queue[i].q_drop);
			}
			q_pool_g.queue[i].q_len_max = q_pool_g.queue[i].q_add = q_pool_g.queue[i].q_del = q_pool_g.queue[i].q_drop = 0;
		}
	}
}
