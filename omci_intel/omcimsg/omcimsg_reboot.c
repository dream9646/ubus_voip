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
 * File    : omcimsg_reboot.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "notify_chain.h"

static void pre_reboot()
{

	//stop mib_upload if necessary
	if (omcimsg_mib_upload_is_in_progress())
	{
		dbprintf(LOG_ERR, "reboot while mib uploading\n");
		omcimsg_mib_upload_stop_immediately();
	}

	//stop progress function
	if (omcimsg_get_all_alarms_is_in_progress()) {
		dbprintf(LOG_ERR, "reboot while get all alarms\n");
		omcimsg_get_all_alarms_next_stop();
	}

	//todo: maybe need to save mib_data_sync or ...
}

int 
omcimsg_reboot_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid, meid;
	struct omcimsg_reboot_response_t *msg_reboot_response;
	char reboot_script[384];

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);

	msg_reboot_response = (struct omcimsg_reboot_response_t *) resp_msg->msg_contents;

	classid=omcimsg_header_get_entity_class_id(req_msg);
	meid=omcimsg_header_get_entity_instance_id(req_msg);

	if (classid == 256) {		// ONT-G
		// currently we only have reboot for ONT-G meid=0
		if (meid==0) {
        		pid_t pid;
			pre_reboot();
			util_omcipath_filename(reboot_script, 384, "script/omci_reboot.sh");
			if((pid=fork()) < 0) {
				dbprintf(LOG_ERR, "fork error on exec reboot\n");
			} else if (pid == 0) {
				int i;
				sleep(1);
				for (i=0; i <= 1024; i++)
					close(i);
				execl(reboot_script, "omci_reboot.sh", NULL, NULL);
				exit(0);
			}
			msg_reboot_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;
			return OMCIMSG_RW_OK;
		} else {
			dbprintf(LOG_ERR, "reboot ONT-G with meid %u?\n", meid);
			msg_reboot_response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
			return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
		}

	} else if (classid == 6) {	// Circuit pack
		dbprintf(LOG_ERR, "reboot Circuit pack not supported, meid=%u\n", meid);
		msg_reboot_response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	}

	msg_reboot_response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME;
	return OMCIMSG_ERROR_UNKNOWN_ME;
}

