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
 * File    : omcidump_get_all_alarms.c
 *
 ******************************************************************/

#include "omcidump.h"
#include "util.h"

int omcidump_get_all_alarms(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_get_all_alarms_t *alarms_val;
	int arc;

	omcidump_header(fd, msg);

	//dump msg content
	alarms_val = (struct omcimsg_get_all_alarms_t *) msg->msg_contents;
	arc=ntohs(alarms_val->alarm_retrieval_mode);

	if ( arc == 0 )
		util_fdprintf(fd, "alarm retrieval mode=%d, regardless ARC\n", arc);
	else
		util_fdprintf(fd, "alarm retrieval mode=%d, under ARC\n", arc);

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_get_all_alarms_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_get_all_alarms_response_t *response_val;
	omcidump_header(fd, msg);

	//dump msg content
	response_val = (struct omcimsg_get_all_alarms_response_t *) msg->msg_contents;
	util_fdprintf(fd, "subsequent_cmd_total=%d\n", ntohs(response_val->subsequent_cmd_total));

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_get_all_alarms_next(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_get_all_alarms_next_t *alarms_next_val;

        omcidump_header(fd, msg);

        //dump msg content
	alarms_next_val = (struct omcimsg_get_all_alarms_next_t *) msg->msg_contents;
	util_fdprintf(fd, "Command sequence number=%u\n", ntohs(alarms_next_val->sequence_num));

        util_fdprintf(fd, "\n");

        return 0;
}

int omcidump_get_all_alarms_next_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_get_all_alarms_next_response_t *response_val;
	unsigned short classid, meid;
	int order;

        omcidump_header(fd, msg);

	//dump msg content
	response_val= (struct omcimsg_get_all_alarms_next_response_t *) msg->msg_contents;
	classid=ntohs(response_val->entity_class);
	meid=ntohs(response_val->entity_instance);

	util_fdprintf(fd, "Alarm Report Classid=0x%x(%u), Meid=0x%x(%u)\n",classid, classid, meid, meid);

	for (order = 0; order < 224; order++) {
		if (util_alarm_mask_get_bit(response_val->bit_map_alarms, order)) {
			util_fdprintf(fd, "Response Alarm [%d]\n", order);
		}
	}

        util_fdprintf(fd, "\n");

        return 0;
}

