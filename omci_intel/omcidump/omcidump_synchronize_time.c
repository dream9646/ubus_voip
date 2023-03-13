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
 * Module  : omcidump
 * File    : omcidump_synchronize_time.c
 *
 ******************************************************************/

#include "omcidump.h"
#include "util.h"

int omcidump_synchronize_time(int fd, struct omcimsg_layout_t *msg)
{
	omcidump_header(fd, msg);

	//dump msg content
	util_fdprintf(fd, "synchronize time request has no content.\n");

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_synchronize_time_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_synchronize_time_response_t *synchronize_time_response_contents = (struct omcimsg_synchronize_time_response_t *)msg->msg_contents;

	omcidump_header(fd, msg);

	//dump msg content
	util_fdprintf(fd, "result: %d, %s\n", synchronize_time_response_contents->result_reason,
		omcidump_reason_str(synchronize_time_response_contents->result_reason));

	util_fdprintf(fd, "\n");

	return 0;
}

