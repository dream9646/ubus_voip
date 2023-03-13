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
 * File    : omcimsg_synchronize_time.c
 *
 ******************************************************************/

#include <stdlib.h>
#include "util.h"
#include "util_run.h"
#include "omcimsg.h"

int
omcimsg_synchronize_time_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid;
	struct omcimsg_synchronize_time_response_t *sync_time_response_contents;

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);

	sync_time_response_contents = (struct omcimsg_synchronize_time_response_t *) resp_msg->msg_contents;
	classid = omcimsg_header_get_entity_class_id(req_msg);

	if (classid != 256)
	{
		sync_time_response_contents->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "sync time handler, err=%d", OMCIMSG_ERROR_PARM_ERROR);
		return OMCIMSG_ERROR_PARM_ERROR;
	}

	sync_time_response_contents->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;

	// set timestamp if "year" is not zero
	if (req_msg->msg_contents[0] != 0 && req_msg->msg_contents[1] != 0)
	{
		unsigned short year = req_msg->msg_contents[0] << 8 | req_msg->msg_contents[1];
		unsigned char month = req_msg->msg_contents[2];
		unsigned char day = req_msg->msg_contents[3];
		unsigned char hour = req_msg->msg_contents[4];
		unsigned char minute = req_msg->msg_contents[5];
		unsigned char second = req_msg->msg_contents[6];
		char cmd[32] = {0};
		sprintf(cmd, "date -s %04d%02d%02d%02d%02d.%02d", year, month, day, hour, minute, second);
		util_run_by_system(cmd);
	}

	return 0;
}




