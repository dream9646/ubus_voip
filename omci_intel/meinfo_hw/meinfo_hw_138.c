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
 * File    : meinfo_hw_138.c
 *
 ******************************************************************/

#include <string.h>
#include <time.h>

#include "voip_hw.h"
#include "util.h"
#include "meinfo_hw_util.h"
#include "er_group_hw.h"
#include "batchtab_cb.h"

//9.9.18 VoIP_config_data
static int
meinfo_hw_138_get(struct me_t *me, unsigned char attr_mask[2])
{
	int ret, temp_int;
#ifdef OMCI_METAFILE_ENABLE
	char *p;
#endif
	unsigned char uc, bitmap_temp[4];
	struct attr_value_t attr;
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t *kv_p;
#endif
	unsigned short port_id;

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
		/* 1: Available_signalling_protocols (1byte,bit field,R) */
		/* 1.0: Reserved (5bit,unsigned integer) */
		/* 1.1: MGCP (1bit,unsigned integer) */
		/* 1.2: H248 (1bit,unsigned integer) */
		/* 1.3: SIP (1bit,unsigned integer) */
		util_bitmap_set_value(&uc, 8, 7, 1, 1);
		attr.data = 0;
		attr.ptr = &uc;
		meinfo_me_attr_set(me, 1, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		/* 2: Signalling_protocol_used (1byte,enumeration,RW) */
		/* Signalling protocol used: This attribute specifies the VoIP signalling protocol to use. Only one type of protocol is allowed at a time. Valid values are: */
		/* 0 None */
		/* 1 SIP */
		/* 2 ITU-T H.248 */
		/* 3 MGCP */
		/* 0xFF Selected by non-OMCI management interface */
		/* (R, W) (mandatory) (1 byte) */
		attr.data = 1;
		meinfo_me_attr_set(me, 2, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		/* 3: Available_VoIP_configuration_methods (4byte,bit field,R) */
		/* 3.0: Proprietary (8bit,unsigned integer) */
		/* 3.1: Reserved (20bit,unsigned integer) */
		/* 3.2: Sipping_config_framework (1bit,unsigned integer) */
		/* 3.3: TR069 (1bit,unsigned integer) */
		util_bitmap_set_value(bitmap_temp, 8*4, 29, 1, 1);
		/* 3.4: Configuration_file (1bit,unsigned integer) */
		util_bitmap_set_value(bitmap_temp, 8*4, 30, 1, 1);
		/* 3.5: OMCI (1bit,unsigned integer) */
		util_bitmap_set_value(bitmap_temp, 8*4, 31, 1, 1);
		attr.ptr = bitmap_temp;
		meinfo_me_attr_set(me, 3, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		/* 4: VoIP_configuration_method_used (1byte,unsigned integer,RW) */
		/* 0 Do not configure – ONU default */
		/* 1 OMCI */
		/* 2 Configuration file retrieval */
		/* 3 Broadband Forum TR-069 */
		/* 4 IETF sipping config framework */
#ifdef OMCI_METAFILE_ENABLE
		if (0 == meinfo_hw_util_is_mkey_voip_changed(&kv_p)) {
			dbprintf_voip(LOG_INFO, "No diff VOIP metakeys, no need to refresh me(%d)\n", me->classid);
			return 0;
		}
#endif
		attr.data = 1;
#ifdef OMCI_METAFILE_ENABLE
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, "voip_general_sip_mode", 0))) {
			if (1 == util_atoi(p)) {
				attr.data = 241;
			}
		}
#endif
		meinfo_me_attr_set(me, 4, &attr, 1);
	}
	if (util_attr_mask_get_bit(attr_mask, 6)) {
		if (0 <= (temp_int=meinfo_hw_util_get_omci_by_fvcli_epregstate(port_id))) {
			attr.data=(unsigned char)temp_int;
			meinfo_me_attr_set(me, 6, &attr, 1);
			dbprintf_voip(LOG_INFO, "SIP_status(%u) fvcli[epregstate %d]\n", attr.data, port_id);
		} else {
			dbprintf_voip(LOG_ERR, "fvcli[epregstate %d] failed\n", port_id);
		}
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_138 = {
	.get_hw		= meinfo_hw_138_get
};
