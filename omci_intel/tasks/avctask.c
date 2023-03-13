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
 * File    : avctask.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "omcimsg.h"
#include "tranxmod.h"
#include "meinfo.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "gpon_sw.h"
#include "tasks.h"
#include "avctask.h"

int avctask_msg_qid_g = -1;
static int avctask_timer_qid_g = -1;
static int avctask_qsetid_g = -1;
static int avctask_loop_g = 0;
static struct fwk_thread_t avctask_thread_g;
static int avctask_interval_timer_id_g = -1;

static int
avctask_me_collect_value(struct me_t *me, unsigned char avc_notify_mask[2], unsigned char *value) 
{
	struct meinfo_t *miptr;
	unsigned short attr_order;
	int next_start_byte=0;
	unsigned char attr_mask[2];

	if (me==NULL || (miptr=meinfo_util_miptr(me->classid))==NULL)
		return AVCTASK_ERROR_UNKNOWN_ME;

	memset(attr_mask, 0, sizeof(attr_mask));	// for value fill on routine omcimsg_util_set_attr_by_mask
	memset(avc_notify_mask, 0, sizeof(char)*2);	// for avc msg
	memset(value, 0, AVC_VALUE_SIZE);

	for (attr_order=1; attr_order<= miptr->attr_total; attr_order++) {
		// check if any each avc changed mask set
		if (util_attr_mask_get_bit(miptr->avc_mask, attr_order) &&
		    util_attr_mask_get_bit(me->avc_changed_mask, attr_order)) {
			// fill attr into value,
			// but for table in avc msg, value is not required
			if (meinfo_util_attr_get_format(me->classid, attr_order)!=ATTR_FORMAT_TABLE) {
			    	struct attr_value_t attr;
			    	int ret;
			    	
			    	attr.data=0;
			    	attr.ptr=malloc_safe(AVC_VALUE_SIZE);

				next_start_byte += meinfo_util_attr_get_byte_size(me->classid, attr_order);
				if (next_start_byte > AVC_VALUE_SIZE) {	// total bytes > attrs limit
					dbprintf(LOG_WARNING, "avc size full, classid=%u, meid=%u, attr_order=%u\n",
						 me->classid, me->meid, attr_order);
					break;
				}

				ret=meinfo_me_attr_get(me, attr_order, &attr);
				if (ret<0) {
					dbprintf(LOG_ERR, "attr from mib failed, classid=%u, meid=%u, attr_order=%u, err=%d\n",
						 me->classid, me->meid, attr_order, ret);
					// clear bit in avc_changed_mask to avoid repeat avc on this attr again
					util_attr_mask_clear_bit(me->avc_changed_mask, attr_order);
					free_safe(attr.ptr);
					continue;					
				}

				util_attr_mask_set_bit(attr_mask, attr_order);				

				ret=omcimsg_util_set_attr_by_mask
				    (value, AVC_VALUE_SIZE, me->classid, attr_order, attr_mask, &attr);
				if (ret<0) {	// out of space within AVC_VALUE_SIZE ?
					dbprintf(LOG_ERR, "attr to content failed, classid=%u, meid=%u, attr_order=%u, err=%d\n",
						 me->classid, me->meid, attr_order, ret);
					// remove this attr in attrmask
					util_attr_mask_clear_bit(attr_mask, attr_order);
					free_safe(attr.ptr);
					continue;					
				}

				free_safe(attr.ptr);
			}
			// set avc notify mask
			util_attr_mask_clear_bit(me->avc_changed_mask, attr_order);
			util_attr_mask_set_bit(avc_notify_mask, attr_order);
		}
	}
	return AVCTASK_RW_OK;
}

static int
avctask_me_check(unsigned short avc_classid, unsigned short avc_instance_num) 
{
	int index;
	int avc_sent=0;

	//traverse all me with avc
	for (index = 0; index < MEINFO_INDEX_TOTAL; index++) {
		int classid=meinfo_index2classid(index);
		struct meinfo_t *miptr;
		struct list_head *pos;
		struct me_t *me;

		if (avc_classid && avc_classid!=classid)
			continue;

		// skip class whose attr doesn't require avc
		miptr=meinfo_util_miptr(classid);
		if (miptr == NULL || miptr->is_private || (miptr->avc_mask[0]==0 && miptr->avc_mask[1]==0))
			continue;

		// traverse all instance
		list_for_each(pos, &miptr->me_instance_list) {
			unsigned char avc_notify_mask[2];
			unsigned char content[AVC_VALUE_SIZE];
			int retry;

			// quit avc is task paused
			if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_AVC))
				return avc_sent;
			if (pos->next == LIST_POISON1 || pos->prev == LIST_POISON2) //maybe has uexpected released by gc
				return avc_sent;
			if (omcimsg_mib_upload_is_in_progress() || omcimsg_get_all_alarms_is_in_progress())
				return avc_sent;

			me = list_entry(pos, struct me_t, instance_node);
			if (me->is_private)
				continue;
			if (avc_instance_num && avc_instance_num!=me->instance_num)
				continue;
			if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW && miptr->hardware.get_hw) {
				unsigned char avc_mask[2];
				int hw_ret;
				avc_mask[0] = miptr->avc_mask[0];
				avc_mask[1] = miptr->avc_mask[1];
				if ((hw_ret = miptr->hardware.get_hw(me, avc_mask)) < 0) {
					dbprintf(LOG_ERR,
					 "get_hw err=%d, classid=%u, meid=%u, ret=%d, mask[0]=0x%2x, mask[1]=0x%2x\n",
					 OMCIMSG_ERROR_HW_ME_GET, classid, me->meid, hw_ret, avc_mask[0], avc_mask[1]);
				}
			}			

			// NOTE!
			// as some house keeping be hooked in hw get handler, eg: led
			// so we always do above hw get no matter if avc msg will be sent or not

			// avc if me administrative state != locked
			if (!meinfo_me_is_admin_locked(me) &&
			    tasks_util_is_avc_alarm_tca_allowed()) {
				// we retry here as one me may have several attrs changed, which can not be contained in one avc
				for (retry=0; retry<8; retry++) {
					if (me->avc_changed_mask[0] || me->avc_changed_mask[1]) {
						dbprintf(LOG_DEBUG, "avc check for classid=%u, meid=%u, changed mask=%02x%02x\n", 
							me->classid, me->meid, 
							me->avc_changed_mask[0], me->avc_changed_mask[1]);
	
						avctask_me_collect_value(me, avc_notify_mask, content);
						// send avc msg if avc notify mask set
						if (avc_notify_mask[0] || avc_notify_mask[1]) {
							omcimsg_avc_generator(me->classid, me->meid, avc_notify_mask, content);
							dbprintf(LOG_INFO, "avcmsg sent for classid=%u, meid=%u\n", me->classid, me->meid);
							avc_sent++;
						}
					} else {	// no more acv changed bit
						break;
					}
				}
			}
						
			if (avc_instance_num)
				break;
		}
		if (avc_classid)
			break;
	}
	return avc_sent;
}
			
static int 
avctask_timer_handler_interval_refersh(struct fwk_timer_msg_t *timer_msg)
{
	dbprintf(LOG_DEBUG, "timer id=%d\n", timer_msg->timer_id);

	//check timer id
	if (timer_msg->timer_id != avctask_interval_timer_id_g)
	{
		dbprintf(LOG_ERR, "timer id error, %d, %d\n", timer_msg->timer_id, avctask_interval_timer_id_g);
		return AVCTASK_ERROR_TIMER_ERROR;
	}

	{ // do avc check only if in O5
		unsigned int state = GPON_ONU_STATE_O5;
		if (gpon_hw_g.onu_state_get)
			gpon_hw_g.onu_state_get(&state);
		if (state == GPON_ONU_STATE_O5 &&
		    !omcimsg_mib_upload_is_in_progress() &&
		    !omcimsg_get_all_alarms_is_in_progress())
			avctask_me_check(0, 0);	// check avc for all classid, all instance num
	}
	
	//create next timer
	if ((avctask_interval_timer_id_g =
	     fwk_timer_create(avctask_timer_qid_g, AVCTASK_INTERVAL_REFRESH_TIMEOUT, omci_env_g.avc_refresh_interval * 1000, NULL)) < 0) {
		dbprintf(LOG_ERR, "timer create error=%d\n", avctask_interval_timer_id_g);
		return AVCTASK_ERROR_TIMER_ERROR;
	}

	return AVCTASK_RW_OK;
}

static void *
avctask_func(void *ptr)
{
	int ret;
	
	struct list_head *entry;
	struct avctask_msg_t *avctask_msg = NULL;
	struct fwk_timer_msg_t *avctask_timer_msg = NULL;

	util_set_thread_name("omci-avc");
	omci_env_g.taskname[ENV_TASK_NO_AVC] = "AVC";
	omci_env_g.taskid[ENV_TASK_NO_AVC] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_AVC] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_AVC] = 0;
	omci_env_g.taskts[ENV_TASK_NO_AVC] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_AVC] = 0; //idle

	/*create the first timer*/
	if ((avctask_interval_timer_id_g =
	     fwk_timer_create(avctask_timer_qid_g, AVCTASK_INTERVAL_REFRESH_TIMEOUT, omci_env_g.avc_refresh_interval * 1000, NULL)) < 0) {
		dbprintf(LOG_ERR, "timer create error=%d\n", avctask_interval_timer_id_g);
	}
	
	while (avctask_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_AVC]++;
		omci_env_g.taskts[ENV_TASK_NO_AVC] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_AVC] = 0; //idle

		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_AVC)) {
			msleep(100);
			continue;
		}

		if ((ret = fwk_msgq_select(avctask_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) < 0) {
			dbprintf(LOG_ERR, "select error(%d)\n", ret);
			continue;
		}

		omci_env_g.taskstate[ENV_TASK_NO_AVC] = 1; //running
		
		if (fwk_msgq_set_isset(avctask_qsetid_g, avctask_msg_qid_g)) {
			if ((ret = fwk_msgq_recv(avctask_msg_qid_g, &entry)) < 0) {
				dbprintf(LOG_ERR, "recv error(%d)\n", ret);
				continue;
			}

			/* process data */
			avctask_msg = list_entry(entry, struct avctask_msg_t, q_entry);
			
			switch (avctask_msg->cmd) {
			case AVCTASK_CMD_CHECKVALUE:
				dbprintf(LOG_DEBUG, "check classid=%u, inst=%u\n",
					avctask_msg->classid, avctask_msg->instance_num);
				avctask_me_check(avctask_msg->classid, avctask_msg->instance_num);			
				break;
			case AVCTASK_CMD_TERMINATE:
				dbprintf(LOG_ERR, "task terminate\n");
				break;
			default:
				dbprintf(LOG_ERR, "get wrong data\n");
			}
			
			free_safe(avctask_msg);
			avctask_msg = NULL;
		}
		
		if (fwk_msgq_set_isset(avctask_qsetid_g, avctask_timer_qid_g)) {
			if ((ret = fwk_msgq_recv(avctask_timer_qid_g, &entry)) < 0) {
				dbprintf(LOG_ERR, "timer q: recv error(%d)\n", ret);
				continue;
			}

			/*process timer msg */
			avctask_timer_msg = list_entry(entry, struct fwk_timer_msg_t, node);
						
			switch (avctask_timer_msg->event) {
			case AVCTASK_INTERVAL_REFRESH_TIMEOUT:
				dbprintf(LOG_DEBUG, "timer q: event=%d\n", avctask_timer_msg->event);
				avctask_timer_handler_interval_refersh(avctask_timer_msg);
				break;
			default:
				dbprintf(LOG_ERR, "timer q: unknown event=%d\n", avctask_timer_msg->event);
			}

			free_safe(avctask_timer_msg);
			avctask_timer_msg = NULL;
		}
	} 

	omci_env_g.taskstate[ENV_TASK_NO_AVC] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_AVC] = 0;
	return 0;
}

int
avctask_init(void)
{
	int ret;
	
	/*allocate queue and qset */
	if ((avctask_msg_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_AVC_CMD")) < 0) {
		dbprintf(LOG_ERR, "queue alloc error(%d)\n", avctask_msg_qid_g);
		return -1;
	}
	if ((avctask_timer_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_AVC_TIMER")) < 0) {
		fwk_msgq_release(avctask_msg_qid_g);
		dbprintf(LOG_ERR, "timer queue alloc error(%d)\n", avctask_timer_qid_g);
		return -1;
	}
	if ((avctask_qsetid_g = fwk_msgq_set_alloc()) < 0) {
		fwk_msgq_release(avctask_msg_qid_g);
		fwk_msgq_release(avctask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set alloc error(%d)\n", avctask_qsetid_g);
		return -1;
	}

	/* set msgq set */
	if ((ret = fwk_msgq_set_zero(avctask_qsetid_g)) < 0) {
		fwk_msgq_set_release(avctask_qsetid_g);
		fwk_msgq_release(avctask_msg_qid_g);
		fwk_msgq_release(avctask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set zero error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set(avctask_qsetid_g, avctask_msg_qid_g)) < 0) {
		fwk_msgq_set_release(avctask_qsetid_g);
		fwk_msgq_release(avctask_msg_qid_g);
		fwk_msgq_release(avctask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set(avctask_qsetid_g, avctask_timer_qid_g)) < 0) {
		fwk_msgq_set_release(avctask_qsetid_g);
		fwk_msgq_release(avctask_msg_qid_g);
		fwk_msgq_release(avctask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}

	avctask_interval_timer_id_g = -1;
	
	return 0;
}

int
avctask_start(void)
{
	int ret;

	/*create pm task */
	avctask_thread_g.policy = FWK_THREAD_POLICY_OMCI_NOTIFY;
	avctask_thread_g.priority = FWK_THREAD_PRIO_OMCI_NOTIFY;
	avctask_thread_g.stack_size = 128*1024;
	avctask_thread_g.join = 1;
	sprintf(avctask_thread_g.name, "omci-AVC");
	avctask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&avctask_thread_g, avctask_func, NULL)) < 0) {
		dbprintf(LOG_ERR, "thread create error(%d)\n", ret);
		return -1;
	}
	return 0;
}

int
avctask_stop(void)
{
	int ret;
	struct avctask_msg_t *avctask_msg;
	avctask_loop_g = 0;	//stop loop

	avctask_msg = malloc_safe(sizeof (struct avctask_msg_t));

	INIT_LIST_NODE(&avctask_msg->q_entry);
	avctask_msg->cmd = AVCTASK_CMD_TERMINATE;
	if (fwk_msgq_send(avctask_msg_qid_g, &avctask_msg->q_entry) < 0)
	{
		free_safe(avctask_msg);
	}
	
	if ((ret = fwk_thread_join(&avctask_thread_g)) < 0) {
		dbprintf(LOG_ERR, "join error(%d)\n", ret);
	}

	return 0;
}

int
avctask_shutdown(void)
{
	fwk_msgq_set_release(avctask_qsetid_g);
	fwk_msgq_release(avctask_msg_qid_g);
	fwk_msgq_release(avctask_timer_qid_g);

	avctask_msg_qid_g = -1;
	avctask_timer_qid_g = -1;
	avctask_qsetid_g = -1;
	avctask_interval_timer_id_g = -1;
	
	return 0;
}

