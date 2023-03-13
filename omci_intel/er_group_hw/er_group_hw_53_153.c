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
 * Module  : er_group_hw
 * File    : er_group_hw_53_153.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "hwresource.h"
#include "voip_hw.h"
#include "er_group_hw.h"
#include "er_group_hw_util.h"
#include "batchtab.h"

// 53 Physical_path_termination_point_POTS_UNI
// 153 SIP_user_data

//53@254,153@2,153@3,153@4,153@5,153@6,153@11,153@12
int
er_group_hw_sip_user_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	struct voip_hw_sip_user_data_t sud;
	struct me_t *temp_me1, *temp_me2;
	unsigned char large_string1[376], large_string2[376];
	struct voip_hw_authentication_security_method_t authentication_security_method1, authentication_security_method2;
	struct voip_hw_network_address_t network_address;
	struct attr_value_t attr;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = attr_inst->er_attr_instance[0].attr_value.data;

	memset(&sud, 0x00, sizeof(sud));

	//user part aor
	if ((temp_me1 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[1].attr_value.data)) == NULL) {
		sud.user_part_aor = NULL;
	} else {
		memset(large_string1, 0x00, sizeof(large_string1));
		meinfo_me_copy_from_large_string_me(temp_me1, large_string1, 375);
		sud.user_part_aor = large_string1;
	}

	//sip display name
	memcpy(sud.sip_display_name, attr_inst->er_attr_instance[2].attr_value.ptr, 25);

	//username/password
	if ((temp_me1 = meinfo_me_get_by_meid(148, (unsigned short)attr_inst->er_attr_instance[3].attr_value.data)) == NULL) {
		sud.username_password = NULL;
	} else {
		memset(&authentication_security_method1, 0x00, sizeof(authentication_security_method1));

		//caliddation scheme
		meinfo_me_attr_get(temp_me1, 1, &attr);
		authentication_security_method1.validation_scheme = (unsigned char) attr.data;

		//username 1
		attr.ptr = authentication_security_method1.username_1;
		meinfo_me_attr_get(temp_me1, 2, &attr);

		//password
		attr.ptr = authentication_security_method1.password;
		meinfo_me_attr_get(temp_me1, 3, &attr);

		//realm
		attr.ptr = authentication_security_method1.realm;
		meinfo_me_attr_get(temp_me1, 4, &attr);

		//username 2
		attr.ptr = authentication_security_method1.username_2;
		meinfo_me_attr_get(temp_me1, 5, &attr);

		sud.username_password = &authentication_security_method1;
	}

	//voice mail server sip uri
	if ((temp_me1 = meinfo_me_get_by_meid(137, (unsigned short)attr_inst->er_attr_instance[4].attr_value.data)) == NULL) {
		sud.voicemail_server_sip_uri = NULL;
	} else {
		memset(&network_address, 0x00, sizeof(network_address));

		//security_pointer
		meinfo_me_attr_get(temp_me1, 1, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(148, (unsigned short)attr.data)) == NULL) {
			network_address.security_pointer = NULL;
		} else {
			memset(&authentication_security_method2, 0x00, sizeof(authentication_security_method2));

			//caliddation scheme
			meinfo_me_attr_get(temp_me2, 1, &attr);
			authentication_security_method2.validation_scheme = (unsigned char) attr.data;

			//username 1
			attr.ptr = authentication_security_method2.username_1;
			meinfo_me_attr_get(temp_me2, 2, &attr);

			//password
			attr.ptr = authentication_security_method2.password;
			meinfo_me_attr_get(temp_me2, 3, &attr);

			//realm
			attr.ptr = authentication_security_method2.realm;
			meinfo_me_attr_get(temp_me2, 4, &attr);

			//username 2
			attr.ptr = authentication_security_method2.username_2;
			meinfo_me_attr_get(temp_me2, 5, &attr);

			network_address.security_pointer = &authentication_security_method2;
		}

		//address_pointer
		meinfo_me_attr_get(temp_me1, 2, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(157, (unsigned short)attr.data)) == NULL) {
			network_address.address_pointer = NULL;
		} else {
			memset(large_string2, 0x00, sizeof(large_string2));
			meinfo_me_copy_from_large_string_me(temp_me2, large_string2, 375);
			network_address.address_pointer = large_string2;
		}
		sud.voicemail_server_sip_uri = &network_address;
	}

	//voicemail subscription expiration time
	sud.voicemail_subscription_expiration_time = (unsigned int)attr_inst->er_attr_instance[5].attr_value.data;

	//release timer
	sud.release_timer = (unsigned char)attr_inst->er_attr_instance[6].attr_value.data;

	//roh timer
	sud.roh_timer = (unsigned char)attr_inst->er_attr_instance[7].attr_value.data;

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_attr_group_util_get_attr_mask_by_classid(attr_inst, 153, attr_mask);
	case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:
		if ((ret = er_group_hw_voip_wrapper_sip_user_data(VOIP_HW_ACTION_SET, port_id, attr_mask, &sud)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//add
		if (er_group_hw_sip_user_data(ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&sud, 0x00, sizeof(sud));
		if ((ret = er_group_hw_voip_wrapper_sip_user_data(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &sud)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
		;
	}
	return 0;
}
