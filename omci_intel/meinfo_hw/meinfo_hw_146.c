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
 * Module  : meinfo_hw
 * File    : meinfo_hw_146.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "batchtab_cb.h"

//9.9.8 VoIP_application_service_profile
static int
meinfo_hw_146_get(struct me_t *me, unsigned char attr_mask[2])
{
#ifdef OMCI_METAFILE_ENABLE
	int ret;
	unsigned char uc, bitmap_temp[4];
	unsigned short port_id;
	unsigned int ui;
	char buf[256], *p;
	struct attr_value_t attr;
	struct metacfg_t *kv_p;
	unsigned char calling_number;
	unsigned char calling_name;
	unsigned char cid_blocking;
	unsigned char anonymous_cid_blocking;
	unsigned char call_waiting;
	unsigned char caller_id_announcement;
	unsigned short threeway;
	unsigned short call_transfer;
	unsigned short do_not_disturb;
	unsigned short flash_on_emergency_service_call;
	unsigned short message_waiting_indication_special_dial_tone;
	unsigned short message_waiting_indication_visual_indication;
	unsigned char direct_connect_feature_enabled;
	unsigned char dial_tone_feature_delay_option;

	if (me == NULL) {
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}
	if (BATCHTAB_POTS_MKEY_HW_SYNC_DONE != (ret=batchtab_cb_pots_is_hw_sync())) {
		dbprintf(LOG_NOTICE, "pots_mkey is not hw_sync (%d)\n", ret);
		return 0;
	}
	if (0 == meinfo_hw_util_is_mkey_voip_changed(&kv_p)) {
		dbprintf_voip(LOG_INFO, "No diff VOIP metakeys, no need to refresh me(%d)\n", me->classid);
		return 0;
	}
	dbprintf_voip(LOG_INFO, "attr_mask[0](0x%02x) attr_mask[1](0x%02x)\n", attr_mask[0], attr_mask[1]);

	port_id = me->instance_num;
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		/* 1: CID_features (1byte,bit field,RWS) */
		uc = *(unsigned char*)meinfo_util_me_attr_ptr(me, 1);
		/* 1.0: Reserved (2bit,unsigned integer) */

		snprintf(buf, 127, "voip_ep%u_cid_blocking_option", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			ui = util_atoi(p);
			/* 1.1: Anonymous_CID_blocking (1bit,unsigned integer) */
			anonymous_cid_blocking = ((ui & 0x02)>>1);
			util_bitmap_set_value(&uc, 8, 2, 1, anonymous_cid_blocking);
			/* 1.4: CID_blocking (1bit,unsigned integer) */
			cid_blocking = (ui & 0x01);
			util_bitmap_set_value(&uc, 8, 5, 1, cid_blocking);
		}
		/* 1.2: CID_name (1bit,unsigned integer) */
		/* 1.3: CID_number (1bit,unsigned integer) */
		/* No support */

		/* 1.5: Calling_name (1bit,unsigned integer) */
		snprintf(buf, 127, "voip_ep%u_cid_data_msg_format", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			calling_name = util_atoi(p) & 0x01;
			util_bitmap_set_value(&uc, 8, 6, 1, calling_name);
		}
		/* 1.6: Calling_number (1bit,unsigned integer) */
		snprintf(buf, 127, "voip_ep%u_cid_type", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			calling_number = util_atoi(p) & 0x01;
			util_bitmap_set_value(&uc, 8, 7, 1, calling_number);
		}
		attr.data = 0;
		attr.ptr = &uc;
		meinfo_me_attr_set(me, 1, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		/* 2: Call_waiting_features (1byte,bit field,RWS) */
		uc = *(unsigned char*)meinfo_util_me_attr_ptr(me, 2);
		/* 2.0: Reserved (6bit,unsigned integer) */

		snprintf(buf, 127, "voip_ep%d_cid_type", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			ui = util_atoi(p);
			/* 2.1: Caller_ID_announcement (1bit,unsigned integer) */
			caller_id_announcement = ((ui & 0x02)>>1);
			util_bitmap_set_value(&uc, 8, 6, 1, caller_id_announcement);
		}
		snprintf(buf, 127, "voip_ep%d_call_waiting_option", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			ui = util_atoi(p);
			/* 2.2: Call_waiting (1bit,unsigned integer) */
			call_waiting = (ui & 0x01);
			util_bitmap_set_value(&uc, 8, 7, 1, call_waiting);
		}
		attr.data = 0;
		attr.ptr = &uc;
		meinfo_me_attr_set(me, 2, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		bzero(bitmap_temp, sizeof(bitmap_temp));
		/* 3: Call_progress_or_transfer_features (2byte,bit field,RWS) */
		/* 3.0: Reserved (8bit,unsigned integer) */
		/* 3.1: 6way (1bit,unsigned integer) */
		util_bitmap_set_value(bitmap_temp, 8*2, 8, 1, util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 3), 8*2, 8, 1));
		/* 3.2: Emergency_service_originating_hold (1bit,unsigned integer) */
		util_bitmap_set_value(bitmap_temp, 8*2, 9, 1, util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 3), 8*2, 9, 1));

		snprintf(buf, 127, "voip_ep%d_flashhook_option", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			ui = util_atoi(p);
			/* 3.3: Flash_on_emergency_service_call (1bit,unsigned integer) */
			if (ui & 0x04) { //0:disable, 1:enable
				flash_on_emergency_service_call = 1;
			} else {
				flash_on_emergency_service_call = 0;
			}
			util_bitmap_set_value(bitmap_temp, 8*2, 10, 1, flash_on_emergency_service_call);
		}
		snprintf(buf, 127, "voip_ep%d_dnd_option", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			ui = util_atoi(p);
			/* 3.4: Do_not_disturb (1bit,unsigned integer) */
			do_not_disturb = ui & 0x01;
			util_bitmap_set_value(bitmap_temp, 8*2, 11, 1, do_not_disturb);
		}
		/* 3.5: Call_park (1bit,unsigned integer) */
		util_bitmap_set_value(bitmap_temp, 8*2, 12, 1, util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 3), 8*2, 12, 1));
		/* 3.6: Call_hold (1bit,unsigned integer) */
		util_bitmap_set_value(bitmap_temp, 8*2, 13, 1, util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 3), 8*2, 13, 1));

		snprintf(buf, 127, "voip_ep%d_transfer_option", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			ui = util_atoi(p);
			/* 3.7: Call_transfer (1bit,unsigned integer) */
			call_transfer = ui & 0x01;
			util_bitmap_set_value(bitmap_temp, 16, 14, 1, call_transfer);
		}
		snprintf(buf, 127, "voip_ep%d_threeway_option", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			ui = util_atoi(p);
			/* 3.8: 3way (1bit,unsigned integer) */
			threeway = ui & 0x01;
			util_bitmap_set_value(bitmap_temp, 16, 15, 1, threeway);
		}
		attr.data = 0;
		attr.ptr = bitmap_temp;
		meinfo_me_attr_set(me, 3, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		bzero(bitmap_temp, sizeof(bitmap_temp));
		/* 4: Call_presentation_features (2byte,bit field,RWS) */
		/* 4.0: Reserved (12bit,unsigned integer) */
		/* 4.1: Call_forwarding_indication (1bit,unsigned integer) */
		util_bitmap_set_value(bitmap_temp, 8*2, 12, 1, util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 4), 8*2, 12, 1));

		snprintf(buf, 127, "voip_ep%d_mwi_type", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			ui = util_atoi(p);
			/* 4.2: Message_waiting_indication_visual_indication (1bit,unsigned integer) */
			if (ui & 0x04) { //0:disable, 1:enable
				message_waiting_indication_visual_indication = 1;
			} else {
				message_waiting_indication_visual_indication = 0;
			}
			util_bitmap_set_value(bitmap_temp, 8*2, 13, 1, message_waiting_indication_visual_indication);
			/* 4.3: Message_waiting_indication_special_dial_tone (1bit,unsigned integer) */
			if (ui & 0x01) { //0:disable, 1:enable
				message_waiting_indication_special_dial_tone = 1;
			} else {
				message_waiting_indication_special_dial_tone = 0;
			}
			util_bitmap_set_value(bitmap_temp, 8*2, 14, 1, message_waiting_indication_special_dial_tone);
		}
		/* 4.4: Message_waiting_indication_splash_ring (1bit,unsigned integer) */
		util_bitmap_set_value(bitmap_temp, 8*2, 15, 1, util_bitmap_get_value(meinfo_util_me_attr_ptr(me, 4), 8*2, 15, 1));
		attr.data = 0;
		attr.ptr = bitmap_temp;
		meinfo_me_attr_set(me, 4, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) {
		/* 5: Direct_connect_feature (1byte,bit field,RWS) */
		uc = *(unsigned char*)meinfo_util_me_attr_ptr(me, 5);
		/* 5.0: Reserved (6bit,unsigned integer) */

		snprintf(buf, 127, "voip_ep%d_hotline_option", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			ui = util_atoi(p);
			/* 5.1: Dial_tone_feature_delay_option (1bit,unsigned integer) */
			/* 5.2: Direct_connect_feature_enable (1bit,unsigned integer) */
			if (ui == 0) {
				direct_connect_feature_enabled = 0;
				dial_tone_feature_delay_option = 0;
			} else if (ui == 1) {
				direct_connect_feature_enabled = 1;
				dial_tone_feature_delay_option = 0;
			} else if (ui == 2) {
				direct_connect_feature_enabled = 1;
				dial_tone_feature_delay_option = 1;
			} else {
				goto attr_9;
			}
			util_bitmap_set_value(&uc, 8, 6, 1, dial_tone_feature_delay_option);
			util_bitmap_set_value(&uc, 8, 7, 1, direct_connect_feature_enabled);
		}
		attr.data = 0;
		attr.ptr = &uc;
		meinfo_me_attr_set(me, 5, &attr, 1);
	}
attr_9:
	if (util_attr_mask_get_bit(attr_mask, 9)) {
		/* 9: Dial_tone_feature_delay_Warmline_timer (2byte,unsigned integer,RW) */
		snprintf(buf, 127, "voip_ep%d_hotline_delay_time", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned int) util_atoi(p);
			meinfo_me_attr_set(me, 9, &attr, 1);
		}
	}
#endif

	return 0;
}

struct meinfo_hardware_t meinfo_hw_146 = {
	.get_hw		= meinfo_hw_146_get
};
