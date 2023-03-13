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
 * File    : er_group_hw_53_58_139_142.c
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
// 58 Voice_service_profile
// 139 VoIP_voice_CTP
// 142 VoIP_media_profile

//53@254,58@1,58@2,58@3,58@4,58@5,58@6,58@7,58@8,58@9,58@10,58@11,58@12,58@13,58@14
int
er_group_hw_voice_service_profile(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret;
	struct voip_hw_voice_service_profile_t vvsp;
	struct attr_table_header_t *table_header;
	struct attr_table_entry_t *table_entry_pos;
	struct voip_hw_tone_pattern_entry_t *tp_entry;
	struct voip_hw_tone_event_entry_t *te_entry;
	struct voip_hw_ringing_pattern_entry_t *rp_entry;
	struct voip_hw_ringing_event_entry_t *re_entry;
	unsigned short ls_meid;
	struct me_t *temp_me, *temp_me1;
	unsigned char *ls_ptr;
	struct voip_hw_network_address_t network_address;
	struct voip_hw_authentication_security_method_t authentication_security_method;
	unsigned char large_string[376];
	struct attr_value_t attr;
	struct er_attr_group_instance_t *current_attr_inst;

	if (attr_inst == NULL)
		return -1;

	dbprintf_voip(LOG_WARNING, "action:%d(%s), me=%d:0x%x, attr_mask=0x%02x%02x\n",
		action, er_group_hw_util_action_str(action), me->classid, me->meid, attr_mask[0], attr_mask[1]);

	port_id = attr_inst->er_attr_instance[0].attr_value.data;

	memset(&vvsp, 0x00, sizeof(vvsp));

	//announcement_type
	vvsp.announcement_type = (unsigned char)attr_inst->er_attr_instance[1].attr_value.data;

	//jitter target
	vvsp.jitter_target = (unsigned short)attr_inst->er_attr_instance[2].attr_value.data;

	//jitter buffer max
	vvsp.jitter_buffer_max = (unsigned short)attr_inst->er_attr_instance[3].attr_value.data;

	//echo cancel ind
	vvsp.echo_cancel_ind = (unsigned char)attr_inst->er_attr_instance[4].attr_value.data;

	//pstn protocol variant
	vvsp.pstn_protocol_variant = (unsigned short)attr_inst->er_attr_instance[5].attr_value.data;

	//dtmf digit levels
	vvsp.dtmf_digit_levels = (unsigned short)attr_inst->er_attr_instance[6].attr_value.data;

	//dtmf digit duration
	vvsp.dtmf_digit_duration = (unsigned short)attr_inst->er_attr_instance[7].attr_value.data;

	//hook flash minimum time
	vvsp.hook_flash_minimum_time = (unsigned short)attr_inst->er_attr_instance[8].attr_value.data;

	//hook flash maximum time
	vvsp.hook_flash_maximum_time = (unsigned short)attr_inst->er_attr_instance[9].attr_value.data;

	//tone pattern table
	INIT_LIST_HEAD(&vvsp.tone_pattern_list);
	if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[10].attr_value.ptr) != NULL) {
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node) {
			if (table_entry_pos->entry_data_ptr != NULL) {
				tp_entry = malloc_safe(sizeof(struct voip_hw_tone_pattern_entry_t));

				tp_entry->index = ((unsigned char *)table_entry_pos->entry_data_ptr)[0];
				tp_entry->tone_on = ((unsigned char *)table_entry_pos->entry_data_ptr)[1];
				tp_entry->frequency_1 = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*20, 16, 16);
				tp_entry->power_1 = ((unsigned char *)table_entry_pos->entry_data_ptr)[4];
				tp_entry->frequency_2 = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*20, 40, 16);
				tp_entry->power_2 = ((unsigned char *)table_entry_pos->entry_data_ptr)[7];
				tp_entry->frequency_3 = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*20, 64, 16);
				tp_entry->power_3 = ((unsigned char *)table_entry_pos->entry_data_ptr)[10];
				tp_entry->frequency_4 = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*20, 88, 16);
				tp_entry->power_4 = ((unsigned char *)table_entry_pos->entry_data_ptr)[13];
				tp_entry->modulation_frequency = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*20, 112, 16);
				tp_entry->modulation_power = ((unsigned char *)table_entry_pos->entry_data_ptr)[16];
				tp_entry->duration = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*20, 136, 16);
				tp_entry->next_entry = ((unsigned char *)table_entry_pos->entry_data_ptr)[19];

				list_add_tail(&tp_entry->tone_pattern_node, &vvsp.tone_pattern_list);
			}
		}
	} else {
		dbprintf(LOG_ERR, "attr value 10 was not table\n");
	}

	//tone event table
	INIT_LIST_HEAD(&vvsp.tone_event_list);
	if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[11].attr_value.ptr) != NULL) {
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node) {
			if (table_entry_pos->entry_data_ptr != NULL) {
				te_entry = malloc_safe(sizeof(struct voip_hw_tone_event_entry_t));

				te_entry->event = ((unsigned char *)table_entry_pos->entry_data_ptr)[0];
				te_entry->tone_pattern = ((unsigned char *)table_entry_pos->entry_data_ptr)[1];

				ls_meid = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*7, 16, 16);
				if ((temp_me = meinfo_me_get_by_meid(157, ls_meid)) == NULL) {
					te_entry->tone_file = NULL;
				} else {
					ls_ptr = malloc_safe(376);
					meinfo_me_copy_from_large_string_me(temp_me, ls_ptr, 375);
					te_entry->tone_file = ls_ptr;
				}

				te_entry->tone_file_repetitions = ((unsigned char *)table_entry_pos->entry_data_ptr)[4];
				te_entry->reserved = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*7, 40, 16);

				list_add_tail(&te_entry->tone_event_node, &vvsp.tone_event_list);
			}
		}
	} else {
		dbprintf(LOG_ERR, "attr value 11 was not table\n");
	}

	//ringing pattern table
	INIT_LIST_HEAD(&vvsp.ringing_pattern_list);
	if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[12].attr_value.ptr) != NULL) {
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node) {
			if (table_entry_pos->entry_data_ptr != NULL) {
				rp_entry = malloc_safe(sizeof(struct voip_hw_ringing_pattern_entry_t));

				rp_entry->index = ((unsigned char *)table_entry_pos->entry_data_ptr)[0];
				rp_entry->ringing_on = ((unsigned char *)table_entry_pos->entry_data_ptr)[1];
				rp_entry->duration = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*5, 16, 16);
				rp_entry->next_entry = ((unsigned char *)table_entry_pos->entry_data_ptr)[4];

				list_add_tail(&rp_entry->ringing_pattern_node, &vvsp.ringing_pattern_list);
			}
		}
	} else {
		dbprintf(LOG_ERR, "attr value 12 was not table\n");
	}

	//ringing event table
	INIT_LIST_HEAD(&vvsp.ringing_event_list);
	if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[13].attr_value.ptr) != NULL) {
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node) {
			if (table_entry_pos->entry_data_ptr != NULL) {
				re_entry = malloc_safe(sizeof(struct voip_hw_ringing_event_entry_t));

				re_entry->event = ((unsigned char *)table_entry_pos->entry_data_ptr)[0];
				re_entry->ringing_pattern = ((unsigned char *)table_entry_pos->entry_data_ptr)[1];

				ls_meid = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*7, 16, 16);
				if ((temp_me = meinfo_me_get_by_meid(157, ls_meid)) == NULL) {
					re_entry->ringing_file = NULL;
				} else {
					ls_ptr = malloc_safe(376);
					meinfo_me_copy_from_large_string_me(temp_me, ls_ptr, 375);
					re_entry->ringing_file = ls_ptr;
				}

				re_entry->ringing_file_repetitions = ((unsigned char *)table_entry_pos->entry_data_ptr)[4];

				ls_meid = (unsigned short) util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*7, 40, 16);
				if ((temp_me = meinfo_me_get_by_meid(157, ls_meid)) == NULL) {
					re_entry->ringing_text = NULL;
				} else {
					ls_ptr = malloc_safe(376);
					meinfo_me_copy_from_large_string_me(temp_me, ls_ptr, 375);
					re_entry->ringing_text = ls_ptr;
				}

				list_add_tail(&re_entry->ringing_event_node, &vvsp.ringing_event_list);
			}
		}
	} else {
		dbprintf(LOG_ERR, "attr value 11 was not table\n");
	}

	//Network specific extensions
	if ((temp_me = meinfo_me_get_by_meid(137, (unsigned short)attr_inst->er_attr_instance[14].attr_value.data)) == NULL) {
		vvsp.network_specific_extensions = NULL;
	} else {
		memset(&network_address, 0x00, sizeof(network_address));

		//security_pointer
		meinfo_me_attr_get(temp_me, 1, &attr);

		if ((temp_me1 = meinfo_me_get_by_meid(148, (unsigned short)attr.data)) == NULL) {
			network_address.security_pointer = NULL;
		} else {
			memset(&authentication_security_method, 0x00, sizeof(authentication_security_method));

			//caliddation scheme
			meinfo_me_attr_get(temp_me1, 1, &attr);
			authentication_security_method.validation_scheme = (unsigned char) attr.data;

			//username 1
			attr.ptr = authentication_security_method.username_1;
			meinfo_me_attr_get(temp_me1, 2, &attr);

			//password
			attr.ptr = authentication_security_method.password;
			meinfo_me_attr_get(temp_me1, 3, &attr);

			//realm
			attr.ptr = authentication_security_method.realm;
			meinfo_me_attr_get(temp_me1, 4, &attr);

			//username 2
			attr.ptr = authentication_security_method.username_2;
			meinfo_me_attr_get(temp_me1, 5, &attr);

			network_address.security_pointer = &authentication_security_method;
		}

		//address_pointer
		meinfo_me_attr_get(temp_me, 2, &attr);

		if ((temp_me1 = meinfo_me_get_by_meid(157, (unsigned short)attr.data)) == NULL) {
			network_address.address_pointer = NULL;
		} else {
			memset(large_string, 0x00, sizeof(large_string));
			meinfo_me_copy_from_large_string_me(temp_me1, large_string, 375);
			network_address.address_pointer = large_string;
		}

		vvsp.network_specific_extensions = &network_address;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		er_attr_group_util_get_attr_mask_by_classid(attr_inst, 58, attr_mask);
	case ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE:
		if ((ret = er_group_hw_voip_wrapper_voice_service_profile(VOIP_HW_ACTION_SET, port_id, attr_mask, &vvsp)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			voip_hw_voice_service_profile_release_table(&vvsp);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			voip_hw_voice_service_profile_release_table(&vvsp);
			return -1;
		}
		//add
		if (er_group_hw_voice_service_profile(ER_ATTR_GROUP_HW_OP_ADD_BY_UPDATE, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			voip_hw_voice_service_profile_release_table(&vvsp);
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		batchtab_omci_update("pots_mkey");
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		voip_hw_voice_service_profile_release_table(&vvsp);
		memset(&vvsp, 0x00, sizeof(vvsp));
		INIT_LIST_HEAD(&vvsp.tone_pattern_list);
		INIT_LIST_HEAD(&vvsp.tone_event_list);
		INIT_LIST_HEAD(&vvsp.ringing_pattern_list);
		INIT_LIST_HEAD(&vvsp.ringing_event_list);
		if ((ret = er_group_hw_voip_wrapper_voice_service_profile(VOIP_HW_ACTION_RELEASE, port_id, attr_mask, &vvsp)) < 0) {
			dbprintf(LOG_ERR, "ret=%d\n", ret);
			voip_hw_voice_service_profile_release_table(&vvsp);
			return -1;
		}
		batchtab_omci_update("pots_mkey");
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
		;
	}
	voip_hw_voice_service_profile_release_table(&vvsp);
	return 0;
}
