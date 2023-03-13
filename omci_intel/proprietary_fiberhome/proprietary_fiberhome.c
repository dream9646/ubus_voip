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
 * File    : proprietary_fiberhome.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"
#include "meinfo_cb.h"
#include "er_group.h"
#ifndef X86
#include "meinfo_hw.h"
#include "er_group_hw.h"
#endif
#include "util.h"
#include "proprietary_fiberhome.h"

extern struct meinfo_callback_t meinfo_cb_fiberhome_65286;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65297;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65307;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65345;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65347;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65348;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65410;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65412;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65413;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65414;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65415;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65416;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65417;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65418;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65419;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65420;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65425;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65426;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65530;
extern struct meinfo_callback_t meinfo_cb_fiberhome_65531;

#ifndef X86
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65282;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65285;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65298;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65299;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65307;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65311;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65346;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65410;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65412;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65413;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65414;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65415;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65416;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65417;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65418;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65419;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65420;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65425;
extern struct meinfo_hardware_t meinfo_hw_fiberhome_65426;
#endif

static int
proprietary_fiberhome_me_related_init(void)
{
	me_related_add(ME_RELATED_FUNC_SPEC, 47, 65285, me_related_047_65285);
	me_related_add(ME_RELATED_FUNC_SPEC, 310, 65531, me_related_310_65531);
	me_related_add(ME_RELATED_FUNC_SPEC, 311, 65311, me_related_311_65311);
	me_related_add(ME_RELATED_FUNC_SPEC, 65285, 47, me_related_047_65285);
	me_related_add(ME_RELATED_FUNC_SPEC, 65288, 65290, me_related_65288_65290);
	me_related_add(ME_RELATED_FUNC_SPEC, 65290, 65288, me_related_65290_65288);
	me_related_add(ME_RELATED_FUNC_SPEC, 65416, 53, me_related_65416_053);
	me_related_add(ME_RELATED_FUNC_SPEC, 65418, 53, me_related_65418_053);
	me_related_add(ME_RELATED_FUNC_SPEC, 65426, 53, me_related_65426_053);
	
	return 0;
}

static int
proprietary_fiberhome_meinfo_cb_init(void)
{
	meinfo_cb_register(65286, &meinfo_cb_fiberhome_65286);
	meinfo_cb_register(65297, &meinfo_cb_fiberhome_65297);
	meinfo_cb_register(65307, &meinfo_cb_fiberhome_65307);
	meinfo_cb_register(65345, &meinfo_cb_fiberhome_65345);
	meinfo_cb_register(65347, &meinfo_cb_fiberhome_65347);
	meinfo_cb_register(65348, &meinfo_cb_fiberhome_65348);
	meinfo_cb_register(65410, &meinfo_cb_fiberhome_65410);
	meinfo_cb_register(65412, &meinfo_cb_fiberhome_65412);
	meinfo_cb_register(65413, &meinfo_cb_fiberhome_65413);
	meinfo_cb_register(65414, &meinfo_cb_fiberhome_65414);
	meinfo_cb_register(65415, &meinfo_cb_fiberhome_65415);
	meinfo_cb_register(65416, &meinfo_cb_fiberhome_65416);
	meinfo_cb_register(65417, &meinfo_cb_fiberhome_65417);
	meinfo_cb_register(65418, &meinfo_cb_fiberhome_65418);
	meinfo_cb_register(65419, &meinfo_cb_fiberhome_65419);
	meinfo_cb_register(65420, &meinfo_cb_fiberhome_65420);
	meinfo_cb_register(65425, &meinfo_cb_fiberhome_65425);
	meinfo_cb_register(65426, &meinfo_cb_fiberhome_65426);
	meinfo_cb_register(65530, &meinfo_cb_fiberhome_65530);
	meinfo_cb_register(65531, &meinfo_cb_fiberhome_65531);
	
	return 0;
}

#ifndef X86
static int
proprietary_fiberhome_meinfo_hw_init(void)
{
	meinfo_hw_register(65282, &meinfo_hw_fiberhome_65282);	
	meinfo_hw_register(65285, &meinfo_hw_fiberhome_65285);	
	meinfo_hw_register(65298, &meinfo_hw_fiberhome_65298);
	meinfo_hw_register(65299, &meinfo_hw_fiberhome_65299);
	meinfo_hw_register(65307, &meinfo_hw_fiberhome_65307);	
	meinfo_hw_register(65311, &meinfo_hw_fiberhome_65311);	
	meinfo_hw_register(65346, &meinfo_hw_fiberhome_65346);
	meinfo_hw_register(65410, &meinfo_hw_fiberhome_65410);
	meinfo_hw_register(65412, &meinfo_hw_fiberhome_65412);
	meinfo_hw_register(65413, &meinfo_hw_fiberhome_65413);
	meinfo_hw_register(65414, &meinfo_hw_fiberhome_65414);
	meinfo_hw_register(65415, &meinfo_hw_fiberhome_65415);
	meinfo_hw_register(65416, &meinfo_hw_fiberhome_65416);
	meinfo_hw_register(65417, &meinfo_hw_fiberhome_65417);
	meinfo_hw_register(65418, &meinfo_hw_fiberhome_65418);
	meinfo_hw_register(65419, &meinfo_hw_fiberhome_65419);
	meinfo_hw_register(65420, &meinfo_hw_fiberhome_65420);
	meinfo_hw_register(65425, &meinfo_hw_fiberhome_65425);
	meinfo_hw_register(65426, &meinfo_hw_fiberhome_65426);
	
	return 0;
}

static int 
proprietary_fiberhome_er_group_hw_init(void)
{
	/* er_group_hw_47_310_65531.c */
	er_attr_group_hw_add("l2s_tags_create_add01_multicast_65531", er_group_hw_l2s_tags_create_add01_multicast_65531);

	/* er_group_hw_65285.c */
	er_attr_group_hw_add("broadcast_rate_limit", er_group_hw_broadcast_rate_limit);
	er_attr_group_hw_add("multicast_rate_limit", er_group_hw_multicast_rate_limit);
	er_attr_group_hw_add("unknown_rate_limit", er_group_hw_unknown_rate_limit);

	/* er_group_hw_65288.c */
	er_attr_group_hw_add("omci_me_sw_port_acl_rule", er_group_hw_omci_me_sw_port_acl_rule);

	/* er_group_hw_65298.c */
	er_attr_group_hw_add("mac_land_attack_alarm", er_group_hw_mac_land_attack);
	er_attr_group_hw_add("ip_land_attack_alarm", er_group_hw_ip_land_attack);
	er_attr_group_hw_add("tcp_blat_attack_alarm", er_group_hw_tcp_blat_attack);
	er_attr_group_hw_add("udp_blat_attack_alarm", er_group_hw_udp_blat_attack);
	er_attr_group_hw_add("tcp_null_scan_attack_alarm", er_group_hw_tcp_null_scan_attack);
	er_attr_group_hw_add("tcp_xmas_scan_attack_alarm", er_group_hw_xmas_scan_attack);
	er_attr_group_hw_add("tcp_syn_fin_alarm", er_group_hw_tcp_syn_fin_alarm);
	er_attr_group_hw_add("tcp_syn_err_alarm", er_group_hw_tcp_syn_err_alarm);
	er_attr_group_hw_add("tcp_short_hdr_alarm", er_group_hw_tcp_short_hdr_alarm);
	er_attr_group_hw_add("tcp_frag_err_alarm", er_group_hw_tcp_frag_err_alarm);
	er_attr_group_hw_add("icmpv4_frag_attack_alarm", er_group_hw_icmpv4_frag_attack_alarm);
	er_attr_group_hw_add("icmpv6_frag_attack_alarm", er_group_hw_icmpv6_frag_attack_alarm);
	er_attr_group_hw_add("icmpv4_longping_attack_alarm", er_group_hw_icmpv4_longping_attack_alarm);
	er_attr_group_hw_add("icmpv6_longping_attack_alarm", er_group_hw_icmpv6_longping_attack_alarm);

	/* er_group_hw_65282.c */
	er_attr_group_hw_add("loopcheck_status", er_group_hw_loopcheck_status);
	er_attr_group_hw_add("loopcheck_interval", er_group_hw_loopcheck_interval);

	/* er_group_hw_65299.c */
	er_attr_group_hw_add("mac_aging_time", er_group_hw_mac_aging_time);

	/* er_group_hw_65307.c */
	er_attr_group_hw_add("switchport_upstream_rate_control", er_group_hw_switchport_upstream_rate_control);
	er_attr_group_hw_add("switchport_downstream_rate_control", er_group_hw_switchport_downstream_rate_control);
	
	/* er_group_hw_65410.c */
	er_attr_group_hw_add("wri_rtp_resource_name", er_group_hw_wri_rtp_resource_name);

	/* er_group_hw_65412.c */
	er_attr_group_hw_add("wri_rfc2833_config_data", er_group_hw_wri_rfc2833_config_data);

	/* er_group_hw_65413.c */
	er_attr_group_hw_add("wri_keepalive_config_data", er_group_hw_wri_keepalive_config_data);

	/* er_group_hw_65414.c */
	er_attr_group_hw_add("wri_pos_config_data", er_group_hw_wri_pos_config_data);

	/* er_group_hw_65415.c */
	er_attr_group_hw_add("wri_ipt_config_data", er_group_hw_wri_ipt_config_data);

	/* er_group_hw_65416.c */
	er_attr_group_hw_add("wri_mg_port_register_control", er_group_hw_wri_mg_port_register_control);

	/* er_group_hw_65417.c */
	er_attr_group_hw_add("wri_digit_map_match_mode", er_group_hw_wri_digit_map_match_mode);

	/* er_group_hw_65418.c */
	er_attr_group_hw_add("wri_fax_config_data", er_group_hw_wri_fax_config_data);

	/* er_group_hw_65419.c */
	er_attr_group_hw_add("wri_sip_config", er_group_hw_wri_sip_config);

	/* er_group_hw_65420.c */
	er_attr_group_hw_add("wri_softswitch_progile_config", er_group_hw_wri_softswitch_progile_config);

	/* er_group_hw_65425.c */
	er_attr_group_hw_add("wri_ngn_resource_info", er_group_hw_wri_ngn_resource_info);

	/* er_group_hw_65426.c */
	er_attr_group_hw_add("wri_ngn_port_statics", er_group_hw_wri_ngn_port_statics);

	/* er_group_hw_65333.c */
	er_attr_group_hw_add("onu_pon_limit_speed", er_group_hw_onu_pon_limit_speed);

	/* er_group_hw_65349.c */
	er_attr_group_hw_add("remote_telnet_session", er_group_hw_remote_telnet_session);

	return 0;
}
#endif

int 
proprietary_fiberhome(void)
{
	proprietary_fiberhome_me_related_init();
	proprietary_fiberhome_meinfo_cb_init();
#ifndef X86
	proprietary_fiberhome_meinfo_hw_init();
	proprietary_fiberhome_er_group_hw_init();
#endif

	return 0;
}
