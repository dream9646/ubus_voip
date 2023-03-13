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
 * Module  : proprietary_fiberhome
 * File    : meinfo_hw_65298.c
 *
 ******************************************************************/

#include <stdlib.h>

#include "meinfo_hw.h"
#include "util.h"
#include "regexp.h"

static int 
meinfo_hw_65298_get(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	char *data_ptr = NULL;

	struct regexp_context_t re_context;

	util_readcmd("/bin/cat /proc/defense",&data_ptr);
	attr.ptr=NULL;

	if (data_ptr==NULL)
		return MEINFO_ERROR_HW_ME_GET;

	regexp_prepare(&re_context, "mac_land_attack_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 1) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 1, &attr, 0);	
	}
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "ip_land_attack_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 2) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 2, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "tcp_blat_attack_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 3) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 3, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "udp_blat_attack_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 4) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 4, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "tcp_null_scan_attack_alarm=([0-1]).*" );
	if (util_attr_mask_get_bit(attr_mask, 5) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 5, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "tcp_xmas_scan_attack_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 6) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 6, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "tcp_syn_fin_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 7) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 7, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "tcp_syn_err_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 8) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 8, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "tcp_short_hdr_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 9) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 9, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "tcp_frag_err_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 10) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 10, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "icmpv4_frag_attack_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 11) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 11, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "icmpv6_frag_attack_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 12) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 12, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "icmpv4_longping_attack_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 13) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 13, &attr, 0);	
	}	
	regexp_release(&re_context);	
	
	regexp_prepare(&re_context, "icmpv6_longping_attack_alarm=([0-1]).*");
	if (util_attr_mask_get_bit(attr_mask, 14) == 1
			&& regexp_match(&re_context, data_ptr)) {
		attr.data =util_atoll(regexp_match_str(&re_context, 1));
		meinfo_me_attr_set(me, 14, &attr, 0);	
	}	
	regexp_release(&re_context);	

	free_safe(data_ptr);
	return 0;
}

struct meinfo_hardware_t meinfo_hw_fiberhome_65298 = {
	.get_hw		= meinfo_hw_65298_get,
};
