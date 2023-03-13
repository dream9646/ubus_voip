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
 * File    : pmtask.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pmtask.h"
#include "tranxmod.h"
#include "meinfo.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "tasks.h"
#include "switch.h"
#include "gpon_sw.h"

#ifndef X86
#include "er_group_hw.h"
#endif

extern struct gpon_hw_t gpon_hw_g;

int pmtask_qid_g = -1;
static int pmtask_timer_qid_g = -1;
static int pmtask_qsetid_g = -1;
static int pmtask_loop_g = 0;
static struct fwk_thread_t pmtask_thread_g;
static int pmtask_interval_timer_id_g = 0;
static unsigned char pmtask_interval_tick_g = 0;	// wrap to 0 when reached 256

struct fwk_mutex_t pmtask_history_time_mutex;
struct tm pmtask_history_start_time;
struct tm pmtask_history_end_time;

static void
pmtask_init_history_time(void)
{
	fwk_mutex_lock(&pmtask_history_time_mutex);

	util_get_localtime(&pmtask_history_start_time);
	pmtask_history_end_time = pmtask_history_start_time;

	fwk_mutex_unlock(&pmtask_history_time_mutex);

	return;
}

static void
pmtask_swap_history_time(void)
{
	fwk_mutex_lock(&pmtask_history_time_mutex);

	pmtask_history_start_time = pmtask_history_end_time;
	util_get_localtime(&pmtask_history_end_time);

	fwk_mutex_unlock(&pmtask_history_time_mutex);

	return;
}

void
pmtask_get_history_time(struct tm *start_time, struct tm *end_time)
{
	fwk_mutex_lock(&pmtask_history_time_mutex);

	*start_time = pmtask_history_start_time;
	*end_time = pmtask_history_end_time;

	fwk_mutex_unlock(&pmtask_history_time_mutex);

	return;
}

static int 
pmtask_handler_sync_time(struct pmtask_msg_t *event)
{
	int index;
	
	//delete the pm interval timer
	if (pmtask_interval_timer_id_g > 0) {
		fwk_timer_delete(pmtask_interval_timer_id_g);
		pmtask_interval_timer_id_g = 0;
	}
	
	//start the interval timer as soon as possible
	if ((pmtask_interval_timer_id_g =
	     fwk_timer_create(pmtask_timer_qid_g, PMTASK_INTERVAL_REFRESH_TIMEOUT, omci_env_g.pm_refresh_interval * 1000, NULL)) < 0) {
		dbprintf(LOG_ERR, "timer create error=%d\n", pmtask_interval_timer_id_g);
	}

	//reset interval_tick
	pmtask_interval_tick_g = 0;

	pmtask_init_history_time();

	for (index = 0; index < MEINFO_INDEX_TOTAL; index++)
	{
		int i=meinfo_index2classid(index);
		struct meinfo_t *miptr=meinfo_util_miptr(i);

		//only pm entites have get_current_data action
		if (miptr != NULL && meinfo_util_get_action(i) & MEINFO_ACTION_MASK_GET_CURRENT_DATA)
		{
			struct list_head *pos;
			struct me_t *me_pos_ptr;
			
			//clear all values of pm attributes, including interval end time
			list_for_each(pos, &miptr->me_instance_list) {
				int j, k, ret;
				struct attr_value_t attr_value;
				unsigned char attr_mask[2];

				// quit tca if task is paused
				if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_PM))
					return 0;
				if (pos->next == LIST_POISON1 || pos->prev == LIST_POISON2) //maybe has uexpected released by gc
					return 0;
				me_pos_ptr = list_entry(pos, struct me_t, instance_node);

				//clear
				for (j = 1; j <= meinfo_util_get_attr_total(i); j++) {
					switch(j) {
					case 1:	// clear interval end time counter
						memset(&attr_value, 0x00, sizeof(struct attr_value_t));	// same as pmtask_interval_tick_g init value
						if ((ret = meinfo_me_attr_set(me_pos_ptr, j, &attr_value, 0))<0) {
							dbprintf(LOG_ERR, "set interval end time err=%d, classid=%u, meid=%u, attr_order=%u\n",
								ret, i, me_pos_ptr->meid, j);
						}
						break;
					case 2:
						//do noting
						break;
					default:	// clear history of pm attr
						if ((ret = meinfo_me_attr_time_sync_for_pm(me_pos_ptr, j))<0) {
							dbprintf(LOG_ERR, "clear pm attr err=%d, classid=%u, meid=%u, attr_order=%u\n",
								ret, i, me_pos_ptr->meid, j);
						}
					}
				}

				//reset hw counter
				if ((omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW || omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_SWHW )
					&& miptr->hardware.pm_reset_hw) {
					if ((ret = miptr->hardware.pm_reset_hw(me_pos_ptr)) < 0) {
						dbprintf(LOG_ERR, "hw pm reset cb err=%d, classid=%u, meid=%u\n",
								ret, i, me_pos_ptr->meid);
					}
				}

				// tricky! we want the me init with data=0, current=0, accum=hw_value
				// a. get pm data from hw (so current, accum has value)
				// b. clear current
				if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW &&
					miptr->hardware.get_hw) {
					memset(attr_mask, 0x00, sizeof(attr_mask));
					for (k = 3; k <=  meinfo_util_get_attr_total(i); k++)
						util_attr_mask_set_bit(attr_mask, k);
					if ((ret = miptr->hardware.get_hw(me_pos_ptr, attr_mask)) < 0) {
						dbprintf(LOG_ERR, "hw pm get cb err=%d, classid=%u, meid=%u, attr_mask[0]=0x%.2x, attr_mask[1]=0x%.2x\n",
							ret, i, me_pos_ptr->meid, attr_mask[0], attr_mask[1]);
					}
				}
				//clear curr, his
				for (j = 3; j <= meinfo_util_get_attr_total(i); j++) {
					me_pos_ptr->attr[j].value.data = 0;			//his
					me_pos_ptr->attr[j].value_current_data.data = 0;	//curr
				}
				
				// update result mask by tca check
				meinfo_me_determine_tca(i, me_pos_ptr->meid);

				// send tca to olt if different
				if (tasks_util_is_avc_alarm_tca_allowed() &&
				    memcmp(me_pos_ptr->alarm_report_mask, me_pos_ptr->alarm_result_mask, 28) != 0)
				{
					// send tca & update report mask
					omcimsg_alarm_generator(i, me_pos_ptr->meid, me_pos_ptr->alarm_result_mask);
					memcpy(me_pos_ptr->alarm_report_mask, me_pos_ptr->alarm_result_mask, 28);
				}
			}
		}
	}

#if 0	//Remove MGLC only code.
	/*call global pm reset callback*/
//#ifndef X86
	er_group_hw_util_pm_stop();
	er_group_hw_util_pm_start();
#endif

	return PMTASK_RW_OK;
}

static int 
pmtask_timer_handler_interval_refersh(struct fwk_timer_msg_t *timer_msg)
{
	int swap_flag = 0;
	int index;

	//check timer id
	if (timer_msg->timer_id != pmtask_interval_timer_id_g) {
		dbprintf(LOG_ERR, "timer id error, %d, %d\n", timer_msg->timer_id, pmtask_interval_timer_id_g);
		return -1;
	}

	//create next timer as soon as possible
	if ((pmtask_interval_timer_id_g =
	     fwk_timer_create(pmtask_timer_qid_g, PMTASK_INTERVAL_REFRESH_TIMEOUT, omci_env_g.pm_refresh_interval * 1000, NULL)) < 0) {
		dbprintf(LOG_ERR, "timer create error=%d\n", pmtask_interval_timer_id_g);
	}

	pmtask_interval_tick_g++;
	
	//traverse all pm entity instances, check threshhold
	for (index = 0; index < MEINFO_INDEX_TOTAL; index++)
	{
		int i=meinfo_index2classid(index);
		struct meinfo_t *miptr=meinfo_util_miptr(i);

		//only pm entites have get_current_data action
		if (miptr != NULL && (meinfo_util_get_action(i) & MEINFO_ACTION_MASK_GET_CURRENT_DATA))
		{
			struct list_head *pos;
			struct me_t *me_pos_ptr;

			//clear all values of pm attributes, including interval end time
			list_for_each(pos, &miptr->me_instance_list)
			{
				struct attr_value_t attr_value;
				int ret;
				int k;
				unsigned char attr_mask[2], attr_order;

				// quit tca if task is paused
				if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_PM))
					return 0;

				if (pos->next == LIST_POISON1 || pos->prev == LIST_POISON2) //maybe has uexpected released by gc
					return 0;
				me_pos_ptr = list_entry(pos, struct me_t, instance_node);

				//get pm data from hw
				if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW &&
					miptr->hardware.get_hw)
				{
					memset(attr_mask, 0x00, sizeof(attr_mask));
					for (k = 3; k <=  meinfo_util_get_attr_total(i); k++)
					{
						util_attr_mask_set_bit(attr_mask, k);
					}

					if ((ret = miptr->hardware.get_hw(me_pos_ptr, attr_mask)) < 0)
					{
						dbprintf(LOG_ERR, "hw pm get cb err=%d, classid=%u, meid=%u, attr_mask[0]=0x%.2x, attr_mask[1]=0x%.2x\n",
							ret, i, me_pos_ptr->meid, attr_mask[0], attr_mask[1]);
					}
				}

				// update result mask by tca check
				meinfo_me_determine_tca(i, me_pos_ptr->meid);

				// send tca to olt if different
				if (tasks_util_is_avc_alarm_tca_allowed() &&
				    memcmp(me_pos_ptr->alarm_report_mask, me_pos_ptr->alarm_result_mask, 28) != 0) {
					// send tca & update report mask
					omcimsg_alarm_generator(i, me_pos_ptr->meid, me_pos_ptr->alarm_result_mask);
					memcpy(me_pos_ptr->alarm_report_mask, me_pos_ptr->alarm_result_mask, 28);
				}

				//determine if swapping is needed for this me
				if (omci_env_g.pm_swap_interval) {
					unsigned char me_interval_end_time;
				
					meinfo_me_attr_get(me_pos_ptr, 1, &attr_value);
					me_interval_end_time = (unsigned char)attr_value.data;

					// the me has reached 15 min interval since previous interval end time
					// note: we intensionally use (unsign - unsign), so the result is also unsign
					if ((unsigned char)(pmtask_interval_tick_g - me_interval_end_time) >= 
						omci_env_g.pm_swap_interval / omci_env_g.pm_refresh_interval) {

						//unset tca and increase counter for interval end time
                				if (util_is_have_alarm_mask(me_pos_ptr->alarm_result_mask) != 0) {
	                        			memset(me_pos_ptr->alarm_result_mask, 0x00, 28);
        	                			omcimsg_alarm_generator(i, me_pos_ptr->meid, me_pos_ptr->alarm_result_mask);
                				}
						// mark timer_tick to me interval end time
						attr_value.data = pmtask_interval_tick_g;
						meinfo_me_attr_set(me_pos_ptr, 1, &attr_value, 0);

						for( attr_order=3; attr_order <= meinfo_util_get_attr_total(i); attr_order++) {
							meinfo_me_attr_swap_pm_data(me_pos_ptr, attr_order);
						}
					}
				}
			}
		}
	}

#if 0	//remove MGLC only code
	/*call global pm swap callback*/
//#ifndef X86
	if (swap_flag)
	{
		er_group_hw_util_pm_swap();
	}
#endif
	if (swap_flag)
	{
		pmtask_swap_history_time();
	}

	return 0;
}

static int 
pmtask_handler_determine_tca(struct pmtask_msg_t *event)
{
	struct meinfo_t *miptr=meinfo_util_miptr(event->classid);
	struct me_t *me;

	if (miptr != NULL && (miptr->action_mask & MEINFO_ACTION_MASK_GET_CURRENT_DATA) &&
		(me = meinfo_me_get_by_meid(event->classid, event->meid)) != NULL)
	{
		// update result mask by tca check
		meinfo_me_determine_tca(me->classid, me->meid);

		// send tca to olt if different
		if (tasks_util_is_avc_alarm_tca_allowed() &&
		    memcmp(me->alarm_report_mask, me->alarm_result_mask, 28) != 0)
		{
			// send tca & update report mask
			omcimsg_alarm_generator(me->classid, me->meid, me->alarm_result_mask);
			memcpy(me->alarm_report_mask, me->alarm_result_mask, 28);
		}
	} else {
		dbprintf(LOG_ERR, "determine unappropriate me's tca, classid=%u, meid=%u\n",
			event->classid, event->meid);
	}

	return 0;
}

static int
pmtask_timer_dispatcher(struct fwk_timer_msg_t *timer_msg)
{
	static unsigned int seq = 0;

	dbprintf(LOG_DEBUG, "timer event=%d\n", timer_msg->event);
	switch (timer_msg->event)
	{
	case PMTASK_INTERVAL_REFRESH_TIMEOUT:
		if (seq %10 == 0) {
			// the pm_rfresh() itself can cover hw counter wrap for once
			// but hw counter may wrap for more than once between calls to pm_refrsh()
			// so we still need to call pm_refresh() periodically
			switch_hw_g.pm_refresh(0);
		}
		pmtask_timer_handler_interval_refersh(timer_msg);
		seq++;
		break;
	default:
		dbprintf(LOG_ERR, "unknown timer event=%d\n", timer_msg->event);
	}

	return 0;
}

static void *
pmtask_func(void *ptr)
{
	int ret;
	
	struct list_head *entry;
	struct pmtask_msg_t *pmtask_msg = NULL;
	struct fwk_timer_msg_t *pmtask_timer_msg = NULL;

	util_set_thread_name("omci-pm");
	omci_env_g.taskname[ENV_TASK_NO_PM] = "PM";
	omci_env_g.taskid[ENV_TASK_NO_PM] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_PM] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_PM] = 0;
	omci_env_g.taskts[ENV_TASK_NO_PM] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_PM] = 0; //idle

	/*create the first timer*/
	if ((pmtask_interval_timer_id_g =
	     fwk_timer_create(pmtask_timer_qid_g, PMTASK_INTERVAL_REFRESH_TIMEOUT, omci_env_g.pm_refresh_interval * 1000, NULL)) < 0) {
		dbprintf(LOG_ERR, "timer create error=%d\n", pmtask_interval_timer_id_g);
	}

	pmtask_init_history_time();

	while (pmtask_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_PM]++;
		omci_env_g.taskts[ENV_TASK_NO_PM] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_PM] = 0; //idle

		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_PM)) {
			msleep(100);
			continue;
		}

		if ((ret = fwk_msgq_select(pmtask_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) < 0) {
			dbprintf(LOG_ERR, "select error(%d)\n", ret);
			continue;
		}

		omci_env_g.taskstate[ENV_TASK_NO_PM] = 1; //running
		
		//event
		if (fwk_msgq_set_isset(pmtask_qsetid_g, pmtask_qid_g)) {
			if ((ret = fwk_msgq_recv(pmtask_qid_g, &entry)) < 0) {
				dbprintf(LOG_ERR, "recv error(%d)\n", ret);
				continue;
			}
			
			/*process data */
			pmtask_msg = list_entry(entry, struct pmtask_msg_t, q_entry);
			
			switch (pmtask_msg->cmd) {
			case PMTASK_CMD_SYNC_TIME:		// olt send synctime
				pmtask_handler_sync_time(pmtask_msg);
				break;
			case PMTASK_CMD_TCA_DETERMINATION:	// olt get pm
				pmtask_handler_determine_tca(pmtask_msg);
				break;
			case PMTASK_CMD_TERMINATE:
				dbprintf(LOG_ERR, "task terminate\n");
				break;
			default:
				dbprintf(LOG_ERR, "get wrong data\n");
			}

			free_safe(pmtask_msg);
			pmtask_msg = NULL;
		}
		
		//timer
		if (fwk_msgq_set_isset(pmtask_qsetid_g, pmtask_timer_qid_g)) {
			if ((ret = fwk_msgq_recv(pmtask_timer_qid_g, &entry)) < 0) {
				dbprintf(LOG_ERR, "recv timer error(%d)\n", ret);
				continue;
			}

			/*process timer msg */
			pmtask_timer_msg = list_entry(entry, struct fwk_timer_msg_t, node);
			pmtask_timer_dispatcher(pmtask_timer_msg);
			free_safe(pmtask_timer_msg);
			pmtask_timer_msg = NULL;
		}
	} 

	omci_env_g.taskstate[ENV_TASK_NO_PM] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_PM] = 0;

	return 0;
}

int
pmtask_init(void)
{
	int ret;

	/*allocate queue and set */
	if ((pmtask_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_PM_CMD")) < 0) {
		dbprintf(LOG_ERR, "queue alloc error(%d)\n", pmtask_qid_g);
		return (-1);
	}
	if ((pmtask_timer_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_PM_TIMER")) < 0) {
		fwk_msgq_release(pmtask_qid_g);
		dbprintf(LOG_ERR, "timer queue alloc error(%d)\n", pmtask_timer_qid_g);
		return (-1);
	}
	if ((pmtask_qsetid_g = fwk_msgq_set_alloc()) < 0) {
		fwk_msgq_release(pmtask_qid_g);
		fwk_msgq_release(pmtask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set alloc error(%d)\n", pmtask_qsetid_g);
		return (-1);
	}

	/*set msgq set */
	if ((ret = fwk_msgq_set_zero(pmtask_qsetid_g)) < 0) {
		fwk_msgq_set_release(pmtask_qsetid_g);
		fwk_msgq_release(pmtask_qid_g);
		fwk_msgq_release(pmtask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set zero error(%d)\n", ret);
		return (-1);
	}
	if ((ret = fwk_msgq_set_set(pmtask_qsetid_g, pmtask_qid_g)) < 0) {
		fwk_msgq_set_release(pmtask_qsetid_g);
		fwk_msgq_release(pmtask_qid_g);
		fwk_msgq_release(pmtask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return (-1);
	}
	if ((ret = fwk_msgq_set_set(pmtask_qsetid_g, pmtask_timer_qid_g)) < 0) {
		fwk_msgq_set_release(pmtask_qsetid_g);
		fwk_msgq_release(pmtask_qid_g);
		fwk_msgq_release(pmtask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return (-1);
	}

	fwk_mutex_create(&pmtask_history_time_mutex);

	pmtask_interval_timer_id_g = 0;
	pmtask_interval_tick_g = 0;
	
	return 0;
}

int
pmtask_start(void)
{
	int ret;

	/*create pm task */
	pmtask_thread_g.policy = FWK_THREAD_POLICY_OMCI_NOTIFY;
	pmtask_thread_g.priority = FWK_THREAD_PRIO_OMCI_NOTIFY;
	pmtask_thread_g.stack_size = 128*1024;
	pmtask_thread_g.join = 1;
	sprintf(pmtask_thread_g.name, "omci-PM");
	pmtask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&pmtask_thread_g, pmtask_func, NULL)) < 0) {
		dbprintf(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}
	return 0;
}

int
pmtask_shutdown(void)
{
	fwk_msgq_set_release(pmtask_qsetid_g);
	fwk_msgq_release(pmtask_qid_g);
	fwk_msgq_release(pmtask_timer_qid_g);

	fwk_mutex_destroy(&pmtask_history_time_mutex);

	pmtask_qid_g = -1;
	pmtask_timer_qid_g = -1;
	pmtask_qsetid_g = -1;
	pmtask_interval_timer_id_g = 0;
	pmtask_interval_tick_g = 0;
	
	return 0;
}

int
pmtask_stop(void)
{
	int ret;
	struct pmtask_msg_t *pmtask_msg;
	pmtask_loop_g = 0;	//stop loop

	pmtask_msg = malloc_safe(sizeof (struct pmtask_msg_t));

	INIT_LIST_NODE(&pmtask_msg->q_entry);
	pmtask_msg->cmd = PMTASK_CMD_TERMINATE;
	if (fwk_msgq_send(pmtask_qid_g, &pmtask_msg->q_entry) < 0)
	{
		free_safe(pmtask_msg);
	}
	
	if ((ret = fwk_thread_join(&pmtask_thread_g)) < 0) {
		dbprintf(LOG_ERR, "join error(%d)\n", ret);
	}

	return 0;
}


