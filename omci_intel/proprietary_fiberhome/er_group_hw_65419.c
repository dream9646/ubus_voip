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
 * Module  : proprietary_fiberhome
 * File    : er_group_hw_65419.c
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
#include "proprietary_fiberhome.h"

// 65419 WRI_sip_config

//65419@1,65419@2,65419@3,65419@4,65419@5,65419@6
int
er_group_hw_wri_sip_config(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	unsigned char large_string1[376];
	unsigned char large_string2[376];
	unsigned char large_string3[376];
	struct me_t *temp_me1;
	struct voip_hw_wri_sip_config_t wsc;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	port_id = VOIP_HW_GLOBAL_PORT_DEF;

	memset(&wsc, 0x00, sizeof(wsc));

	if ((temp_me1 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[0].attr_value.data)) == NULL)
	{
		wsc.secondary_sip_registrar_server_address_pointer = NULL;
	} else {
		memset(large_string1, 0x00, sizeof(large_string1));
		meinfo_me_copy_from_large_string_me(temp_me1, large_string1, 375);
		wsc.secondary_sip_registrar_server_address_pointer = large_string1;
	}

	if ((temp_me1 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[1].attr_value.data)) == NULL)
	{
		wsc.secondary_proxy_server_address_pointer = NULL;
	} else {
		memset(large_string2, 0x00, sizeof(large_string2));
		meinfo_me_copy_from_large_string_me(temp_me1, large_string2, 375);
		wsc.secondary_proxy_server_address_pointer = large_string2;
	}

	wsc.first_registrar_port = (unsigned short) attr_inst->er_attr_instance[2].attr_value.data;
	wsc.first_proxy_port = (unsigned short) attr_inst->er_attr_instance[3].attr_value.data;
	wsc.secondary_registrar_port = (unsigned short) attr_inst->er_attr_instance[4].attr_value.data;
	wsc.secondary_proxy_port = (unsigned short) attr_inst->er_attr_instance[5].attr_value.data;

	if ((temp_me1 = meinfo_me_get_by_meid(157, (unsigned short)attr_inst->er_attr_instance[6].attr_value.data)) == NULL)
	{
		wsc.current_sip_registrar_pointer = NULL;
	} else {
		memset(large_string3, 0x00, sizeof(large_string3));
		meinfo_me_copy_from_large_string_me(temp_me1, large_string3, 375);
		wsc.current_sip_registrar_pointer = large_string3;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((ret = er_group_hw_voip_wrapper_wri_sip_config(VOIP_HW_ACTION_SET, port_id, attr_mask, &wsc)) < 0)
		{
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value 
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//add
		if (er_group_hw_wri_sip_config(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}

		er_attr_group_util_release_attrinst(current_attr_inst);

		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		memset(&wsc, 0x00, sizeof(wsc));
		if ((ret = er_group_hw_voip_wrapper_wri_sip_config(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &wsc)) < 0)
		{
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			return -1;
		}
		break;
	default:
		dbprintf(LOG_ERR,"Unknow operator\n");
		;
	}
	return 0;
}

