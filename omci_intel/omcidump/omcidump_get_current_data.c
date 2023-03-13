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
 * File    : omcidump_get_current_data.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "omcidump.h"
#include "omcimsg.h"
#include "util.h"

int omcidump_get_current_data(int fd, struct omcimsg_layout_t *msg)
{
	struct meinfo_t *miptr = meinfo_util_miptr(omcimsg_header_get_entity_class_id(msg));
	struct omcimsg_get_current_data_t *get_current_data_contents;

	omcidump_header(fd, msg);

	//dump msg content
	if (miptr)
	{
		int i;
		get_current_data_contents = (struct omcimsg_get_current_data_t *) msg->msg_contents;

		//get attr order, and check for table
		for (i = 1; i <= meinfo_util_get_attr_total(omcimsg_header_get_entity_class_id(msg)); i++) {
			if (util_attr_mask_get_bit(get_current_data_contents->attr_mask, i))
			{
				util_fdprintf(fd, "attr %2u (%s)\n", i, miptr->attrinfo[i].name);
			}
		}
	}

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_get_current_data_response(int fd, struct omcimsg_layout_t *msg)
{
	int i;
	struct meinfo_t *miptr = meinfo_util_miptr(omcimsg_header_get_entity_class_id(msg));
	struct attr_value_t attr;
	struct omcimsg_get_current_data_response_t *get_current_data_response_contents;
	char ostr[128];

	omcidump_header(fd, msg);

	get_current_data_response_contents = (struct omcimsg_get_current_data_response_t *) msg->msg_contents;

	util_fdprintf(fd, "result: %d, %s\n", get_current_data_response_contents->result_reason,
		omcidump_reason_str(get_current_data_response_contents->result_reason));
	util_fdprintf(fd, "optional mask: 0x%.2x%.2x\n", get_current_data_response_contents->optional_attr_mask[0], get_current_data_response_contents->optional_attr_mask[1]);
	util_fdprintf(fd, "execution mask: 0x%.2x%.2x\n", get_current_data_response_contents->attr_exec_mask[0], get_current_data_response_contents->attr_exec_mask[1]);

	//dump msg content
	if (miptr)
	{
		attr.data=0;
		attr.ptr = malloc_safe(128);

		//get attr order, and check for table
		for (i = 1; i <= meinfo_util_get_attr_total(omcimsg_header_get_entity_class_id(msg)); i++) {
			if (util_attr_mask_get_bit(get_current_data_response_contents->attr_mask, i))
			{
				omcimsg_util_get_attr_by_mask(get_current_data_response_contents->attr_value,
					25,
					omcimsg_header_get_entity_class_id(msg),
					i,
					get_current_data_response_contents->attr_mask,
					&attr);
				meinfo_conv_attr_to_string(omcimsg_header_get_entity_class_id(msg), i, &attr, ostr, 128);
				util_fdprintf(fd, "attr %2u (%s): %s\n", i, miptr->attrinfo[i].name, ostr);
			}
		}

		free_safe(attr.ptr);
	}


	util_fdprintf(fd, "\n");

	return 0;
}
