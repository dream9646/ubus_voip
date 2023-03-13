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
 * File    : omcidump_create.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "omcidump.h"
#include "omcimsg.h"
#include "util.h"

int omcidump_create(int fd, struct omcimsg_layout_t *msg)
{
	int i;
	struct meinfo_t *miptr = meinfo_util_miptr(omcimsg_header_get_entity_class_id(msg));
	struct attr_value_t attr;
	struct omcimsg_create_t *create_contents;
	char ostr[1024];

	omcidump_header(fd, msg);

	//dump msg content
	if (miptr)
	{
		attr.data=0;
		attr.ptr = malloc_safe(1024);

		create_contents = (struct omcimsg_create_t *) msg->msg_contents;

		for (i = 1; i <= miptr->attr_total; i++)
		{
			if (meinfo_util_attr_is_sbc(omcimsg_header_get_entity_class_id(msg), i)) {
				omcimsg_util_get_attr_from_create(create_contents->attr_value, omcimsg_header_get_entity_class_id(msg), i, &attr);
				meinfo_conv_attr_to_string(omcimsg_header_get_entity_class_id(msg), i, &attr, ostr, 1024);
				util_fdprintf(fd, "attr %2u (%s): %s\n", i, miptr->attrinfo[i].name, ostr);
			}
		}
		
		free_safe(attr.ptr);
	}

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_create_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_create_response_t *create_response_contents = (struct omcimsg_create_response_t *)msg->msg_contents;

	omcidump_header(fd, msg);

	//dump msg content
	util_fdprintf(fd, "result: %d, %s\n", create_response_contents->result_reason, 
		omcidump_reason_str(create_response_contents->result_reason));
	util_fdprintf(fd, "execution mask: 0x%.2x%.2x\n", 
		create_response_contents->attr_exec_mask[0], create_response_contents->attr_exec_mask[1]);

	util_fdprintf(fd, "\n");

	return 0;
}

