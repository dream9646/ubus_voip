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
 * File    : meinfo_hw_150.c
 *
 ******************************************************************/

#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

#include "util.h"
#include "util_inet.h"
#include "meinfo_hw_util.h"
#include "alarmtask.h"
#include "er_group_hw.h"
#include "batchtab_cb.h"

static int
meinfo_hw_150_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
#ifdef OMCI_METAFILE_ENABLE
	unsigned int ipv4;
	char *p, buf[128];
#endif
	int ret, temp_int;
	struct attr_value_t attr;
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t *kv_p;
#endif

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
	if (util_attr_mask_get_bit(attr_mask, 9)) {
		/* 9: SIP_status (1byte,enumeration,R) */
		if (0 <= (temp_int=meinfo_hw_util_get_omci_by_fvcli_epregstate(port_id))) {
			attr.data=(unsigned char)temp_int;
			meinfo_me_attr_set(me, 9, &attr, 0);
			dbprintf_voip(LOG_INFO, "SIP_status(%u) fvcli[epregstate %d]\n", attr.data, port_id);
		} else {
			dbprintf_voip(LOG_INFO, "fvcli[epregstate %d] failed\n", port_id);
		}
	}

#ifdef OMCI_METAFILE_ENABLE
	if (0 == meinfo_hw_util_is_mkey_voip_changed(&kv_p)) {
		dbprintf_voip(LOG_INFO, "No diff VOIP metakeys, no need to refresh me(%d)\n", me->classid);
		return 0;
	}
	if (util_attr_mask_get_bit(attr_mask, 1)) {
		meinfo_hw_util_attr_sync_class_157(150, 1, me, port_id, kv_p, "voip_ep%u_sip_proxies_1");
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		meinfo_hw_util_attr_sync_class_157(150, 2, me, port_id, kv_p, "voip_ep%u_outbound_proxies_1");
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		/* 3: Primary_SIP_DNS */
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, "voip_general_dns_server_1", 0))) {
			util_inet_pton(AF_INET, p, &ipv4);
			attr.data=(unsigned int)ipv4;
			meinfo_me_attr_set(me, 3, &attr, 0);
			dbprintf_voip(LOG_INFO, "Primary_SIP_DNS(%u)(0x%08x)[%s] mkey[voip_general_dns_server_1]\n", (unsigned int)attr.data, (unsigned int)attr.data, p);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		/* 4: Secondary_SIP_DNS */
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, "voip_general_dns_server_2", 0))) {
			util_inet_pton(AF_INET, p, &ipv4);
			attr.data=(unsigned int)ipv4;
			meinfo_me_attr_set(me, 4, &attr, 0);
			dbprintf_voip(LOG_INFO, "Secondary_SIP_DNS(%u)(0x%08x)[%s] mkey[voip_general_dns_server_2]\n", (unsigned int)attr.data, (unsigned int)attr.data, p);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 6)) {
		/* 6: SIP_reg_exp_time */
		snprintf(buf, 127, "voip_ep%u_sip_registration_timeout", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data=(unsigned int)util_atoi(p);
			meinfo_me_attr_set(me, 6, &attr, 0);
			dbprintf_voip(LOG_INFO, "SIP_reg_exp_time(%u) mkey[%s]\n", attr.data, buf);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) {
		/* 7: SIP_rereg_head_start_time */
		snprintf(buf, 127, "voip_ep%u_sip_reregistration_head_start_time", port_id);
		if (NULL != (p=metacfg_adapter_config_get_value(kv_p, buf, 0))) {
			attr.data=(unsigned int)util_atoi(p);
			meinfo_me_attr_set(me, 7, &attr, 0);
			dbprintf_voip(LOG_INFO, "SIP_rereg_head_start_time(%u) mkey[%s]\n", attr.data, buf);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 8)) {
		meinfo_hw_util_attr_sync_class_157(150, 8, me, port_id, kv_p, "voip_ep%u_domain_name");
	}
	if (util_attr_mask_get_bit(attr_mask, 10)) {
		meinfo_hw_util_attr_sync_class_137(150, 10, me, port_id, kv_p, "voip_ep%u_sip_registrars_1");
	}
#endif

	return 0;
}

struct meinfo_hardware_t meinfo_hw_150 = {
	.get_hw		= meinfo_hw_150_get
};
