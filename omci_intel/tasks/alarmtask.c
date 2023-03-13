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
 * File    : alarmtask.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>
#include "meinfo.h"
#include "list.h"
#include "util.h"
#include "util_run.h"
#include "omcimsg.h"
#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "notify_chain.h"
#include "tasks.h"
#include "alarmtask.h"
#include "switch.h"
#include "omciutil_usage.h"
#include "gpon_sw.h"
#include "coretask.h"

#ifdef OMCI_BM_ENABLE
#include "omci_bm.h"
#endif

extern struct omcimsg_getallalarms_state_t omcimsg_getallalarms_state;
static struct fwk_thread_t alarmtask_thread;
static int alarmtask_queue;
static int alarmtask_queue_set;
static unsigned char alarmtask_loop_g;

int     
alarmtask_send_msg(unsigned short classid, unsigned short instance_num)
{               
        struct alarmtask_msg_t *alarmtask_msg = NULL;
                        
        /*alloc omci msg memory, omci data */
	alarmtask_msg = malloc_safe(sizeof (struct alarmtask_msg_t));
	INIT_LIST_NODE(&alarmtask_msg->alarmtask_node);
	alarmtask_msg->cmd=ALARMTASK_CMD_PROCESS;
	//classid=0, instance_num=0, check all me
	alarmtask_msg->classid=classid;
	alarmtask_msg->instance_num=instance_num;
	if (fwk_msgq_send(alarmtask_queue, &alarmtask_msg->alarmtask_node) < 0)
	{
		free_safe(alarmtask_msg);
	}
        return 0;
}

int     
alarmtask_send_update_msg(unsigned short classid, unsigned short instance_num, unsigned char *alarm_mask, unsigned char *alarm_value)
{               
	struct alarmtask_msg_t *alarmtask_msg = NULL;
                        
	/*alloc omci msg memory, omci data */
	alarmtask_msg = malloc_safe(sizeof (struct alarmtask_msg_t));
	INIT_LIST_NODE(&alarmtask_msg->alarmtask_node);
	alarmtask_msg->cmd=ALARMTASK_CMD_UPDATE;
	alarmtask_msg->classid=classid;
	alarmtask_msg->instance_num=instance_num;
	memcpy(alarmtask_msg->alarm_mask, alarm_mask, 28);
	memcpy(alarmtask_msg->alarm_value, alarm_value, 28);
	if (fwk_msgq_send(alarmtask_queue, &alarmtask_msg->alarmtask_node) < 0)
	{
		free_safe(alarmtask_msg);
	}
	return 0;
}

int     
alarmtask_boot_notify(unsigned char event,
                      unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr)
{               
        /*alloc omci msg memory, omci data */
	if (event == NOTIFY_CHAIN_EVENT_BOOT) {
		//classid=0, instance_num=0, check all me
		alarmtask_send_msg(0,0);	
        	return 0;
	}
        return 1;
}

static int
alarmtask_check_me(struct me_t *me, int is_arc_cared)
{
	struct meinfo_t *miptr;

	if (me==NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return MEINFO_ERROR_ME_PTR_NULL;
	}		
	if ((miptr=meinfo_util_miptr(me->classid))==NULL) {
		dbprintf(LOG_ERR, "null miptr?\n");
		return MEINFO_ERROR_CLASSID_UNDEF;
	}
	if (me->is_private)
		return 0;
	
	if (util_is_have_alarm_mask(miptr->alarm_mask)) {
		int is_alarm_allowed = 1;
	
		// call check function, update alarm_result_mask
		if (miptr->callback.alarm_cb && miptr->callback.alarm_cb(me, me->alarm_result_mask) < 0) {
			dbprintf(LOG_ERR, "alarm_cb error?\n");
			return MEINFO_ERROR_CB_ME_ALARM;
		}
		if (miptr->hardware.alarm_hw && miptr->hardware.alarm_hw(me, me->alarm_result_mask) < 0) {
			dbprintf(LOG_ERR, "alarm_hw error?\n");
			return MEINFO_ERROR_HW_ME_ALARM;
		}

		// NOTE!
		// as some house keeping be hooked in alarm handler, eg: led
		// so we always do above alarm check no matter if alarm msg will be sent or not

		// no alarm if admin locked
		if (meinfo_me_is_admin_locked(me)) {
			is_alarm_allowed=0;
			// clear all alarm while admin locked
			// so alarm will be re-generated when admin unlocked
			memset(me->alarm_report_mask, 0, 28);
		}
		
		// no alarm if arc cared and enabled
		if (is_arc_cared && meinfo_me_is_arc_enabled(me))
			is_alarm_allowed=0;
		
		if (!tasks_util_is_avc_alarm_tca_allowed())
			is_alarm_allowed=0;
			
		// send alarm to OLT if different
		if (is_alarm_allowed &&
		    memcmp(me->alarm_report_mask, me->alarm_result_mask, 28) != 0) { 
			struct me_t *alarm_me=me;
#if defined(USE_MEINFO_ME_COPY_ON_WRITE)
			// if me ref count >1, the me maybe already used by get all alarms,
			// so we need to do COW to ensure the change of alarm_result_mask won't happen on me 
			// that belongs to get all alarms
			if (me->refcount > 1) {
				if (meinfo_me_copy_on_write(me, &alarm_me) < 0) {
					dbprintf(LOG_ERR, "me copy_on_write error?");
					return MEINFO_ERROR_ME_COPY_ON_WRITE;
				}
			}
#endif
			// send alarm
			dbprintf(LOG_NOTICE, "result different, send alarm nocice to OLT\n");
			omcimsg_alarm_generator(alarm_me->classid, alarm_me->meid, me->alarm_result_mask);
			// update alarm report mask
			memcpy(alarm_me->alarm_report_mask, me->alarm_result_mask, 28);
		}
	}
	return 0;
}

static int
alarmtask_check_all_me(int is_arc_cared)
{
	unsigned short index;
	unsigned char tmp_attr_mask[2] = {0xff, 0xff};

	for (index = 1; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid=meinfo_index2classid(index);
		struct meinfo_t *miptr=meinfo_util_miptr(classid);
		struct list_head *pos,*n;
		struct me_t *me_pos_ptr;

		if (miptr && !miptr->is_private && util_is_have_alarm_mask(miptr->alarm_mask)) {
			// use safe routine because me_pos_ptr may be removed 
			// in meinfo_me_copy_on_write() within alarmtask_check_me()
			list_for_each_safe(pos, n, &miptr->me_instance_list) {
				// quit alarm if task paused
				if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_ALARM))
					return 0;
				if (pos->next == LIST_POISON1 || pos->prev == LIST_POISON2) //maybe has uexpected released by gc
					return 0;
				if (omcimsg_mib_upload_is_in_progress() || omcimsg_get_all_alarms_is_in_progress())
					return 0;
				me_pos_ptr = list_entry(pos, struct me_t, instance_node);
				if (me_pos_ptr->is_private)
					continue;
				alarmtask_check_me(me_pos_ptr, is_arc_cared);
			}
		}

		//for ready state check
		if (miptr && (miptr->callback.is_ready_cb != NULL || miptr->hardware.is_ready_hw != NULL))
		{
			list_for_each(pos, &miptr->me_instance_list)
			{
				// quit alarm if task paused
				if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_ALARM))
					return 0;
				if (pos->next == LIST_POISON1 || pos->prev == LIST_POISON2) //maybe has uexpected released by gc
					return 0;
				if (omcimsg_mib_upload_is_in_progress() || omcimsg_get_all_alarms_is_in_progress())
					return 0;
				me_pos_ptr = list_entry(pos, struct me_t, instance_node);
				meinfo_me_check_ready_state(me_pos_ptr, tmp_attr_mask);
			}
		}
	}
	return 0;
}

static int
alarmtask_update_me(unsigned short classid, unsigned short instance_num, unsigned char *alarm_mask, unsigned char *alarm_value)
{
	int i;
	struct me_t *me;
	struct meinfo_t *miptr;

	if ((me = meinfo_me_get_by_instance_num(classid, instance_num)) == NULL ||
		alarm_mask == NULL || alarm_value == NULL) {
		dbprintf(LOG_ERR, "null me ptr?\n");
		return -1;
	}

	miptr = meinfo_util_miptr(classid);

	if (util_is_have_alarm_mask(miptr->alarm_mask)) {
		int is_alarm_allowed=1;
	
		// update result mask
		for (i = 0; i < 28; i++) {
			if (alarm_mask[i] != 0) {
				me->alarm_result_mask[i] = ((alarm_value[i] & alarm_mask[i]) |
								(me->alarm_result_mask[i] & ~alarm_mask[i]));
			}
		}

		// no alarm if admin locked or if arc cared and enabled
		if (meinfo_me_is_admin_locked(me) ||
		    (meinfo_me_is_arc_enabled(me)))
			is_alarm_allowed=0;

		if (!tasks_util_is_avc_alarm_tca_allowed())
			is_alarm_allowed=0;

		// send alarm to OLT if different
		if (is_alarm_allowed &&
		    memcmp(me->alarm_report_mask, me->alarm_result_mask, 28) != 0) { 
			struct me_t *alarm_me=me;
#if defined(USE_MEINFO_ME_COPY_ON_WRITE)
			// if me ref count >1, the me maybe already used by get all alarms,
			// so we need to do COW to ensure the change of alarm_result_mask won't happen on me 
			// that belongs to get all alarms
			if (me->refcount > 1) {
				if (meinfo_me_copy_on_write(me, &alarm_me) < 0) {
					dbprintf(LOG_ERR, "me copy_on_write error?");
					return MEINFO_ERROR_ME_COPY_ON_WRITE;
				}
			}
#endif
			// send alarm
			dbprintf(LOG_NOTICE, "result different, send alarm nocice to OLT, classid=%u, meid=%u, alarm=0x%.2x\n", alarm_me->classid, alarm_me->meid, alarm_me->alarm_result_mask[0]);
			omcimsg_alarm_generator(alarm_me->classid, alarm_me->meid, alarm_me->alarm_result_mask);
			// update alarm report mask
			memcpy(alarm_me->alarm_report_mask, alarm_me->alarm_result_mask, 28);
		}
	}
	return 0;
}

static void
alarmtask_hw_watchdog(void)
{
	// clear dummy pq which is used to store unwanted pkt for upstremam flow
	gpon_hw_g.util_clearpq_dummy();

	if (omci_env_g.watchdog_omccmiss_timeout[0] || omci_env_g.watchdog_omccmiss_timeout[1]) {
		int remain_time = gpon_hw_g.util_omccmiss_detect();
		if (omci_env_g.watchdog_omccmiss_timeout[1] && remain_time > omci_env_g.watchdog_omccmiss_timeout[1]) {
			system("/etc/init.d/info_collect.sh fast save");
			openlog("omcimain", LOG_PID, LOG_USER); syslog(LOG_CRIT, "omccmiss: reboot...\n"); closelog();
			dbprintf(LOG_ERR, "omccmiss: reboot...\n");
			util_run_by_system("reboot");
		} else if (omci_env_g.watchdog_omccmiss_timeout[0] && remain_time > omci_env_g.watchdog_omccmiss_timeout[0]) {
			if (gpon_hw_g.util_reset_serdes) {
				static int reset_total = 0;
				dbprintf(LOG_ERR, "omccmiss: reset serdes %d...\n", reset_total++);
				//coretask_cmd(CORETASK_CMD_MIB_RESET);
				meinfo_util_increase_datasync();
				gpon_hw_g.util_reset_serdes(10);	// 10ms
			}
		}
	}
	if (omci_env_g.watchdog_tcontmiss_timeout[0] || omci_env_g.watchdog_tcontmiss_timeout[1]) {
		int remain_time = gpon_hw_g.util_tcontmiss_detect();
		if (omci_env_g.watchdog_tcontmiss_timeout[1] && remain_time > omci_env_g.watchdog_tcontmiss_timeout[1]) {
			system("/etc/init.d/info_collect.sh fast save");
			openlog("omcimain", LOG_PID, LOG_USER); syslog(LOG_CRIT, "tcontmiss: reboot...\n"); closelog();
			dbprintf(LOG_ERR, "tcontmiss: reboot...\n");
			util_run_by_system("reboot");
		} else if (omci_env_g.watchdog_tcontmiss_timeout[0] && remain_time > omci_env_g.watchdog_tcontmiss_timeout[0]) {
			if (gpon_hw_g.util_reset_serdes) {
				static int reset_total = 0;
				dbprintf(LOG_ERR, "tcontmiss: reset serdes %d...\n", reset_total++);
				//coretask_cmd(CORETASK_CMD_MIB_RESET);
				meinfo_util_increase_datasync();
				gpon_hw_g.util_reset_serdes(10);	// 10ms
			}
		}
	}

	if (omci_env_g.watchdog_o2stuck_timeout[0] || omci_env_g.watchdog_o2stuck_timeout[1]) {
		int remain_time = gpon_hw_g.util_o2stuck_detect();
		if (omci_env_g.watchdog_o2stuck_timeout[1] && remain_time > omci_env_g.watchdog_o2stuck_timeout[1]) {
			system("/etc/init.d/info_collect.sh fast save");
			openlog("omcimain", LOG_PID, LOG_USER); syslog(LOG_CRIT, "o2stuck: reboot...\n"); closelog();
			dbprintf(LOG_ERR, "o2stuck: reboot...\n");
			util_run_by_system("reboot");
		} else if (omci_env_g.watchdog_o2stuck_timeout[0] && remain_time > omci_env_g.watchdog_o2stuck_timeout[0]) {
			if (gpon_hw_g.util_reset_serdes) {
				static int reset_total = 0;
				dbprintf(LOG_ERR, "o2stuck: reset serdes %d...\n", reset_total++);
				//coretask_cmd(CORETASK_CMD_MIB_RESET);
				meinfo_util_increase_datasync();
				gpon_hw_g.util_reset_serdes(10);	// 10ms
			}
		}
	}

	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
		// idlebw gaurd is only required for CALIX olt,
		// since other olt assigns bw to ont based on both dbru and idle gem pkt
		// but CALIX olt assigns bw based on dbru only
		if (omci_env_g.watchdog_idlebw_guard[0] && 
		    gpon_hw_g.util_idlebw_is_overalloc(omci_env_g.watchdog_idlebw_guard) >0) {
		    	static int reset_total = 0;
			unsigned int state;
			dbprintf(LOG_ERR, "idlebw over requested: reset dbru %d...\n", reset_total++);
			gpon_hw_g.util_protect_start(&state);
			gpon_hw_g.util_clearpq(0); 
			gpon_hw_g.util_reset_dbru();
			gpon_hw_g.util_protect_end(state);
		}
	}
	return;
}

static void *
alarmtask_func(void *ptr)
{
	int ret;
	struct list_head *alarmtask_entry;
	struct alarmtask_msg_t *alarmtask_msg = NULL;

	util_set_thread_name("omci-alarm");
	omci_env_g.taskname[ENV_TASK_NO_ALARM] = "ALARM";
	omci_env_g.taskid[ENV_TASK_NO_ALARM] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_ALARM] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_ALARM] = 0;
	omci_env_g.taskts[ENV_TASK_NO_ALARM] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_ALARM] = 0; //idle

	while (alarmtask_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_ALARM]++;
		omci_env_g.taskts[ENV_TASK_NO_ALARM] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_ALARM] = 0; //idle

		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_ALARM)) {
			msleep(100);
			continue;
		}

		if ((ret = fwk_msgq_select(alarmtask_queue_set, omci_env_g.alarm_refresh_interval * 1000)) > 0) {
			omci_env_g.taskstate[ENV_TASK_NO_ALARM] = 1; //running
			if (fwk_msgq_set_isset(alarmtask_queue_set, alarmtask_queue)) {
				dbprintf(LOG_NOTICE, "alarmtask receive notify\n", ret);
				if ((ret = fwk_msgq_recv(alarmtask_queue, &alarmtask_entry)) < 0) {
					dbprintf(LOG_ERR, "recv alarmtask error(%d)\n", ret);
					continue;
				}
				
				/*process alarmtask msg */
				alarmtask_msg = list_entry(alarmtask_entry, struct alarmtask_msg_t, alarmtask_node);

				switch(alarmtask_msg->cmd) {
				case ALARMTASK_CMD_PROCESS:
					if ( alarmtask_msg->classid==0 && alarmtask_msg->instance_num==0 ){
						// case for get all alarm message from OLT
						dbprintf(LOG_NOTICE, "alarmtask_check_all_me\n");
						alarmtask_check_all_me(omcimsg_get_all_alarms_arc());
					} else {
						// case for notification from local event
						struct me_t *me = meinfo_me_get_by_instance_num(alarmtask_msg->classid, alarmtask_msg->instance_num);
						dbprintf(LOG_NOTICE, "alarmtask_check_me, classid=%u, instance=%d\n", alarmtask_msg->classid, alarmtask_msg->instance_num);
						if (me)
							alarmtask_check_me(me, 1);
					}
					break;
				case ALARMTASK_CMD_UPDATE:
					alarmtask_update_me(alarmtask_msg->classid, alarmtask_msg->instance_num, alarmtask_msg->alarm_mask, alarmtask_msg->alarm_value);
					dbprintf(LOG_NOTICE, "alarmtask_update_me, classid=%u, instance_num=%d\n", alarmtask_msg->classid, alarmtask_msg->instance_num);
					break;
				case ALARMTASK_CMD_TERMINATE:
					dbprintf(LOG_ERR, "task terminate\n");
					break;
				default:
					dbprintf(LOG_ERR, "get wrong data\n");
				}
				free_safe(alarmtask_msg);
			}
		} else if (ret == FWK_COND_ERROR_TIMEOUT) {
			omci_env_g.taskstate[ENV_TASK_NO_ALARM] = 1; //running
			
			dbprintf(LOG_NOTICE, "polling interval process\n");

			{ // do alarm check only if in O5 & !mib_upload && !get_all_alarms
				unsigned int state = GPON_ONU_STATE_O5;
				if (gpon_hw_g.onu_state_get)
					gpon_hw_g.onu_state_get(&state);
				if (state == GPON_ONU_STATE_O5 && 
				    !omcimsg_mib_upload_is_in_progress() &&
				    !omcimsg_get_all_alarms_is_in_progress())
					alarmtask_check_all_me(1);
			}
			
			// cpu usage
			if (omci_env_g.cpu_usage_threshold) {
				if (omciutil_usage_cpu_used_percentage()/100 >= omci_env_g.cpu_usage_threshold)
					dbprintf(LOG_ERR, "%s\n", omciutil_usage_cpu_counter_to_str(0, NULL));
			}
			// gpon hw watchdog
			alarmtask_hw_watchdog();

#ifdef OMCI_BM_ENABLE
			omci_bm_check2(3600*24);	// check per day
#endif	
		} else {
			omci_env_g.taskstate[ENV_TASK_NO_ALARM] = 1; //running			
			dbprintf(LOG_ERR, "fwk_msgq_select error (%d)\n", ret);
		}
	}

	omci_env_g.taskstate[ENV_TASK_NO_ALARM] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_ALARM] = 0;

	return 0;
}

int
alarmtask_init(void)
{
	int ret;
	/*allocate queue and queue set */
	if ((alarmtask_queue = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_ALARM_CMD")) < 0) {
		dbprintf(LOG_ERR, "alarmtask_queue alloc error(%d)\n", alarmtask_queue);
		return -1;
	}

	if ((alarmtask_queue_set = fwk_msgq_set_alloc()) < 0) {
		fwk_msgq_release(alarmtask_queue);
		dbprintf(LOG_ERR, "queue set alloc error(%d)\n", alarmtask_queue_set);
		return -1;
	}

	/*set alarm task queue set */
	if ((ret = fwk_msgq_set_zero(alarmtask_queue_set)) < 0) {
		fwk_msgq_set_release(alarmtask_queue_set);
		fwk_msgq_release(alarmtask_queue);
		dbprintf(LOG_ERR, "queue set zero error(%d)\n", ret);
		return -1;
	}

	if ((ret = fwk_msgq_set_set(alarmtask_queue_set, alarmtask_queue)) < 0) {
		fwk_msgq_set_release(alarmtask_queue_set);
		fwk_msgq_release(alarmtask_queue);
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}

	return 0;
}

int
alarmtask_start(void)
{
	int ret;

	/*create recv task */
	alarmtask_thread.policy = FWK_THREAD_POLICY_OMCI_NOTIFY;
	alarmtask_thread.priority = FWK_THREAD_PRIO_OMCI_NOTIFY;
	alarmtask_thread.stack_size = 128*1024;
	alarmtask_thread.join = 1;
	sprintf(alarmtask_thread.name, "omci-ALARM");
	alarmtask_loop_g = 1;	//start loop

	if ((ret = fwk_thread_create(&alarmtask_thread, alarmtask_func, NULL)) < 0) {
		dbprintf(LOG_ERR, "thread create error(%d)\n", ret);
		return -1;
	}
	return 0;
}

int
alarmtask_stop(void)
{
	int ret;
	struct alarmtask_msg_t *alarmtask_msg;

	alarmtask_loop_g = 0;	//stop loop

	alarmtask_msg = malloc_safe(sizeof (struct alarmtask_msg_t));
	INIT_LIST_NODE(&alarmtask_msg->alarmtask_node);
	alarmtask_msg->cmd = ALARMTASK_CMD_TERMINATE;
	if (fwk_msgq_send(alarmtask_queue, &alarmtask_msg->alarmtask_node) < 0)
	{
		free_safe(alarmtask_msg);
	}

	if ((ret = fwk_thread_join(&alarmtask_thread)) < 0) {
		dbprintf(LOG_ERR, "alarmtask_stop: join error(%d)\n", ret);
	}

	return 0;
}

int
alarmtask_shutdown(void)
{
	fwk_msgq_set_release(alarmtask_queue_set);
	fwk_msgq_release(alarmtask_queue);
	alarmtask_queue = -1;
	alarmtask_queue_set = -1;
	alarmtask_loop_g = 0;
	return 0;
}
