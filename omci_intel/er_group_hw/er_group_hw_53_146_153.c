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
 * File    : er_group_hw_53_146_153.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
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
// 146 VoIP_application_service_profile
// 153 SIP_user_data

//53@254,146@1,146@2,146@3,146@4,146@5,146@6,146@7,146@8,146@9
int
er_group_hw_voip_application_service_profile(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret, delay_time;
	struct voip_hw_voip_application_service_profile_t vasp;
	struct me_t *temp_me1, *temp_me2;
	unsigned char large_string1[376], large_string2[376], large_string3[376], *separator;
	struct voip_hw_authentication_security_method_t authentication_security_method1, authentication_security_method2, authentication_security_method3;
	struct voip_hw_network_address_t network_address1, network_address2, network_address3;
	struct attr_value_t attr;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = attr_inst->er_attr_instance[0].attr_value.data;

	memset(&vasp, 0x00, sizeof(vasp));

	//cid features
	vasp.cid_features.reserved = util_bitmap_get_value(attr_inst->er_attr_instance[1].attr_value.ptr, 8, 0, 2);
	vasp.cid_features.anonymous_cid_blocking = util_bitmap_get_value(attr_inst->er_attr_instance[1].attr_value.ptr, 8, 2, 1);
	vasp.cid_features.cid_name = util_bitmap_get_value(attr_inst->er_attr_instance[1].attr_value.ptr, 8, 3, 1);
	vasp.cid_features.cid_number = util_bitmap_get_value(attr_inst->er_attr_instance[1].attr_value.ptr, 8, 4, 1);
	vasp.cid_features.cid_blocking = util_bitmap_get_value(attr_inst->er_attr_instance[1].attr_value.ptr, 8, 5, 1);
	vasp.cid_features.calling_name = util_bitmap_get_value(attr_inst->er_attr_instance[1].attr_value.ptr, 8, 6, 1);
	vasp.cid_features.calling_number = util_bitmap_get_value(attr_inst->er_attr_instance[1].attr_value.ptr, 8, 7, 1);

	//call waiting features
	vasp.call_waiting_features.reserved = util_bitmap_get_value(attr_inst->er_attr_instance[2].attr_value.ptr, 8, 0, 6);
	vasp.call_waiting_features.caller_id_announcement = util_bitmap_get_value(attr_inst->er_attr_instance[2].attr_value.ptr, 8, 6, 1);
	vasp.call_waiting_features.call_waiting = util_bitmap_get_value(attr_inst->er_attr_instance[2].attr_value.ptr, 8, 7, 1);

	//call progress or transfer features
	vasp.call_progress_or_transfer_features.reserved = util_bitmap_get_value(attr_inst->er_attr_instance[3].attr_value.ptr, 8*2, 0, 8);
	vasp.call_progress_or_transfer_features.six_way = util_bitmap_get_value(attr_inst->er_attr_instance[3].attr_value.ptr, 8*2, 8, 1);
	vasp.call_progress_or_transfer_features.emergency_service_originating_hold = util_bitmap_get_value(attr_inst->er_attr_instance[3].attr_value.ptr, 8*2, 9, 1);
	vasp.call_progress_or_transfer_features.flash_on_emergency_service_call = util_bitmap_get_value(attr_inst->er_attr_instance[3].attr_value.ptr, 8*2, 10, 1);
	vasp.call_progress_or_transfer_features.do_not_disturb = util_bitmap_get_value(attr_inst->er_attr_instance[3].attr_value.ptr, 8*2, 11, 1);
	vasp.call_progress_or_transfer_features.call_park = util_bitmap_get_value(attr_inst->er_attr_instance[3].attr_value.ptr, 8*2, 12, 1);
	vasp.call_progress_or_transfer_features.call_hold = util_bitmap_get_value(attr_inst->er_attr_instance[3].attr_value.ptr, 8*2, 13, 1);
	vasp.call_progress_or_transfer_features.call_transfer = util_bitmap_get_value(attr_inst->er_attr_instance[3].attr_value.ptr, 8*2, 14, 1);
	vasp.call_progress_or_transfer_features.threeway = util_bitmap_get_value(attr_inst->er_attr_instance[3].attr_value.ptr, 8*2, 15, 1);

	//call presentation features
	vasp.call_presentation_features.reserved = util_bitmap_get_value(attr_inst->er_attr_instance[4].attr_value.ptr, 8*2, 0, 12);
	vasp.call_presentation_features.call_forwarding_indication = util_bitmap_get_value(attr_inst->er_attr_instance[4].attr_value.ptr, 8*2, 12, 1);
	vasp.call_presentation_features.message_waiting_indication_visual_indication = util_bitmap_get_value(attr_inst->er_attr_instance[4].attr_value.ptr, 8*2, 13, 1);
	vasp.call_presentation_features.message_waiting_indication_special_dial_tone = util_bitmap_get_value(attr_inst->er_attr_instance[4].attr_value.ptr, 8*2, 14, 1);
	vasp.call_presentation_features.message_waiting_indication_splash_ring = util_bitmap_get_value(attr_inst->er_attr_instance[4].attr_value.ptr, 8*2, 15, 1);

	//direct connect feature
	vasp.direct_connect_feature.reserved = util_bitmap_get_value(attr_inst->er_attr_instance[5].attr_value.ptr, 8, 0, 6);
	vasp.direct_connect_feature.dial_tone_feature_delay_option = util_bitmap_get_value(attr_inst->er_attr_instance[5].attr_value.ptr, 8, 6, 1);
	vasp.direct_connect_feature.direct_connect_feature_enabled = util_bitmap_get_value(attr_inst->er_attr_instance[5].attr_value.ptr, 8, 7, 1);

	//direct connect uri pointer
	if ((temp_me1 = meinfo_me_get_by_meid(137, (unsigned short)attr_inst->er_attr_instance[6].attr_value.data)) == NULL)
	{
		vasp.direct_connect_uri_pointer = NULL;
	} else {
		memset(&network_address1, 0x00, sizeof(network_address1));

		//security_pointer
		meinfo_me_attr_get(temp_me1, 1, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(148, (unsigned short)attr.data)) == NULL) {
			network_address1.security_pointer = NULL;
		} else {
			memset(&authentication_security_method1, 0x00, sizeof(authentication_security_method1));

			//caliddation scheme
			meinfo_me_attr_get(temp_me2, 1, &attr);
			authentication_security_method1.validation_scheme = (unsigned char) attr.data;

			//username 1
			attr.ptr = authentication_security_method1.username_1;
			meinfo_me_attr_get(temp_me2, 2, &attr);

			//password
			attr.ptr = authentication_security_method1.password;
			meinfo_me_attr_get(temp_me2, 3, &attr);

			//realm
			attr.ptr = authentication_security_method1.realm;
			meinfo_me_attr_get(temp_me2, 4, &attr);

			//username 2
			attr.ptr = authentication_security_method1.username_2;
			meinfo_me_attr_get(temp_me2, 5, &attr);

			network_address1.security_pointer = &authentication_security_method1;
		}

		//address_pointer
		meinfo_me_attr_get(temp_me1, 2, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(157, (unsigned short)attr.data)) == NULL) {
			network_address1.address_pointer = NULL;
		} else {
			memset(large_string1, 0x00, sizeof(large_string1));
			meinfo_me_copy_from_large_string_me(temp_me2, large_string1, 375);
			//direct_connect_uri!delay_time
			if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
				if( (separator = (char *) strchr(large_string1, '!')) != NULL ) {
					*separator='\0';
					separator++;
					delay_time= atoi(separator);
					vasp.voip_hw_dial_tone_feature_delay_timer = delay_time;
				}
			}
			network_address1.address_pointer = large_string1;
		}
		vasp.direct_connect_uri_pointer = &network_address1;
	}

	//bridged line agent uri pointer
	if ((temp_me1 = meinfo_me_get_by_meid(137, (unsigned short)attr_inst->er_attr_instance[7].attr_value.data)) == NULL) {
		vasp.bridged_line_agent_uri_pointer = NULL;
	} else {
		memset(&network_address2, 0x00, sizeof(network_address2));

		//security_pointer
		meinfo_me_attr_get(temp_me1, 1, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(148, (unsigned short)attr.data)) == NULL) {
			network_address2.security_pointer = NULL;
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

			network_address2.security_pointer = &authentication_security_method2;
		}

		//address_pointer
		meinfo_me_attr_get(temp_me1, 2, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(157, (unsigned short)attr.data)) == NULL) {
			network_address2.address_pointer = NULL;
		} else {
			memset(large_string2, 0x00, sizeof(large_string2));
			meinfo_me_copy_from_large_string_me(temp_me2, large_string2, 375);
			network_address2.address_pointer = large_string2;
		}
		vasp.bridged_line_agent_uri_pointer = &network_address2;
	}

	//conference factory uri pointer
	if ((temp_me1 = meinfo_me_get_by_meid(137, (unsigned short)attr_inst->er_attr_instance[8].attr_value.data)) == NULL) {
		vasp.conference_factory_uri_pointer = NULL;
	} else {
		memset(&network_address3, 0x00, sizeof(network_address3));

		//security_pointer
		meinfo_me_attr_get(temp_me1, 1, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(148, (unsigned short)attr.data)) == NULL) {
			network_address3.security_pointer = NULL;
		} else {
			memset(&authentication_security_method3, 0x00, sizeof(authentication_security_method3));

			//caliddation scheme
			meinfo_me_attr_get(temp_me2, 1, &attr);
			authentication_security_method3.validation_scheme = (unsigned char) attr.data;

			//username 1
			attr.ptr = authentication_security_method3.username_1;
			meinfo_me_attr_get(temp_me2, 2, &attr);

			//password
			attr.ptr = authentication_security_method3.password;
			meinfo_me_attr_get(temp_me2, 3, &attr);

			//realm
			attr.ptr = authentication_security_method3.realm;
			meinfo_me_attr_get(temp_me2, 4, &attr);

			//username 2
			attr.ptr = authentication_security_method3.username_2;
			meinfo_me_attr_get(temp_me2, 5, &attr);

			network_address3.security_pointer = &authentication_security_method3;
		}

		//address_pointer
		meinfo_me_attr_get(temp_me1, 2, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(157, (unsigned short)attr.data)) == NULL) {
			network_address3.address_pointer = NULL;
		} else {
			memset(large_string3, 0x00, sizeof(large_string3));
			meinfo_me_copy_from_large_string_me(temp_me2, large_string3, 375);
			network_address3.address_pointer = large_string3;
		}
		vasp.conference_factory_uri_pointer = &network_address3;
	}
	if (omci_env_g.olt_workaround_enable != ENV_OLT_WORKAROUND_CALIX) {
		//calix not support this
		//Dial tone feature delay/Warmline timer
		vasp.voip_hw_dial_tone_feature_delay_timer = (unsigned short)attr_inst->er_attr_instance[9].attr_value.data;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_attr_group_util_get_attr_mask_by_classid(attr_inst, 146, attr_mask);
	case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:
		if ((ret = er_group_hw_voip_wrapper_voip_application_service_profile(VOIP_HW_ACTION_SET, port_id, attr_mask, &vasp)) < 0) {
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
		if (er_group_hw_voip_application_service_profile(ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&vasp, 0x00, sizeof(vasp));
		if ((ret = er_group_hw_voip_wrapper_voip_application_service_profile(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &vasp)) < 0) {
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
