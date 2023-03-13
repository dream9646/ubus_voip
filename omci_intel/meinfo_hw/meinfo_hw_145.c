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
 * File    : meinfo_hw_145.c
 *
 ******************************************************************/

#include <string.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "batchtab_cb.h"
#include "voipclient_cmd.h"
#include "voipclient_comm.h"

//9.9.10 Network_dial_plan_table
static int
meinfo_hw_145_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int ret, temp_int;
#ifdef OMCI_METAFILE_ENABLE
	char buf[256], *p;
	struct metacfg_t *kv_p;
#endif
	struct attr_value_t attr;

	if (me == NULL) {
		dbprintf(LOG_ERR, "input me error\n");
		return -1;
	}
	if (BATCHTAB_POTS_MKEY_HW_SYNC_DONE != (ret=batchtab_cb_pots_is_hw_sync())) {
		dbprintf(LOG_NOTICE, "pots_mkey is not hw_sync (%d)\n", ret);
		return 0;
	}
	dbprintf_voip(LOG_INFO, "attr_mask[0](0x%02x) attr_mask[1](0x%02x)\n", attr_mask[0], attr_mask[1]);
	port_id = me->instance_num;
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		/* 1: Dial_plan_number (2byte,unsigned integer,R) */
		if (0 == (temp_int=voipclient_cmd_get_int(&temp_int, "digitmapnum %d", port_id))) {
			attr.data=(unsigned short)temp_int;
			meinfo_me_attr_set(me, 1, &attr, 0);
		} else {
			dbprintf_voip(LOG_ERR, "fvcli get digitmapnum failed port(%d)\n", port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		/* 2: Dial_plan_table_max_size (2byte,unsigned integer,RWS) */
		if (0 == (temp_int=voipclient_cmd_get_int(&temp_int, "digitmapmaxnum %d", port_id))) {
			attr.data=(unsigned short)temp_int;
			meinfo_me_attr_set(me, 2, &attr, 0);
		} else {
			dbprintf_voip(LOG_ERR, "fvcli get digitmapmaxnum failed port(%d)\n", port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) {
		/* 5: Dial_plan_format (1byte,enumeration,RWS) */
		if ((strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0) &&
		(omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX)) {
			/* Support Vendor-specific format (3) only. */
			/* E7 only uses value 3(Vendor-specific format).  */
			attr.data = (unsigned char)3;
			meinfo_me_attr_set(me, 5, &attr, 0);
		}
	}
#ifdef OMCI_METAFILE_ENABLE
	if (util_attr_mask_get_bit(attr_mask, 6)) {
		/* 6: Dial_plan_table (30byte,table,RW) */
		/* 6.0: P Dial_plan_id (8bit,unsigned integer) */
		/* 6.1:   Action (8bit,unsigned integer) */
		/* 6.2:   Dial_plan_token (224bit,string) */
	}
	if (0 == meinfo_hw_util_is_mkey_voip_changed(&kv_p)) {
		dbprintf_voip(LOG_INFO, "No diff VOIP metakeys, no need to refresh me(%d)\n", me->classid);
		return 0;
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		/* 3: Critical_dial_timeout (2byte,unsigned integer,RWS) */
		snprintf(buf, 63, "voip_ep%d_digit_start_timeout", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned short)util_atoi(p);
			meinfo_me_attr_set(me, 3, &attr, 0);
		} else {
			dbprintf_voip(LOG_ERR, "3:Critical_dial_timeout no found metakey[%s]! port(%d)\n", buf, port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		/* 4: Partial_dial_timeout (2byte,unsigned integer,RWS) */
		snprintf(buf, 63, "voip_ep%d_dialtone_timeout", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data = (unsigned short)util_atoi(p);
			meinfo_me_attr_set(me, 4, &attr, 0);
		} else {
			dbprintf_voip(LOG_ERR, "4:Partial_dial_timeout no found metakey[%s]! port(%d)\n", buf, port_id);
		}
	}
#endif
	return 0;
}

struct meinfo_hardware_t meinfo_hw_145 = {
	.get_hw		= meinfo_hw_145_get
};
