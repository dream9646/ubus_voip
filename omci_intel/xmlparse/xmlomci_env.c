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
 * Module  : xmlparse
 * File    : xmlomci_env.c
 *
 ******************************************************************/

#include <string.h>

#include "xmlparse.h"
#include "xmlomci.h"
#include "util.h"
#include "cli.h"
#include "env.h"
#include "conv.h"

// func used for omcienv.xml ////////////////////////////////////////////////////////////////////

static int
xmlomci_env_add_tag(struct xmlparse_env_t *env, char *tag)
{
	if (strcmp(env->url, "/omcienv") == 0)
	{
			//omci_env_g.debug_level = 4;
	}

	return 0;
}

static int
xmlomci_env_add_attr(struct xmlparse_env_t *env, char *tag, char *attr, char *attrval)
{
	return 0;
}

static int
xmlomci_env_finish_attrs(struct xmlparse_env_t *env, char *tag)
{
	return 0;
}

static int
xmlomci_env_add_content(struct xmlparse_env_t *env, char *tag, char *s)
{
	dbprintf_xml(LOG_DEBUG, "env->url=%s, tag=%s, s=%s\n", env->url, tag, s);

#if 0
// global
	if (strcmp(env->url, "/omcienv/cli_ip") == 0) {
		omci_env_g.cli_ip=(strcmp(s, "0") == 0)?0:ntohl(inet_addr(s)); 
	} else if (strcmp(env->url, "/omcienv/cli_port") == 0) {
		omci_env_g.cli_port = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/transport_type") == 0) {
		omci_env_g.transport_type = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/etc_omci_path") == 0) {
		strncpy(omci_env_g.etc_omci_path, s, ENV_SIZE_PATH);
	} else if (strcmp(env->url, "/omcienv/etc_omci_path2") == 0) {
		strncpy(omci_env_g.etc_omci_path2, s, ENV_SIZE_PATH);
	} else if (strcmp(env->url, "/omcienv/start_cmd_file") == 0) {
		strncpy(omci_env_g.start_cmd_file, s, ENV_SIZE_PATH);

// interface config
	} else if (strcmp(env->url, "/omcienv/trap_if") == 0) {
		strncpy(omci_env_g.trap_if, s, ENV_SIZE_IFNAME);
	} else if (strcmp(env->url, "/omcienv/omcc_if") == 0) {
		strncpy(omci_env_g.omcc_if, s, ENV_SIZE_IFNAME);
	} else if (strcmp(env->url, "/omcienv/wifi_if") == 0) {
		strncpy(omci_env_g.wifi_if, s, ENV_SIZE_IFNAME);
	} else if (strcmp(env->url, "/omcienv/router_wan_if") == 0) {
		strncpy(omci_env_g.router_wan_if, s, ENV_SIZE_IFNAME);
	} else if (strcmp(env->url, "/omcienv/router_lan_if") == 0) {
		strncpy(omci_env_g.router_lan_if, s, ENV_SIZE_IFNAME);

// connnecting olt simulator
	} else if (strcmp(env->url, "/omcienv/sim_olt_ip") == 0) {
		strncpy(omci_env_g.sim_olt_ip, s, ENV_SIZE_IPSTR);
	} else if (strcmp(env->url, "/omcienv/sim_olt_udp_port") == 0) {
		omci_env_g.sim_olt_udp_port = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/sim_pon_id") == 0) {
		omci_env_g.sim_pon_id= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/sim_onu_id") == 0) {
		omci_env_g.sim_onu_id = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/sim_tcont_enable") == 0) {
		omci_env_g.sim_tcont_enable = util_atoi(s);

// omci msg
	} else if (strcmp(env->url, "/omcienv/omcc_rx_crc_type") == 0) {
		omci_env_g.omcc_rx_crc_type = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/omcc_tx_crc_type") == 0) {
		omci_env_g.omcc_tx_crc_type = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/omci_tranx_cache_num") == 0) {
		omci_env_g.omci_tranx_cache_num = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/omci_tranx_match_mode") == 0) {
		omci_env_g.omci_tranx_match_mode = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/omci_tranx_bypass") == 0) {
		omci_env_g.omci_tranx_bypass = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/omci_tranx_busy_enable") == 0) {
		omci_env_g.omci_tranx_busy_enable = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/omci_history_enable") == 0) {
		omci_env_g.omci_history_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/omci_history_size") == 0) {
		omci_env_g.omci_history_size = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/omci_history_msgmask") == 0) {
		omci_env_g.omci_history_msgmask = util_atoll(s);

	} else if (strcmp(env->url, "/omcienv/omci_exec_time_limitation") == 0) {
		omci_env_g.omci_exec_time_limitation = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/omci_tx_flag") == 0) {
		omci_env_g.omci_tx_flag = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/omci_tx_resend_flag") == 0) {
		omci_env_g.omci_tx_resend_flag = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/omcidump_enable") == 0) {
		omci_env_g.omcidump_enable = util_atoll(s);
	} else if (strcmp(env->url, "/omcienv/omcidump_msgmask") == 0) {
		omci_env_g.omcidump_msgmask = util_atoll(s);

	} else if (strcmp(env->url, "/omcienv/omccrx_log_enable") == 0) {
		omci_env_g.omccrx_log_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/omccrx_log_msgmask") == 0) {
		omci_env_g.omccrx_log_msgmask = util_atoll(s);
	} else if (strcmp(env->url, "/omcienv/omccrx_log_file") == 0) {
		strncpy(omci_env_g.omccrx_log_file, s, ENV_SIZE_PATH);

	} else if (strcmp(env->url, "/omcienv/restore_log_enable") == 0) {
		omci_env_g.restore_log_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/restore_log_file") == 0) {
		strncpy(omci_env_g.restore_log_file, s, ENV_SIZE_PATH);

// mib
	} else if (strcmp(env->url, "/omcienv/xml_section") == 0) {
		memset(omci_env_g.xml_section, 0, sizeof(omci_env_g.xml_section));
		conv_numlist2array(s, omci_env_g.xml_section, sizeof(unsigned short), 32);
	} else if (strcmp(env->url, "/omcienv/xml_load_namestr") == 0) {
		omci_env_g.xml_load_namestr = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/mib_upload_order") == 0) {
		memset(omci_env_g.mib_upload_order, 0, sizeof(omci_env_g.mib_upload_order));
		conv_numlist2array(s, omci_env_g.mib_upload_order, sizeof(unsigned short), ENV_SIZE_MIB_UPLOAD_ORDER);
	} else if (strcmp(env->url, "/omcienv/mib_boundary_check") == 0) {
		omci_env_g.mib_boundary_check = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mib_pointer_check") == 0) {
		omci_env_g.mib_pointer_check = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mib_readonly_check") == 0) {
		omci_env_g.mib_readonly_check = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mib_rw_mode") == 0) {
		omci_env_g.mib_rw_mode = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mib_aliasid_enable") == 0) {
		omci_env_g.mib_aliasid_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mib_rare_meid_start") == 0) {
		omci_env_g.mib_rare_meid_start = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mib_rare2_meid_start") == 0) {
		omci_env_g.mib_rare2_meid_start = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mib_nonexist_del_err") == 0) {
		omci_env_g.mib_nonexist_del_err = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/autouni_enable") == 0) {
		omci_env_g.autouni_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/autoveip_enable") == 0) {
		omci_env_g.autoveip_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/erdiag_mon_enable") == 0) {
		omci_env_g.erdiag_mon_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mibreset_dump_enable") == 0) {
		omci_env_g.mibreset_dump_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mib_uni_adminlock_default") == 0) {
		omci_env_g.mib_uni_adminlock_default = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/adminlock_trigger_opstate") == 0) {
		omci_env_g.adminlock_trigger_opstate = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/untag_reserved_vid") == 0) {
		omci_env_g.untag_reserved_vid = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/port_forward_operation_default") == 0) {
		omci_env_g.port_forward_operation_default = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/pseudo_ts_pointer_enable") == 0) {
		omci_env_g.pseudo_ts_pointer_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/invert_weight_enable") == 0) {
		omci_env_g.invert_weight_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/invert_priority_enable") == 0) {
		omci_env_g.invert_priority_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/pq_tcont_relation") == 0) {
		omci_env_g.pq_tcont_relation = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/tcont_map_enable") == 0) {
		omci_env_g.tcont_map_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tcont_map_policy") == 0) {
		env_set_tcont_map_policy(s);
	} else if (strcmp(env->url, "/omcienv/tcont_map_ts") == 0) {
		memset(omci_env_g.tcont_map_ts, 0, sizeof(omci_env_g.tcont_map_ts));
		conv_numlist2array(s, omci_env_g.tcont_map_ts, sizeof(char), ENV_SIZE_TCONT_MAP);
	} else if (strcmp(env->url, "/omcienv/tcont_map_pq") == 0) {
		memset(omci_env_g.tcont_map_pq, 0, sizeof(omci_env_g.tcont_map_pq));
		conv_numlist2array(s, omci_env_g.tcont_map_pq, sizeof(char), ENV_SIZE_TCONT_MAP);

// sw_download
	} else if (strcmp(env->url, "/omcienv/sw_download_end_behavior") == 0) {
		omci_env_g.sw_download_end_behavior= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/sw_download_active_always_ok") == 0) {
		omci_env_g.sw_download_active_always_ok= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/sw_download_commit_always_ok") == 0) {
		omci_env_g.sw_download_commit_always_ok= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/sw_download_window_size") == 0) {
		omci_env_g.sw_download_window_size= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/sw_download_script") == 0) {
		strncpy(omci_env_g.sw_download_script, s, ENV_SIZE_PATH);
	} else if (strcmp(env->url, "/omcienv/sw_download_image_name") == 0) {
		strncpy(omci_env_g.sw_download_image_name, s, ENV_SIZE_PATH);

// thread tasks 
	// flags for task debug
	} else if (strcmp(env->url, "/omcienv/task_pause_mask") == 0) {
		omci_env_g.task_pause_mask= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/avc_refresh_interval") == 0) {
		omci_env_g.avc_refresh_interval = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/alarm_refresh_interval") == 0) {
		omci_env_g.alarm_refresh_interval = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/pm_refresh_interval") == 0) {
		omci_env_g.pm_refresh_interval = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/pm_swap_interval") == 0) {
		omci_env_g.pm_swap_interval = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/cpuport_history_enable") == 0) {
		omci_env_g.cpuport_history_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_history_size") == 0) {
		omci_env_g.cpuport_history_size = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_arp_enable") == 0) {
		omci_env_g.cpuport_arp_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_pppoe_enable") == 0) {
		omci_env_g.cpuport_pppoe_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_pppoe_cust_tag") == 0) {
		memset(omci_env_g.cpuport_pppoe_cust_tag, 0x00, sizeof(omci_env_g.cpuport_pppoe_cust_tag));
		conv_numlist2array(s, omci_env_g.cpuport_pppoe_cust_tag, sizeof(unsigned char), ENV_SIZE_STRING);
	} else if (strcmp(env->url, "/omcienv/cpuport_dhcp_enable") == 0) {
		omci_env_g.cpuport_dhcp_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_dhcp_cust_tag") == 0) {
		memset(omci_env_g.cpuport_dhcp_cust_tag, 0x00, sizeof(omci_env_g.cpuport_dhcp_cust_tag));
		conv_numlist2array(s, omci_env_g.cpuport_dhcp_cust_tag, sizeof(unsigned char), ENV_SIZE_STRING);
	} else if (strcmp(env->url, "/omcienv/cpuport_dhcp_server_onlan") == 0) {
		omci_env_g.cpuport_dhcp_server_onlan = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_pkt_ssdp_fwd_wan") == 0) {
		omci_env_g.cpuport_pkt_ssdp_fwd_wan = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_im_enable") == 0) {
		omci_env_g.cpuport_im_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_im_lan2lan_enable") == 0) {
		omci_env_g.cpuport_im_lan2lan_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_wifi_block_mask") == 0) {
		omci_env_g.cpuport_wifi_block_mask = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_wifi_block_ra") == 0) {
		omci_env_g.cpuport_wifi_block_ra = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_mcast_drop") == 0) {
		omci_env_g.cpuport_mcast_drop = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/stp_enable") == 0) {
		omci_env_g.stp_enable= util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/cpu_usage_threshold") == 0) {
		omci_env_g.cpu_usage_threshold = util_atoi(s);
	// for cpu limit task no
	} else if (strcmp(env->url, "/omcienv/cpul_task_no") == 0) {
		memset(omci_env_g.cpul_task_no, 0xff, sizeof(omci_env_g.cpul_task_no));
		conv_numlist2array(s, omci_env_g.cpul_task_no, sizeof(char), ENV_SIZE_TASK);
	} else if (strcmp(env->url, "/omcienv/cpul_total_usage") == 0) {
		omci_env_g.cpul_total_usage= util_atoi(s);

// debug
	} else if (strcmp(env->url, "/omcienv/debug_level") == 0) {
		omci_env_g.debug_level = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_level_bat") == 0) {
		omci_env_g.debug_level_bat = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_level_cfm") == 0) {
		omci_env_g.debug_level_cfm = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_level_cpuport") == 0) {
		omci_env_g.debug_level_cpuport = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_level_fwk") == 0) {
		omci_env_g.debug_level_fwk = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_level_igmp") == 0) {
		omci_env_g.debug_level_igmp = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_level_lldp") == 0) {
		omci_env_g.debug_level_lldp = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_level_vacl") == 0) {
		omci_env_g.debug_level_vacl = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_level_voip") == 0) {
		omci_env_g.debug_level_voip = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_level_xml") == 0) {
		omci_env_g.debug_level_xml = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_log_time") == 0) {
		omci_env_g.debug_log_time = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_log_type") == 0) {
		omci_env_g.debug_log_type = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/debug_log_file") == 0) {
		strncpy(omci_env_g.debug_log_file, s, ENV_SIZE_PATH);
	} else if (strcmp(env->url, "/omcienv/memdbg_enable") == 0) {
		omci_env_g.memdbg_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/er_group_hw_debug_enable") == 0) {
		omci_env_g.er_group_hw_debug_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/er_group_hw_time_enable") == 0) {
		omci_env_g.er_group_hw_time_enable = util_atoi(s);

// olt
	} else if (strcmp(env->url, "/omcienv/olt_vendor_check_enable") == 0) {
		omci_env_g.olt_vendor_check_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/olt_vendor_config_change") == 0) {
		omci_env_g.olt_vendor_config_change = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/olt_workaround_enable") == 0) {
		omci_env_g.olt_workaround_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/olt_alu_mcast_support") == 0) {
		omci_env_g.olt_alu_mcast_support = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/olt_huawei_ignore_vlan_drop_rule") == 0) {
		omci_env_g.olt_huawei_ignore_vlan_drop_rule = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/olt_proprietary_support") == 0) {
		strncpy(omci_env_g.olt_proprietary_support, s, ENV_SIZE_PROPRIETARY_VENDOR);
	} else if (strcmp(env->url, "/omcienv/ont_force_vendor_id") == 0) {
		strncpy(omci_env_g.ont_force_vendor_id, s, 5);

// anig
	} else if (strcmp(env->url, "/omcienv/anig_type") == 0) {
		omci_env_g.anig_type = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/anig_support") == 0) {
		omci_env_g.anig_support = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/anig_txrx_threshold") == 0) {
		memset(omci_env_g.anig_txrx_threshold, 0, sizeof(omci_env_g.anig_txrx_threshold));
		conv_numlist2array(s, omci_env_g.anig_txrx_threshold, sizeof(char), 4);
	} else if (strcmp(env->url, "/omcienv/anig_devport") == 0) {
		omci_env_g.anig_devport = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/anig_devaddr0") == 0) {
		omci_env_g.anig_devaddr0 = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/anig_mon_mode") == 0) {
		omci_env_g.anig_mon_mode = util_atoi(s);

// logical port id/mask
	} else if (strcmp(env->url, "/omcienv/wan_logical_portid") == 0) {
		omci_env_g.wan_logical_portid = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/uni_logical_portmask") == 0) {
		omci_env_g.uni_logical_portmask = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpu_logical_portmask") == 0) {
		omci_env_g.cpu_logical_portmask = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/wifi_logical_portmask") == 0) {
		omci_env_g.wifi_logical_portmask = util_atoi(s);

// switch
	} else if (strcmp(env->url, "/omcienv/ivlan_enable") == 0) {
		omci_env_g.ivlan_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/stp_conf_before_omci") == 0) {
		omci_env_g.stp_conf_before_omci = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/switch_history_mode") == 0) {
		omci_env_g.switch_history_mode = util_atoi(s);		
	} else if (strcmp(env->url, "/omcienv/switch_history_interval") == 0) {
		omci_env_g.switch_history_interval = util_atoi(s);		

	} else if (strcmp(env->url, "/omcienv/br_allowed_srcmask") == 0) {
		omci_env_g.br_allowed_srcmask= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/rt_allowed_srcmask") == 0) {
		omci_env_g.rt_allowed_srcmask= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/trunk_group_portmask") == 0) {
		omci_env_g.trunk_group_portmask = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/localbridging_default") == 0) {
		omci_env_g.localbridging_default = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/localbridging_addon_portmask") == 0) {
		omci_env_g.localbridging_addon_portmask = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/ctc_internet_bridge_enabled_srcmask") == 0) {
		omci_env_g.ctc_internet_bridge_enabled_srcmask= util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/port2port_enable") == 0) {
		omci_env_g.port2port_enable = util_atoi(s);		
	} else if (strcmp(env->url, "/omcienv/maclearn_drop_enable") == 0) {
		omci_env_g.maclearn_drop_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/veip_for_wifi_enable") == 0) {
		omci_env_g.veip_for_wifi_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cpuport_ds_passthrough") == 0) {
		omci_env_g.cpuport_ds_passthrough = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/rma_forward_mask") == 0) {
		omci_env_g.rma_forward_mask = util_atoull(s);
	} else if (strcmp(env->url, "/omcienv/bundle_rate_support") == 0) {
		omci_env_g.bundle_rate_support = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/linkdown_debounce_time") == 0) {
		omci_env_g.linkdown_debounce_time = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/linkup_debounce_time") == 0) {
		omci_env_g.linkup_debounce_time = util_atoi(s);

// rldp
	} else if (strcmp(env->url, "/omcienv/rldp_enable") == 0) {
		omci_env_g.rldp_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/rldp_def_portmask") == 0) {
		omci_env_g.rldp_def_portmask= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/rldp_passive_only") == 0) {
		omci_env_g.rldp_passive_only= util_atoi(s);

// tag processing
	} else if (strcmp(env->url, "/omcienv/tag_treatment_correct") == 0) {
		omci_env_g.tag_treatment_correct = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_discard_reversing") == 0) {
		omci_env_g.tag_discard_reversing = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_preview_table") == 0) {
		omci_env_g.tag_preview_table = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_auto_reverse") == 0) {
		omci_env_g.tag_auto_reverse = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_mc_ds_from_us") == 0) {
		omci_env_g.tag_mc_ds_from_us = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_mc_sort_priority") == 0) {
		omci_env_g.tag_mc_sort_priority = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_pbit_workaround") == 0) {
		omci_env_g.tag_pbit_workaround = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_pbit_portmask") == 0) {
		omci_env_g.tag_pbit_portmask = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_pbit_remap_table") == 0) {
		memset(omci_env_g.tag_pbit_remap_table, 0, sizeof(omci_env_g.tag_pbit_remap_table));
		conv_numlist2array(s, omci_env_g.tag_pbit_remap_table, sizeof(char), 8);
	} else if (strcmp(env->url, "/omcienv/tag_ds_tci_dont_reverse_workaround") == 0) {
		omci_env_g.tag_ds_tci_dont_reverse_workaround = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_insert_65305_rules") == 0) {
		omci_env_g.tag_insert_65305_rules = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_ctc_translation_path") == 0) {
		strncpy(omci_env_g.tag_ctc_translation_path, s, ENV_SIZE_PATH);
	} else if (strcmp(env->url, "/omcienv/tag_mc_ds_wildcard") == 0) {
		omci_env_g.tag_mc_ds_wildcard = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_l2_rules_reverse") == 0) {
		omci_env_g.tag_l2_rules_reverse = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_l2_rules_ds_mac") == 0) {
		omci_env_g.tag_l2_rules_ds_mac = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tag_iphost_vid_map") == 0) {
		memset(omci_env_g.tag_iphost_vid_map, 0, sizeof(omci_env_g.tag_iphost_vid_map));
		conv_numlist2array(s, omci_env_g.tag_iphost_vid_map, sizeof(unsigned short), ENV_SIZE_IPHOST_PORT);

// classf
	} else if (strcmp(env->url, "/omcienv/classf_tag_mode") == 0) {
		omci_env_g.classf_tag_mode = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_add_default_rules") == 0) {
		omci_env_g.classf_add_default_rules = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_force_stpid") == 0) {
		omci_env_g.classf_force_stpid = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_stag_tpid") == 0) {
		omci_env_g.classf_stag_tpid = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_hw_stag_tpid") == 0) {
		omci_env_g.classf_hw_stag_tpid = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_et_combine") == 0) {
		omci_env_g.classf_et_combine = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_ds_gem_dg") == 0) {
		omci_env_g.classf_ds_gem_dg = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_dscp2pbit_mode") == 0) {
		omci_env_g.classf_dscp2pbit_mode = util_atoi(s);
#if 0 // Remove VEIP related workaround
	} else if (strcmp(env->url, "/omcienv/classf_veip_orig_tci_map") == 0) {
		memset(omci_env_g.classf_veip_orig_tci_map, 0, sizeof(omci_env_g.classf_veip_orig_tci_map));
		conv_numlist2array(s, omci_env_g.classf_veip_orig_tci_map, sizeof(unsigned short), ENV_SIZE_VEIP_PORT);
	} else if (strcmp(env->url, "/omcienv/classf_veip_real_tci_map") == 0) {
		memset(omci_env_g.classf_veip_real_tci_map, 0, sizeof(omci_env_g.classf_veip_real_tci_map));
		conv_numlist2array(s, omci_env_g.classf_veip_real_tci_map, sizeof(unsigned short), ENV_SIZE_VEIP_PORT);
	} else if (strcmp(env->url, "/omcienv/classf_veip_real_tag_rule_priority") == 0) {
		omci_env_g.classf_veip_real_tag_rule_priority = util_atoi(s);
#endif
	} else if (strcmp(env->url, "/omcienv/classf_ds_unmatch") == 0) {
		omci_env_g.classf_ds_unmatch = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_ds_mc_extend") == 0) {
		omci_env_g.classf_ds_mc_extend = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_ds_mc_force_strip") == 0) {
		omci_env_g.classf_ds_mc_force_strip = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_veip_rules_reassign") == 0) {
		omci_env_g.classf_veip_rules_reassign = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_dropping_rules_add") == 0) {
		omci_env_g.classf_dropping_rules_add = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_vmap_pbit_workaround") == 0) {
		omci_env_g.classf_vmap_pbit_workaround = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_hw_2tags_mode") == 0) {
		omci_env_g.classf_hw_2tags_mode = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_hw_2tags_workaround") == 0) {
		omci_env_g.classf_hw_2tags_workaround = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_add_ipv4mc_strip") == 0) {
		omci_env_g.classf_add_ipv4mc_strip = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_add_hwnat_1tag_rules") == 0) {
		omci_env_g.classf_add_hwnat_1tag_rules = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_add_hwnat_untag_rules") == 0) {
		omci_env_g.classf_add_hwnat_untag_rules = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_add_hwnat_untag_vid") == 0) {
		omci_env_g.classf_add_hwnat_untag_vid = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_add_hwnat_untag_vid_mask") == 0) {
		omci_env_g.classf_add_hwnat_untag_vid_mask = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_hw_rules_migration") == 0) {
		omci_env_g.classf_hw_rules_migration = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_hw_colour_force") == 0) {
		omci_env_g.classf_hw_colour_force = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_keep_candi") == 0) {
		omci_env_g.classf_keep_candi = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_statistic_enable") == 0) {
		omci_env_g.classf_statistic_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_tlan_pattern") == 0) {
		omci_env_g.classf_tlan_pattern = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/mc_ds_untag_discard_enable") == 0) {
		omci_env_g.mc_ds_untag_discard_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/classf_add_default_rules_by_me_84") == 0) {
		omci_env_g.classf_add_default_rules_by_me_84 = util_atoi(s);

// cfm
	} else if (strcmp(env->url, "/omcienv/cfm_enable") == 0) {
		omci_env_g.cfm_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_y1731_enable") == 0) {
		omci_env_g.cfm_y1731_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_portmask") == 0) {
		omci_env_g.cfm_portmask = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_mac_ifname") == 0) {
		strncpy(omci_env_g.cfm_mac_ifname, s, ENV_SIZE_IFNAME);
	} else if (strcmp(env->url, "/omcienv/cfm_send_timeout") == 0) {
		omci_env_g.cfm_send_timeout = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_reverse_mep_direction") == 0) {
		omci_env_g.cfm_reverse_mep_direction = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_rmep_ccm_before_hold") == 0) {
		omci_env_g.cfm_rmep_ccm_before_hold = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_rmep_holdtime_before_active") == 0) {
		omci_env_g.cfm_rmep_holdtime_before_active = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_rmep_ccm_lost_threshold") == 0) {
		omci_env_g.cfm_rmep_ccm_lost_threshold = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_defect_mask_in_rdi_gen") == 0) {
		omci_env_g.cfm_defect_mask_in_rdi_gen = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_defect_mask_in_ais_gen") == 0) {
		omci_env_g.cfm_defect_mask_in_ais_gen = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_defect_mask_supressed_by_ais") == 0) {
		omci_env_g.cfm_defect_mask_supressed_by_ais = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/cfm_defect_mask_supressed_by_lck") == 0) {
		omci_env_g.cfm_defect_mask_supressed_by_lck = util_atoi(s);

// lldp
	} else if (strcmp(env->url, "/omcienv/lldp_enable") == 0) {
		omci_env_g.lldp_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/lldp_ifname") == 0) {
		strncpy(omci_env_g.lldp_ifname, s, ENV_SIZE_IFNAME);
	} else if (strcmp(env->url, "/omcienv/lldp_dst_mac") == 0) {
		omci_env_g.lldp_dst_mac = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/lldp_send_interval") == 0) {
		omci_env_g.lldp_send_interval = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/lldp_fast_tx") == 0) {
		omci_env_g.lldp_fast_tx = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/lldp_credit_max") == 0) {
		omci_env_g.lldp_credit_max = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/lldp_tx_fast_init") == 0) {
		omci_env_g.lldp_tx_fast_init = util_atoi(s);

	} else if (strcmp(env->url, "/omcienv/lldp_system_name") == 0) {
		strncpy(omci_env_g.lldp_system_name, s, ENV_SIZE_STRING);
	} else if (strcmp(env->url, "/omcienv/lldp_system_desc") == 0) {
		strncpy(omci_env_g.lldp_system_desc, s, ENV_SIZE_STRING);
	} else if (strcmp(env->url, "/omcienv/lldp_med_opt") == 0) {
		omci_env_g.lldp_med_opt = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/lldp_med_cap") == 0) {
		omci_env_g.lldp_med_cap = util_atoi(s);

// extoam
	} else if (strcmp(env->url, "/omcienv/extoam_enable") == 0) {
		omci_env_g.extoam_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/extoam_port") == 0) {
		omci_env_g.extoam_port = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/extoam_mac_ifname") == 0) {
		strncpy(omci_env_g.extoam_mac_ifname, s, ENV_SIZE_IFNAME);
	} else if (strcmp(env->url, "/omcienv/extoam_itas_image_name") == 0) {
		strncpy(omci_env_g.extoam_itas_image_name, s, 256);

// tm
	} else if (strcmp(env->url, "/omcienv/tm_pq_gem_qos_method") == 0) {
		omci_env_g.tm_pq_gem_qos_method = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tm_gemus_rate_factor") == 0) {
		omci_env_g.tm_gemus_rate_factor = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tm_gemds_rate_factor") == 0) {
		omci_env_g.tm_gemds_rate_factor = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tm_unius_rate_factor") == 0) {
		omci_env_g.tm_unius_rate_factor = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tm_unids_rate_factor") == 0) {
		omci_env_g.tm_unids_rate_factor = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tm_pir_zero_action") == 0) {
		omci_env_g.tm_pir_zero_action = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/tm_protect_enable") == 0) {
		omci_env_g.tm_protect_enable = util_atoi(s);
		
// igmp
	} else if (strcmp(env->url, "/omcienv/igmp_enable") == 0) {
		omci_env_g.igmp_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/igmp_version_overwrite") == 0) {
		omci_env_g.igmp_version_overwrite = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/igmp_mcast_me_support") == 0) {
		omci_env_g.igmp_mcast_me_support = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/igmp_router_mode") == 0) {
		omci_env_g.igmp_proxy_snooping = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/igmp_join_default_grant") == 0) {
		omci_env_g.igmp_join_default_grant = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/igmp_query_pass_through") == 0) {
		omci_env_g.igmp_query_pass_through= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/igmp_preassign_list") == 0) {
		env_set_igmp_preassign_list(s);
	} else if (strcmp(env->url, "/omcienv/igmp_v1_ignore") == 0) {
		omci_env_g.igmp_v1_ignore = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/igmp_mcast_mode_src") == 0) {
		omci_env_g.igmp_mcast_mode_src = util_atoi(s);

// voip
	} else if (strcmp(env->url, "/omcienv/voip_enable") == 0) {
		omci_env_g.voip_enable = util_atoi(s);
	//flags for voip debug
	} else if (strcmp(env->url, "/omcienv/voip_debug_set_flag") == 0) {
		omci_env_g.voip_debug_set_flag= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/voip_debug_get_flag") == 0) {
		omci_env_g.voip_debug_get_flag= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/voip_debug_release_flag") == 0) {
		omci_env_g.voip_debug_release_flag= util_atoi(s);

// misc
	} else if (strcmp(env->url, "/omcienv/batchtab_extevent_idletime") == 0) {
		omci_env_g.batchtab_extevent_idletime = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/batchtab_retry_timeout") == 0) {
		omci_env_g.batchtab_retry_timeout = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/batchtab_wanif_feature") == 0) {
		omci_env_g.batchtab_wanif_feature = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/batchtab_hardbridge_enable") == 0) {
		omci_env_g.batchtab_hardbridge_enable = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/exclude_flowid_0") == 0) {
		omci_env_g.exclude_flowid_0 = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/avoid_pir_drop") == 0) {
		omci_env_g.avoid_pir_drop = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/uni_policing_pbit_mask") == 0) {
		omci_env_g.uni_policing_pbit_mask = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/uni_policing_default_bs") == 0) {
		omci_env_g.uni_policing_default_bs = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/uni_policing_aggregate_acl") == 0) {
		omci_env_g.uni_policing_aggregate_acl = util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/uni_linkdown_lock") == 0) {
		omci_env_g.uni_linkdown_lock= util_atoi(s);
	} else if (strcmp(env->url, "/omcienv/watchdog_omccmiss_timeout") == 0) {
		conv_numlist2array(s, omci_env_g.watchdog_omccmiss_timeout, sizeof(unsigned short), 2);
	} else if (strcmp(env->url, "/omcienv/watchdog_tcontmiss_timeout") == 0) {
		conv_numlist2array(s, omci_env_g.watchdog_tcontmiss_timeout, sizeof(unsigned short), 2);
	} else if (strcmp(env->url, "/omcienv/watchdog_o2stuck_timeout") == 0) {
		conv_numlist2array(s, omci_env_g.watchdog_o2stuck_timeout, sizeof(unsigned short), 2);
	} else if (strcmp(env->url, "/omcienv/watchdog_idlebw_guard") == 0) {
		conv_numlist2array(s, omci_env_g.watchdog_idlebw_guard, sizeof(unsigned short), 4);
	} else if (strcmp(env->url, "/omcienv/uni_config_lock") == 0) {
		omci_env_g.uni_config_lock= util_atoi(s);
	} 
#endif

	return 0;
}

static int
xmlomci_env_finish_tag(struct xmlparse_env_t *env, char *tag)
{
	return 0;
}

int
xmlomci_env_init(char *env_xml_filename)
{
	struct xmlparse_env_t env;

	env.dataptr = NULL;
	env.add_tag = xmlomci_env_add_tag;
	env.add_attr = xmlomci_env_add_attr;
	env.finish_attrs = xmlomci_env_finish_attrs;
	env.add_content = xmlomci_env_add_content;
	env.finish_tag = xmlomci_env_finish_tag;

	return xmlparse_parse(env_xml_filename, &env);
}

