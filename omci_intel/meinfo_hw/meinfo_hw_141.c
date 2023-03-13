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
 * File    : meinfo_hw_141.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "voip_hw.h"
#include "meinfo_hw_util.h"
#include "batchtab_cb.h"
#include "voipclient_cmd.h"
#include "voipclient_comm.h"

//9.9.11 VoIP_line_status
static int
meinfo_hw_141_get(struct me_t *me, unsigned char attr_mask[2])
{
	unsigned short port_id;
	int l, ret, temp_int;
	struct attr_value_t attr;
	char buf[256], cmd[64];

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
		/* 1: Voip_codec_used (2byte,enumeration,R) */
		if (0 == (ret=voipclient_cmd_get_string(buf, 63, "currentcodec %d", port_id))) {
			if (0 <= (temp_int=voip_hw_util_get_codec_id(buf))) {
				attr.data=(unsigned short)temp_int;
				meinfo_me_attr_set(me, 1, &attr, 0);
			}
		} else {
			dbprintf_voip(LOG_ERR, "fvcli currentcodec failed port(%d)\n", port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 2)) {
		/* 2: Voip_voice_server_status (1byte,enumeration,R) */
		if (0 <= (temp_int=meinfo_hw_util_get_omci_by_fvcli_epregstate(port_id))) {
			attr.data=(unsigned char)temp_int;
			meinfo_me_attr_set(me, 2, &attr, 0);
			dbprintf_voip(LOG_INFO, "SIP_status(%u) fvcli[epregstate %d]\n", attr.data, port_id);
		} else {
			dbprintf_voip(LOG_ERR, "fvcli[epregstate %d] failed\n", port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 3)) {
		/* 3: Voip_port_session_type (1byte,enumeration,R) */
		if (0 == (ret=voipclient_cmd_get_int(&temp_int, "voipsessiontype %d", port_id))) {
			if (temp_int == 0) {
				ret = CLASS_141_ATTR_3_IDLE;
			} else if (temp_int == 1) {
				ret = CLASS_141_ATTR_3_2WAY;
			} else if (temp_int == 2) {
				ret = CLASS_141_ATTR_3_3WAY;
			} else if (temp_int == 3) {
				ret = CLASS_141_ATTR_3_FAX;
			} else if (temp_int == 4) {
				ret = CLASS_141_ATTR_3_2WAY;
			} else {
				ret = -1;
				dbprintf_voip(LOG_ERR, "Not support port session type (%d)\n", temp_int);
			}
			if (ret >= 0) {
				attr.data=(unsigned char)ret;
				meinfo_me_attr_set(me, 3, &attr, 0);
			}
		} else {
			dbprintf_voip(LOG_ERR, "fvcli voipsessiontype failed port(%d)\n", port_id);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 4)) {
		/* 4: Voip_call_1_packet_period (2byte,unsigned integer,R) */
		snprintf(cmd, 63, "get txpacketsize1 %d\n\n", port_id);
		buf[0]='\0';
		if (0 == (ret=voipclient_comm_cli(cmd, buf, 63)) &&
			buf[0] != '\n') {
			attr.data=(unsigned short)atoi(buf);
			meinfo_me_attr_set(me, 4, &attr, 0);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 5)) {
		/* 5: Voip_call_2_packet_period (2byte,unsigned integer,R) */
		snprintf(cmd, 63, "get txpacketsize2 %d\n\n", port_id);
		buf[0]='\0';
		if (0 == (ret=voipclient_comm_cli(cmd, buf, 63)) &&
			buf[0] != '\n') {
			attr.data=(unsigned short)atoi(buf);
			meinfo_me_attr_set(me, 5, &attr, 0);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 6)) {
		/* 6: Voip_call_1_dest_addr (25byte,string,R) */
		snprintf(cmd, 63, "get calldestaddr1 %d\n\n", port_id);
		buf[0]='\0';
		if (0 == (ret=voipclient_comm_cli(cmd, buf, 255)) &&
			buf[0] != '\n') {
			l=strlen(buf);
			attr.ptr = (char *)malloc_safe(l+1);
			snprintf(attr.ptr, l+1, "%s", buf);
			meinfo_me_attr_set(me, 6, &attr, 0);
		}
	}
	if (util_attr_mask_get_bit(attr_mask, 7)) {
		/* 7: Voip_call_2_dest_addr (25byte,string,R) */
		snprintf(cmd, 63, "get calldestaddr2 %d\n\n", port_id);
		buf[0]='\0';
		if (0 == (ret=voipclient_comm_cli(cmd, buf, 255)) &&
			buf[0] != '\n') {
			l=strlen(buf);
			attr.ptr = (char *)malloc_safe(l+1);
			snprintf(attr.ptr, l+1, "%s", buf);
			meinfo_me_attr_set(me, 7, &attr, 0);
		}
	}
	return 0;
}

struct meinfo_hardware_t meinfo_hw_141 = {
	.get_hw		= meinfo_hw_141_get
};
