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
 * File    : omcidump_delete.c
 *
 ******************************************************************/

#include "omcidump.h"
#include "util.h"

int omcidump_delete(int fd, struct omcimsg_layout_t *msg)
{
	omcidump_header(fd, msg);

	//dump msg content
	util_fdprintf(fd, "delete request has no content.\n");


	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_delete_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_delete_response_t *delete_response_contents = (struct omcimsg_delete_response_t *)msg->msg_contents;

	omcidump_header(fd, msg);

	//dump msg content
	util_fdprintf(fd, "result: %d, %s\n", delete_response_contents->result_reason,
		omcidump_reason_str(delete_response_contents->result_reason));

	util_fdprintf(fd, "\n");

	return 0;
}

