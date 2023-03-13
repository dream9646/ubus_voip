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
 * File    : omcimsg_avc.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "util.h"

int
omcimsg_avc_generator(unsigned short classid, unsigned short meid, unsigned char *attr_mask, unsigned char *attr_value)
{
	struct omcimsg_layout_t *omcimsg;
	struct omcimsg_attr_value_change_t *omcimsg_avc;

	omcimsg = malloc_safe(sizeof (struct omcimsg_layout_t));
	omcimsg_header_set_transaction_id(omcimsg, 0);	//all active message

	omcimsg_header_set_flag_db(omcimsg, 0);
	omcimsg_header_set_flag_ar(omcimsg, 0);
	omcimsg_header_set_flag_ak(omcimsg, 0);
	omcimsg_header_set_type(omcimsg, OMCIMSG_ATTRIBUTE_VALUE_CHANGE);

	omcimsg_header_set_device_id(omcimsg, 0xa);

	omcimsg_header_set_entity_class_id(omcimsg, classid);
	omcimsg_header_set_entity_instance_id(omcimsg, meid);

	omcimsg_avc = (struct omcimsg_attr_value_change_t *) omcimsg->msg_contents;
	memcpy(omcimsg_avc->attr_mask, attr_mask, 2);
	memcpy(omcimsg_avc->attr_value, attr_value, 30);

	//sendto network
	return omcimsg_handler_coretask_tranx_send_omci_msg(omcimsg, 0);
}

