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
 * Module  : omcimsg
 * File    : omcimsg_mib_reset.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "omciutil_stp.h"
#include "notify_chain.h"
#include "cli.h"
#include "switch.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"

static unsigned char mib_reset_clear_is_running;
static unsigned char mib_reset_init_is_running;
static long mib_reset_finish_timestamp=0;

int omcimsg_mib_reset_init(void)
{
	mib_reset_init_is_running = 1;

	// reset alarm_sequence_num
	omcimsg_alarm_set_sequence_num(0);

	// FIXME, KEVIN, stale code?
	//if (switch_hw_g.intermediate_init)
	//	switch_hw_g.intermediate_init();

	// trigger me creation through boot event
	notify_chain_notify(NOTIFY_CHAIN_EVENT_BOOT, 256, 1, 0);

	// set all port stp state to STP_DISABLE or STP_OFF
	omciutil_stp_set_before_omci();

	mib_reset_init_is_running = 0;

	// unmask pause flag
	omci_env_g.task_pause_mask &= ~(1<<ENV_TASK_NO_ALARM); //alarm
	omci_env_g.task_pause_mask &= ~(1<<ENV_TASK_NO_PM); //pm
	omci_env_g.task_pause_mask &= ~(1<<ENV_TASK_NO_AVC); //avc


	mib_reset_finish_timestamp = util_get_uptime_sec();
	omcimsg_mib_upload_after_reset_set(0);

	return 0;
}

int omcimsg_mib_reset_clear(void)
{
	unsigned short index;

	//stop mib_upload if necessary
	if (omcimsg_mib_upload_is_in_progress())
	{
		dbprintf(LOG_ERR, "mib reset while mib uploading\n");
		omcimsg_mib_upload_stop_immediately();
	}

	//stop progress function
	if (omcimsg_get_all_alarms_is_in_progress()) {
		dbprintf(LOG_ERR, "mib reset while get all alarms\n");
		omcimsg_get_all_alarms_next_stop();
	}

	//dump me before mib reset
	if (meinfo_util_get_data_sync()!=0)
		cli_me_mibreset_dump();

	//mask pause flag
	omci_env_g.task_pause_mask |= (1<<ENV_TASK_NO_ALARM); //alarm
	omci_env_g.task_pause_mask |= (1<<ENV_TASK_NO_PM); //pm
	omci_env_g.task_pause_mask |= (1<<ENV_TASK_NO_AVC); //avc
	//yield so other thread can check task_pause_mask immediately
	fwk_thread_yield();
	
	mib_reset_clear_is_running = 1;

	//release me created by olt
	for (index = 1; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid = meinfo_index2classid(index);
		struct meinfo_t *miptr=meinfo_util_miptr(classid);

		if (miptr && meinfo_me_delete_by_classid_and_cbo(classid, 1) != 0) //created by olt
			return -1;
	}

	//send event shutdown
	notify_chain_notify(NOTIFY_CHAIN_EVENT_SHUTDOWN, 256, 1, 0);

	//release misc item 
	for (index = 1; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid = meinfo_index2classid(index);
		struct meinfo_t *miptr=meinfo_util_miptr(classid);

		if (miptr && meinfo_me_delete_by_classid(classid) != 0)
			return -1;
	}

	mib_reset_clear_is_running = 0;

	return 0;
}

int
omcimsg_mib_reset_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct omcimsg_mib_reset_response_t *msg_mib_reset_response;
	int ret1=0, ret2=0;

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);

	msg_mib_reset_response = (struct omcimsg_mib_reset_response_t *) resp_msg->msg_contents;

	if (omcimsg_header_get_entity_class_id(req_msg) != 2) {	//ONT data
		msg_mib_reset_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		return OMCIMSG_ERROR_PARM_ERROR;
	}

	if (omcimsg_header_get_entity_instance_id(req_msg) != 0) {
		msg_mib_reset_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		return OMCIMSG_ERROR_PARM_ERROR;
	}

	if (omcimsg_header_get_device_id(req_msg) != 0x0a) {
		msg_mib_reset_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		return OMCIMSG_ERROR_PARM_ERROR;
	}

	ret1 = omcimsg_mib_reset_clear();
	ret2 = omcimsg_mib_reset_init(); 
	if ( ret1 < 0 || ret2 < 0) {
		msg_mib_reset_response->result_reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
		return OMCIMSG_ERROR_ATTR_FAILED_UNKNOWN;
	}
	
	msg_mib_reset_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;

	return 0;
}

int
omcimsg_mib_reset_clear_is_in_progress(void)
{
	return mib_reset_clear_is_running;
}

int
omcimsg_mib_reset_is_in_progress(void)
{
	return mib_reset_clear_is_running || mib_reset_init_is_running;
}

int
omcimsg_mib_reset_finish_time_till_now(void)
{
	return util_get_uptime_sec() - mib_reset_finish_timestamp ;
}

int
omcimsg_mib_reset_finish_timestamp(void)
{
	return mib_reset_finish_timestamp ;
}
