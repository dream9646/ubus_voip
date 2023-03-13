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
 * File    : omcidump_avc.c
 *
 ******************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "omcidump.h"
#include "util.h"

int omcidump_avc(int fd, struct omcimsg_layout_t *msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(msg);
	struct omcimsg_attr_value_change_t *omcimsg_avc;
	unsigned char attr_mask[2], attr_value[30];
	int next_start_byte, attr_order;

	omcidump_header(fd, msg);

	//dump msg content
	omcimsg_avc = (struct omcimsg_attr_value_change_t *) msg->msg_contents;
	memcpy(attr_mask, omcimsg_avc->attr_mask, 2);
	memcpy(attr_value, omcimsg_avc->attr_value, 30);

	util_fdprintf(fd, "attr mask: 0x%.2x%.2x\n", attr_mask[0], attr_mask[1]);

	for (next_start_byte = 0, attr_order = 1; attr_order < MEINFO_ATTR_ORDER_MAX; attr_order++) {
		if (util_attr_mask_get_bit(attr_mask, attr_order)) {
			char *name=meinfo_util_attr_get_name(classid, attr_order);
			struct attr_value_t attr;
			char str[1024];

			util_fdprintf(fd, "attr %d (%s): ", attr_order, name?name:"");

			if (meinfo_util_attr_get_format(classid, attr_order) != ATTR_FORMAT_TABLE ) {
				next_start_byte += meinfo_util_attr_get_byte_size(classid, attr_order);

				attr.data=0;
				attr.ptr=malloc_safe(30);

				if (next_start_byte <= 30 &&
					omcimsg_util_get_attr_by_mask(attr_value, 30,
							classid, attr_order, attr_mask, &attr)==OMCIMSG_RW_OK) {
					meinfo_conv_attr_to_string(classid, attr_order, &attr, str, 1024);
					util_fdprintf(fd, "%s\n", str);
				} else {
					util_fdprintf(fd, "\n");
				}
				if (attr.ptr)
					free_safe(attr.ptr);
			} else {
				util_fdprintf(fd, "(table)\n");
			}
		}
	}

	util_fdprintf(fd, "\n");
	
	return 0;
}

