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
 * File    : omcimsg_delete.c
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
omcimsg_delete_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(req_msg);
	unsigned short meid = omcimsg_header_get_entity_instance_id(req_msg);
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	int found;
	struct me_t *me;
	struct omcimsg_delete_response_t *delete_response_contents;

	dbprintf(LOG_INFO, "classid(%d)\n", omcimsg_header_get_entity_class_id(req_msg));

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	delete_response_contents = (struct omcimsg_delete_response_t *) resp_msg->msg_contents;

	if (miptr==NULL) {
		//send unknown managed entity error response back
		delete_response_contents->result_reason = OMCIMSG_RESULT_UNKNOWN_ME;
		dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME);
		return OMCIMSG_ERROR_UNKNOWN_ME;
	}

	found = 0;
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->meid == meid) {
			found = 1;
			break;
		}
	}

	if (!found) {
		if (omci_env_g.mib_nonexist_del_err==ENV_MIB_NONEXIST_DEL_ERR_ME_TABENTRY ||
		    omci_env_g.mib_nonexist_del_err==ENV_MIB_NONEXIST_DEL_ERR_ME) {
			//send unknown managed entity instance error response back
			delete_response_contents->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
			dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE);
			return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
		}
	} else {
		meinfo_me_delete_by_instance_num(classid, me->instance_num);
	}

	//send success response
	delete_response_contents->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;

	return 0;
}
