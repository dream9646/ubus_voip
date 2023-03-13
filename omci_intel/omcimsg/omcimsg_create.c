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
 * File    : omcimsg_create.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "meinfo.h"
#include "util.h"

int
omcimsg_create_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(req_msg);
	unsigned short meid = omcimsg_header_get_entity_instance_id(req_msg);
	struct meinfo_t *miptr=meinfo_util_miptr(classid);

	struct omcimsg_create_t *create_contents;
	struct omcimsg_create_response_t *create_response_contents;

	short instance_num;
	int i;
	struct me_t *me;

	dbprintf(LOG_INFO, "classid=%u\n", classid);

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	create_response_contents = (struct omcimsg_create_response_t *) resp_msg->msg_contents;

	if (miptr == NULL) {
		create_response_contents->result_reason = OMCIMSG_RESULT_UNKNOWN_ME;
		dbprintf(LOG_ERR, "classid=%u, err=%d\n", classid, OMCIMSG_ERROR_UNKNOWN_ME);
		return OMCIMSG_ERROR_UNKNOWN_ME;
	}

	if (meinfo_me_is_exist(classid, meid) ) {
		//send instance exist error response back
		create_response_contents->result_reason = OMCIMSG_RESULT_INSTANCE_EXISTS;
		dbprintf(LOG_ERR, "classid=%u, meid=%u, err=%d\n", classid, meid, OMCIMSG_ERROR_INSTANCE_EXISTS);
		return OMCIMSG_ERROR_INSTANCE_EXISTS;
	}

	if ((instance_num = meinfo_instance_alloc_free_num(classid)) < 0) {
		//send unknown managed entity instance error response back
		create_response_contents->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
		dbprintf(LOG_ERR, "classid=%u, err=%d\n", classid, OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE);
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}

	me = meinfo_me_alloc(classid, instance_num);
	if (!me) {
		//send unknown managed entity error response back
		create_response_contents->result_reason = OMCIMSG_RESULT_UNKNOWN_ME;
		dbprintf(LOG_ERR, "classid=%u, err=%d\n", classid, OMCIMSG_ERROR_NOMEM);
		return OMCIMSG_ERROR_UNKNOWN_ME;
	}

	// set default from config
	meinfo_me_load_config(me);

	// assign meid
	me->meid = meid;

	//create by olt
	me->is_cbo = 1;

	create_contents = (struct omcimsg_create_t *) req_msg->msg_contents;

	dbprintf(LOG_INFO, "classid=%u, meid=%u, execmask=0x%02x%02x\n", classid, meid,
			create_response_contents->attr_exec_mask[0],
			create_response_contents->attr_exec_mask[1]);
	//assign value of set_by_create
	for (i = 1; i <= miptr->attr_total; i++) {
		if (meinfo_util_attr_is_sbc(classid, i)) {
			util_attr_mask_set_bit(miptr->access_create_mask, i);
		
			//alloc memory for bit field and string
			switch (meinfo_util_attr_get_format(classid, i)) {
			case ATTR_FORMAT_BIT_FIELD:
			case ATTR_FORMAT_STRING:
				if (me->attr[i].value.ptr == NULL)
					me->attr[i].value.ptr = malloc_safe(meinfo_util_attr_get_byte_size(classid, i));
				break;
			default:
				;	//do nothing
			}

			omcimsg_util_get_attr_from_create(create_contents->attr_value, classid, i, &me->attr[i].value);

			//check for value validity
			if (!meinfo_me_attr_is_valid(me, i, &me->attr[i].value)) {
				util_attr_mask_set_bit((unsigned char *)
						       create_response_contents->attr_exec_mask, i);
			}

			dbprintf(LOG_INFO, "attr %d, execmask=0x%02x%02x\n", i,
				create_response_contents->attr_exec_mask[0],
				create_response_contents->attr_exec_mask[1]);
		}
		
	}

	if (	create_response_contents->attr_exec_mask[0] != 0 || 
		create_response_contents->attr_exec_mask[1] != 0) {
		//release
		meinfo_me_release(me);

		//send parameter error response back
		create_response_contents->result_reason = OMCIMSG_RESULT_PARM_ERROR;

		dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_PARM_ERROR);
		return OMCIMSG_ERROR_PARM_ERROR;
	}
	//attach to instance list
	if (meinfo_me_create(me) < 0) {
		//release
		meinfo_me_release(me);

		//send unknown managed entity error response back
		create_response_contents->result_reason = OMCIMSG_RESULT_CMD_ERROR;

		dbprintf(LOG_ERR, "create fail, classid=%u, err=%d\n", classid, OMCIMSG_RESULT_CMD_ERROR);
		return OMCIMSG_RESULT_CMD_ERROR;
	}
	//send success response
	create_response_contents->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;

	return 0;
}
