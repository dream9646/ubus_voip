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
 * File    : omcidump_mib_upload.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "omcidump.h"
#include "util.h"

int omcidump_mib_upload(int fd, struct omcimsg_layout_t *msg)
{
	omcidump_header(fd, msg);

	//dump msg content
	util_fdprintf(fd, "mib upload request has no content.\n");

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_mib_upload_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_mib_upload_response_t *mib_upload_response_contents = (struct omcimsg_mib_upload_response_t *)msg->msg_contents;

	omcidump_header(fd, msg);

	//dump msg content
	util_fdprintf(fd, "subsequent cmds total: 0x%x(%u)\n", ntohs(mib_upload_response_contents->subsequent_cmd_total), ntohs(mib_upload_response_contents->subsequent_cmd_total));

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_mib_upload_next(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_mib_upload_next_t *mib_upload_next_contents = (struct omcimsg_mib_upload_next_t *)msg->msg_contents;

	omcidump_header(fd, msg);

	 //dump msg content
	util_fdprintf(fd, "sequence num: 0x%x(%u)\n", ntohs(mib_upload_next_contents->sequence_num), ntohs(mib_upload_next_contents->sequence_num));

	util_fdprintf(fd, "\n");

	return 0;
}

int omcidump_mib_upload_next_response(int fd, struct omcimsg_layout_t *msg)
{
	struct meinfo_t *miptr;
	struct attr_value_t attr;
	struct omcimsg_mib_upload_next_response_t *mib_upload_next_response_contents = (struct omcimsg_mib_upload_next_response_t *)msg->msg_contents;
	unsigned short upload_classid=ntohs(mib_upload_next_response_contents->entity_class);
	char ostr[1024];
	int i;

	omcidump_header(fd, msg);

	//dump msg content
	miptr = meinfo_util_miptr(upload_classid);
	if (miptr)
		util_fdprintf(fd, "mib upload classid: 0x%x(%u), %s\n", 
			upload_classid, upload_classid, miptr->name?miptr->name:"?");
	else
		util_fdprintf(fd, "mib upload classid: 0x%x(%u)\n", upload_classid, upload_classid);
	util_fdprintf(fd, "mib upload iid: 0x%x(%u)\n", ntohs(mib_upload_next_response_contents->entity_instance), ntohs(mib_upload_next_response_contents->entity_instance));
	
	if (miptr)
	{
		attr.data=0;
		attr.ptr = malloc_safe(1024);

		//get attr order, and check for table
		for (i = 1; i <= meinfo_util_get_attr_total(upload_classid); i++) {
			if (util_attr_mask_get_bit(mib_upload_next_response_contents->attr_mask, i))
			{
				omcimsg_util_get_attr_by_mask(mib_upload_next_response_contents->value_first_attr,
					26, upload_classid, i,
					mib_upload_next_response_contents->attr_mask,
					&attr);
				meinfo_conv_attr_to_string(upload_classid, i, &attr, ostr, 1024);
				util_fdprintf(fd, "attr %2u (%s): %s\n", i, miptr->attrinfo[i].name, ostr);
			}
		}

		free_safe(attr.ptr);
	}

	util_fdprintf(fd, "\n");

	return 0;
}

