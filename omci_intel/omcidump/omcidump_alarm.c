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
 * File    : omcidump_alarm.c
 *
 ******************************************************************/

#include "omcidump.h"
#include "util.h"

int omcidump_alarm(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_alarm_t *omcimsg_alarm = (struct omcimsg_alarm_t *) msg->msg_contents;
	int order;
	int is_prompt_printed=0;

	omcidump_header(fd, msg);

	//dump msg content
	for (order = 0; order < 224; order++) {		//from alarm 0-alarm 223
		if (util_alarm_mask_get_bit(omcimsg_alarm->alarm_mask, order)) {
			if (!is_prompt_printed) {
				util_fdprintf(fd, "alam_mask set: %d", order);
				is_prompt_printed=1;
			} else {
				util_fdprintf(fd, ",%d", order);
			}
		}
	}
	if (is_prompt_printed)
		util_fdprintf(fd, "\n");
				
	util_fdprintf(fd, "alarm_sequence_num: %d\n", omcimsg_alarm->alarm_sequence_num);

	util_fdprintf(fd, "\n");

	return 0;
}

