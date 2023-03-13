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
 * File    : meinfo_hw_142.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "batchtab_cb.h"

//9.9.5 VoIP_media_profile
static int
meinfo_hw_142_get(struct me_t *me, unsigned char attr_mask[2])
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
		/* 1: Fax_mode (1byte,enumeration,RWS) */
		snprintf(buf, 127, "voip_ep%d_fax_params__dot__t38_fax_opt", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned int)util_atoi(p);
			dbprintf_voip(LOG_INFO, "1:Fax_mode[%s] port(%d)\n", p, port_id);
			meinfo_me_attr_set(me, 1, &attr, 0);
		} else {
			dbprintf_voip(LOG_ERR, "1:Fax_mode no found metakey[%s]! port(%d)\n", buf, port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 3) ||
	    util_attr_mask_get_bit(attr_mask, 6) ||
	    util_attr_mask_get_bit(attr_mask, 9) ||
	    util_attr_mask_get_bit(attr_mask, 12)) {
		snprintf(buf, 63, "voip_ep%d_codec_list_1", port_id);
		snprintf(buf+64, 63, "voip_ep%d_codec_list_4", port_id);
		if (util_attr_mask_get_bit(attr_mask, 3)) {
			/* 3: Codec_selection_1st_order (1byte,enumeration,RWS) */
			if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
				if (0 <= (temp_int=voip_hw_util_get_codec_id(p))) {
					attr.data=(unsigned char)temp_int;
					meinfo_me_attr_set(me, 3, &attr, 0);
				}
			} else {
				dbprintf_voip(LOG_ERR, "3:Codec_selection_1st_order metakey[%s] get value failed! port(%d)\n", buf, port_id);
			}
		}
		if (util_attr_mask_get_bit(attr_mask, 6)) {
			/* 6: Codec_selection_2nd_order (1byte,enumeration,RWS) */
			snprintf(buf, 63, "voip_ep%d_codec_list_2", port_id);
			if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
				if (0 <= (temp_int=voip_hw_util_get_codec_id(p))) {
					attr.data=(unsigned char)temp_int;
					meinfo_me_attr_set(me, 6, &attr, 0);
				}
			} else {
				dbprintf_voip(LOG_ERR, "6:Codec_selection_2nd_order metakey[%s] get value failed! port(%d)\n", buf, port_id);
			}
		}
		if (util_attr_mask_get_bit(attr_mask, 9)) {
			/* 9: Codec_selection_3rd_order (1byte,enumeration,RWS) */
			snprintf(buf, 63, "voip_ep%d_codec_list_3", port_id);
			if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
				if (0 <= (temp_int=voip_hw_util_get_codec_id(p))) {
					attr.data=(unsigned char)temp_int;
					meinfo_me_attr_set(me, 9, &attr, 0);
				}
			} else {
				dbprintf_voip(LOG_ERR, "9:Codec_selection_3rd_order metakey[%s] get value failed! port(%d)\n", buf, port_id);
			}
		}
		if (util_attr_mask_get_bit(attr_mask, 12)) {
			/* 12: Codec_selection_4th_order (1byte,enumeration,RWS) */
			snprintf(buf, 63, "voip_ep%d_codec_list_4", port_id);
			if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
				if (0 <= (temp_int=voip_hw_util_get_codec_id(p))) {
					attr.data=(unsigned char)temp_int;
					meinfo_me_attr_set(me, 12, &attr, 0);
				}
			} else {
				dbprintf_voip(LOG_ERR, "12:Codec_selection_4th_order metakey[%s] get value failed! port(%d)\n", buf, port_id);
			}
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 4) ||
	    util_attr_mask_get_bit(attr_mask, 7) ||
	    util_attr_mask_get_bit(attr_mask, 10) ||
	    util_attr_mask_get_bit(attr_mask, 13)) {
		snprintf(buf, 63, "voip_ep%d_ptime", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			temp_int = util_atoi(p);
			attr.data=(unsigned char)temp_int;
			if (util_attr_mask_get_bit(attr_mask, 4)) {
				/* 4: Packet_period_selection_1st_order (1byte,unsigned integer,RWS) [10 ~ 30] */
				meinfo_me_attr_set(me, 4, &attr, 0);
			}
			if (util_attr_mask_get_bit(attr_mask, 7)) {
				meinfo_me_attr_set(me, 7, &attr, 0);
			}
			if (util_attr_mask_get_bit(attr_mask, 10)) {
				meinfo_me_attr_set(me, 10, &attr, 0);
			}
			if (util_attr_mask_get_bit(attr_mask, 13)) {
				meinfo_me_attr_set(me, 13, &attr, 0);
			}
		} else {
			dbprintf_voip(LOG_ERR, "Packet_period_selection metakey[%s] get value failed! port(%d)\n", buf, port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 5) ||
	    util_attr_mask_get_bit(attr_mask, 8) ||
	    util_attr_mask_get_bit(attr_mask, 11) ||
	    util_attr_mask_get_bit(attr_mask, 14)) {
		snprintf(buf, 63, "voip_ep%d_vad_type", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			temp_int = util_atoi(p);
			attr.data=(unsigned char)temp_int;
			if (util_attr_mask_get_bit(attr_mask, 5)) {
				/* 5: Silence_suppression_1st_order (1byte,enumeration,RWS) */
				meinfo_me_attr_set(me, 5, &attr, 0);
			}
			if (util_attr_mask_get_bit(attr_mask, 8)) {
				meinfo_me_attr_set(me, 8, &attr, 0);
			}
			if (util_attr_mask_get_bit(attr_mask, 11)) {
				meinfo_me_attr_set(me, 11, &attr, 0);
			}
			if (util_attr_mask_get_bit(attr_mask, 14)) {
				meinfo_me_attr_set(me, 14, &attr, 0);
			}
		} else {
			dbprintf_voip(LOG_ERR, "Silence_suppression metakey[%s] get value failed! port(%d)\n", buf, port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 15)) {
		snprintf(buf, 63, "voip_ep%d_dtmf_type", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			temp_int = util_atoi(p);
			if (temp_int == 4) {
				attr.data = 2;
				meinfo_me_attr_set(me, 15, &attr, 0);
			} else if (temp_int == 3) {
				attr.data = 1;
				meinfo_me_attr_set(me, 15, &attr, 0);
			} else if (temp_int == 0) {
				attr.data = 0;
				meinfo_me_attr_set(me, 15, &attr, 0);
			} else {
				dbprintf_voip(LOG_ERR, "OOB_DTMF metakey[%s] value(%d) not valid\n", buf, temp_int);
			}
		} else {
			dbprintf_voip(LOG_ERR, "OOB_DTMF metakey[%s] get value failed! port(%d)\n", buf, port_id);
		}
	}
#endif
	return 0;
}

struct meinfo_hardware_t meinfo_hw_142 = {
	.get_hw		= meinfo_hw_142_get
};
