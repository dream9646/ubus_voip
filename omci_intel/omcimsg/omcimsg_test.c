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
 * File    : omcimsg_test.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "env.h"
#include "meinfo.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "testtask.h"
#include "fwk_msgq.h"

int
omcimsg_test_result_generator(unsigned short classid, unsigned short meid, unsigned short transaction_id, unsigned char *result_content)
{
	struct omcimsg_layout_t *omcimsg;

	omcimsg = malloc_safe(sizeof (struct omcimsg_layout_t));
	omcimsg_header_set_transaction_id(omcimsg, transaction_id);

	omcimsg_header_set_flag_db(omcimsg, 0);
	omcimsg_header_set_flag_ar(omcimsg, 0);
	omcimsg_header_set_flag_ak(omcimsg, 0);
	omcimsg_header_set_type(omcimsg, OMCIMSG_TEST_RESULT);

	omcimsg_header_set_device_id(omcimsg, 0xa);

	omcimsg_header_set_entity_class_id(omcimsg, classid);
	omcimsg_header_set_entity_instance_id(omcimsg, meid);

	memcpy(omcimsg->msg_contents, result_content, sizeof(omcimsg->msg_contents));

	//sendto network
	return omcimsg_handler_coretask_tranx_send_omci_msg(omcimsg, 0);
}


int
omcimsg_test_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(req_msg);
	unsigned short meid = omcimsg_header_get_entity_instance_id(req_msg);
	struct meinfo_t *miptr = meinfo_util_miptr(classid);
	struct me_t *me=meinfo_me_get_by_meid(classid, meid);
	int (*test_is_supported_func)(struct me_t *me, unsigned char *req);

	struct omcimsg_test_response_t *response;
	struct testtask_msg_t *test_msg;	// ipc msg to test thread

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	response = (struct omcimsg_test_response_t *) resp_msg->msg_contents;

	if (miptr == NULL) {
		response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME;
		dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME);
		return OMCIMSG_ERROR_UNKNOWN_ME;
	}
	if ((me = meinfo_me_get_by_meid(classid, meid)) == NULL) {
		response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
		dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE);
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}

	// select test_is_support_func ptr, hw is preferred over cb 
	test_is_supported_func=miptr->hardware.test_is_supported_hw;
	if (!test_is_supported_func)
		test_is_supported_func=miptr->callback.test_is_supported_cb;		
	if (!test_is_supported_func) {
		dbprintf(LOG_ERR, "err=%d, classid=%u, meid=0x%x(%u), test func ptr null?\n",
			 OMCIMSG_RESULT_CMD_ERROR, classid, meid, meid);
		response->result_reason=OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	}

	if (test_is_supported_func(me, req_msg->msg_contents)==0) {
		dbprintf(LOG_ERR, "err=%d, classid=%u, meid=0x%x(%u), unsupported test %u\n",
			 OMCIMSG_RESULT_CMD_ERROR, classid, meid, meid, req_msg->msg_contents[0]&0x7f);
		response->result_reason=OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	}
		
	// prepare ipc msg to test thread
	test_msg=(struct testtask_msg_t *)malloc_safe(sizeof(struct testtask_msg_t));
	test_msg->req_msg=(struct omcimsg_layout_t *)malloc_safe(sizeof(struct omcimsg_layout_t));	
	memcpy(test_msg->req_msg, req_msg, sizeof(struct omcimsg_layout_t));
	test_msg->classid=classid;
	test_msg->meid=meid;
	test_msg->cmd=TESTTASK_CMD_TEST;
	
	// send ipc msg to test thread
	if (fwk_msgq_send(testtask_msg_qid_g, &test_msg->q_entry)<0) {
		free_safe(test_msg->req_msg);
		free_safe(test_msg);
		response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		return OMCIMSG_ERROR_CMD_ERROR;
	}
	response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;
	return 0;
}
	

#if 0
int
omcimsg_test_handler_orig(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(req_msg);
	unsigned short meid = omcimsg_header_get_entity_instance_id(req_msg);

	struct omcimsg_test_response_t *response;
	int reason = OMCIMSG_RESULT_CMD_SUCCESS;	// default reason: success

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);

	response = (struct omcimsg_test_response_t *) resp_msg->msg_contents;

	if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_SWSW) {
		response->result_reason=OMCIMSG_RESULT_CMD_SUCCESS;
		return 0;
	}
			
	// 256 ONT-G, 263 ANI-G, 6 CircuitPack
	if (classid==256 || classid==263 || classid==6) {
		struct omcimsg_test_ont_ani_circuit_t *req = (struct omcimsg_test_ont_ani_circuit_t *) req_msg->msg_contents;
		if (req->select_test==7) {		// selft test
			omcimsg_test_sendmsg(req_msg);
		} else if (req->select_test>=8 && req->select_test<=15) { // vendor specific test
			dbprintf(LOG_ERR,
				 "err=%d, classid=%u, meid=0x%x(%u), unsupported test %u\n",
				 OMCIMSG_RESULT_CMD_NOT_SUPPORTED, classid, meid, meid, req->select_test);
			response->result_reason=OMCIMSG_RESULT_CMD_NOT_SUPPORTED;
			return OMCIMSG_ERROR_CMD_NOT_SUPPORTED;

		} else {
			dbprintf(LOG_ERR,
				 "err=%d, classid=%u, meid=0x%x(%u), invalid test %u\n",
				 OMCIMSG_RESULT_PARM_ERROR, classid, meid, meid, req->select_test);
			response->result_reason=OMCIMSG_RESULT_PARM_ERROR;
			return OMCIMSG_ERROR_PARM_ERROR;
		}

	// 134 IP Host Config Data
	} else if (classid==134) {
		struct omcimsg_test_ip_host_t *req = (struct omcimsg_test_ip_host_t *) req_msg->msg_contents;
		if (req->select_test==1) {		// ping
			omcimsg_test_sendmsg(req_msg);
		} else if (req->select_test==2) {	// traceroute
			omcimsg_test_sendmsg(req_msg);
		} else if (req->select_test>=8 && req->select_test<=15) {      // vendor specific
			dbprintf(LOG_ERR,
				 "err=%d, classid=%u, meid=0x%x(%u), unsupported test %u\n",
				 OMCIMSG_RESULT_CMD_NOT_SUPPORTED, classid, meid, meid, req->select_test);
			response->result_reason=OMCIMSG_RESULT_CMD_NOT_SUPPORTED;
			return OMCIMSG_ERROR_CMD_NOT_SUPPORTED;
		} else {
			dbprintf(LOG_ERR,
				 "err=%d, classid=%u, meid=0x%x(%u), invalid test %u\n",
				 OMCIMSG_RESULT_PARM_ERROR, classid, meid, meid, req->select_test);
			response->result_reason=OMCIMSG_RESULT_PARM_ERROR;
			return OMCIMSG_ERROR_PARM_ERROR;
		}

	// 53 PPTP POTS UNI, 80 PPTP ISDN UNI
	} else if (classid==53 || classid==80) {
		struct omcimsg_test_pots_isdn_t*req=(struct omcimsg_test_pots_isdn_t *) req_msg->msg_contents;
		if (req->select_test<=8) {
			omcimsg_test_sendmsg(req_msg);
		} else if (req->select_test>=9 && req->select_test<=11) {	// vendor specefic, return result in test result msg
			dbprintf(LOG_ERR,
				 "err=%d, classid=%u, meid=0x%x(%u), unsupported test %u\n",
				 OMCIMSG_RESULT_CMD_NOT_SUPPORTED, classid, meid, meid, req->select_test);
			response->result_reason=OMCIMSG_RESULT_CMD_NOT_SUPPORTED;
			return OMCIMSG_ERROR_CMD_NOT_SUPPORTED;
		} else if (req->select_test>=12 && req->select_test<=15) {	// vendor specific, return reault in general purpose buffer ME
			if (meinfo_me_get_by_meid(308, req->pointer_to_a_general_purpose_buffer_me) == NULL) {
				dbprintf(LOG_ERR,
					 "err=%d, classid=%u, meid=0x%x(%u), test %u, invalid general purpose meid 0x%x(%u)\n",
					 OMCIMSG_RESULT_PARM_ERROR, classid, meid, meid, req->select_test, 
					 req->pointer_to_a_general_purpose_buffer_me, 
					 req->pointer_to_a_general_purpose_buffer_me);
				response->result_reason=OMCIMSG_RESULT_PARM_ERROR;
				return OMCIMSG_ERROR_PARM_ERROR;
			}			
			dbprintf(LOG_ERR,
				 "err=%d, classid=%u, meid=0x%x(%u), unsupported test %u\n",
				 OMCIMSG_RESULT_CMD_NOT_SUPPORTED, classid, meid, meid, req->select_test);
			response->result_reason=OMCIMSG_RESULT_CMD_NOT_SUPPORTED;
			return OMCIMSG_ERROR_CMD_NOT_SUPPORTED;		
		} else {
			dbprintf(LOG_ERR,
				 "err=%d, classid=%u, meid=0x%x(%u), invalid test %u\n",
				 OMCIMSG_RESULT_PARM_ERROR, classid, meid, meid, req->select_test);
			response->result_reason=OMCIMSG_RESULT_PARM_ERROR;
			return OMCIMSG_ERROR_PARM_ERROR;
		}		
	} else {
		dbprintf(LOG_ERR,
			 "err=%d, classid=%u, meid=0x%x(%u), invalid classid\n",
			 OMCIMSG_RESULT_PARM_ERROR, classid, meid, meid);
		response->result_reason=OMCIMSG_RESULT_CMD_NOT_SUPPORTED;
	}

	response->result_reason = reason;
	return 0;
}
#endif
