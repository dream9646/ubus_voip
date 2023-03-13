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
 * File    : meinfo_hw_58.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "batchtab_cb.h"

//9.9.6 Voice_service_profile
static int
meinfo_hw_58_get(struct me_t *me, unsigned char attr_mask[2])
{
#ifdef OMCI_METAFILE_ENABLE
	unsigned short port_id;
	int ret, temp_int;
	char buf[256], *p;
	struct attr_value_t attr;
	struct metacfg_t *kv_p;

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
		/* 1: Announcement_type (1byte,enumeration,RWS) */
		/* Support fast busy(0x03) only */
		attr.data = (unsigned char)3;
		meinfo_me_attr_set(me, 1, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		/* 2: Jitter_target (2byte,unsigned integer,RWS) */
		snprintf(buf, 127, "voip_ep%d_jb_type", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0)) &&
		    0 == util_atoi(p)) {
			snprintf(buf, 127, "voip_ep%d_fjb_normal_delay", port_id);
			if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
				temp_int = util_atoi(p);
			} else {
				temp_int = 0;
			}
		} else {
			temp_int = 0;
		}
		attr.data = (unsigned int)temp_int;
		meinfo_me_attr_set(me, 2, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		/* 3: Jitter_buffer_max (2byte,unsigned integer,RWS) */
		snprintf(buf, 127, "voip_ep%d_ajb_max_delay", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned int)util_atoi(p);
			meinfo_me_attr_set(me, 3, &attr, 1);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		/* 4: Echo_cancel_ind (1byte,enumeration,RWS) */
		snprintf(buf, 127, "voip_ep%d_aec_enable", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned char)util_atoi(p);
			meinfo_me_attr_set(me, 4, &attr, 1);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) {
		/* 5: PSTN_protocol_variant (2byte,unsigned integer,RWS) */
		snprintf(buf, 127, "voip_general_e164_country_code");
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned int)util_atoi(p);
			meinfo_me_attr_set(me, 5, &attr, 1);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 8)) {
		/* 8: Hook_flash_minimum_time (2byte,unsigned integer,RWS) */
		snprintf(buf, 127, "voip_ep%d_flashhook_timeout_lb", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned int)util_atoi(p);
			meinfo_me_attr_set(me, 8, &attr, 1);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 9)) {
		/* 9: Hook_flash_maximum_time (2byte,unsigned integer,RWS) */
		snprintf(buf, 127, "voip_ep%d_flashhook_timeout_ub", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned int)util_atoi(p);
			meinfo_me_attr_set(me, 9, &attr, 1);
		}
	}
#endif
	return 0;
}

struct meinfo_hardware_t meinfo_hw_58 = {
	.get_hw		= meinfo_hw_58_get
};
