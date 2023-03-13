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
 * File    : omcidump_set.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <assert.h>
#include "omcidump.h"
#include "util.h"

#define SET_ATTRS_SIZE	30

/*
struct omcimsg_layout_t {
	unsigned short transaction_id;
	unsigned char msg_type;
	unsigned char device_id_type;
	unsigned short	entity_class_id;
	unsigned short	entity_instance_id;
	unsigned char msg_contents[32];
	unsigned short cpcsuu_cpi;
	unsigned short cpcssdu_len;
	unsigned int crc;
} __attribute__ ((packed));

struct omcimsg_set_t {
	unsigned char attr_mask[2];
	unsigned char attr_value[30];
} __attribute__ ((packed));

struct omcimsg_set_response_t {
	unsigned char result_reason;
	unsigned char optional_attr_mask[2];
	unsigned char attr_exec_mask[2];
	unsigned char padding[27];
} __attribute__ ((packed));

*/

int omcidump_set(int fd, struct omcimsg_layout_t *msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(msg);
	struct meinfo_t *miptr = meinfo_util_miptr(classid);

	omcidump_header(fd, msg);

	if (miptr) {
		struct omcimsg_set_t *req = (struct omcimsg_set_t *) msg->msg_contents;
		int attr_order, next_start_byte;
		
		for (next_start_byte = 0, attr_order = 1; attr_order < MEINFO_ATTR_ORDER_MAX; attr_order++) {
			if (util_attr_mask_get_bit(req->attr_mask, attr_order)) {
				char *name=meinfo_util_attr_get_name(classid, attr_order);
				struct attr_value_t attr;				
				char str[1024];
	
				util_fdprintf(fd, "attr %d (%s): ", attr_order, name?name:"");
	
				if (meinfo_util_attr_get_format(classid, attr_order) == ATTR_FORMAT_TABLE ) {
					next_start_byte += meinfo_util_attr_get_table_entry_byte_size(classid, attr_order);
				} else {
					next_start_byte += meinfo_util_attr_get_byte_size(classid, attr_order);		
				}
	
				attr.data=0;
				attr.ptr=malloc_safe(SET_ATTRS_SIZE);
				if (next_start_byte <= SET_ATTRS_SIZE &&
					omcimsg_util_get_attr_by_mask(req->attr_value, SET_ATTRS_SIZE,
								classid, attr_order, req->attr_mask, &attr)==OMCIMSG_RW_OK) {
					meinfo_conv_attr_to_string(classid, attr_order, &attr, str, 1024);
					util_fdprintf(fd, "%s\n", str);
				} else {
					util_fdprintf(fd, "\n");
				}
				if (attr.ptr)
					free_safe(attr.ptr);
			}
		}
	}

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_set_response(int fd, struct omcimsg_layout_t *msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(msg);
	struct meinfo_t *miptr=meinfo_util_miptr(classid);	
	struct omcimsg_set_response_t *response = (struct omcimsg_set_response_t *) msg->msg_contents;

	omcidump_header(fd, msg);

	util_fdprintf(fd, "result: %d, %s\n", response->result_reason, omcidump_reason_str(response->result_reason));
	util_fdprintf(fd, "optional attr mask: 0x%.2x%.2x\n", response->optional_attr_mask[0], response->optional_attr_mask[1]);
	util_fdprintf(fd, "attr execution mask: 0x%.2x%.2x\n", response->attr_exec_mask[0], response->attr_exec_mask[1]);
				
	//dump msg content
	if (miptr) {
		int attr_order;
		
		for (attr_order = 1; attr_order < MEINFO_ATTR_ORDER_MAX; attr_order++) {
			if (response->result_reason==OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN) {
				if (util_attr_mask_get_bit(response->optional_attr_mask, attr_order)) {
					char *name=meinfo_util_attr_get_name(classid, attr_order);
					util_fdprintf(fd, "attr %d (%s): unsupported?\n", attr_order, name?name:"");
				} else if (util_attr_mask_get_bit(response->attr_exec_mask, attr_order)) {
					char *name=meinfo_util_attr_get_name(classid, attr_order);
					util_fdprintf(fd, "attr %d (%s): failed?\n", attr_order, name?name:"");
				}
			}
		}
	}

	util_fdprintf(fd, "\n");

	return 0;
}

