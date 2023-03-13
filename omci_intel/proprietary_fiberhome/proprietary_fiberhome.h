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
 * File    : proprietary_fiberhome.h
 *
 ******************************************************************/

#ifndef __PROPRIETARY_FIBERHOME_H__
#define __PROPRIETARY_FIBERHOME_H__

#ifndef X86
#include "voip_hw.h"
#include "omciutil_vlan.h"
#include "er_group.h"
#endif

/* me_related_047.c */
int me_related_047_65285(struct me_t *me1, struct me_t *me2);

/* me_related_310.c */
int me_related_310_65531(struct me_t *me1, struct me_t *me2);

/* me_related_311.c */
int me_related_311_65311(struct me_t *me1, struct me_t *me2);

/* me_related_65285.c */
int me_related_65285_047(struct me_t *me1, struct me_t *me2);

/* me_related_65288.c */
int me_related_65288_65290(struct me_t *me1, struct me_t *me2);

/* me_related_65290.c */
int me_related_65290_65288(struct me_t *me1, struct me_t *me2);

/* me_related_65416.c */
int me_related_65416_053(struct me_t *me1, struct me_t *me2);

/* me_related_65418.c */
int me_related_65418_053(struct me_t *me1, struct me_t *me2);

/* me_related_65418.c */
int me_related_65426_053(struct me_t *me1, struct me_t *me2);

#ifndef X86
/* er_group_hw_47_310_65531.c */
int er_group_hw_l2s_tags_create_add01_multicast_65531(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65285.c */
int er_group_hw_storm_rate_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2], unsigned char storm_type);
int er_group_hw_broadcast_rate_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_multicast_rate_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_unknown_rate_limit(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65288_65290.c */
int er_group_hw_omci_me_sw_port_acl_rule(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65298.c */
int er_group_hw_mac_land_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_ip_land_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_tcp_blat_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_udp_blat_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_tcp_null_scan_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_xmas_scan_attack(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_tcp_syn_fin_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_tcp_syn_err_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_tcp_short_hdr_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_tcp_frag_err_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_icmpv4_frag_attack_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_icmpv6_frag_attack_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_icmpv4_longping_attack_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_icmpv6_longping_attack_alarm(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65282.c */
int er_group_hw_loopcheck_status(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_loopcheck_interval(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65299.c */
int er_group_hw_mac_aging_time(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65307.c */
int er_group_hw_switchport_upstream_rate_control(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);
int er_group_hw_switchport_downstream_rate_control(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65410.c */
int er_group_hw_wri_rtp_resource_name(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65412.c */
int er_group_hw_wri_rfc2833_config_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65413.c */
int er_group_hw_wri_keepalive_config_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65414.c */
int er_group_hw_wri_pos_config_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65415.c */
int er_group_hw_wri_ipt_config_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65416.c */
int er_group_hw_wri_mg_port_register_control(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65417.c */
int er_group_hw_wri_digit_map_match_mode(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65418.c */
int er_group_hw_wri_fax_config_data(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65419.c */
int er_group_hw_wri_sip_config(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65425.c */
int er_group_hw_wri_ngn_resource_info(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65426.c */
int er_group_hw_wri_ngn_port_statics(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65333.c */
int er_group_hw_onu_pon_limit_speed(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65420.c */
int er_group_hw_wri_softswitch_progile_config(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_65349.c */
int er_group_hw_remote_telnet_session(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2]);

/* er_group_hw_voip_wrapper.c */
int er_group_hw_voip_wrapper_wri_rtp_resource_name(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rtp_resource_name_t *parm);
int er_group_hw_voip_wrapper_wri_rfc2833_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_rfc2833_config_data_t *parm);
int er_group_hw_voip_wrapper_wri_keepalive_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_keepalive_config_data_t *parm);
int er_group_hw_voip_wrapper_wri_pos_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_pos_config_data_t *parm);
int er_group_hw_voip_wrapper_wri_ipt_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ipt_config_data_t *parm);
int er_group_hw_voip_wrapper_wri_mg_port_register_control(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_mg_port_register_control_t *parm);
int er_group_hw_voip_wrapper_wri_digit_map_match_mode(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_digit_map_match_mode_t *parm);
int er_group_hw_voip_wrapper_wri_fax_config_data(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_fax_config_data_t *parm);
int er_group_hw_voip_wrapper_wri_ngn_resource_info(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_resource_info_t *parm);
int er_group_hw_voip_wrapper_wri_ngn_port_statics(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_ngn_port_statics_t *parm);
int er_group_hw_voip_wrapper_wri_sip_config(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_sip_config_t *parm);
int er_group_hw_voip_wrapper_wri_softswitch_progile_config(enum voip_hw_action_t action, int port, unsigned char mask[2], struct voip_hw_wri_softswitch_progile_config_t *parm);

#endif

/* proprietary_fiberhome.c */
int proprietary_fiberhome(void);

#endif
