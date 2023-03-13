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
 * File    : meinfo_hw_143.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "batchtab_cb.h"

//9.9.7 RTP_profile_data
static int 
meinfo_hw_143_get(struct me_t *me, unsigned char attr_mask[2])
{
#ifdef OMCI_METAFILE_ENABLE
	int ret;
	char *p;
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
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		/* 1: Local_port_min */
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, "voip_general_rtp_port__dot__base", 0))) {
			attr.data=util_atoi(p);
			meinfo_me_attr_set(me, 1, &attr, 0);
			dbprintf_voip(LOG_INFO, "Local_port_min(%d) mkey[voip_general_rtp_port__dot__base]\n", attr.data);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		/* 2: Local_port_max */
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, "voip_general_rtp_port__dot__limit", 0))) {
			attr.data=util_atoi(p);
			meinfo_me_attr_set(me, 2, &attr, 0);
			dbprintf_voip(LOG_INFO, "Local_port_max(%d) mkey[voip_general_rtp_port__dot__limit]\n", attr.data);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		/* 3: DSCP_mark */
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, "voip_general_media_tos", 0))) {
			attr.data=(util_atoi(p)>>2);
			meinfo_me_attr_set(me, 3, &attr, 0);
			dbprintf_voip(LOG_INFO, "DSCP_mark(%d)(0x%x) mkey[voip_general_media_tos]\n", attr.data, attr.data);
		}
	}
#endif
	return 0;
}

struct meinfo_hardware_t meinfo_hw_143 = {
	.get_hw		= meinfo_hw_143_get
};
