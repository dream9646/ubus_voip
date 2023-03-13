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
 * File    : er_group_hw_138.c
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

// 138 VoIP_config_data

//138@1,138@2,138@3,138@4,138@5,138@6,138@7,138@8
int
er_group_hw_voip_config_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	int ret;
	struct voip_hw_voip_config_data_t vcd;
	struct me_t *temp_me1, *temp_me2;
	unsigned char large_string[376];
	struct voip_hw_authentication_security_method_t authentication_security_method;
	struct voip_hw_network_address_t network_address;
	struct attr_value_t attr;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	memset(&vcd, 0x00, sizeof(vcd));

	vcd.available_signalling_protocols.reserved = util_bitmap_get_value(attr_inst->er_attr_instance[0].attr_value.ptr, 8, 0, 5);
	vcd.available_signalling_protocols.mgcp = util_bitmap_get_value(attr_inst->er_attr_instance[0].attr_value.ptr, 8, 5, 1);
	vcd.available_signalling_protocols.h248 = util_bitmap_get_value(attr_inst->er_attr_instance[0].attr_value.ptr, 8, 6, 1);
	vcd.available_signalling_protocols.sip = util_bitmap_get_value(attr_inst->er_attr_instance[0].attr_value.ptr, 8, 7, 1);

	vcd.signalling_protocols_used = (unsigned char) attr_inst->er_attr_instance[1].attr_value.data;

	vcd.available_voip_configuration_methods.proprietary = util_bitmap_get_value(attr_inst->er_attr_instance[2].attr_value.ptr, 8*4, 0, 8);
	vcd.available_voip_configuration_methods.reserved = util_bitmap_get_value(attr_inst->er_attr_instance[2].attr_value.ptr, 8*4, 8, 20);
	vcd.available_voip_configuration_methods.sipping_config_framework = util_bitmap_get_value(attr_inst->er_attr_instance[2].attr_value.ptr, 8*4, 28, 1);
	vcd.available_voip_configuration_methods.tr069 = util_bitmap_get_value(attr_inst->er_attr_instance[2].attr_value.ptr, 8*4, 29, 1);
	vcd.available_voip_configuration_methods.configuration_file = util_bitmap_get_value(attr_inst->er_attr_instance[2].attr_value.ptr, 8*4, 30, 1);
	vcd.available_voip_configuration_methods.omci = util_bitmap_get_value(attr_inst->er_attr_instance[2].attr_value.ptr, 8*4, 31, 1);

	vcd.voip_configuration_method_used = (unsigned char) attr_inst->er_attr_instance[3].attr_value.data;

	//direct connect uri pointer
	if ((temp_me1 = meinfo_me_get_by_meid(137, (unsigned short)attr_inst->er_attr_instance[4].attr_value.data)) == NULL) {
		vcd.voip_configuration_address_pointer = NULL;
	} else {
		memset(&network_address, 0x00, sizeof(network_address));

		//security_pointer
		meinfo_me_attr_get(temp_me1, 1, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(148, (unsigned short)attr.data)) == NULL) {
			network_address.security_pointer = NULL;
		} else {
			memset(&authentication_security_method, 0x00, sizeof(authentication_security_method));

			//caliddation scheme
			meinfo_me_attr_get(temp_me2, 1, &attr);
			authentication_security_method.validation_scheme = (unsigned char) attr.data;

			//username 1
			attr.ptr = authentication_security_method.username_1;
			meinfo_me_attr_get(temp_me2, 2, &attr);

			//password
			attr.ptr = authentication_security_method.password;
			meinfo_me_attr_get(temp_me2, 3, &attr);

			//realm
			attr.ptr = authentication_security_method.realm;
			meinfo_me_attr_get(temp_me2, 4, &attr);

			//username 2
			attr.ptr = authentication_security_method.username_2;
			meinfo_me_attr_get(temp_me2, 5, &attr);

			network_address.security_pointer = &authentication_security_method;
		}

		//address_pointer
		meinfo_me_attr_get(temp_me1, 2, &attr);

		if ((temp_me2 = meinfo_me_get_by_meid(157, (unsigned short)attr.data)) == NULL) {
			network_address.address_pointer = NULL;
		} else {
			memset(large_string, 0x00, sizeof(large_string));
			meinfo_me_copy_from_large_string_me(temp_me2, large_string, 375);
			network_address.address_pointer = large_string;
		}
		vcd.voip_configuration_address_pointer = &network_address;
	}

	vcd.voip_configuration_state = (unsigned char) attr_inst->er_attr_instance[5].attr_value.data;
	vcd.retrieve_profile = (unsigned char) attr_inst->er_attr_instance[6].attr_value.data;

	attr.ptr = vcd.profile_version;
	meinfo_me_attr_copy_value(138, 8, &attr, &(attr_inst->er_attr_instance[7].attr_value));

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((ret = er_group_hw_voip_wrapper_voip_config_data(VOIP_HW_ACTION_SET, VOIP_HW_GLOBAL_PORT_DEF, attr_mask, &vcd)) < 0) {
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
		if (er_group_hw_voip_config_data(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&vcd, 0x00, sizeof(vcd));
		if ((ret = er_group_hw_voip_wrapper_voip_config_data(VOIP_HW_ACTION_RELEASE, VOIP_HW_GLOBAL_PORT_DEF, attr_mask, &vcd)) < 0) {
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
