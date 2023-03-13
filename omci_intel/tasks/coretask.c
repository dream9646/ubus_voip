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
 * File    : coretask.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "notify_chain.h"
#include "coretask.h"
#include "util.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "tranxmod.h"
#include "meinfo.h"
#include "pmtask.h"
#include "cli.h"
#include "restore.h"
#include "omcidump.h"
#include "batchtab.h"
#include "batchtab_cb.h"
#include "switch.h"

#ifdef OMCI_BM_ENABLE
#include "omci_bm.h"
#endif

#ifndef X86
#include "er_group_hw.h"
#endif

int coretask_hi_qid_g = -1;
int coretask_lo_qid_g = -1;
int coretask_timer_qid_g = -1;

static int coretask_qsetid_g = -1;
static int coretask_loop_g = 0;
static struct fwk_thread_t coretask_thread_g;
static void *coretask_func(void *ptr);
static int coretask_erdiag_timerid = 0;
static int coretask_batchtab_timerid = 0;
static int coretask_switch_history_timerid = 0;
static int coretask_onesecond_timerid = 0;
#ifdef	OMCI_AUTHORIZATION_PROTECT_ENABLE
static int coretask_oneminute_timerid = 0;
#endif
static unsigned int coretask_onu_state_prev = 0;
static unsigned int coretask_o5_sequence = 0;

#ifdef	OMCI_AUTHORIZATION_PROTECT_ENABLE
int omci_time_sync_state=0;				// Time sync from OLT, time may be from me 131 or other private me
int omci_protect_permit_state=0;

//#define OMCI_AUTHORIZATION_PROTECT_DEBUG 		1
#define OMCI_AUTHORIZATION_PROTECT_DUE_DAY 		"2021/12/01"
#define OMCI_AUTHORIZATION_PROTECT_EXPIRED_COUNT	10  // default: 10*60 seconds (10 minutes)

void
coretask_omci_set_time_sync_state(int state, struct timeval *tv)
{
	if (state) {
		omci_time_sync_state = 1;
	} else {
		omci_time_sync_state = 0;
	}
	
	dbprintf(LOG_ERR, "OMCI time sync(%d), T: %s\n",omci_time_sync_state, ctime(&tv->tv_sec));
}

static int
coretask_omci_authorization_permited(void)
{
	int ret=0;
	time_t cur_time=0, s_time=0, e_time=0;
	struct tm s_tm, e_tm;
	char s_time_str[64]={0},*e_time_str=OMCI_AUTHORIZATION_PROTECT_DUE_DAY;

	if (*e_time_str == '\0')
		return 1;

	snprintf(s_time_str, 64, "%s", __DATE__); // Get build date

	cur_time = time(NULL);

	memset(&s_tm, 0, sizeof(struct tm));
	memset(&e_tm, 0, sizeof(struct tm));

	strptime(s_time_str, "%B %d %Y", &s_tm);
	s_time = mktime(&s_tm);

	strptime(e_time_str, "%Y/%m/%d", &e_tm);
	e_time = mktime(&e_tm);

#ifdef OMCI_AUTHORIZATION_PROTECT_DEBUG
{
	struct tm *ptm0=NULL, 
	ptm0 = gmtime(&cur_time);
	dbprintf(LOG_ERR, "System current time: %d-%-d-%d %d:%d:%d, %ld\n", ptm0->tm_year+1900,
		ptm0->tm_mon+1, ptm0->tm_mday, ptm0->tm_hour, ptm0->tm_min, ptm0->tm_sec, cur_time);

	dbprintf(LOG_ERR, "Start time: %s, %d-%-d-%d %d:%d:%d, %ld\n", s_time_str, s_tm.tm_year+1900,
		s_tm.tm_mon+1, s_tm.tm_mday, s_tm.tm_hour, s_tm.tm_min, s_tm.tm_sec, s_time);

	dbprintf(LOG_ERR, "End time: %s, %d-%-d-%d %d:%d:%d, %ld\n", e_time_str, e_tm.tm_year+1900,
		e_tm.tm_mon+1, e_tm.tm_mday, e_tm.tm_hour, e_tm.tm_min, e_tm.tm_sec, e_time);
}
#endif
	if ((cur_time >= s_time) && (cur_time < e_time)) {
		ret = 1; // Authorization permited
#ifdef OMCI_AUTHORIZATION_PROTECT_DEBUG
		dbprintf(LOG_ERR, "OMCI permited.");
#endif
	}

	return ret;
}

static int
coretask_check_omci_allowed(int expired_count)
{
	int ret=0;
	static int count=0;

	if (expired_count == 0)
		return 0;

	if (omci_time_sync_state) {
		if (coretask_omci_authorization_permited()) { // If time is between the expired date and build date
			omci_protect_permit_state = 1;
			count = 0;
		} else {
			omci_protect_permit_state = 0;
		}
	}

	if (omci_protect_permit_state==0) {
		if (count >= expired_count) {
			coretask_cmd(CORETASK_CMD_MIB_RESET);
			coretask_cmd(CORETASK_CMD_TERMINATE);
			dbprintf(LOG_ERR, "OMCI authorization is not permited. Sync:%d\n",omci_time_sync_state);
		} else {
			count++;
		}
	}

	return ret;
}
#endif

int
coretask_get_o5_sequence(void)
{
	return coretask_o5_sequence;
}

int
coretask_cmd(enum coretask_cmd_t cmd)
{
	struct coretask_msg_t *coretask_msg = malloc_safe(sizeof (struct coretask_msg_t));

	INIT_LIST_NODE(&coretask_msg->q_entry);
	coretask_msg->cmd = cmd;
	if (fwk_msgq_send(coretask_hi_qid_g, &coretask_msg->q_entry) < 0) {
		free_safe(coretask_msg);
		return -1;
	}
	// msg would be free by receiver
	return 0;
}

int
coretask_init(void)
{
	int ret;
	struct omcimsg_handler_coretask_t omcimsg_handler_coretask;

	/*allocate queue and set */
	if ((coretask_hi_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, CORETASK_QUEUE_MAX_LEN, "Q_CORE_OMCI_HI")) < 0) {
		dbprintf(LOG_ERR, "hi queue alloc error(%d)\n", coretask_hi_qid_g);
		return (-1);
	}
	if ((coretask_lo_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, CORETASK_QUEUE_MAX_LEN, "Q_CORE_OMCI_LO")) < 0) {
		fwk_msgq_release(coretask_hi_qid_g);
		dbprintf(LOG_ERR, "lo queue alloc error(%d)\n", coretask_lo_qid_g);
		return (-1);
	}
	if ((coretask_timer_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, CORETASK_QUEUE_MAX_LEN, "Q_CORE_TIMER")) < 0) {
		fwk_msgq_release(coretask_hi_qid_g);
		fwk_msgq_release(coretask_lo_qid_g);
		dbprintf(LOG_ERR, "timer queue alloc error(%d)\n", coretask_timer_qid_g);
		return (-1);
	}
	if ((coretask_qsetid_g = fwk_msgq_set_alloc()) < 0) {
		fwk_msgq_release(coretask_hi_qid_g);
		fwk_msgq_release(coretask_lo_qid_g);
		fwk_msgq_release(coretask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set alloc error(%d)\n", coretask_qsetid_g);
		return (-1);
	}

	/*set msgq set */
	if ((ret = fwk_msgq_set_zero(coretask_qsetid_g)) < 0) {
		fwk_msgq_set_release(coretask_qsetid_g);
		fwk_msgq_release(coretask_hi_qid_g);
		fwk_msgq_release(coretask_lo_qid_g);
		fwk_msgq_release(coretask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set zero error(%d)\n", ret);
		return (-1);
	}
	if ((ret = fwk_msgq_set_set(coretask_qsetid_g, coretask_hi_qid_g)) < 0) {
		fwk_msgq_set_release(coretask_qsetid_g);
		fwk_msgq_release(coretask_hi_qid_g);
		fwk_msgq_release(coretask_lo_qid_g);
		fwk_msgq_release(coretask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return (-1);
	}
	if ((ret = fwk_msgq_set_set(coretask_qsetid_g, coretask_lo_qid_g)) < 0) {
		fwk_msgq_set_release(coretask_qsetid_g);
		fwk_msgq_release(coretask_hi_qid_g);
		fwk_msgq_release(coretask_lo_qid_g);
		fwk_msgq_release(coretask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return (-1);
	}
	if ((ret = fwk_msgq_set_set(coretask_qsetid_g, coretask_timer_qid_g)) < 0) {
		fwk_msgq_set_release(coretask_qsetid_g);
		fwk_msgq_release(coretask_hi_qid_g);
		fwk_msgq_release(coretask_lo_qid_g);
		fwk_msgq_release(coretask_timer_qid_g);
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return (-1);
	}

	tranxmod_init();

	//init the handler for omcimsg
	omcimsg_handler_coretask.tranx_send_omci_msg_cb = tranxmod_send_omci_msg;
	omcimsg_handler_coretask.send_tca_determination_pmtask_cb = coretask_send_tca_determination_pmtask;
	omcimsg_handler_coretask.timer_qid = coretask_timer_qid_g;
	omcimsg_handler_init(&omcimsg_handler_coretask);

	omcimsg_mib_upload_seq_init();

	return 0;
}

int
coretask_start(void)
{
	int ret;

	/*create core task */
	coretask_thread_g.policy = FWK_THREAD_POLICY_OMCI_CORE;
	coretask_thread_g.priority = FWK_THREAD_PRIO_OMCI_CORE;
	coretask_thread_g.stack_size = 128*1024;
	coretask_thread_g.join = 1;
	sprintf(coretask_thread_g.name, "omci-CORE");
	coretask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&coretask_thread_g, coretask_func, NULL)) < 0) {
		dbprintf(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}
	return 0;
}

int
coretask_shutdown(void)
{
	omcimsg_handler_shutdown();
	tranxmod_shutdown();
	fwk_msgq_set_release(coretask_qsetid_g);
	fwk_msgq_release(coretask_hi_qid_g);
	fwk_msgq_release(coretask_lo_qid_g);
	fwk_msgq_release(coretask_timer_qid_g);
	coretask_hi_qid_g = -1;
	coretask_lo_qid_g = -1;
	coretask_timer_qid_g = -1;
	coretask_qsetid_g = -1;
	return 0;
}

int
coretask_stop(void)
{
	int ret;
	coretask_loop_g = 0;	//stop loop
	coretask_cmd(CORETASK_CMD_TERMINATE);

	if ((ret = fwk_thread_join(&coretask_thread_g)) < 0) {
		dbprintf(LOG_ERR, "join error(%d)\n", ret);
	}
	return 0;
}

int
coretask_send_tca_determination_pmtask(unsigned short classid, unsigned short meid)
{
	int ret;
	struct pmtask_msg_t *pmtask_msg;

	pmtask_msg = malloc_safe(sizeof(struct pmtask_msg_t));
	INIT_LIST_NODE(&pmtask_msg->q_entry);

	pmtask_msg->cmd = PMTASK_CMD_TCA_DETERMINATION;
	pmtask_msg->classid = classid;
	pmtask_msg->meid = meid;

	if ((ret = fwk_msgq_send(pmtask_qid_g, &pmtask_msg->q_entry)) < 0)
	{
		dbprintf(LOG_ERR, "send tca determination error, ret=%d\n", ret);
		free_safe(pmtask_msg);
		return ret;
	}

	return 0;
}

static int
coretask_send_sync_time_pmtask(struct omcimsg_layout_t *msg)
{
	int ret;
	struct pmtask_msg_t *pmtask_msg;

	pmtask_msg = malloc_safe(sizeof(struct pmtask_msg_t));
	INIT_LIST_NODE(&pmtask_msg->q_entry);

	pmtask_msg->cmd = PMTASK_CMD_SYNC_TIME;

	if ((ret = fwk_msgq_send(pmtask_qid_g, &pmtask_msg->q_entry)) < 0)
	{
		dbprintf(LOG_ERR, "send sync time error, ret=%d\n", ret);
		free_safe(pmtask_msg);
		return ret;
	}

	return 0;
}

int
coretask_msg_dispatcher(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned int msgtype=omcimsg_header_get_type(req_msg);

	if (req_msg == NULL || resp_msg == NULL)
		return -1;

	dbprintf(LOG_INFO, "type(%d)\n", msgtype);

	//switch for request msg type
	switch (msgtype) {
	case OMCIMSG_CREATE:
		omcimsg_create_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_DELETE:
		omcimsg_delete_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_SET:
	case OMCIMSG_SET_TABLE:
		omcimsg_set_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_GET:
		omcimsg_get_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_GET_ALL_ALARMS:
		omcimsg_get_all_alarms_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_GET_ALL_ALARMS_NEXT:
		omcimsg_get_all_alarms_next_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_MIB_UPLOAD:
		omcimsg_mib_upload_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_MIB_UPLOAD_NEXT:
		omcimsg_mib_upload_next_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_MIB_RESET:
		omcimsg_mib_reset_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_SYNCHRONIZE_TIME:
		coretask_send_sync_time_pmtask(req_msg);
		omcimsg_synchronize_time_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_GET_NEXT:
		omcimsg_get_next_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_GET_CURRENT_DATA:
		omcimsg_get_current_data_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_REBOOT:
		omcimsg_reboot_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_TEST:
		omcimsg_test_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_START_SOFTWARE_DOWNLOAD:
		omcimsg_sw_download_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_DOWNLOAD_SECTION:
		omcimsg_download_section_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_END_SOFTWARE_DOWNLOAD:
		omcimsg_end_sw_download_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_ACTIVATE_SOFTWARE:
		omcimsg_activate_image_handler(req_msg, resp_msg);
		break;
	case OMCIMSG_COMMIT_SOFTWARE:
		omcimsg_commit_image_handler(req_msg, resp_msg);
		break;
	default:
		dbprintf(LOG_ERR, "get unknown action type=%d\n", msgtype);

		omcimsg_util_fill_preliminary_error_resp(req_msg, resp_msg, OMCIMSG_RESULT_CMD_NOT_SUPPORTED);
	}

	//increase mib data sync
	if (omcimsg_util_msgtype_is_inc_mibdatasync(msgtype) &&
		//all those actions that increase mib data sync have result in contents[0]
		resp_msg->msg_contents[0] == OMCIMSG_RESULT_CMD_SUCCESS) {
		// For ME256 attr.12 or ME65530 attr.4, do not increase mib_data_sync counter
		if((msgtype == OMCIMSG_SET) && (htons(req_msg->entity_class_id) == 256) && (req_msg->msg_contents[1] & 0x10))
			; // Do not increase data sync
		else if((msgtype == OMCIMSG_SET) && (htons(req_msg->entity_class_id) == 65530) && (req_msg->msg_contents[0] & 0x10))
			; // Do not increase data sync
		else
			meinfo_util_increase_datasync();
	}

	// reschedule erdiag generation in several seconds later
	if (omci_env_g.erdiag_mon_enable) {
		if (omcimsg_util_msgtype_is_inc_mibdatasync(msgtype) || msgtype == OMCIMSG_MIB_RESET) {
			if (coretask_erdiag_timerid)
				fwk_timer_delete(coretask_erdiag_timerid);
			coretask_erdiag_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_ERDIAG, CORETASK_ERDIAG_MON_INTERVAL * 1000, NULL);
		} else {
			if (coretask_erdiag_timerid) {
				fwk_timer_delete(coretask_erdiag_timerid);
				coretask_erdiag_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_ERDIAG, CORETASK_ERDIAG_MON_INTERVAL * 1000, NULL);
			}
		}
	}

	// schedule a timer for possible batchtab hw sync
	if (omcimsg_util_msgtype_is_inc_mibdatasync(msgtype) || msgtype == OMCIMSG_MIB_RESET) {
		if (coretask_batchtab_timerid)
			fwk_timer_delete(coretask_batchtab_timerid);
		coretask_batchtab_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_BATCHTAB, CORETASK_BATCHTAB_CHECK_INTERVAL * 1000, NULL);
	}
	return 0;
}

static void * 
func_erdiag(void *data)
{
	int console_fd=util_dbprintf_get_console_fd();

	util_set_thread_name("func_erdiag");
	nice(19);
	util_fdprintf(console_fd, "\n");
	cli_bridge(console_fd, 0xffff);
	//cli_tcont(console_fd, 0xffff, 1);
	cli_pots(console_fd, 0xffff);
	util_fdprintf(console_fd, "\n");
	cli_unrelated(console_fd);
	return 0;
}

static int
func_omci_rxq_has_data(void)
{
	if (fwk_msgq_len(coretask_hi_qid_g) >0)
		return 1;
	if (fwk_msgq_len(coretask_lo_qid_g) >0)
		return 1;
	return 0;
}

static int
coretask_onesecond_routine(void)
{
	unsigned int onu_state;

	gpon_hw_g.onu_state_get(&onu_state);
	if (coretask_onu_state_prev != onu_state) {
		if (onu_state == GPON_ONU_STATE_O5) {
			coretask_o5_sequence++;
			// mark o5 change as external event for batchtab
			batchtab_extevent_update();
			notify_chain_notify(NOTIFY_CHAIN_EVENT_O5_SEQUENCE, 0, 0, 0);	// this event is not for specific me but all
		}
		coretask_onu_state_prev = onu_state;
	}

	switch_update_linkup_logical_portmask();
	dbprintf(LOG_INFO, "linkup_portmask=0x%x\n", switch_get_linkup_logical_portmask());
#ifndef X86
	switch_loop_detection();
#endif
#ifdef OMCI_BM_ENABLE
	omci_bm_check(3600);	// check per hour
#endif	
	batchtab_cb_linkready_check_debounce();
	return 0;
}

static int
coretask_timer_dispatcher(struct fwk_timer_msg_t *timer_msg)
{
	switch (timer_msg->event) {
	case OMCIMSG_MIBUPLOAD_TIMEOUT:
		omcimsg_mib_upload_timer_handler(timer_msg);
		break;
	case OMCIMSG_GETALLALARMS_TIMEOUT:
		omcimsg_get_all_alarms_next_timeout();
		break;
	case OMCIMSG_GETTABLE_TIMEOUT:
		omcimsg_get_table_timer_handler(timer_msg);
		break;
	case CORETASK_TIMER_EVENT_GCLIST:
		meinfo_util_gclist_aging(CORETASK_GCLIST_AGING_INTERVAL);
		fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_GCLIST, CORETASK_GCLIST_CHECK_INTERVAL * 1000, NULL);
		break;
	case CORETASK_TIMER_EVENT_ERDIAG:
		if (coretask_erdiag_timerid!=0 && timer_msg->timer_id==coretask_erdiag_timerid) {
			coretask_erdiag_timerid = 0;
			fwk_thread_run_function(func_erdiag, NULL, FWK_THREAD_RUN_FUNC_ASYNC, FWK_THREAD_POLICY_OMCI_MISC, FWK_THREAD_PRIO_OMCI_MISC);
		} else {
			dbprintf(LOG_CRIT, "msg->timerid=0x%x, coretask_erdiag_timerid=0x%x\n", timer_msg->timer_id, coretask_erdiag_timerid);
		}
		break;
	case CORETASK_TIMER_EVENT_BATCHTAB:
		if (coretask_batchtab_timerid!=0 && timer_msg->timer_id==coretask_batchtab_timerid) {
			if (!func_omci_rxq_has_data())
				batchtab_table_gen_hw_sync_all(func_omci_rxq_has_data);
			coretask_batchtab_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_BATCHTAB, CORETASK_BATCHTAB_CHECK_INTERVAL * 1000, NULL);
		} else {
			dbprintf(LOG_CRIT, "msg->timerid=0x%x, coretask_batchtab_timerid=0x%x\n", timer_msg->timer_id, coretask_batchtab_timerid);
		}
		break;
	case CORETASK_TIMER_EVENT_SWITCH_HISTORY:
		if (coretask_switch_history_timerid!=0 && timer_msg->timer_id==coretask_switch_history_timerid) {
			switch_hw_g.history_add();
			gpon_hw_g.history_add();
			coretask_switch_history_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_SWITCH_HISTORY, omci_env_g.switch_history_interval * 60 * 1000, NULL);
		} else {
			dbprintf(LOG_CRIT, "msg->timerid=0x%x, coretask_switch_history_timerid=0x%x\n", timer_msg->timer_id, coretask_switch_history_timerid);
		}
		break;
	case CORETASK_TIMER_EVENT_ONESECOND:
		if (coretask_onesecond_timerid!=0 && timer_msg->timer_id==coretask_onesecond_timerid) {
			coretask_onesecond_routine();
			coretask_onesecond_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_ONESECOND, 1000, NULL);
		} else {
			dbprintf(LOG_CRIT, "msg->timerid=0x%x, coretask_onesecond_timerid=0x%x\n", timer_msg->timer_id, coretask_onesecond_timerid);
		}
		break;
#ifdef	OMCI_AUTHORIZATION_PROTECT_ENABLE
	case CORETASK_TIMER_EVENT_ONEMINUTE:
		if (coretask_oneminute_timerid!=0 && timer_msg->timer_id==coretask_oneminute_timerid) {
			coretask_check_omci_allowed(OMCI_AUTHORIZATION_PROTECT_EXPIRED_COUNT);
			coretask_oneminute_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_ONEMINUTE, CORETASK_OMCI_PERMIT_CHECK_INTERVAL*60*1000, NULL);
		} else {
			dbprintf(LOG_CRIT, "msg->timerid=0x%x, coretask_oneminute_timerid=0x%x\n", timer_msg->timer_id, coretask_oneminute_timerid);
		}
		break;
#endif
	default:
		dbprintf(LOG_ERR, "get unknown timer event=%d\n", timer_msg->event);
	}
	return 0;
}

static void
coretask_process_omci_msg(unsigned char *data, unsigned char from)
{
	int ret;
	struct omcimsg_layout_t *req_msg;
	struct omcimsg_layout_t *resp_msg;

	// mark omcimsg processing as external event for batchtab
	batchtab_extevent_update();

	if (data != NULL) {
		int console_fd=util_dbprintf_get_console_fd();
		unsigned char msgtype, ar, ak;
		req_msg = (struct omcimsg_layout_t *) data;
		msgtype=omcimsg_header_get_type(req_msg);
		ar=omcimsg_header_get_flag_ar(req_msg);
		ak=omcimsg_header_get_flag_ak(req_msg);

		if (omci_env_g.omcidump_msgmask & (1<<msgtype)) {
			switch (omci_env_g.omcidump_enable) {
			case 1:	omcidump_print_msg(console_fd, req_msg, OMCIDUMP_DIR_RX);
				break;
			case 2:	omcidump_print_raw(console_fd, req_msg, OMCIDUMP_DIR_RX);
				break;
			case 3:	omcidump_print_raw(console_fd, req_msg, OMCIDUMP_DIR_RX);
				omcidump_print_msg(console_fd, req_msg, OMCIDUMP_DIR_RX);
				break;
			case 4:	omcidump_print_line(console_fd, req_msg, OMCIDUMP_DIR_RX, 0);
				break;
			case 8:	omcidump_print_char(console_fd, req_msg, OMCIDUMP_DIR_RX);
				break;
			}
		}

		//only accept request message
		if ((!ar && ak)	|| // ack msg
		    (!ar && !ak && msgtype != OMCIMSG_DOWNLOAD_SECTION) ||
		    (ar && ak))	//unreasonable
		{
			//drop
			free_safe(data);
			dbprintf(LOG_ERR, "unreasonable ar/ak\n");
		} else {
			//allocate response msg
			resp_msg = (struct omcimsg_layout_t *) malloc_safe(sizeof (struct omcimsg_layout_t));

			//send to tranx module
			ret = tranxmod_recv_omci_msg(req_msg, from);

			switch(ret)
			{
			case TRANXMOD_OK:
			case TRANXMOD_BYPASS:
				//add to omcc log
				if (omci_env_g.omccrx_log_enable) {
					if (omci_env_g.omccrx_log_msgmask & (1<<msgtype))
						omcidump_print_raw_to_file(req_msg, omci_env_g.omccrx_log_file, OMCIDUMP_DIR_RX);
				}

				//add to history
				cli_history_add_msg(req_msg);

				if (omcimsg_util_check_err_for_header(req_msg, resp_msg) == 0) {
					if (msgtype<32)
						omcimsg_count_g[msgtype]++;

					// save msg to restore.log
					if (omci_env_g.restore_log_enable)
						restore_append_omcclog(req_msg, omci_env_g.restore_log_file);

					//no error, dispatch msg to handler
					if (coretask_msg_dispatcher(req_msg, resp_msg) < 0)
						dbprintf(LOG_ERR, "dispatcher req msg error, tid=0x%.4x, type=0x%.2x\n", req_msg->transaction_id, req_msg->msg_type);
				}
				if (ret==TRANXMOD_BYPASS) {
					free_safe(data);
					data = NULL;
				}
				break;
			case TRANXMOD_ERROR_BUSY:
				//add to omcc log
				if (omci_env_g.omccrx_log_enable) {
					if (omci_env_g.omccrx_log_msgmask & (1<<msgtype))
						omcidump_print_raw_to_file(req_msg, omci_env_g.omccrx_log_file, OMCIDUMP_DIR_RX);
				}

				//add to history
				cli_history_add_msg(req_msg);

				omcimsg_util_fill_preliminary_error_resp(req_msg, resp_msg, OMCIMSG_RESULT_DEVICE_BUSY);
				free_safe(data); //the same with req_msg
				data = NULL;
				break;
			case TRANXMOD_ERROR_RETRANS_MATCH:
			default:
				free_safe(data);
				data = NULL;
			}

			//send back response, check resp_msg's transaction id and device id for retrans match and noack download section
			if (omcimsg_header_get_transaction_id(resp_msg) != 0 ||
				omcimsg_header_get_device_id(resp_msg) != 0)
			{
				tranxmod_send_omci_msg(resp_msg, from);
			} else {
				//free response msg
				free_safe(resp_msg);
				resp_msg = NULL;
			}
		}
	}
}

static void *
coretask_func(void *ptr)
{
	int ret;
	int hi_queue_flag;
	struct list_head *entry;
	struct coretask_msg_t *coretask_msg = NULL;
	struct fwk_timer_msg_t *coretask_timer_msg = NULL;

	util_set_thread_name("omci-core");
	omci_env_g.taskname[ENV_TASK_NO_CORE] = "CORE";
	omci_env_g.taskid[ENV_TASK_NO_CORE] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_CORE] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_CORE] = 0;
	omci_env_g.taskts[ENV_TASK_NO_CORE] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_CORE] = 0; //idle

	// init gc timer
	fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_GCLIST, CORETASK_GCLIST_CHECK_INTERVAL * 1000, NULL);

	// init batchtab timer, switch_history timer  
	coretask_batchtab_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_BATCHTAB, CORETASK_BATCHTAB_CHECK_INTERVAL * 1000, NULL);
	coretask_switch_history_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_SWITCH_HISTORY, omci_env_g.switch_history_interval * 60 * 1000, NULL);
	coretask_onesecond_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_ONESECOND, 1000, NULL);
#ifdef	OMCI_AUTHORIZATION_PROTECT_ENABLE
	coretask_oneminute_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_ONEMINUTE, CORETASK_OMCI_PERMIT_CHECK_INTERVAL * 60 * 1000, NULL);
#endif

	while (coretask_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_CORE]++;
		omci_env_g.taskts[ENV_TASK_NO_CORE] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_CORE] = 0; //idle

		ret = fwk_msgq_select(coretask_qsetid_g, FWK_MSGQ_WAIT_INFINITE);
		if (ret < 0) {
			dbprintf(LOG_ERR, "select error(%d)\n", ret);
			continue;
		}

		omci_env_g.taskstate[ENV_TASK_NO_CORE] = 1; //running

		// ret >= 0
		if (fwk_msgq_set_isset(coretask_qsetid_g, coretask_hi_qid_g)) {
			hi_queue_flag = 1;
			while (hi_queue_flag && (ret = fwk_msgq_recv(coretask_hi_qid_g, &entry)) == 0) {

				/*process data */
				coretask_msg = list_entry(entry, struct coretask_msg_t, q_entry);
				switch (coretask_msg->cmd) {
				case CORETASK_CMD_PROCESS_MSG:
					{
						if (coretask_msg->omci_data != NULL) {
							coretask_process_omci_msg(coretask_msg->omci_data, coretask_msg->from);
							coretask_msg->omci_data = NULL;	//hang over to transaction db
						} else {
							dbprintf(LOG_ERR, "coretask_msg->omci_data NULL?\n");
						}
						break;
					}
				case CORETASK_CMD_TERMINATE:
					hi_queue_flag = 0;
					dbprintf(LOG_ERR, "task terminate\n");
					break;
				case CORETASK_CMD_MIB_RESET:
					{
					struct timeval tv0, tv1;
					int ms_diff, ret1, ret2;
					util_get_uptime(&tv0);
					ret1=omcimsg_mib_reset_clear();
					ret2=omcimsg_mib_reset_init();
					util_get_uptime(&tv1);
					ms_diff=util_timeval_diff_msec(&tv1, &tv0);
					dbprintf(LOG_ERR, "mib_reset %s (%dms)\n", (ret1==0 && ret2==0)?"ok":"fail", ms_diff);
					}
					break;
				case CORETASK_CMD_SWITCH_HISTORY_INTERVAL_CHANGE:
					fwk_timer_delete(coretask_switch_history_timerid);
					coretask_switch_history_timerid=fwk_timer_create(coretask_timer_qid_g, CORETASK_TIMER_EVENT_SWITCH_HISTORY, omci_env_g.switch_history_interval * 60 * 1000, NULL);
					dbprintf(LOG_INFO, "change switch history interval to %d minute\n", omci_env_g.switch_history_interval);
					break;
				default:
					dbprintf(LOG_ERR, "high queue get wrong data\n");
				}
				if (coretask_msg->omci_data != NULL) {
					free_safe(coretask_msg->omci_data);
					coretask_msg->omci_data = NULL;
				}
				free_safe(coretask_msg);
				coretask_msg = NULL;
			}
		}
		if (fwk_msgq_set_isset(coretask_qsetid_g, coretask_lo_qid_g)) {
			if ((ret = fwk_msgq_recv(coretask_lo_qid_g, &entry)) < 0) {
				dbprintf(LOG_ERR, "recv lo error(%d)\n", ret);
				continue;
			}

			/*process data */
			coretask_msg = list_entry(entry, struct coretask_msg_t, q_entry);
			switch (coretask_msg->cmd) {
			case CORETASK_CMD_PROCESS_MSG:
				{
					if (coretask_msg->omci_data != NULL) {
						coretask_process_omci_msg(coretask_msg->omci_data, coretask_msg->from);
						coretask_msg->omci_data = NULL;	//hang over to transaction db
					}
					break;
				}
			default:
				dbprintf(LOG_ERR, "low queue get wrong data\n");
			}
			if (coretask_msg->omci_data != NULL) {
				free_safe(coretask_msg->omci_data);
				coretask_msg->omci_data = NULL;
			}
			free_safe(coretask_msg);
			coretask_msg = NULL;
		}
		if (fwk_msgq_set_isset(coretask_qsetid_g, coretask_timer_qid_g)) {
			if ((ret = fwk_msgq_recv(coretask_timer_qid_g, &entry)) < 0) {
				dbprintf(LOG_ERR, "recv timer error(%d)\n", ret);
				continue;
			}

			/*process timer msg */
			coretask_timer_msg = list_entry(entry, struct fwk_timer_msg_t, node);
			coretask_timer_dispatcher(coretask_timer_msg);
			free_safe(coretask_timer_msg);
			coretask_timer_msg = NULL;
		}
	}

	omci_env_g.taskstate[ENV_TASK_NO_CORE] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_CORE] = 0;

	return 0;
}

