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
 * File    : omcimsg_alarm.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "util.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"

//todo: maybe need move to other file?
static unsigned char alarm_sequence_num;

unsigned char
omcimsg_alarm_get_sequence_num(void)
{
	return alarm_sequence_num;
}

void
omcimsg_alarm_set_sequence_num(unsigned char val)
{
	alarm_sequence_num=val;
}

int
omcimsg_alarm_generator(unsigned short classid, unsigned short meid, unsigned char *alarm_mask)
{
	struct omcimsg_layout_t *omcimsg;
	struct omcimsg_alarm_t *omcimsg_alarm;

	if (alarm_sequence_num == 255)
		alarm_sequence_num = 1;
	else
		alarm_sequence_num++;

	omcimsg = malloc_safe(sizeof (struct omcimsg_layout_t));
	omcimsg_header_set_transaction_id(omcimsg, 0);	//all active message

	omcimsg_header_set_flag_db(omcimsg, 0);
	omcimsg_header_set_flag_ar(omcimsg, 0);
	omcimsg_header_set_flag_ak(omcimsg, 0);
	omcimsg_header_set_type(omcimsg, OMCIMSG_ALARM);

	omcimsg_header_set_device_id(omcimsg, 0xa);

	omcimsg_header_set_entity_class_id(omcimsg, classid);
	omcimsg_header_set_entity_instance_id(omcimsg, meid);

	omcimsg_alarm = (struct omcimsg_alarm_t *) omcimsg->msg_contents;
	memcpy(omcimsg_alarm->alarm_mask, alarm_mask, 28);	//???

	omcimsg_alarm->alarm_sequence_num = alarm_sequence_num;
	//sendto network
	return omcimsg_handler_coretask_tranx_send_omci_msg(omcimsg, 0);
}
