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
 * File    : meinfo_hw_153.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "batchtab_cb.h"

//9.9.2 SIP_user_data
static int 
meinfo_hw_153_get(struct me_t *me, unsigned char attr_mask[2])
{
#ifdef OMCI_METAFILE_ENABLE
	int ret;
	unsigned short port_id;
	char *p, buf[128];
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

	port_id = me->instance_num;
	dbprintf_voip(LOG_INFO, "attr_mask[0](0x%02x) attr_mask[1](0x%02x) port(%d)\n", attr_mask[0], attr_mask[1], port_id);

	if (util_attr_mask_get_bit(attr_mask, 2)) {
		/* 2: User_part_AOR (2byte,pointer,RWS) [classid 157] */
		meinfo_hw_util_attr_sync_class_157(153, 2, me, port_id, kv_p, "voip_ep%u_phone_number");
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		/* 3: SIP_display_name (25byte,string,RW) */
		snprintf(buf, 127, "voip_ep%u_display_name", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.ptr = strdup(p);
			dbprintf_voip(LOG_INFO, "3:SIP_display_name[%s] port(%d)\n", p, port_id);
			meinfo_me_attr_set(me, 3, &attr, 0);
		} else {
			dbprintf_voip(LOG_ERR, "3:SIP_display_name no found metakey[%s]! port(%d)\n", buf, port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		/* 4: Username/password (2byte,pointer,RWS) [classid 148] */
		meinfo_hw_util_attr_sync_class_148(153, 4, me, port_id, kv_p, "voip_ep%u_proxy_user_name", "voip_ep%u_proxy_password");
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) {
		/* 5: Voicemail_server_SIP_URI (2byte,pointer,RWS) [classid 137] */
		meinfo_hw_util_attr_sync_class_137(153, 5, me, port_id, kv_p, "voip_ep%u_mwi_server_sip_uri");
	}
	if (util_attr_mask_get_bit(attr_mask, 6)) {
		/* 6: Voicemail_subscription_expiration_time (4byte,unsigned integer,RWS) */
		snprintf(buf, 127, "voip_ep%u_mwi_subscribe_expire_time", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned int)util_atoi(p);
			dbprintf_voip(LOG_INFO, "6:Voicemail_subscription_expiration_time(%u) port(%d)\n", (unsigned int)attr.data, port_id);
			meinfo_me_attr_set(me, 6, &attr, 0);
		} else {
			dbprintf_voip(LOG_ERR, "6:Voicemail_subscription_expiration_time no found metakey[%s] port(%d)\n", buf, port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 11)) {
		/* 11: Release_timer (1byte,unsigned integer,RW) */
		snprintf(buf, 127, "voip_ep%u_release_timeout", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned int)util_atoi(p);
			dbprintf_voip(LOG_INFO, "11:Release_timer(%u) port(%d)\n", (unsigned int)attr.data, port_id);
			meinfo_me_attr_set(me, 11, &attr, 0);
		} else {
			dbprintf_voip(LOG_ERR, "11:Release_timer no found metakey[%s] port(%d)\n", buf, port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 12)) {
		/* 12: ROH_timer (1byte,unsigned integer,RW) */
		snprintf(buf, 127, "voip_ep%u_roh_timeout", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned int)util_atoi(p);
			dbprintf_voip(LOG_INFO, "12:ROH_timer(%u) port(%d)\n", (unsigned int)attr.data, port_id);
			meinfo_me_attr_set(me, 12, &attr, 0);
		} else {
			dbprintf_voip(LOG_ERR, "12:ROH_timer no found metakey[%s] port(%d)\n", buf, port_id);
		}
	}
#endif
	return 0;
}

struct meinfo_hardware_t meinfo_hw_153 = {
	.get_hw		= meinfo_hw_153_get
};

