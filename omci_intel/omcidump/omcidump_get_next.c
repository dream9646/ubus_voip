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
 * File    : omcidump_get_next.c
 *
 ******************************************************************/

#include "omcidump.h"
#include "util.h"

int omcidump_get_next(int fd, struct omcimsg_layout_t *msg)
{
	int i;
	struct meinfo_t *miptr = meinfo_util_miptr(omcimsg_header_get_entity_class_id(msg));
	struct omcimsg_get_next_t *get_next_contents = (struct omcimsg_get_next_t *) msg->msg_contents;
	
	omcidump_header(fd, msg);

	//dump msg content
	if (miptr)
	{
		//get attr order, and check for table
		for (i = 1; i <= meinfo_util_get_attr_total(omcimsg_header_get_entity_class_id(msg)); i++) {
			if (util_attr_mask_get_bit(get_next_contents->attr_mask, i))
				util_fdprintf(fd, "attr %2u (%s)\n", i, miptr->attrinfo[i].name);
		}
	}

	util_fdprintf(fd, "sequence num: 0x%x(%u)\n", ntohs(get_next_contents->cmd_sequence_num), ntohs(get_next_contents->cmd_sequence_num));

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_get_next_response(int fd, struct omcimsg_layout_t *msg)
{
	int i;
	struct meinfo_t *miptr = meinfo_util_miptr(omcimsg_header_get_entity_class_id(msg));
	struct omcimsg_get_next_response_t *get_next_response_contents = (struct omcimsg_get_next_response_t *) msg->msg_contents;
	
	omcidump_header(fd, msg);

	util_fdprintf(fd, "result: %d, %s\n", get_next_response_contents->result_reason,
		omcidump_reason_str(get_next_response_contents->result_reason));
	
	//dump msg content
	if (miptr)
	{
		//get attr order, and check for table
		for (i = 1; i <= meinfo_util_get_attr_total(omcimsg_header_get_entity_class_id(msg)); i++) {
			if (util_attr_mask_get_bit(get_next_response_contents->attr_mask, i))
			{
				util_fdprintf(fd, "attr %2u (%s): ...\n", i, miptr->attrinfo[i].name);
			}
		}
	}
	
	util_fdprintf(fd, "\n");

	return 0;
}

